#pragma once
#include <ice/engine_runner.hxx>
#include <ice/engine_devui.hxx>

#include <ice/task.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/sync_manual_events.hxx>

#include <ice/input/input_types.hxx>

#include <ice/render/render_driver.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_framebuffer.hxx>
#include <ice/render/render_pass.hxx>

#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>
#include <ice/memory/forward_allocator.hxx>
#include <ice/unique_ptr.hxx>

#include "world/iceshard_world_tracker.hxx"
#include <ice/gfx/gfx_runner.hxx>

#include "gfx/iceshard_gfx_device.hxx"
#include "gfx/iceshard_gfx_frame.hxx"

namespace ice
{

    class IceshardEngine;
    class IceshardWorldManager;
    class IceshardWorldTracker;

    class IceshardMemoryFrame;

    class IceshardEngineRunner final : public ice::EngineRunner
    {
    public:
        IceshardEngineRunner(
            ice::Allocator& alloc,
            ice::IceshardEngine& engine,
            ice::IceshardWorldManager& world_manager,
            ice::UniquePtr<ice::input::InputTracker> input_tracker,
            ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
        ) noexcept;
        ~IceshardEngineRunner() noexcept override;

        auto clock() const noexcept -> ice::Clock const& override;

        [[deprecated]]
        auto platform_events() noexcept -> ice::Span<ice::platform::Event const> override;

        auto input_tracker() noexcept -> ice::input::InputTracker& override;
        void process_device_queue(
            ice::input::DeviceQueue const& device_queue
        ) noexcept override;

        auto thread_pool() noexcept -> ice::TaskThreadPool& override;

        auto graphics_device() noexcept -> ice::gfx::GfxDevice& override;
        auto graphics_frame() noexcept -> ice::gfx::GfxFrame& override;

        auto previous_frame() const noexcept -> ice::EngineFrame const& override;
        auto current_frame() const noexcept -> ice::EngineFrame const& override;
        auto current_frame() noexcept -> ice::EngineFrame& override;
        void next_frame(
            ice::Span<ice::platform::Event const> events
        ) noexcept override;

        void set_graphics_runner(
            ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
        ) noexcept;

        auto logic_frame_task() noexcept -> ice::Task<>;
        auto excute_frame_task() noexcept -> ice::Task<>;

        void execute_task(ice::Task<> task, ice::EngineContext context) noexcept override;
        void remove_finished_tasks() noexcept;

        auto schedule_current_frame() noexcept -> ice::CurrentFrameOperation override;
        auto schedule_next_frame() noexcept -> ice::NextFrameOperation override;

    protected:
        void activate_worlds() noexcept;
        void deactivate_worlds() noexcept;

    private:
        void schedule_internal(
            ice::CurrentFrameOperationData& operation
        ) noexcept override;

        void schedule_internal(
            ice::NextFrameOperationData& operation
        ) noexcept override;

    private:
        ice::memory::ProxyAllocator _allocator;
        ice::SystemClock _clock;

        ice::IceshardEngine& _engine;
        ice::devui::DevUIExecutionKey const _devui_key;

        ice::IceshardWorld* _gfx_world;
        ice::UniquePtr<ice::gfx::GfxRunner> _gfx_runner;

        ice::UniquePtr<ice::TaskThreadPool> _thread_pool;

        ice::memory::ProxyAllocator _frame_allocator;
        ice::memory::ScratchAllocator _frame_data_allocator[2];
        ice::u32 _next_free_allocator;

        ice::UniquePtr<ice::IceshardMemoryFrame> _previous_frame;
        ice::UniquePtr<ice::IceshardMemoryFrame> _current_frame;

        ice::IceshardWorldManager& _world_manager;
        ice::IceshardWorldTracker _world_tracker;

        ice::Span<ice::platform::Event const> _events;
        ice::UniquePtr<ice::input::InputTracker> _input_tracker;

        ice::ManualResetEvent _mre_frame_logic = true;
        ice::ManualResetEvent _mre_graphics_frame = true;

        struct TraitTask
        {
            ice::ManualResetEvent* event;
            std::coroutine_handle<> coroutine;
        };
        ice::pod::Array<TraitTask> _runner_tasks;

        std::atomic<ice::CurrentFrameOperationData*> _current_op_head;
        std::atomic<ice::NextFrameOperationData*> _next_op_head;
    };

} // namespace ice
