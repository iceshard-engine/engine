/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_command_buffer.hxx>
#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    struct WebGPUCommandBuffer
    {
        CommandBufferType type;
        WGPUCommandEncoder command_encoder;
        WGPUCommandBuffer command_buffer;
        WGPURenderPassEncoder renderpass_encoder;

        static auto handle(WebGPUCommandBuffer* native) noexcept
        {
            return static_cast<ice::render::CommandBuffer>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::CommandBuffer handle) noexcept
        {
            return reinterpret_cast<WebGPUCommandBuffer*>(static_cast<uintptr_t>(handle));
        }
    };

} // ice::render::webgpu
