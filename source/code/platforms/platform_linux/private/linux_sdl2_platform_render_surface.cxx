/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "linux_sdl2_platform_render_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/static_string.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>
#include <ice/log.hxx>
#include <ice/log_formatters.hxx>
#include <ice/params.hxx>

#include "linux_sdl2.hxx"

namespace ice::platform::linux::sdl2
{

    static bool Global_EnabledVideoDrivers[3]{};
    static ice::StaticString<32> Global_SupportedVideoDrivers[3]{};

    static auto video_driver_count() noexcept -> ice::u32
    {
        return ice::min<ice::u32>(
            SDL_GetNumVideoDrivers(),
            ice::count(Global_SupportedVideoDrivers)
        );
    }

    void UnixWindow_X11WaylandSDL2::define_options(ice::Params& params) noexcept
    {
        ice::u32 const driver_num = video_driver_count();
        for (ice::u32 i = 0; i < driver_num && i < 5; ++i)
        {
            ice::StaticString<32>& driver_name = Global_SupportedVideoDrivers[i];
            driver_name.push_format("--{}", SDL_GetVideoDriver(i));

            if (driver_name != "--x11" && driver_name != "--wayland")
            {
                continue;
            }
            ice::params_define(params, ice::ParamDefinition{
                .name = driver_name,
                .group = "video options"_str,
            }, Global_EnabledVideoDrivers[i]);
        }
    }

    void UnixWindow_X11WaylandSDL2::initialize_subsystem() noexcept
    {
        ice::u32 const driver_num = video_driver_count();

        ice::String driver_selected{};
        for (ice::u32 i = 0; i < driver_num && i < 5; ++i)
        {
            if (Global_EnabledVideoDrivers[i])
            {
                if (driver_selected.is_empty())
                {
                    driver_selected = Global_SupportedVideoDrivers[i].substr(2);
                    SDL_VideoInit(driver_selected.data());
                    IPT_LOG(LogSeverity::Retail, LogTag::Engine, "Force selected video-driver: {}", driver_selected);
                }
                else
                {
                    IPT_LOG(LogSeverity::Warning, LogTag::Engine, "Ignoring additional video-driver options.");
                }
            }
        }
    }

    void UnixWindow_X11WaylandSDL2::shutdown_subsystem() noexcept
    {
        ice::u32 const driver_num = video_driver_count();
        for (ice::u32 i = 0; i < driver_num && i < 5; ++i)
        {
            // We call SDL_VideoQuit only if we used VideoInit previously.
            if (Global_EnabledVideoDrivers[i])
            {
                SDL_VideoQuit();
                return;
            }
        }
    }

    UnixWindow_X11WaylandSDL2::UnixWindow_X11WaylandSDL2(
        ice::Span<ice::Shard const> params
    ) noexcept
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);

        for (ice::Shard const shard : params)
        {
            bool param_error = false;
            if (shard.id == PlatformParam_InitialWindowSize)
            {
                param_error = ice::shard_inspect(shard, _initial_dimensions) == false;
                ICE_LOG_IF(
                    param_error == false,
                    LogSeverity::Info, LogTag::Core,
                    "Initial surface dimensions set to: {}",
                    _initial_dimensions
                );
            }
            if (shard.id == PlatformParam_StartFullscreen)
            {
                param_error = ice::shard_inspect(shard, _initial_fullscreen) == false;
            }
            ICE_LOG_IF(
                param_error,
                LogSeverity::Error, LogTag::Core,
                "Invalid value type for {} platform parameter.",
                shard.id.name.value
            );
        }
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

        ice::vec2u surface_dimensions = surface_params.dimensions;
        if (surface_dimensions.x == 0 || surface_dimensions.y == 0)
        {
            if (_initial_dimensions.x == 0 || _initial_dimensions.y == 0)
            {
                ICE_LOG(LogSeverity::Critical, LogTag::Core, "Failed to provide surface dimensions");
                return E_InvalidArgument;
            }
            surface_dimensions = _initial_dimensions;
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
            static_cast<int>(surface_dimensions.x),
            static_cast<int>(surface_dimensions.y),
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
