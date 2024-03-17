/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_render_surface.hxx>
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
            ice::render::SurfaceInfo surface_info
        ) noexcept
            : _wgpu_surface{ wgpu_surface }
            , _wgpu_surface_format{ wgpu_surface_format }
            , _surface_info{ surface_info }
        {
        }

        ~WebGPURenderSurface() noexcept override
        {
            wgpuSurfaceRelease(_wgpu_surface);
        }

        WGPUSurface const _wgpu_surface;
        WGPUTextureFormat _wgpu_surface_format;
        ice::render::SurfaceInfo const _surface_info;
    };

} // namespace ice::render::webgpu
