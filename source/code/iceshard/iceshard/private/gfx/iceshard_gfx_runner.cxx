#include "iceshard_gfx_runner.hxx"
#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_device.hxx"
#include "iceshard_gfx_world.hxx"

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
        //, _world{ ice::move(world) }
        , _frame_allocator{
            { _allocator, Constant_GfxFrameAllocatorCapacity },
            { _allocator, Constant_GfxFrameAllocatorCapacity }
        }
        , _next_free_allocator{ 0 }
        , _current_frame{ ice::make_unique_null<ice::gfx::IceGfxFrame>() }
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
        //ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame = ice::move(_current_frame);

        // Await the graphics thread context
        co_await *_thread;

        IPT_ZONE_SCOPED_NAMED("Graphis Frame");

        // Wait for the previous render task to end
        //_mre_gfx_draw.wait();
        //_mre_gfx_draw.reset();

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
        //ice::sync_manual_wait(render_frame_task(image_index, ice::move(gfx_frame)), _mre_gfx_draw);
        co_return;
    }

    void IceGfxRunner::add_trait(ice::gfx::GfxTrait* trait) noexcept
    {
        _mre_selected->wait(); // Always wait for the runner to have finished tasks.
    }

} // namespace ice::gfx
