#include "iceshard_execution_instance.hxx"
#include "iceshard_frame.hxx"

#include "systems/iceshard_camera_system.hxx"
#include "systems/iceshard_lights_system.hxx"
#include "systems/iceshard_static_mesh_renderer.hxx"

#include "world/iceshard_world_manager.hxx"
#include "world/iceshard_world.hxx"

#include <core/pod/algorithm.hxx>

#include <cppcoro/sync_wait.hpp>
#include <cppcoro/when_all_ready.hpp>

#include <functional>

namespace iceshard
{

    namespace detail
    {

        static constexpr auto FrameAllocatorCapacity = 256u * 1024u * 1024u;

    } // namespace detail

    ExecutionLock::ExecutionLock(ExecutionInstance** lock) noexcept
        : _lock_reference{ lock }
    {
    }

    ExecutionLock::~ExecutionLock() noexcept
    {
        if (_lock_reference != nullptr)
        {
            *_lock_reference = nullptr;
        }
    }

    ExecutionLock::ExecutionLock(ExecutionLock&& lock) noexcept
        : _lock_reference{ std::exchange(lock._lock_reference, nullptr) }
    {
    }

    auto ExecutionLock::operator=(ExecutionLock&& lock) noexcept -> ExecutionLock&
    {
        _lock_reference = std::exchange(lock._lock_reference, nullptr);
        return *this;
    }

    IceshardExecutionInstance::IceshardExecutionInstance(
        core::allocator& alloc,
        ExecutionLock lock,
        Engine& engine,
        IceshardServiceProvider& services
    ) noexcept
        : _allocator{ alloc }
        , _lock{ std::move(lock) }
        , _engine{ engine }
        , _services{ services }
        , _engine_clock{ core::clock::create_clock() }
        , _render_system{ nullptr, { _allocator } }
        , _device_input_states{ _allocator, _engine_clock }
        , _input_actions_tracker{ _allocator, _engine_clock }
        , _mutable_task_list{ &_frame_tasks[_task_list_index] }
        // Frame related fields
        , _frame_allocator{ _allocator, sizeof(MemoryFrame) * 5 }
        , _frame_data_allocator{ { _allocator, detail::FrameAllocatorCapacity }, { _allocator, detail::FrameAllocatorCapacity } }
        , _current_frame{ nullptr, { _allocator } }
        , _previous_frame{ nullptr, { _allocator } }
    {
        core::clock::update(_engine_clock);

        _previous_frame = core::memory::make_unique<MemoryFrame>(
            _frame_allocator,
            _frame_data_allocator[0],
            _engine,
            *this
        );
        _current_frame = core::memory::make_unique<MemoryFrame>(
            _frame_allocator,
            _frame_data_allocator[1],
            _engine,
            *this
        );

        _current_frame->input_queue().push(
            iceshard::input::create_device_handle(0, iceshard::input::DeviceType::Mouse),
            iceshard::input::DeviceInputType::DeviceConnected
        );
        _current_frame->input_queue().push(
            iceshard::input::create_device_handle(0, iceshard::input::DeviceType::Keyboard),
            iceshard::input::DeviceInputType::DeviceConnected
        );

        _render_system = core::memory::make_unique<iceshard::IceRenderSystem>(
            _allocator,
            _allocator,
            _engine
        );

        // Component systems
        _services.add_system(
            IceshardCameraSystem::SystemName,
            _allocator.make<IceshardCameraSystem>(
                _allocator, _engine,
                *_services.archetype_index(),
                _engine.input_system(),
                _engine.render_system()
            )
        );

        _services.add_system(
            IceshardIlluminationSystem::SystemName,
            _allocator.make<IceshardIlluminationSystem>(
                _allocator, _engine,
                *_services.archetype_index(),
                _engine.render_system()
            )
        );

        _services.add_system(
            IceshardStaticMeshRenderer::SystemName,
            _allocator.make<IceshardStaticMeshRenderer>(
                _allocator, _engine,
                *_services.archetype_index(),
                _engine.render_system(),
                _engine.asset_system()
            )
        );

        _device_input_states.handle_device_inputs(
            _current_frame->input_queue(),
            _current_frame->input_events()
        );

        core::clock::update(_engine_clock);
    }

