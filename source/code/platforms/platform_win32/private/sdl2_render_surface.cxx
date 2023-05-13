/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "sdl2_render_surface.hxx"
#include <ice/string/static_string.hxx>
#include <ice/assert.hxx>

#include <SDL_syswm.h>

namespace ice::platform
{

    SDL2_WindowSurface::SDL2_WindowSurface(
        ice::Allocator& alloc,
        ice::render::RenderDriverAPI driver_api
    ) noexcept
        : WindowSurface{ }
        , _allocator{ alloc }
        , _render_driver{ driver_api }
    {
        using ice::render::RenderDriverAPI;

        SDL_InitSubSystem(SDL_INIT_VIDEO);

        ice::i32 creation_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        ice::StaticString<64> window_title;
        if (_render_driver == RenderDriverAPI::Vulkan)
        {
            window_title = ice::String{ "Iceshard 0.1-Alpha : Win32, SDL2, Vulkan" };
            creation_flags |= SDL_WINDOW_VULKAN;
        }
        if (_render_driver == RenderDriverAPI::OpenGL)
        {
            window_title = ice::String{ "Iceshard 0.1-Alpha : Win32, SDL2, OpenGL" };
            creation_flags |= SDL_WINDOW_OPENGL;
        }

        _window = SDL_CreateWindow(
            ice::string::data(window_title),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1920, 1080,
            creation_flags
        );
    }

    SDL2_WindowSurface::~SDL2_WindowSurface() noexcept
    {
        SDL_DestroyWindow(_window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    bool SDL2_WindowSurface::query_details(
        ice::render::SurfaceInfo& surface_info_out
    ) const noexcept
    {
        SDL_SysWMinfo wm_info{};
        SDL_GetWindowWMInfo(_window, &wm_info);

        surface_info_out.type = ice::render::SurfaceType::Win32_Window;
        surface_info_out.win32.hinstance = wm_info.info.win.hinstance;
        surface_info_out.win32.hwn = wm_info.info.win.window;
        return true;
    }

    auto SDL2_WindowSurface::render_driver() const noexcept -> ice::render::RenderDriverAPI
    {
        return _render_driver;
    }

    auto SDL2_WindowSurface::dimensions() const noexcept -> ice::vec2u
    {
        return { 1920, 1080 };
    }

    auto create_window_surface(
        ice::Allocator& alloc,
        ice::render::RenderDriverAPI driver_api
    ) noexcept -> ice::UniquePtr<ice::platform::WindowSurface>
    {
        return ice::make_unique<SDL2_WindowSurface>(alloc, alloc, driver_api);
    }

} // namespace ice::platform
