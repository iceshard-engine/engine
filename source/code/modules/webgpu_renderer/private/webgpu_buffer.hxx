/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_utils.hxx"
#include <ice/render/render_buffer.hxx>
#include <ice/assert.hxx>

namespace ice::render::webgpu
{

    struct WebGPUBuffer
    {
        WGPUBuffer wgpu_buffer;
        ice::u32 size;

        static auto handle(WebGPUBuffer* native) noexcept
        {
            return static_cast<ice::render::Buffer>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::Buffer handle) noexcept
        {
            return reinterpret_cast<WebGPUBuffer*>(static_cast<uintptr_t>(handle));
        }
    };

} // namespace ice::render::webgpu
