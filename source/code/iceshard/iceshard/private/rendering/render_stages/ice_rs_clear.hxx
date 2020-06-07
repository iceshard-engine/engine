#pragma once
#include "ice_render_stage.hxx"

namespace iceshard
{

    class IceRS_Clear : public IceRenderStage
    {
    public:
        IceRS_Clear(core::allocator& alloc) noexcept;

        auto name() const noexcept -> core::stringid_type override
        {
            return "isr.render-stage.clear"_sid;
        }

        void on_prepare(
            iceshard::Engine& engine,
            iceshard::RenderPass& render_pass
        ) noexcept override;

        void on_cleanup(
            iceshard::Engine& engine,
            iceshard::RenderPass& render_pass
        ) noexcept override;
    };

} // namespace iceshard
