#include "iceshard_gfx_runner.hxx"
#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_world.hxx"
#include "iceshard_gfx_queue.hxx"

#include <ice/render/render_fence.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/profiler.hxx>

namespace ice::gfx
{

    namespace detail
    {

        struct IceGfxSubmitEntry
        {
            ice::gfx::IceGfxQueue* queue;
            ice::render::CommandBuffer command_buffers[1];
            ice::render::RenderFence* fence;
        };

    } // namespace detail

    static constexpr ice::u32 Constant_GfxFrameAllocatorCapacity = 16u * 1024u;

    IceGfxRunner::IceGfxRunner(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::gfx::IceGfxDevice> device,
        ice::UniquePtr<ice::gfx::IceGfxWorld> world
    ) noexcept
        : _allocator{ alloc, "gfx-runner"}
        , _thread{ ice::create_task_thread(_allocator) }
        , _device{ ice::move(device) }
        , _frame_allocator{
            { _allocator, Constant_GfxFrameAllocatorCapacity },
            { _allocator, Constant_GfxFrameAllocatorCapacity }
        }
        , _next_free_allocator{ 0 }
        , _current_frame{ ice::make_unique_null<ice::gfx::IceGfxFrame>() }
        , _gfx_world_name{ "default"_sid }
        , _traits{ _allocator }
        , _runner_trait{ *this }
        , _contexts{ _allocator }
        , _mre_internal{ true }
        , _mre_selected{ &_mre_internal }
    {
        ice::pod::hash::reserve(_contexts, 10);

        _current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator, _frame_allocator[1]
        );

        for (ice::render::RenderFence*& fence_ptr : _fences)
        {
            fence_ptr = _device->device().create_fence();
        }

