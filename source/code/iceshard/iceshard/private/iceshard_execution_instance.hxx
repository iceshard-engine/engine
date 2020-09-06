#pragma once
#include <iceshard/engine.hxx>
#include <iceshard/execution.hxx>
#include <iceshard/input/input_event.hxx>
#include <iceshard/input/input_state_manager.hxx>

#include "iceshard_service_provider.hxx"
#include "rendering/ice_render_system.hxx"

#include "systems/iceshard_mesh_loader.hxx"
#include "systems/iceshard_input_actions.hxx"

namespace iceshard
{

    class MemoryFrame;

    struct ExecutionLock
    {
        ~ExecutionLock() noexcept;
        ExecutionLock(ExecutionInstance**) noexcept;

        ExecutionLock(ExecutionLock&& lock) noexcept;
        auto operator=(ExecutionLock&& lock) noexcept -> ExecutionLock&;

        ExecutionLock(ExecutionLock const& lock) noexcept = delete;
        auto operator=(ExecutionLock const& lock) noexcept -> ExecutionLock& = delete;

        ExecutionInstance** _lock_reference{ nullptr };
    };

    class IceshardExecutionInstance : public ExecutionInstance
    {
    public:
        IceshardExecutionInstance(
            core::allocator& alloc,
            ExecutionLock lock,
            Engine& engine,
            IceshardServiceProvider& services
        ) noexcept;
        ~IceshardExecutionInstance() noexcept;

        auto input_actions() noexcept -> ActionSystem& override;

        auto input_actions() const noexcept -> InputActionsTracker const&
        {
            return _input_actions_tracker;
        }

        auto engine_clock() const noexcept -> core::Clock const& override;

        auto previous_frame() const noexcept -> Frame const& override;

        auto current_frame() noexcept -> Frame& override;

        void next_frame() noexcept override;

        void add_task(cppcoro::task<> task) noexcept;

    private:
        core::allocator& _allocator;
        iceshard::ExecutionLock _lock;
        iceshard::Engine& _engine;
        iceshard::IceshardServiceProvider& _services;

        core::SystemClock _engine_clock;

        core::memory::unique_pointer<iceshard::MeshLoader> _mesh_loader{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<iceshard::IceRenderSystem> _render_system{ nullptr, { core::memory::globals::null_allocator() } };

        iceshard::input::DeviceStateManager _device_input_states;
        iceshard::InputActionsTracker _input_actions_tracker;

        // Tasks to be run this frame.
        size_t _task_list_index = 0;
        std::vector<cppcoro::task<>> _frame_tasks[2]{ {}, {} };

        std::atomic<std::vector<cppcoro::task<>>*> _mutable_task_list = nullptr;

        // Frame allocators.
        uint32_t _next_free_allocator = 0;

        core::memory::scratch_allocator _frame_allocator;
        core::memory::scratch_allocator _frame_data_allocator[2];

        // Frames.
        core::memory::unique_pointer<MemoryFrame> _previous_frame;
        core::memory::unique_pointer<MemoryFrame> _current_frame;
    };

} // namespace iceshard
