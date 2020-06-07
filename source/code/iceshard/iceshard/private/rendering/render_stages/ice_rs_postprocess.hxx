#pragma once
#include "ice_render_stage.hxx"

namespace iceshard
{

    class IceRS_PostProcess : public IceRenderStage
    {
    public:
        IceRS_PostProcess(core::allocator& alloc) noexcept;

        auto name() const noexcept -> core::stringid_type override
        {
            return "isr.render-stage.post-process"_sid;
        }

        void on_prepare(
            iceshard::Engine& engine,
            iceshard::RenderPass& render_pass
        ) noexcept override;

        void on_cleanup(
            iceshard::Engine& engine,
            iceshard::RenderPass& render_pass
        ) noexcept override;

        void on_execute(
            iceshard::Frame& current,
            iceshard::RenderPass& render_pass
        ) noexcept override;

    private:
        static constexpr auto PostProcessResourceSetName = "iceshard.post-process.rset-0"_sid;
        static constexpr auto PostProcessPipelineName = "iceshard.post-process.pipeline"_sid;

        iceshard::renderer::api::Pipeline _pipeline;
        iceshard::renderer::api::ResourceSet _resources;
        iceshard::renderer::api::Buffer _buffers[2];
        iceshard::renderer::api::CommandBuffer _command_buffer;
    };

} // namespace iceshard
