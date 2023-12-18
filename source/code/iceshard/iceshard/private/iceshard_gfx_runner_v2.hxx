#pragma once
#include "gfx/iceshard_gfx_device.hxx"
#include <ice/shard_container.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_render_graph_runtime.hxx>
#include <ice/task_thread.hxx>

namespace ice::gfx::v2
{

    enum class IceshardGfxRunnerState : ice::u8
    {
        Fresh,
        Resumed,
        SwapchainDirty,
        Suspended,
        Shutdown,
    };

    enum class IceshardGfxWorldState : ice::u8
    {
        Ignored,
        Inactive,
        Active,
    };

    struct IceshardGfxRunner : public ice::gfx::v2::GfxRunner, public ice::gfx::v3::GfxStageRegistry
    {
        IceshardGfxRunner(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::gfx::GfxDevice> gfx_device,
            ice::gfx::v2::GfxRunnerCreateInfo const& create_info
        ) noexcept;
        ~IceshardGfxRunner() noexcept override;

        void on_resume() noexcept override;

        void update_states(
            ice::ShardContainer const& shards,
            ice::gfx::v2::GfxStateChange const& gfx_state,
            ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks
        ) noexcept;

        auto draw_frame(
            ice::EngineFrame const& frame,
            ice::gfx::v3::GfxGraphRuntime& graph_runtime,
            ice::Clock const& clock
        ) noexcept -> ice::Task<> override;

        void on_suspend() noexcept override;

        auto device() noexcept -> ice::gfx::GfxDevice& override;

        void destroy() noexcept;

    public:
        void add_stage(ice::StringID_Arg name, v3::GfxStage const* stage) noexcept override;

        void execute_stages(
            ice::EngineFrame const& frame,
            ice::StringID_Arg name,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept override;

    private:
        ice::Allocator& _alloc;
        ice::Engine& _engine;
        ice::UniquePtr<ice::gfx::GfxDevice> _device;
        ice::ManualResetBarrier _barrier, _barrier_state;

        ice::TaskQueue _queue, _queue_transfer, _queue_end;
        ice::TaskScheduler _scheduler;
        ice::UniquePtr<ice::TaskThread> _thread;
        ice::render::RenderFence* _present_fence;

        ice::HashMap<v3::GfxStage const*> _stages;
        ice::gfx::v2::IceshardGfxRunnerState _state;
        ice::HashMap<ice::gfx::v2::IceshardGfxWorldState> _world_states;
    };

} // namespace ice::v2::gfx
