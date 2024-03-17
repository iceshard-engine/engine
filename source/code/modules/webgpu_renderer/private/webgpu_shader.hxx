/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


#pragma once
#include <ice/render/render_shader.hxx>
#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    class WebGPUShader
    {
    public:
        WebGPUShader(
            WGPUShaderModule wgpu_shader
        ) noexcept
            : _wgpu_shader{ wgpu_shader }
        {
        }

        ~WebGPUShader() noexcept
        {
            wgpuShaderModuleRelease(_wgpu_shader);
        }

        WGPUShaderModule const _wgpu_shader;

        static auto handle(WebGPUShader* native) noexcept
        {
            return static_cast<ice::render::Shader>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::Shader handle) noexcept
        {
            return reinterpret_cast<WebGPUShader*>(static_cast<uintptr_t>(handle));
        }
    };

    auto native_attribute_type(ice::render::ShaderAttribType type) noexcept
    {
        switch (type)
        {
        case ShaderAttribType::Vec1i: return WGPUVertexFormat_Sint32;
        case ShaderAttribType::Vec1u: return WGPUVertexFormat_Uint32;
        case ShaderAttribType::Vec1f: return WGPUVertexFormat_Float32;
        case ShaderAttribType::Vec2f: return WGPUVertexFormat_Float32x2;
        case ShaderAttribType::Vec3f: return WGPUVertexFormat_Float32x3;
        case ShaderAttribType::Vec4f: return WGPUVertexFormat_Float32x4;
        case ShaderAttribType::Vec4f_Unorm8: return WGPUVertexFormat_Unorm8x4;
        default: return WGPUVertexFormat_Undefined;
        }
    }

} // namespace ice::render::webgpu
