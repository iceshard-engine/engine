/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_declarations.hxx>

namespace ice::render
{

    enum class [[nodiscard]] ResourceSetLayout : ice::uptr
    {
        Invalid = 0x0
    };

    enum class [[nodiscard]] ResourceSet : ice::uptr
    {
        Invalid = 0x0
    };

    enum class ResourceType
    {
        Sampler,
        SamplerImmutable,
        SampledImage,
        CombinedImageSampler,
        UniformBuffer,
        InputAttachment,
    };

    struct ResourceSetLayoutBinding
    {
        ice::u32 binding_index;
        ice::u32 resource_count = 1;
        ice::render::ResourceType resource_type;
        ice::render::ShaderStageFlags shader_stage_flags;
    };

    struct ResourceBufferInfo
    {
        ice::render::Buffer buffer;
        ice::u32 offset;
        ice::u32 size;
    };

    struct ResourceUpdateInfo
    {
        union
        {
            ice::render::ResourceBufferInfo uniform_buffer;
            ice::render::Image image;
            ice::render::Sampler sampler;
        };
    };

    struct ResourceSetUpdateInfo
    {
        ice::render::ResourceSet resource_set;
        ice::render::ResourceType resource_type;
        ice::u32 binding_index;
        ice::u32 array_element = 0;
        ice::Span<ice::render::ResourceUpdateInfo const> resources;
    };

} // namespace ice::render
