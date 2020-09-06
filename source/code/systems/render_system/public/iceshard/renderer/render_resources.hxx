#pragma once
#include <iceshard/renderer/render_pipeline.hxx>

namespace iceshard::renderer
{

    enum class RenderResourceSetUsage : uint32_t
    {
        Invalid = 0x0,
        ViewProjectionData = 0x1,
        LightsData = 0x2,
        MaterialData = 0x4,
    };

    inline constexpr auto operator|(RenderResourceSetUsage left, RenderResourceSetUsage right) noexcept
    {
        return core::combine_flag(left, right);
    }

    enum class RenderResourceType : uint32_t
    {
        ResIgnored,
        ResUniform,
        ResSampler,
        ResTexture2D,
    };

    struct UniformBuffer
    {
        Buffer buffer;
        uint32_t range;
        uint32_t offset;
    };

    union RenderResourceHandle
    {
        Sampler sampler;
        Texture texture;
        UniformBuffer uniform;
    };

    struct RenderResourceSetInfo
    {
        RenderResourceSetUsage usage;
    };

    struct RenderResource
    {
        uint32_t binding;
        RenderResourceType type;
        RenderResourceHandle handle;
    };

} // namespace iceshard::renderer
