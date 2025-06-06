/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_render_surface.hxx>
#include <ice/render/render_driver.hxx>
#include "linux_sdl2.hxx"

namespace ice::platform::linux::sdl2
{

    class RenderSurface_WaylandX11SDL2 final : public RenderSurface
    {
    public:
        RenderSurface_WaylandX11SDL2() noexcept;
        ~RenderSurface_WaylandX11SDL2() noexcept;

        auto create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result override;
        auto get_dimensions() const noexcept -> ice::vec2u override { return {}; }
        bool get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept override;
        void destroy() noexcept override;

    private:
        ice::render::RenderDriverAPI _render_driver;
        SDL_Window* _window;
    };

} // namespace ice::platform::win32::sdl2
