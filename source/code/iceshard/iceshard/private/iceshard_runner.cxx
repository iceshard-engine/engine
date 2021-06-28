#include "iceshard_runner.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_frame.hxx"

#include "world/iceshard_world.hxx"
#include "world/iceshard_world_manager.hxx"

#include "gfx/iceshard_gfx_pass.hxx"
#include "gfx/iceshard_gfx_queue.hxx"

#include <ice/gfx/gfx_queue.hxx>

#include <ice/task_sync_wait.hxx>
#include <ice/input/input_tracker.hxx>
#include <ice/assert.hxx>


namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 FrameAllocatorCapacity = 256u * 1024u * 1024u;
        static constexpr ice::u32 GfxFrameAllocatorCapacity = 16u * 1024u * 1024u;

        struct RunnerTask
        {
            std::coroutine_handle<> _handle;

            RunnerTask(std::coroutine_handle<> handle) noexcept
                : _handle{ handle }
            {
            }

            struct promise_type
            {
                auto initial_suspend() const noexcept { return std::suspend_never{ }; }
                auto final_suspend() const noexcept { return std::suspend_always{ }; }
                auto return_void() noexcept { }

                auto get_return_object() noexcept;
                void unhandled_exception() noexcept
                {
                    ICE_ASSERT(false, "Unhandled coroutine exception!");
                }
            };
        };

        auto RunnerTask::promise_type::get_return_object() noexcept
        {
            return RunnerTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

    } // namespace detail

    IceshardEngineRunner::IceshardEngineRunner(
        ice::Allocator& alloc,
        ice::IceshardEngine& engine,
        ice::IceshardWorldManager& world_manager,
        ice::UniquePtr<ice::input::InputTracker> input_tracker,
        ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device
    ) noexcept
        : ice::EngineRunner{ }
        , _allocator{ alloc, "engine-runner" }
        , _engine{ engine }
        , _clock{ ice::clock::create_clock() }
        , _thread_pool{ ice::create_simple_threadpool(_allocator, 4) }
        , _graphics_thread{ ice::create_task_thread(_allocator) }
        , _frame_allocator{ _allocator, "frame-allocator" }
        , _frame_data_allocator{
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity, "frame-alloc-1" },
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity, "frame-alloc-2" }
        }
        , _frame_gfx_allocator{
            ice::memory::ProxyAllocator{ _frame_allocator, "gfx-frame-alloc-1" },
            ice::memory::ProxyAllocator{ _frame_allocator, "gfx-frame-alloc-2" }
        }
        , _next_free_allocator{ 0 }
        , _previous_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _current_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _world_manager{ world_manager }
        , _world_tracker{ _allocator }
        , _input_tracker{ ice::move(input_tracker) }
        , _gfx_device{ ice::move(gfx_device) }
        , _gfx_current_frame{ ice::make_unique_null<ice::gfx::IceGfxFrame>() }
        , _runner_tasks{ _allocator }
    {
        _previous_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[0]
        );
        _current_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[1]
        );

        _gfx_current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator, _frame_gfx_allocator[1]
        );

        ice::pod::array::reserve(_runner_tasks, 10);

        activate_worlds();
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
        for (TraitTask& trait_task : _runner_tasks)
        {
            trait_task.coroutine.destroy();
            _allocator.destroy(trait_task.event);
        }

         _graphics_thread->stop();
         _graphics_thread->join();
         _graphics_thread = nullptr;
         _thread_pool = nullptr;

        deactivate_worlds();

        _gfx_current_frame = nullptr;

        _gfx_device = nullptr;
        _current_frame = nullptr;
        _previous_frame = nullptr;
    }

    auto IceshardEngineRunner::clock() const noexcept -> ice::Clock const&
    {
        return _clock;
    }

    auto IceshardEngineRunner::input_tracker() noexcept -> ice::input::InputTracker&
    {
        return *_input_tracker;
    }

    void IceshardEngineRunner::process_device_queue(
        ice::input::DeviceQueue const& device_queue
    ) noexcept
    {
        _input_tracker->process_device_queue(device_queue, _current_frame->input_events());
    }

    auto IceshardEngineRunner::thread_pool() noexcept -> ice::TaskThreadPool&
    {
        return *_thread_pool;
    }

    auto IceshardEngineRunner::graphics_device() noexcept -> ice::gfx::GfxDevice&
    {
        return *_gfx_device;
    }

    //auto IceshardEngineRunner::graphics_frame() noexcept -> ice::gfx::GfxScheduleFrameOperation
    //{
    //    return ice::gfx::GfxScheduleFrameOperation{ *_graphics_thread, *_gfx_current_frame };
    //}

    auto IceshardEngineRunner::graphics_frame() noexcept -> ice::gfx::GfxFrame&
    {
        return *_gfx_current_frame;
    }

    //auto IceshardEngineRunner::previous_frame() const noexcept -> EngineFrame const&
    //{
    //    return *_previous_frame;
    //}

    auto IceshardEngineRunner::current_frame() const noexcept -> EngineFrame const&
    {
        return *_current_frame;
    }

    auto IceshardEngineRunner::current_frame() noexcept -> EngineFrame&
    {
        return *_current_frame;
    }

    auto IceshardEngineRunner::logic_frame_task() noexcept -> ice::Task<>
    {
        //_mre_frame_start.wait();
        ////_mre_frame_logic.reset();
        //// TODO: Aquire the logic lock for the new current_frame

        //// NOTE: We can now safely access results from the previous frame and calculate logic for the next frame
        //_current_frame = ice::make_unique<IceshardMemoryFrame>(
        //    _allocator,
        //    _frame_data_allocator[_next_free_allocator]
        //);

        //// NOTE: We can update the allocator idex to be used next time
        //_next_free_allocator += 1;
        //_next_free_allocator %= ice::size(_frame_data_allocator);

        // Handle requests for the next frame
        for (ice::EngineRequest const& request : _previous_frame->requests())
        {
            switch (request.name.hash_value)
            {
            case ice::stringid_hash(Request_ActivateWorld):
                _world_tracker.activate_world(
                    _engine, *this,
                    static_cast<ice::IceshardWorld*>(
                        reinterpret_cast<ice::World*>(request.payload)
                    )
                );
                break;
            case ice::stringid_hash(Request_DeactivateWorld):
                _world_tracker.deactivate_world(
                    _engine, *this,
                    static_cast<ice::IceshardWorld*>(
                        reinterpret_cast<ice::World*>(request.payload)
                    )
                );
                break;
            default:
                break;
            }
        }

        // Handle all tasks for this 'next' frame
        {
            ice::NextFrameOperationData* operation_head = _next_op_head;
            while (_next_op_head.compare_exchange_weak(operation_head, nullptr, std::memory_order::relaxed, std::memory_order::acquire) == false)
            {
                continue;
            }

            ice::NextFrameOperationData* operation = operation_head;
            while (operation != nullptr)
            {
                ice::NextFrameOperationData* next_operation = operation->next;
                operation->frame = _current_frame.get();
                operation->coroutine.resume();
                operation = next_operation;
            }
        }

        _world_tracker.update_active_worlds(*this);


        _current_frame->start_all();

        ice::ManualResetEvent tasks_finished_event;
        ice::sync_manual_wait(excute_frame_task(), tasks_finished_event);

        while (tasks_finished_event.is_set() == false)
        {
            ice::CurrentFrameOperationData* operation_head = _current_op_head;
            while (_current_op_head.compare_exchange_weak(operation_head, nullptr, std::memory_order::relaxed, std::memory_order::acquire) == false)
            {
                continue;
            }

            ice::EngineTaskOperationBaseData* operation = operation_head;
            while (operation != nullptr)
            {
                ice::EngineTaskOperationBaseData* next_operation = operation->next;
                operation->coroutine.resume();
                operation = next_operation;
            }
        }

        tasks_finished_event.wait();


#if 1
        //_mre_gfx_commands.wait();
        //_mre_gfx_commands.reset();

        //_previous_frame = ice::move(_current_frame);

        //ice::sync_manual_wait(graphics_frame_task(), _mre_gfx_commands);

        //// Reset the frame allocator inner pointers.
        //[[maybe_unused]]
        //bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        //ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        //ice::clock::update(_clock);

        //_current_frame = ice::make_unique<IceshardMemoryFrame>(
        //    _allocator,
        //    _frame_data_allocator[_next_free_allocator]
        //);

        //_gfx_current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
        //    _allocator,
        //    _frame_gfx_allocator[_next_free_allocator]
        //);

        //// We need to update the allocator index
        //_next_free_allocator += 1;
        //_next_free_allocator %= ice::size(_frame_data_allocator);
#endif
        //// TODO: We wait for the graphics frame to be finished with command recording, as this will ensure that the previous frame is no longer required.

        //_previous_frame = ice::move(_current_frame);
        //// TODO: Notify that we finished calculating the current frame

        //// NOTE: We await resuming on the graphics thread
        //co_await *_graphics_thread;

        //// NOTE: We aquire the next image index to be rendered.
        //ice::u32 const image_index = _gfx_device->next_frame();

        //// NOTE: We aquire the graphics queues for this frame index.
        //ice::gfx::IceGfxQueueGroup& queue_group = _gfx_device->queue_group(image_index);

        //// TODO: We need to record command buffers here.

        //// TODO: We need to notify we finished command buffer recording.

        //// TODO: We need to submit all recorded command buffers here and wait for results.

        //// ?? TODO: Notify we finished graphics tasks
        co_return;
    }

    auto IceshardEngineRunner::excute_frame_task() noexcept -> ice::Task<>
    {
        co_await thread_pool();
        _current_frame->wait_ready();
    }

    auto IceshardEngineRunner::graphics_frame_task() noexcept -> ice::Task<>
    {
        ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame = ice::move(_gfx_current_frame);

        // Await the graphics thread context
        co_await *_graphics_thread;

        // Wait for the previous render task to end
        _mre_gfx_draw.wait();
        _mre_gfx_draw.reset();

        // NOTE: We aquire the next image index to be rendered.
        ice::u32 const image_index = _gfx_device->next_frame();

        // NOTE: We aquire the graphics queues for this frame index.
        ice::gfx::IceGfxQueueGroup& queue_group = _gfx_device->queue_group(image_index);

        gfx_frame->resume_on_start_stage();
        gfx_frame->resume_on_commands_stage("default"_sid, queue_group.get_queue("default"_sid));
        gfx_frame->resume_on_end_stage();

        gfx_frame->execute_passes(*_previous_frame, queue_group);

        // Start the draw task and
        ice::sync_manual_wait(render_frame_task(image_index, ice::move(gfx_frame)), _mre_gfx_draw);
        co_return;
    }

    auto IceshardEngineRunner::render_frame_task(
        ice::u32 framebuffer_index,
        ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame
    ) noexcept -> ice::Task<>
    {
        co_await thread_pool();

        // NOTE: We are presenting the resulting image.
        _gfx_device->present(framebuffer_index);
        co_return;
    }

    void IceshardEngineRunner::next_frame() noexcept
    {

#if 0

        // [issue #??] We cannot move this wait lower as for now we are still accessing a single GfxPass object from both the Gfx and Runtim threads.
        //_graphics_thread_event.wait();
        //_graphics_thread_event.reset();

        // Update all active worlds
        //_world_tracker.update_active_worlds(*this);
        _mre_frame_logic.wait();
        _mre_frame_logic.reset();

        // Move the current frame to the 'previous' slot.
        _previous_frame = ice::move(_current_frame);
        //_previous_frame->start_all();

        _mre_gfx_commands.wait();
        _mre_gfx_commands.reset();
        ice::sync_manual_wait(graphics_frame_task(), _mre_gfx_commands);

        // Start the graphics task
        //ice::sync_manual_wait(render_frame_task(), _mre_gfx_draw);

        //_graphics_thread->schedule(
        //    graphics_task(
        //        ice::move(_gfx_current_frame),
        //        &_graphics_thread_event
        //    )
        //);

        // Reset the frame allocator inner pointers.
        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        _gfx_current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator,
            _frame_gfx_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);

        ice::sync_manual_wait(logic_frame_task(), _mre_frame_logic);
