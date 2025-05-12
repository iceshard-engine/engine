/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_render_surface.hxx>
#include <ice/log.hxx>

#include "webasm_include.hxx"

namespace ice::platform::webasm
{

    class WebASM_RenderSurface : public ice::platform::RenderSurface
    {
    public:
        auto create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result override;
        auto get_dimensions() const noexcept -> ice::vec2u override;
        bool get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept override;
        void destroy() noexcept override;

    private:
        bool _initialized = false;
    };

} // namespace ice::platform::webasm