        add_trait("ice.gfx.runner"_sid, &_runner_trait);
    }

    IceGfxRunner::~IceGfxRunner() noexcept
    {
        _mre_selected->wait();

        // Wait for the current GfxFrame to send final tasks
        ice::IceshardTaskExecutor frame_tasks = _current_frame->create_task_executor();
        frame_tasks.start_all();
        frame_tasks.wait_ready();

        // Now stop the thread and wait for everything to finish.
        _thread->stop();
        _thread->join();
        _thread = nullptr;

        for (auto const& entry : _contexts)
        {
            _allocator.destroy(entry.value);
        }

        for (ice::render::RenderFence*& fence_ptr : _fences)
        {
            _device->device().destroy_fence(fence_ptr);
        }

        // Destroy the frame safely.
        _current_frame = nullptr;
    }

    void IceGfxRunner::add_trait(ice::StringID_Arg name, ice::gfx::GfxTrait* trait) noexcept
    {
        ice::pod::array::push_back(_traits, { name, trait });
    }

    void IceGfxRunner::set_graphics_world(ice::StringID_Arg world_name) noexcept
    {
        _gfx_world_name = world_name;
    }

    void IceGfxRunner::prepare_world(ice::World* world) noexcept
    {
        ICE_ASSERT(
            world->state_hint() == WorldState::Managed,
            "A GfxRunner can only prepare a managed world."
        );

        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            world->add_trait(entry.name, entry.trait);
        }
    }

    void IceGfxRunner::cleanup_world(ice::World* world) noexcept
    {
        ICE_ASSERT(
            world->state_hint() == WorldState::Managed,
            "A GfxRunner can only cleanup a managed world."
        );

        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            world->remove_trait(entry.name);
        }
    }

    auto IceGfxRunner::graphics_world_name() noexcept -> ice::StringID
    {
        return _gfx_world_name;
    }

    void IceGfxRunner::set_event(ice::ManualResetEvent* mre_event) noexcept
    {
        ice::ManualResetEvent* old_event = _mre_selected;
        old_event->wait(); // We need to wait for the gfx frame to finish it's tasks.

        // We can now change the event used for the next task tracking.
        if (mre_event == nullptr)
        {
            _mre_selected = &_mre_internal;
        }
        else
        {
            _mre_selected = mre_event;
        }
    }

    void IceGfxRunner::draw_frame(ice::EngineFrame const& engine_frame) noexcept
    {
        _mre_selected->wait();
        _mre_selected->reset();
        ice::sync_manual_wait(task_frame(engine_frame, ice::move(_current_frame)), *_mre_selected);

        [[maybe_unused]]
        bool const discarded_memory = _frame_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        _current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator,
            _frame_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_allocator);
    }

    void IceGfxRunner::setup_traits() noexcept
    {
        _current_frame->add_task(task_setup_gfx_traits());
    }

    void IceGfxRunner::cleanup_traits() noexcept
    {
        ice::ManualResetEvent _setup_event{ };
        ice::sync_manual_wait(task_cleanup_gfx_contexts(), _setup_event);
        _setup_event.wait();

        _setup_event.reset();
        ice::sync_manual_wait(task_cleanup_gfx_traits(), _setup_event);
        _setup_event.wait();
    }

    auto IceGfxRunner::device() noexcept -> ice::gfx::GfxDevice&
    {
        return *_device;
    }

    auto IceGfxRunner::frame() noexcept -> ice::gfx::GfxFrame&
    {
        return *_current_frame;
    }

    auto IceGfxRunner::task_cleanup_gfx_contexts() noexcept -> ice::Task<>
    {
        co_await *_thread;
        cleanup_gfx_contexts();
        co_return;
    }

    auto IceGfxRunner::task_setup_gfx_traits() noexcept -> ice::Task<>
    {
        setup_gfx_traits();
        co_return;
    }

    auto IceGfxRunner::task_cleanup_gfx_traits() noexcept -> ice::Task<>
    {
        co_await *_thread;
        cleanup_gfx_traits();
        co_return;
    }

    void IceGfxRunner::cleanup_gfx_contexts() noexcept
    {
        for (auto const& entry : _contexts)
        {
            entry.value->clear_context(*_device);
        }
    }

    void IceGfxRunner::setup_gfx_traits() noexcept
    {
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            entry.trait->gfx_setup(*_current_frame, *_device);
        }
    }

    void IceGfxRunner::cleanup_gfx_traits() noexcept
    {
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            entry.trait->gfx_cleanup(*_current_frame, *_device);
        }
    }

    auto IceGfxRunner::task_frame(
        ice::EngineFrame const& engine_frame,
        ice::UniquePtr<ice::gfx::IceGfxFrame> frame
    ) noexcept -> ice::Task<>
    {
        using ice::render::QueueFlags;

        ice::memory::StackAllocator<2048> initial_alloc;
        ice::memory::ScratchAllocator alloc{ initial_alloc, 256 };
        ice::memory::ScratchAllocator temp_alloc{ initial_alloc, 1536 };

        IPT_FRAME_MARK_NAMED("Graphics Frame");

        ice::Span<ice::gfx::IceGfxPassEntry const> pass_list = frame->enqueued_passes();
        ice::u32 const pass_count = ice::size(pass_list);

        ice::pod::Array<ice::gfx::IceGfxContext*> frame_contexts{ alloc };
        ice::pod::array::reserve(frame_contexts, pass_count);

        for (ice::gfx::IceGfxPassEntry const& gfx_pass : pass_list)
        {
            ice::pod::array::push_back(
                frame_contexts,
                get_or_create_context(gfx_pass.pass_name, *gfx_pass.pass)
            );
        }

        // Gather tasks forom all traits.
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            frame->add_task(entry.trait->task_gfx_update(engine_frame, *frame, *_device));
        }

        ice::IceshardTaskExecutor frame_tasks = frame->create_task_executor();

        // Await the graphics thread context
        co_await *_thread;

        IPT_ZONE_SCOPED_NAMED("Graphis Frame");


        // NOTE: We aquire the next image index to be rendered.
        ice::u32 const image_index = _device->next_frame();

        // NOTE: We aquire the graphics queues for this frame index.
        ice::gfx::IceGfxQueueGroup& queue_group = _device->queue_group(image_index);
        queue_group.reset_all();
        frame_tasks.start_all();

        frame->on_frame_begin();

        // First take care of the Transfer queue.
        ice::gfx::IceGfxQueue* transfer_queue = nullptr;
        {
            if (queue_group.get_queue(QueueFlags::Transfer, transfer_queue))
            {
                ice::render::CommandBuffer command_buffer[1];
                transfer_queue->request_command_buffers(
                    ice::render::CommandBufferType::Primary,
                    command_buffer
                );

                bool const has_work = frame->on_frame_stage(
                    engine_frame,
                    command_buffer[0],
                    _device->device().get_commands()
                );

                if (has_work)
                {
                    _fences[0]->reset();
                    transfer_queue->submit_command_buffers(command_buffer, _fences[0]);
                }
            }
        }

        ice::u32 submit_count = 0;
        detail::IceGfxSubmitEntry submit_entries[3]{ };

        for (ice::u32 idx = 0; idx < pass_count; ++idx)
        {
            ice::gfx::IceGfxPassEntry const& gfx_pass = pass_list[idx];
            ice::gfx::IceGfxQueue* const queue = queue_group.get_queue(gfx_pass.queue_name);
            if (queue == nullptr)
            {
                continue;
            }

            ice::pod::Array<ice::StringID_Hash> stage_order{ temp_alloc };
            gfx_pass.pass->query_stage_order(stage_order);

            ice::pod::Array<ice::gfx::GfxContextStage const*> context_stages{ temp_alloc };
            ice::pod::array::resize(context_stages, ice::size(stage_order));

            if (frame->query_queue_stages(stage_order, context_stages))
            {
                // TODO: Lets do this better in the next stage.
                ICE_ASSERT(
                    submit_count < ice::size(submit_entries),
                    "Moved past maximum number of submits in this implementation."
                );

                ice::render::CommandBuffer command_buffer[1];
                queue->request_command_buffers(
                    ice::render::CommandBufferType::Primary,
                    command_buffer
                );

                frame_contexts[idx]->prepare_context(
                    context_stages,
                    *_device
                );

                frame_contexts[idx]->record_commands(
                    engine_frame,
                    command_buffer[0],
                    _device->device().get_commands()
                );

                submit_entries[submit_count].queue = queue;
                submit_entries[submit_count].command_buffers[0] = command_buffer[0];
                submit_entries[submit_count].fence = _fences[submit_count + 1];
                submit_count += 1;
            }
        }

        if (transfer_queue != nullptr)
        {
            _fences[0]->wait(100'000'000);
        }

        for (detail::IceGfxSubmitEntry const& entry : ice::Span<detail::IceGfxSubmitEntry const>{ submit_entries, submit_count })
        {
            entry.fence->reset();
            entry.queue->submit_command_buffers(entry.command_buffers, entry.fence);
            entry.fence->wait(100'000'000);
        }

        frame->on_frame_end();
        frame_tasks.wait_ready();

        // Start the draw task and
        {
            IPT_ZONE_SCOPED_NAMED("Graphis Frame - Present");
            _device->present(image_index);
        }
        co_return;
    }

    auto IceGfxRunner::get_or_create_context(
        ice::StringID_Arg context_name,
        ice::gfx::GfxPass const& gfx_pass
    ) noexcept -> ice::gfx::IceGfxContext*
    {
        ice::gfx::IceGfxContext* result = ice::pod::hash::get(_contexts, ice::hash(context_name), nullptr);

        if (result == nullptr)
        {
            result = _allocator.make<ice::gfx::IceGfxContext>(_allocator, gfx_pass);
            ice::pod::hash::set(_contexts, ice::hash(context_name), result);
        }

        return result;
    }

} // namespace ice::gfx
