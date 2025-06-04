/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_sdl2_platform_render_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/string/static_string.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>

#include "linux_sdl2.hxx"

namespace ice::platform::linux::sdl2
{

    RenderSurface_WaylandSDL2::RenderSurface_WaylandSDL2() noexcept
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    RenderSurface_WaylandSDL2::~RenderSurface_WaylandSDL2() noexcept
    {
        ICE_ASSERT(_window == nullptr, "Render surface was not properly cleaned up!");
        if (_window != nullptr)
        {
            RenderSurface_WaylandSDL2::destroy();
        }

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    auto RenderSurface_WaylandSDL2::create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;

        if (_window != nullptr)
        {
            return E_RenderSurfaceAlreadyExisting;
        }

        if (surface_params.dimensions.x == 0 || surface_params.dimensions.y == 0)
        {
            return E_InvalidArgument;
        }

        using ice::render::RenderDriverAPI;
        ice::i32 creation_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        ice::StaticString<64> window_title{ surface_params.window_title };
        if (ice::string::empty(window_title))
        {
            if (surface_params.driver == RenderDriverAPI::Vulkan)
            {
                window_title = ice::String{ "Iceshard (SDL2, Vulkan)" };
                creation_flags |= SDL_WINDOW_VULKAN;
            }
            else if (surface_params.driver == RenderDriverAPI::OpenGL)
            {
                window_title = ice::String{ "Iceshard (SDL2, OpenGL)" };
                creation_flags |= SDL_WINDOW_OPENGL;
            }
        }

        _window = SDL_CreateWindow(
            ice::string::data(window_title),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            surface_params.dimensions.x,
            surface_params.dimensions.y,
            creation_flags
        );
        return S_Success;
    }

    bool RenderSurface_WaylandSDL2::get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept
    {
        if (_window == nullptr)
        {
            return false;
        }

        SDL_SysWMinfo wm_info{};
        SDL_GetWindowWMInfo(_window, &wm_info);

        out_surface_info.type = ice::render::SurfaceType::Win32_Window;
        out_surface_info.wayland.surface = wm_info.info.wl.surface;
        out_surface_info.wayland.display = wm_info.info.wl.display;
        return true;
    }

    void RenderSurface_WaylandSDL2::destroy() noexcept
    {
        SDL_DestroyWindow(ice::exchange(_window, nullptr));
    }

} // namespace ice::platform::win32::sdl2
