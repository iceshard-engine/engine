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

    RenderSurface_WaylandX11SDL2::RenderSurface_WaylandX11SDL2() noexcept
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    RenderSurface_WaylandX11SDL2::~RenderSurface_WaylandX11SDL2() noexcept
    {
        ICE_ASSERT(_window == nullptr, "Render surface was not properly cleaned up!");
        if (_window != nullptr)
        {
            RenderSurface_WaylandX11SDL2::destroy();
        }

        SDL_VideoQuit();
    }

    auto RenderSurface_WaylandX11SDL2::create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result
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
                window_title = ice::String{ "Iceshard (SDL2, Vulkan, " };
                ice::string::push_back(window_title, SDL_GetCurrentVideoDriver());
                ice::string::push_back(window_title, ")");
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

    bool RenderSurface_WaylandX11SDL2::get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept
    {
        if (_window == nullptr)
        {
            return false;
        }

        SDL_SysWMinfo wm_info{};
        SDL_VERSION(&wm_info.version);
        SDL_GetWindowWMInfo(_window, &wm_info);

#if defined(SDL_VIDEO_DRIVER_X11)
        ICE_LOG(LogSeverity::Info, LogTag::Core, "Checking for 'X11' video driver...");
        if (wm_info.subsystem == SDL_SYSWM_X11)
        {
            ICE_LOG(LogSeverity::Info, LogTag::Core, "Selected 'X11' video driver");
            out_surface_info.type = ice::render::SurfaceType::X11_Window;
            out_surface_info.x11.display = wm_info.info.x11.display;
            out_surface_info.x11.window = wm_info.info.x11.window;
        }
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
        ICE_LOG_IF(
            out_surface_info.type == ice::render::SurfaceType::Unknown,
            LogSeverity::Info, LogTag::Core,
            "Checking for 'Wayland' video driver..."
        );
        if (wm_info.subsystem == SDL_SYSWM_WAYLAND)
        {
            ICE_LOG(LogSeverity::Info, LogTag::Core, "Selected 'Wayland' video driver");
            out_surface_info.type = ice::render::SurfaceType::Wayland_Window;
            out_surface_info.wayland.surface = wm_info.info.wl.surface;
            out_surface_info.wayland.display = wm_info.info.wl.display;
        }
#endif

#if !defined(SDL_VIDEO_DRIVER_WAYLAND) and !defined(SDL_VIDEO_DRIVER_X11)
        ICE_LOG(
            LogSeverity::Error, LogTag::Core,
            "The currently used SDL2 package does not support Wayland nor X11 surfaces!"
        );
#else
        ICE_LOG_IF(
            out_surface_info.type == ice::render::SurfaceType::Unknown,
            LogSeverity::Error, LogTag::Core,
            "Unrecognized SDL2 Surface type!"
        );
#endif
        return out_surface_info.type != ice::render::SurfaceType::Unknown;
    }

    void RenderSurface_WaylandX11SDL2::destroy() noexcept
    {
        SDL_DestroyWindow(ice::exchange(_window, nullptr));
    }

} // namespace ice::platform::win32::sdl2
