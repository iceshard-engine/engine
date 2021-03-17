#pragma once
#include <ice/span.hxx>

namespace ice::render
{

    enum class ShaderStageFlags : ice::u32;

    enum class ResourceType
    {
        Sampler,
        SamplerImmutable,
        SampledImage,
        CombinedImageSampler,
        UniformBuffer,
    };

    struct ResourceSetLayoutBinding
    {
        ice::u32 binding_index;
        ice::u32 resource_count = 1;
        ice::render::ResourceType resource_type;
        ice::render::ShaderStageFlags shader_stage_flags;
    };

    enum class ResourceSetLayout : ice::uptr
    {
        Invalid = 0x0
    };

    enum class ResourceSet : ice::uptr
    {
        Invalid = 0x0
    };

} // namespace ice::render
