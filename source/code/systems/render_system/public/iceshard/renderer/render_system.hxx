#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        [[not_null]]
        virtual auto viewport() noexcept -> RenderViewport* = 0;
    };

} // namespace iceshard::renderer

