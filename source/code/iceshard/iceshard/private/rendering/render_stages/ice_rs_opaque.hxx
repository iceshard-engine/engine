#pragma once
#include "ice_render_stage.hxx"

namespace iceshard
{

    class IceRS_Opaque : public IceRenderStage
    {
    public:
        IceRS_Opaque(core::allocator& alloc) noexcept;

        auto name() const noexcept -> core::stringid_type override
        {
            return "isr.render-stage.opaque"_sid;
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
        iceshard::renderer::api::CommandBuffer _command_buffer;
    };

} // namespace iceshard