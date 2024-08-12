/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webgpu_utils.hxx"
#include <ice/render/render_resource.hxx>
#include <ice/assert.hxx>

namespace ice::render::webgpu
{

    struct WebGPUResourceSet
    {
        WGPUBindGroupLayout _wgpu_group_layout;
        WGPUBindGroup _wgpu_group;

        static auto handle(WebGPUResourceSet* native) noexcept
        {
            return static_cast<ice::render::ResourceSet>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::ResourceSet handle) noexcept
        {
            return reinterpret_cast<WebGPUResourceSet*>(static_cast<uintptr_t>(handle));
        }

        // Layout
        static auto handle(WGPUBindGroupLayout native) noexcept
        {
            return static_cast<ice::render::ResourceSetLayout>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::ResourceSetLayout handle) noexcept
        {
            return reinterpret_cast<WGPUBindGroupLayout>(static_cast<uintptr_t>(handle));
        }
    };

    inline auto native_shader_stages(ice::render::ShaderStageFlags flags) noexcept -> WGPUShaderStageFlags
    {
        bool const unsupported_flags = ice::has_any(flags, ~(ShaderStageFlags::FragmentStage | ShaderStageFlags::VertexStage));
        ICE_ASSERT(unsupported_flags == false, "WebGPU rendered only supports vertex, fragment and compute stages.");

        WGPUShaderStageFlags result = WGPUShaderStage_None;
        if (ice::has_all(flags, ShaderStageFlags::VertexStage))
        {
            result = WGPUShaderStageFlags(result | WGPUShaderStage_Vertex);
        }
        if (ice::has_all(flags, ShaderStageFlags::FragmentStage))
        {
            result = WGPUShaderStageFlags(result | WGPUShaderStage_Fragment);
        }
        return result;
    }

} // namespace ice::render::webgpu
