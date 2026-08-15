/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_sdl2_platform_render_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/static_string.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>

#include "linux_sdl2.hxx"

namespace ice::platform::linux::sdl2
{

    UnixWindow_X11WaylandSDL2::UnixWindow_X11WaylandSDL2() noexcept
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    UnixWindow_X11WaylandSDL2::~UnixWindow_X11WaylandSDL2() noexcept
    {
        ICE_ASSERT(_window == nullptr, "Render surface was not properly cleaned up!");
        if (_window != nullptr)
        {
            UnixWindow_X11WaylandSDL2::destroy();
        }

        SDL_VideoQuit();
    }

    auto UnixWindow_X11WaylandSDL2::create(ice::platform::DrawSurfaceParams const& surface_params) noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;

        if (_window != nullptr)
        {
            return E_DrawSurfaceAlreadyExisting;
        }

        if (surface_params.dimensions.x == 0 || surface_params.dimensions.y == 0)
        {
            return E_InvalidArgument;
        }

        using ice::render::RenderDriverAPI;
        ice::i32 creation_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        ice::StaticString<64> window_title{ surface_params.surface_name };
        if (window_title.is_empty())
        {
            if (surface_params.driver == RenderDriverAPI::Vulkan)
            {
                window_title.push_format("Iceshard (SDL2, Vulkan, {})", SDL_GetCurrentVideoDriver());
                creation_flags |= SDL_WINDOW_VULKAN;
            }
            else if (surface_params.driver == RenderDriverAPI::OpenGL)
            {
                window_title = ice::String{ "Iceshard (SDL2, OpenGL)" };
                creation_flags |= SDL_WINDOW_OPENGL;
            }
        }

        _window = SDL_CreateWindow(
            window_title.data(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            static_cast<int>(surface_params.dimensions.x),
            static_cast<int>(surface_params.dimensions.y),
            creation_flags
        );

        int x, y;
        SDL_GetWindowSize(_window, &x, &y);

        char errmsg[256];
        ICE_LOG_IF(
            _window == nullptr, LogSeverity::Error, LogTag::System,
            "Failed to create SDL2 Window with message: {}",
            SDL_GetErrorMsg(errmsg, 256)
        );
        return _window == nullptr ? E_Fail : S_Ok;
    }

    auto UnixWindow_X11WaylandSDL2::render_driver() const noexcept -> ice::render::RenderDriverAPI
    {
        return _render_driver;
    }

    auto UnixWindow_X11WaylandSDL2::native_surface() const noexcept -> ice::render::NativeSurface const*
    {
        if (_window == nullptr)
        {
            return nullptr;
        }

        return this;
    }

    auto UnixWindow_X11WaylandSDL2::dimensions() const noexcept -> ice::vec2u
    {
        ice::i32 width = 0, height = 0;
        SDL_GetWindowSize(_window, &width, &height);
        return { static_cast<unsigned>(width), static_cast<unsigned>(height) };
    }

    void UnixWindow_X11WaylandSDL2::destroy() noexcept
    {
        SDL_DestroyWindow(ice::exchange(_window, nullptr));
    }

    bool UnixWindow_X11WaylandSDL2::is_valid() const noexcept
    {
        return _window != nullptr && surface_type() != ice::render::SurfaceType::Unknown;
    }

    auto UnixWindow_X11WaylandSDL2::surface_type() const noexcept -> ice::render::SurfaceType
    {
        SDL_SysWMinfo wm_info{};
        SDL_VERSION(&wm_info.version);
        SDL_GetWindowWMInfo(_window, &wm_info);

        if (wm_info.subsystem == SDL_SYSWM_WAYLAND)
        {
            return ice::render::SurfaceType::Wayland_Window;
        }
        else if (wm_info.subsystem == SDL_SYSWM_X11)
        {
            return ice::render::SurfaceType::X11_Window;
        }
        return ice::render::SurfaceType::Unknown;
    }

    void UnixWindow_X11WaylandSDL2::query_surface_info(ice::render::NativeSurfaceInfo& out_surface_info) const noexcept
    {
        SDL_SysWMinfo wm_info{};
        SDL_VERSION(&wm_info.version);
        SDL_GetWindowWMInfo(_window, &wm_info);

        ICE_LOG_IF(
            wm_info.subsystem != SDL_SYSWM_X11 && wm_info.subsystem != SDL_SYSWM_WAYLAND,
            LogSeverity::Error, LogTag::Core,
            "Unrecognized SDL2 Surface type!"
        );

#if defined(SDL_VIDEO_DRIVER_X11)
        ICE_LOG(LogSeverity::Info, LogTag::Core, "Checking for 'X11' video driver...");
        if (wm_info.subsystem == SDL_SYSWM_X11)
        {
            ICE_LOG(LogSeverity::Info, LogTag::Core, "Selected 'X11' video driver");
            out_surface_info.x11.display = wm_info.info.x11.display;
            out_surface_info.x11.window = wm_info.info.x11.window;
        }
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
        ICE_LOG(LogSeverity::Info, LogTag::Core, "Checking for 'Wayland' video driver...");
        if (wm_info.subsystem == SDL_SYSWM_WAYLAND)
        {
            ICE_LOG(LogSeverity::Info, LogTag::Core, "Selected 'Wayland' video driver");
            out_surface_info.wayland.surface = wm_info.info.wl.surface;
            out_surface_info.wayland.display = wm_info.info.wl.display;
        }
#endif

#if !defined(SDL_VIDEO_DRIVER_WAYLAND) and !defined(SDL_VIDEO_DRIVER_X11)
        ICE_LOG(
            LogSeverity::Error, LogTag::Core,
            "The currently used SDL2 package does not support Wayland nor X11 surfaces!"
        );
#endif
    }

} // namespace ice::platform::win32::sdl2
