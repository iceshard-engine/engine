/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_framebuffer.hxx>
#include <ice/container/array.hxx>
#include "webgpu_utils.hxx"
#include "webgpu_image.hxx"

namespace ice::render::webgpu
{

    struct WebGPUFrameBuffer
    {
        ice::Array<WebGPUImage const*> _images;

        WebGPUFrameBuffer(ice::Array<WebGPUImage const*>&& images) noexcept
            : _images{ ice::move(images) }
        {

        }

        static auto handle(WebGPUFrameBuffer* native) noexcept
        {
            return static_cast<ice::render::Framebuffer>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::Framebuffer handle) noexcept
        {
            return reinterpret_cast<WebGPUFrameBuffer*>(static_cast<uintptr_t>(handle));
        }
    };

} // namespace ice::render::webgpu
