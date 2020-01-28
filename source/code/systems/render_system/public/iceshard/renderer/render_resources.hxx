#pragma once
#include <iceshard/renderer/render_types.hxx>

namespace iceshard::renderer
{

    enum class RenderResourceType : uint32_t
    {
        ResIgnored,
        ResUniform,
        ResSampler,
        ResTexture2D,
    };

    union RenderResourceHandle
    {
        Sampler sampler;
        Texture texture;
    };

    struct RenderResource
    {
        RenderResourceType type;
        RenderResourceHandle handle;
    };

} // namespace iceshard::renderer
