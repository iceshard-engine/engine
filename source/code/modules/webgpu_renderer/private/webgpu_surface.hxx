/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_surface.hxx>
#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    class WebGPURenderSurface : public ice::render::RenderSurface
    {
    public:
        WebGPURenderSurface(
            WGPUSurface wgpu_surface,
            WGPUTextureFormat wgpu_surface_format,
            WGPUPresentMode wgpu_present_mode,
            ice::render::NativeSurface const* surface
        ) noexcept
            : _wgpu_surface{ wgpu_surface }
            , _wgpu_surface_format{ wgpu_surface_format }
            , _wgpu_present_mode{ wgpu_present_mode }
            , _surface{ surface }
        {
        }

        ~WebGPURenderSurface() noexcept override
        {
            wgpuSurfaceRelease(_wgpu_surface);
        }

        WGPUSurface const _wgpu_surface;
        WGPUTextureFormat _wgpu_surface_format;
        WGPUPresentMode _wgpu_present_mode;
        ice::render::NativeSurface const* _surface;
    };

} // namespace ice::render::webgpu
