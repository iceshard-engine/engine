#pragma once
#include <iceshard/renderer/render_pipeline.hxx>

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
        uint32_t binding;
        RenderResourceType type;
        RenderResourceHandle handle;
    };

} // namespace iceshard::renderer
