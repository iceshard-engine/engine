#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual void prepare() noexcept { }

        virtual auto renderpass(RenderPassType type) noexcept -> RenderPass = 0;
    };

} // namespace iceshard::renderer

