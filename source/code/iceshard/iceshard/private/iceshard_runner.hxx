#pragma once
#include <ice/engine_runner.hxx>

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
#include <ice/unique_ptr.hxx>

#include "world/iceshard_world_tracker.hxx"

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
            ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device
        ) noexcept;
        ~IceshardEngineRunner() noexcept override;

        auto clock() const noexcept -> ice::Clock const& override;

        auto input_tracker() noexcept -> ice::input::InputTracker& override;
        void process_device_queue(
            ice::input::DeviceQueue const& device_queue
        ) noexcept override;

        auto thread_pool() noexcept -> ice::TaskThreadPool& override;

        auto graphics_device() noexcept -> ice::gfx::GfxDevice& override;
        auto graphics_frame() noexcept -> ice::gfx::GfxFrame& override;

        //auto graphics_frame() noexcept -> ice::gfx::GfxScheduleFrameOperation override;

        //auto previous_frame() const noexcept -> EngineFrame const& override;
        auto current_frame() const noexcept -> EngineFrame const& override;
        auto current_frame() noexcept -> EngineFrame& override;
        void next_frame() noexcept override;

        auto logic_frame_task() noexcept -> ice::Task<>;
        auto excute_frame_task() noexcept -> ice::Task<>;

        auto graphics_frame_task() noexcept -> ice::Task<>;
        auto render_frame_task(
            ice::u32 framebuffer_index,
            ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame
        ) noexcept -> ice::Task<>;

        //auto graphics_task(
        //    ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame,
        //    ice::ManualResetEvent* reset_event
        //) noexcept -> ice::Task<>;

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

        ice::UniquePtr<ice::TaskThreadPool> _thread_pool;
        ice::UniquePtr<ice::TaskThread> _graphics_thread;
        ice::ManualResetEvent _graphics_thread_event = true;

        ice::memory::ProxyAllocator _frame_allocator;
        ice::memory::ScratchAllocator _frame_data_allocator[2];
        ice::memory::ProxyAllocator _frame_gfx_allocator[2];
        ice::u32 _next_free_allocator;

        ice::UniquePtr<ice::IceshardMemoryFrame> _previous_frame;
        ice::UniquePtr<ice::IceshardMemoryFrame> _current_frame;

        ice::IceshardWorldManager& _world_manager;
        ice::IceshardWorldTracker _world_tracker;

        ice::UniquePtr<ice::input::InputTracker> _input_tracker;

        ice::UniquePtr<ice::gfx::IceGfxDevice> _gfx_device;
        ice::UniquePtr<ice::gfx::IceGfxFrame> _gfx_current_frame;

        ice::ManualResetEvent _mre_frame_start = true;
        ice::ManualResetEvent _mre_frame_logic = true;
        ice::ManualResetEvent _mre_gfx_commands = true;
        ice::ManualResetEvent _mre_gfx_draw = true;

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
