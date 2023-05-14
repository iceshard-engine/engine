/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_runner.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_frame.hxx"

#include "world/iceshard_world.hxx"
#include "world/iceshard_world_manager.hxx"

#include "gfx/iceshard_gfx_queue.hxx"

#include <ice/engine_shards.hxx>

#include <ice/gfx/gfx_trait.hxx>
#include <ice/gfx/gfx_queue.hxx>

#include <ice/task_utils.hxx>
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
        ice::TaskScheduler& task_scheduler,
        ice::UniquePtr<ice::input::InputTracker> input_tracker,
        ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
    ) noexcept
        : ice::EngineRunner{ }
        , _allocator{ alloc, "engine-runner" }
        , _clock{ ice::clock::create_clock() }
        , _engine{ engine }
        , _devui_key{ devui::execution_key_from_pointer(ice::addressof(_engine)) }
        , _gfx_world{ nullptr }
        , _gfx_runner{ ice::move(gfx_runner) }
        , _task_scheduler{ task_scheduler }
        , _frame_allocator{ _allocator, "frame-allocator" }
        , _frame_data_allocator{
            ice::RingAllocator{ _frame_allocator, "frame-alloc-1", { detail::FrameAllocatorCapacity } },
            ice::RingAllocator{ _frame_allocator, "frame-alloc-2", { detail::FrameAllocatorCapacity } }
        }
        , _next_free_allocator{ 0 }
        , _previous_frame{  }
        , _current_frame{ }
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
        _gfx_world = static_cast<ice::IceshardWorld*>(_gfx_runner->aquire_world());

        _world_tracker.set_managed_world(_gfx_world);
        _gfx_world->activate(_engine, *this);

        ice::array::reserve(_runner_tasks, 10);
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
        _mre_frame_logic.wait();

        set_graphics_runner({ });

        deactivate_worlds();

        _current_frame = nullptr;
        _previous_frame = nullptr;

        for (TraitTask& trait_task : _runner_tasks)
        {
            trait_task.coroutine.destroy();
            _allocator.destroy(trait_task.event);
        }
    }

    auto IceshardEngineRunner::clock() const noexcept -> ice::Clock const&
    {
        return _clock;
    }

    auto IceshardEngineRunner::entity_index() const noexcept -> ice::ecs::EntityIndex&
    {
        return _engine.entity_index();
    }

    auto IceshardEngineRunner::input_tracker() noexcept -> ice::input::InputTracker&
    {
        return *_input_tracker;
    }

    void IceshardEngineRunner::process_device_queue(
        ice::input::DeviceEventQueue const& device_queue
    ) noexcept
    {
        _input_tracker->process_device_queue(device_queue, _current_frame->input_events());

        ice::ShardContainer& shards = _current_frame->shards();
        for (ice::input::InputEvent input_event : _current_frame->input_events())
        {
            if (input_event.value_type == ice::input::InputValueType::Button)
            {
                ice::shards::push_back(
                    shards,
                    ice::Shard_InputEventButton | input_event
                );
            }
            else if (input_event.value_type == ice::input::InputValueType::AxisInt
                || input_event.value_type == ice::input::InputValueType::AxisFloat)
            {
                ice::shards::push_back(
                    shards,
                    ice::Shard_InputEventAxis | input_event
                );
            }
        }
    }

    auto IceshardEngineRunner::task_scheduler() noexcept -> ice::TaskScheduler&
    {
        return _task_scheduler;
    }

    auto IceshardEngineRunner::asset_storage() noexcept -> ice::AssetStorage&
    {
        return _engine.asset_storage();
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

        // Wait for graphics to finish before changing framebuffers
        if (ice::shards::contains(_current_frame->shards(), ice::platform::Shard_WindowResized))
        {
            _mre_graphics_frame.wait();
        }

        // Handle requests for the next frame
        for (ice::Shard const& shard : _previous_frame->shards())
        {
            ice::World* world;

            switch (shard.id.name.value)
            {
            case ice::Shard_WorldActivate.id.name.value:
            {
                if (ice::shard_inspect(shard, world))
                {
                    _world_tracker.activate_world(_engine, *this, static_cast<IceshardWorld*>(world));
                }
                break;
            }
            case ice::Shard_WorldDeactivate.id.name.value:
            {
                if (ice::shard_inspect(shard, world))
                {
                    _world_tracker.deactivate_world(_engine, *this, static_cast<IceshardWorld*>(world));
                }
                break;
            }
            default:
                break;
            }
        }

        // Handle all tasks for this 'next' frame
        for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(_queue_next_frame._awaitables))
        {
            awaitable->result.ptr = _current_frame.get();
            awaitable->_coro.resume();
        }

        // #todo: We need to remove entity operations beein processed by each world!
        _world_tracker.update_active_worlds(*this);

        // Clear operations before we can make more problems.
        _previous_frame->entity_operations().clear();

        // Always update the gfx world last.
        _gfx_world->update(*this);


        _current_frame->start_all();

        // Remove entities from the entity index
        // We do this after task have been scheduled on threads because we have now some time before they finish.
        {
            ice::ecs::EntityIndex& index = _engine.entity_index();

            // #todo: We need to move this before worlds update.
            //  This is required so the index will return 'is_alive == false'
            //  Currently even when the entity wont exist in a second it will still be seen as 'alive' until the next frame
            ice::shards::inspect_each<ice::ecs::EntityHandle>(
                _current_frame->shards(),
                ice::ecs::Shard_EntityDestroyed,
                [&index](ice::ecs::EntityHandle entity) noexcept
                {
                    ice::ecs::EntityHandleInfo const info = ice::ecs::entity_handle_info(entity);
                    index.destroy(info.entity);
                }
            );
        }

        ice::ManualResetEvent tasks_finished_event;
        ice::manual_wait_for(excute_frame_task(), tasks_finished_event);

        while (tasks_finished_event.is_set() == false)
        {
            // Handle all tasks for this 'next' frame
            for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(_queue_current_frame._awaitables))
            {
                awaitable->result.ptr = _current_frame.get();
                awaitable->_coro.resume();
            }
        }

        tasks_finished_event.wait();
        co_return;
    }

    auto IceshardEngineRunner::excute_frame_task() noexcept -> ice::Task<>
    {
        co_await task_scheduler();

        IPT_ZONE_SCOPED_NAMED("Logic Frame - Wait for tasks");
        _current_frame->wait_ready();
    }

    void IceshardEngineRunner::next_frame(ice::ShardContainer const& shards) noexcept
    {
        IPT_ZONE_SCOPED_NAMED("Runner - Next Frame");
        _mre_frame_logic.reset();

        ice::shards::push_back(_current_frame->shards(), shards._data);

        ice::manual_wait_for(logic_frame_task(), _mre_frame_logic);
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

        _frame_data_allocator[_next_free_allocator].reset();

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::count(_frame_data_allocator);

        remove_finished_tasks();
    }

    void IceshardEngineRunner::set_graphics_runner(
        ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
    ) noexcept
    {
        if (_gfx_runner != nullptr)
        {
            _mre_graphics_frame.wait();

            _gfx_world->deactivate(_engine, *this);
            _gfx_runner->release_world(_gfx_world);
            _world_tracker.unset_manager_world(_gfx_world);

            _gfx_world = nullptr;
            _gfx_runner->set_event(nullptr); // Probably not needed
            _gfx_runner = nullptr;
        }

        if (gfx_runner != nullptr)
        {
            _gfx_runner = ice::move(gfx_runner);
            _gfx_runner->set_event(&_mre_graphics_frame);

            _gfx_world = static_cast<ice::IceshardWorld*>(_gfx_runner->aquire_world());
            _world_tracker.set_managed_world(_gfx_world);
            _gfx_world->activate(_engine, *this);
        }
    }

    void IceshardEngineRunner::execute_task(ice::Task<> task, ice::EngineContext context) noexcept
    {
        switch (context)
        {
        case ice::EngineContext::EngineRunner:
        {
            ice::ManualResetEvent* wait_event = _allocator.create<ManualResetEvent>();
            detail::RunnerTask trait_task = [](ice::Task<void> task, ice::ManualResetEvent* event) noexcept -> detail::RunnerTask
            {
                co_await task;
                event->set();
            }(ice::move(task), wait_event);

            ice::array::push_back(
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
        default:
            ICE_ASSERT(false, "Unknown engine context value for task execution.");
            break;
        }
    }

    void IceshardEngineRunner::remove_finished_tasks() noexcept
    {
        ice::Array<TraitTask> running_tasks{ _allocator };

        for (TraitTask const& trait_task : _runner_tasks)
        {
            if (trait_task.event->is_set())
            {
                trait_task.coroutine.destroy();
                _allocator.destroy(trait_task.event);
            }
            else
            {
                ice::array::push_back(running_tasks, trait_task);
            }
        }

        _runner_tasks = ice::move(running_tasks);
    }

    auto IceshardEngineRunner::stage_current_frame() noexcept -> ice::TaskStage<ice::EngineFrame>
    {
        return { _queue_current_frame };
    }

    auto IceshardEngineRunner::stage_next_frame() noexcept -> ice::TaskStage<ice::EngineFrame>
    {
        return { _queue_next_frame };
    }

    void IceshardEngineRunner::activate_worlds() noexcept
    {
        for (IceshardWorld* world : _world_manager.worlds())
        {
            _world_tracker.activate_world(_engine, *this, world);
        }
    }

    void IceshardEngineRunner::deactivate_worlds() noexcept
    {
        for (IceshardWorld* world : _world_manager.worlds())
        {
            _world_tracker.deactivate_world(_engine, *this, world);
        }
    }

} // namespace ice
