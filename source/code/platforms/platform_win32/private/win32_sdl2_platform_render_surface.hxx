/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_render_surface.hxx>
#include <ice/render/render_driver.hxx>
#include <SDL.h>

namespace ice::platform::win32::sdl2
{

    class RenderSurface_Win32SDL2 final : public RenderSurface
    {
    public:
        RenderSurface_Win32SDL2() noexcept;
        ~RenderSurface_Win32SDL2() noexcept;

        auto create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result override;
        bool get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept override;
        void destroy() noexcept override;

    private:
        ice::render::RenderDriverAPI _render_driver;
        SDL_Window* _window;
    };

} // namespace ice::platform::win32::sdl2
