#pragma once
#include <core/base.hxx>

namespace iceshard::renderer
{

    struct ViewportExtents
    {
        uint32_t width;
        uint32_t height;
    };

    class RenderViewport
    {
    public:
        virtual ~RenderViewport() noexcept = default;

        virtual auto extents() const noexcept -> ViewportExtents = 0;

        virtual bool update_viewport(ViewportExtents extents) const noexcept = 0;
    };

} // namespace iceshard::renderer
