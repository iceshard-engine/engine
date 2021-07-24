#include "iceshard_gfx_runner.hxx"
#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_world.hxx"

#include <ice/gfx/gfx_trait.hxx>
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
        , _mre_internal{ true }
        , _mre_selected{ &_mre_internal }
    {
        _current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator, _frame_allocator[1]
        );
    }

    IceGfxRunner::~IceGfxRunner() noexcept
    {
        _mre_selected->wait();

        // Deactivate Gfx World

        // Wait for the current GfxFrame to send final tasks
        _current_frame->execute_final_tasks();

        // Now stop the thread and wait for everything to finish.
        _thread->stop();
        _thread->join();
        _thread = nullptr;

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
        _mre_selected->wait();
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

    void IceGfxRunner::setup_graphics_world(ice::World* gfx_world) noexcept
    {
        _mre_selected->wait();
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            gfx_world->add_trait(entry.name, entry.trait);
        }
    }

    void IceGfxRunner::teardown_graphics_world(ice::World* gfx_world) noexcept
    {
        _mre_selected->wait();
        for (ice::gfx::IceGfxTraitEntry const& entry : _traits)
        {
            gfx_world->remove_trait(entry.name);
        }
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

    auto IceGfxRunner::task_frame(
        ice::EngineFrame const& engine_frame,
        ice::UniquePtr<ice::gfx::IceGfxFrame> frame
    ) noexcept -> ice::Task<>
    {
        IPT_FRAME_MARK_NAMED("Graphics Frame");

        // Await the graphics thread context
        co_await *_thread;

        IPT_ZONE_SCOPED_NAMED("Graphis Frame");

        // NOTE: We aquire the next image index to be rendered.
        ice::u32 const image_index = _device->next_frame();

        // NOTE: We aquire the graphics queues for this frame index.
        ice::gfx::IceGfxQueueGroup& queue_group = _device->queue_group(image_index);

        frame->resume_on_start_stage();
        frame->resume_on_commands_stage("default"_sid, queue_group.get_queue("default"_sid));
        frame->resume_on_end_stage();

        frame->execute_passes(engine_frame, queue_group);

        // Start the draw task and
        {
            IPT_ZONE_SCOPED_NAMED("Graphis Frame - Present");
            _device->present(image_index);
        }

        co_return;
    }

} // namespace ice::gfx
