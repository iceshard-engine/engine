#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto viewport() noexcept -> RenderViewport& = 0;

        virtual auto renderpass() noexcept -> RenderPassHandle = 0;
    };

} // namespace iceshard::renderer

