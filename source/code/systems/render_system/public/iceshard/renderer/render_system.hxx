#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto renderpass(RenderPassStage type) noexcept -> RenderPass = 0;
    };

} // namespace iceshard::renderer

