#include "iceshard_runner.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_frame.hxx"

#include "world/iceshard_world.hxx"
#include "world/iceshard_world_manager.hxx"

#include "gfx/iceshard_gfx_queue.hxx"

#include <ice/engine_shards.hxx>

#include <ice/gfx/gfx_trait.hxx>
#include <ice/gfx/gfx_queue.hxx>

#include <ice/task_sync_wait.hxx>
#include <ice/input/input_tracker.hxx>
#include <ice/assert.hxx>

#include <ice/profiler.hxx>

namespace ice
{

    namespace devui
    {

        inline auto execution_key_from_pointer(void const* ptr) noexcept
        {
            return static_cast<DevUIExecutionKey>(static_cast<ice::u32>(reinterpret_cast<ice::uptr>(ptr) >> 16));
        }

    } // namespace devui

    namespace detail
    {

        static constexpr ice::u32 FrameAllocatorCapacity = 24u * 1024u * 1024u;
        static constexpr ice::u32 GfxFrameAllocatorCapacity = 16u * 1024u;

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
        ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
    ) noexcept
        : ice::EngineRunner{ }
        , _allocator{ alloc, "engine-runner" }
        , _clock{ ice::clock::create_clock() }
        , _engine{ engine }
        , _devui_key{ devui::execution_key_from_pointer(ice::addressof(_engine)) }
        , _gfx_world{ nullptr }
        , _gfx_trait_names{ _allocator }
        , _gfx_runner{ ice::move(gfx_runner) }
        , _thread_pool{ ice::create_simple_threadpool(_allocator, 6) }
        , _frame_allocator{ _allocator, "frame-allocator" }
        , _frame_data_allocator{
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity, "frame-alloc-1" },
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity, "frame-alloc-2" }
        }
        , _next_free_allocator{ 0 }
        , _previous_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _current_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _world_manager{ world_manager }
        , _world_tracker{ _allocator }
        , _input_tracker{ ice::move(input_tracker) }
        , _runner_tasks{ _allocator }
    {
        ICE_ASSERT(_gfx_runner != nullptr, "Currently every Engine Runner requires a valid Graphics Runner on creation.");

        _engine.developer_ui().internal_set_key(_devui_key);

        _previous_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[0]
        );
        _current_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[1]
        );

        _gfx_runner->set_event(&_mre_graphics_frame);
        _gfx_world = static_cast<ice::IceshardWorld*>(_world_manager.find_world(_gfx_runner->get_graphics_world()));
        ICE_ASSERT(
            _gfx_world != nullptr,
            "The request Graphics Runner world {} does not exist in World Manager.",
            ice::stringid_hint(_gfx_runner->get_graphics_world())
        );

        ice::pod::Array<ice::gfx::GfxTrait*> traits{ _allocator };

        ice::u32 const trait_count = _gfx_runner->trait_count();
        ice::pod::array::resize(_gfx_trait_names, trait_count);
        ice::pod::array::resize(traits, trait_count);
        _gfx_runner->query_traits(_gfx_trait_names, traits);

        for (ice::u32 idx = 0; idx < trait_count; ++idx)
        {
            _gfx_world->add_trait(_gfx_trait_names[idx], traits[idx]);
        }
        _world_tracker.activate_world(_engine, *this, _gfx_world);

        ice::pod::array::reserve(_runner_tasks, 10);
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
        for (TraitTask& trait_task : _runner_tasks)
        {
            trait_task.coroutine.destroy();
            _allocator.destroy(trait_task.event);
        }

        _mre_frame_logic.wait();

        set_graphics_runner(ice::make_unique_null<ice::gfx::GfxRunner>());

        deactivate_worlds();

        // First kill the gfx runner, so we dont care for any gfx data flying around.
        _gfx_runner = nullptr;
        _thread_pool = nullptr;
        _current_frame = nullptr;
        _previous_frame = nullptr;
    }

    auto IceshardEngineRunner::clock() const noexcept -> ice::Clock const&
    {
        return _clock;
    }

    auto IceshardEngineRunner::platform_events() noexcept -> ice::Span<ice::platform::Event const>
    {
        return _events;
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
        return _gfx_runner->device();
    }

    auto IceshardEngineRunner::graphics_frame() noexcept -> ice::gfx::GfxFrame&
    {
        return _gfx_runner->frame();
    }

    auto IceshardEngineRunner::previous_frame() const noexcept -> ice::EngineFrame const&
    {
        return *_previous_frame;
    }

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
        IPT_FRAME_MARK;
        IPT_ZONE_SCOPED_NAMED("Logic Frame");

        {
            ice::EntityIndex& index = _engine.entity_index();
            ice::Span<ice::Shard const> commands = _previous_frame->entity_commands().commands();

            ice::Entity entity;
            for (ice::Shard const& command : commands)
            {
                if (command == Shard_EntityDestroy && ice::shard_inspect(command, entity) && index.is_alive(entity))
                {
                    index.destroy(entity);

                    ice::Shard const shards[1]{ command >> Shard_EntityDestroyed };
                    _current_frame->push_shards(shards);
                }
            }
        }

        for (ice::platform::Event const& ev : _events)
        {
            if (ev.type == ice::platform::EventType::WindowSizeChanged)
            {
                // Force wait for the graphics frame to finish!
                _mre_graphics_frame.wait();

                ice::Shard const shards[1]{ ice::platform::Shard_WindowSizeChanged | ev.data.window.size };
                _current_frame->push_shards(shards);
            }
        }

        // Handle requests for the next frame
        for (ice::Shard const& shard : _previous_frame->shards())
        {
            ice::World* world;

            switch (shard.name)
            {
            case ice::Shard_WorldActivate.name:
            {
                if (ice::shard_inspect(shard, world))
                {
                    if (world != _gfx_world)
                    {
                        _world_tracker.activate_world(_engine, *this, static_cast<IceshardWorld*>(world));
                    }
                }
                break;
            }
            case ice::Shard_WorldDeactivate.name:
            {
                if (ice::shard_inspect(shard, world))
                {
                    if (world != _gfx_world)
                    {
                        _world_tracker.deactivate_world(_engine, *this, static_cast<IceshardWorld*>(world));
                    }
                }
                break;
            }
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
        co_return;
    }

    auto IceshardEngineRunner::excute_frame_task() noexcept -> ice::Task<>
    {
        co_await thread_pool();

        IPT_ZONE_SCOPED_NAMED("Logic Frame - Wait for tasks");
        _current_frame->wait_ready();
    }

    void IceshardEngineRunner::next_frame(
        ice::Span<ice::platform::Event const> events
    ) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("Runner - Next Frame");
        _mre_frame_logic.reset();

        _events = events;
        ice::sync_manual_wait(logic_frame_task(), _mre_frame_logic);
        _mre_frame_logic.wait();

        // Wait for the graphics runner to set this event, we know that it finished it's work.
        _mre_graphics_frame.wait();

        {
            IPT_ZONE_SCOPED_NAMED("Runner Frame - Build Developer UI");
            _engine.developer_ui().internal_build_widgets(*_current_frame, _devui_key);
        }

        // Move the current frame to the 'previous' slot.
        _previous_frame = ice::move(_current_frame);

        _gfx_runner->draw_frame(*_previous_frame);

        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);

        remove_finished_tasks();
    }

    void IceshardEngineRunner::set_graphics_runner(
        ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
    ) noexcept
    {
        if (_gfx_runner != nullptr)
        {
            _mre_graphics_frame.wait();

            _world_tracker.deactivate_world(_engine, *this, _gfx_world);
            for (ice::StringID_Arg trait_name : _gfx_trait_names)
            {
                _gfx_world->remove_trait(trait_name);
            }

            _gfx_runner->set_event(nullptr); // Probably not needed
            _gfx_runner = nullptr;
        }

        if (gfx_runner != nullptr)
        {
            _gfx_runner = ice::move(gfx_runner);
            _gfx_runner->set_event(&_mre_graphics_frame);

            ice::u32 const trait_count = _gfx_runner->trait_count();

            ice::pod::Array<ice::gfx::GfxTrait*> traits{ _allocator };
            ice::pod::array::resize(_gfx_trait_names, trait_count);
            ice::pod::array::resize(traits, trait_count);
            _gfx_runner->query_traits(_gfx_trait_names, traits);

            for (ice::u32 idx = 0; idx < trait_count; ++idx)
            {
                _gfx_world->add_trait(_gfx_trait_names[idx], traits[idx]);
            }

            _world_tracker.activate_world(_engine, *this, _gfx_world);
        }
    }

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
            static_cast<ice::gfx::IceGfxFrame&>(graphics_frame()).add_task(ice::move(task));
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
