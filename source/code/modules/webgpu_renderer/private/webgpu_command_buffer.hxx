/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_command_buffer.hxx>
#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    struct WebGPURenderPass;
    struct WebGPUFrameBuffer;

    struct WebGPUCommandBuffer
    {
        CommandBufferType type;
        WGPUCommandEncoder command_encoder;
        WGPUCommandBuffer command_buffer;

        WGPURenderPassEncoder renderpass_encoder;
        WebGPUFrameBuffer const* framebuffer;
        WebGPURenderPass const* renderpass;
        ice::u32 renderpass_subpass;

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