#else
        _mre_frame_logic.reset();

        ice::sync_manual_wait(logic_frame_task(), _mre_frame_logic);
        _mre_frame_logic.wait();

        // Move the current frame to the 'previous' slot.
        _previous_frame = ice::move(_current_frame);

        _mre_gfx_commands.wait();
        _mre_gfx_commands.reset();
        ice::sync_manual_wait(graphics_frame_task(), _mre_gfx_commands);

        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        _gfx_current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator,
            _frame_gfx_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);

        remove_finished_tasks();
#endif

        //// Handle requests for the next frame
        //for (ice::EngineRequest const& request : _previous_frame->requests())
        //{
        //    switch (request.name.hash_value)
        //    {
        //    case ice::stringid_hash(Request_ActivateWorld):
        //        _world_tracker.activate_world(
        //            _engine, *this,
        //            static_cast<ice::IceshardWorld*>(
        //                reinterpret_cast<ice::World*>(request.payload)
        //            )
        //        );
        //        break;
        //    case ice::stringid_hash(Request_DeactivateWorld):
        //        _world_tracker.deactivate_world(
        //            _engine, *this,
        //            static_cast<ice::IceshardWorld*>(
        //                reinterpret_cast<ice::World*>(request.payload)
        //            )
        //        );
        //        break;
        //    default:
        //        break;
        //    }
        //}
    }

    //auto IceshardEngineRunner::graphics_task(
    //    ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame,
    //    ice::ManualResetEvent* reset_event
    //) noexcept -> ice::Task<>
    //{
    //    ice::u32 const image_index = _gfx_device->next_frame();
    //    ice::gfx::IceGfxQueueGroup& queue_group = _gfx_device->queue_group(image_index);

    //    gfx_frame->start_all();
    //    //gfx_frame->execute_passes(previous_frame(), queue_group);
    //    gfx_frame->wait_ready();
    //    _previous_frame->wait_ready();

    //    _gfx_device->present(image_index);

    //    reset_event->set();
    //    co_return;
    //}

    void IceshardEngineRunner::execute_task(ice::Task<> task, ice::EngineContext context) noexcept
    {
        switch (context)
        {
        case ice::EngineContext::EngineRunner:
        {
            ice::ManualResetEvent* wait_event = _allocator.make<ManualResetEvent>();
            detail::RunnerTask trait_task = [](ice::Task<void> task, ice::ManualResetEvent* event) noexcept -> detail::RunnerTask
            {
                co_await task;
                event->set();
            }(ice::move(task), wait_event);

            ice::pod::array::push_back(
                _runner_tasks,
                TraitTask{
                    .event = wait_event,
                    .coroutine = trait_task._handle
                }
            );
            break;
        }
        case ice::EngineContext::LogicFrame:
            _current_frame->execute_task(ice::move(task));
            break;
        case ice::EngineContext::GraphicsFrame:
            _gfx_current_frame->execute_task(ice::move(task));
            break;
        default:
            break;
        }
    }

    void IceshardEngineRunner::remove_finished_tasks() noexcept
    {
        ice::pod::Array<TraitTask> running_tasks{ _allocator };

        for (TraitTask const& trait_task : _runner_tasks)
        {
            if (trait_task.event->is_set())
            {
                trait_task.coroutine.destroy();
                _allocator.destroy(trait_task.event);
            }
            else
            {
                ice::pod::array::push_back(running_tasks, trait_task);
            }
        }

        _runner_tasks = ice::move(running_tasks);
    }

    auto IceshardEngineRunner::schedule_current_frame() noexcept -> ice::CurrentFrameOperation
    {
        return CurrentFrameOperation{ *this };
    }

    auto IceshardEngineRunner::schedule_next_frame() noexcept -> ice::NextFrameOperation
    {
        return NextFrameOperation{ *this };
    }

    void IceshardEngineRunner::schedule_internal(
        ice::CurrentFrameOperationData& operation
    ) noexcept
    {
        ice::CurrentFrameOperationData* expected_head = _current_op_head.load(std::memory_order_acquire);

        do
        {
            operation.next = expected_head;
        }
        while (
            _current_op_head.compare_exchange_weak(
                expected_head,
                &operation,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceshardEngineRunner::schedule_internal(
        ice::NextFrameOperationData& operation
    ) noexcept
    {
        ice::NextFrameOperationData* expected_head = _next_op_head.load(std::memory_order_acquire);

        do
        {
            operation.next = expected_head;
        }
        while (
            _next_op_head.compare_exchange_weak(
                expected_head,
                &operation,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceshardEngineRunner::activate_worlds() noexcept
    {
        for (auto const& entry : _world_manager.worlds())
        {
            _world_tracker.activate_world(_engine, *this, entry.value);
        }
    }

    void IceshardEngineRunner::deactivate_worlds() noexcept
    {
        for (auto const& entry : _world_manager.worlds())
        {
            _world_tracker.deactivate_world(_engine, *this, entry.value);
        }
    }

} // namespace ice
