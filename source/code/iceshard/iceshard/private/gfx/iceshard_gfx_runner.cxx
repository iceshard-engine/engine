#include "iceshard_gfx_runner.hxx"
#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_world.hxx"

#include <ice/render/render_fence.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/profiler.hxx>

namespace ice::gfx
{

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
        , _mre_internal{ true }
        , _mre_selected{ &_mre_internal }
    {
        _current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator, _frame_allocator[1]
        );

        _fences[0] = _device->device().create_fence();
        _fences[1] = _device->device().create_fence();

        add_trait("ice.gfx.runner"_sid, &_runner_trait);
    }

    IceGfxRunner::~IceGfxRunner() noexcept
    {
        _mre_selected->wait();

        // Wait for the current GfxFrame to send final tasks
        _current_frame->execute_final_tasks();

        // Now stop the thread and wait for everything to finish.
        _thread->stop();
        _thread->join();
        _thread = nullptr;

        _device->device().destroy_fence(_fences[1]);
        _device->device().destroy_fence(_fences[0]);

        // Destroy the frame safely.
        _current_frame = nullptr;
    }

    auto IceGfxRunner::trait_count() const noexcept -> ice::u32
    {
        return ice::pod::array::size(_traits);
    }

    void IceGfxRunner::query_traits(
        ice::Span<ice::StringID> out_trait_names,
        ice::Span<ice::gfx::GfxTrait*> out_traits
    ) const noexcept
    {
        if (ice::size(out_trait_names) != trait_count())
        {
            return;
        }

        ice::u32 idx = 0;
        if (ice::size(out_trait_names) != ice::size(out_traits))
        {
            for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
            {
                out_trait_names[idx] = entry.name;
                idx += 1;
            }
        }
        else
        {
            for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
            {
                out_trait_names[idx] = entry.name;
                out_traits[idx] = entry.trait;
                idx += 1;
            }
        }
    }

    void IceGfxRunner::add_trait(ice::StringID_Arg name, ice::gfx::GfxTrait* trait) noexcept
    {
        ice::pod::array::push_back(_traits, { name, trait });
    }

    void IceGfxRunner::set_graphics_world(ice::StringID_Arg world_name) noexcept
    {
        _gfx_world_name = world_name;
    }

    auto IceGfxRunner::get_graphics_world() noexcept -> ice::StringID
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

        _current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator,
            _frame_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_allocator);
    }

    auto IceGfxRunner::device() noexcept -> ice::gfx::GfxDevice&
    {
        return *_device;
    }

    auto IceGfxRunner::frame() noexcept -> ice::gfx::GfxFrame&
    {
        return *_current_frame;
    }

    auto IceGfxRunner::task_setup_gfx_traits() noexcept -> ice::Task<>
    {
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            entry.trait->gfx_context_setup(*_device, _context);
        }
        co_return;
    }

    auto IceGfxRunner::task_cleanup_gfx_traits() noexcept -> ice::Task<>
    {
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            entry.trait->gfx_context_cleanup(*_device, _context);
        }
        co_return;
    }

    auto IceGfxRunner::task_frame(
        ice::EngineFrame const& engine_frame,
        ice::UniquePtr<ice::gfx::IceGfxFrame> frame
    ) noexcept -> ice::Task<>
    {
        using ice::render::QueueFlags;

        ice::memory::StackAllocator<1024> initial_alloc;
        ice::memory::ScratchAllocator alloc{ initial_alloc, 1024 };

        IPT_FRAME_MARK_NAMED("Graphics Frame");

        // Await the graphics thread context
        co_await *_thread;

        IPT_ZONE_SCOPED_NAMED("Graphis Frame");


        // NOTE: We aquire the next image index to be rendered.
        ice::u32 const image_index = _device->next_frame();

        // NOTE: We aquire the graphics queues for this frame index.
        ice::gfx::IceGfxQueueGroup& queue_group = _device->queue_group(image_index);
        queue_group.reset_all();

        for (auto* fence : _fences)
        {
            fence->reset();
        }

        {
            IPT_ZONE_SCOPED_NAMED("Graphis Frame - Initial Tasks");

            // Secondly call all GfxTrait::gfx_update methods
            for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
            {
                entry.trait->gfx_update(engine_frame, *_device, _context, *frame);
            }

            // First run all tasks that where scheduled from previous frames
            frame->resume_on_start_stage();
        }

        {
            IPT_ZONE_SCOPED_NAMED("Graphis Frame - Command recording and execution");

            // Find operations
            ice::pod::Array<ice::StringID_Hash> queue_names{ alloc };
            queue_group.query_queues(queue_names);
            ice::pod::Array<GfxCmdOperation*> queue_operations{ alloc };
            ice::pod::array::resize(queue_operations, ice::pod::array::size(queue_names));
            frame->query_operations(queue_names, queue_operations);

            // Find whole executors
            ice::UniquePtr<IceGfxPassExecutor> transfer_executor = ice::gfx::create_pass_executor(
                _allocator,
                *_fences[0],
                QueueFlags::Transfer,
                queue_group,
                *frame,
                queue_names,
                queue_operations
            );

            ice::UniquePtr<IceGfxPassExecutor> graphics_executor = ice::gfx::create_pass_executor(
                _allocator,
                *_fences[1],
                QueueFlags::Graphics,
                queue_group,
                *frame,
                queue_names,
                queue_operations
            );

            if (transfer_executor != nullptr)
            {
                transfer_executor->record(engine_frame, _device->device().get_commands());
                transfer_executor->execute();
            }

            if (graphics_executor != nullptr)
            {
                graphics_executor->record(engine_frame, _device->device().get_commands());
            }

            if (transfer_executor != nullptr)
            {
                _fences[0]->wait(100'000'000);
            }

            if (graphics_executor != nullptr)
            {
                graphics_executor->execute();
                _fences[1]->wait(100'000'000);
            }
        }

        {
            IPT_ZONE_SCOPED_NAMED("Graphis Frame - Final tasks");
            frame->resume_on_end_stage();
        }

        // Start the draw task and
        {
            IPT_ZONE_SCOPED_NAMED("Graphis Frame - Present");
            _device->present(image_index);
        }
        co_return;
    }

} // namespace ice::gfx