    IceshardExecutionInstance::~IceshardExecutionInstance() noexcept
    {
        _allocator.destroy(_services.remove_system(IceshardStaticMeshRenderer::SystemName));
        _allocator.destroy(_services.remove_system(IceshardIlluminationSystem::SystemName));
        _allocator.destroy(_services.remove_system(IceshardCameraSystem::SystemName));

        _frame_tasks[0].clear();
        _frame_tasks[1].clear();

        _current_frame = nullptr;
        _previous_frame = nullptr;
    }

    auto IceshardExecutionInstance::input_actions() noexcept -> ActionSystem&
    {
        return _input_actions_tracker;
    }

    auto IceshardExecutionInstance::engine_clock() const noexcept -> core::Clock const&
    {
        return _engine_clock;
    }

    auto IceshardExecutionInstance::previous_frame() const noexcept -> const Frame&
    {
        return *_previous_frame;
    }

    auto IceshardExecutionInstance::current_frame() noexcept -> Frame&
    {
        return *_current_frame;
    }

    void IceshardExecutionInstance::next_frame() noexcept
    {
        _services.for_each_system([this](ComponentSystem& system) noexcept
            {
                system.end_frame(*_current_frame, *_previous_frame);
            });

        {
            _task_list_index += 1;
            std::vector<cppcoro::task<>>* expected_list = &_frame_tasks[(_task_list_index - 1) % 2];
            std::vector<cppcoro::task<>>* exchange_list = &_frame_tasks[_task_list_index % 2];

            while (_mutable_task_list.compare_exchange_weak(expected_list, exchange_list) == false)
            {
                expected_list = &_frame_tasks[(_task_list_index - 1) % 2];
            }

            cppcoro::sync_wait(cppcoro::when_all_ready(std::move(*expected_list)));
        }

        _render_system->execute_passes(
            *_current_frame, *_previous_frame
        );

        _render_system->end_frame();

        // Move the current frame to the 'previous' slot.
        _previous_frame = std::move(_current_frame);

        // Reset the frame allocator inner pointers.
        [[maybe_unused]] const bool successful_reset = _frame_data_allocator[_next_free_allocator].reset();
        IS_ASSERT(successful_reset == true, "Memory was discarded during frame allocator reset!");

        core::clock::update(_engine_clock);

        _current_frame = core::memory::make_unique<MemoryFrame>(
            _frame_allocator,
            _frame_data_allocator[_next_free_allocator],
            _engine,
            *this
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= std::size(_frame_data_allocator);

        // Now we want to get all messages for the current frame.
        _engine.input_system().query_events(
            _current_frame->messages(),
            _current_frame->input_queue()
        );

        _device_input_states.handle_device_inputs(
            _current_frame->input_queue(),
            _current_frame->input_events()
        );

        _input_actions_tracker.update_actions(*_current_frame, _current_frame->input_actions());

        // Update all worlds
        static_cast<iceshard::IceshardWorldManager&>(_engine.world_manager())
            .foreach_world([this](IceshardWorld& world) noexcept
                {
                    world.update(*this);
                });

        // Update all global component systems
        _render_system->begin_frame();

        _services.for_each_system([this](ComponentSystem& system) noexcept
            {
                system.update(*_current_frame, *_previous_frame);
            });
    }

    void IceshardExecutionInstance::add_task(cppcoro::task<> task) noexcept
    {
        std::vector<cppcoro::task<>>* expected_list = &_frame_tasks[_task_list_index % 2];
        {
            while (_mutable_task_list.compare_exchange_weak(expected_list, nullptr) == false)
            {
                expected_list = &_frame_tasks[_task_list_index % 2];
            }
        }

        expected_list->push_back(std::move(task));

        _mutable_task_list.store(expected_list);
    }

}
