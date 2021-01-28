#include "sdl2_render_surface.hxx"
#include <ice/stack_string.hxx>
#include <ice/assert.hxx>

#include <SDL_syswm.h>

namespace ice::platform
{

    SDL2_RenderSurface::SDL2_RenderSurface(
        ice::Allocator& alloc,
        ice::platform::RenderDriver render_driver
    ) noexcept
        : RenderSurface{ }
        , _allocator{ alloc }
        , _render_driver{ render_driver }
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);

        ice::i32 creation_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;
        ice::StackString<64> window_title;
        if (_render_driver == RenderDriver::Vulkan)
        {
            window_title = "Iceshard 0.1-Alpha : Win32, SDL2, Vulkan";
            creation_flags |= SDL_WINDOW_VULKAN;
        }
        if (_render_driver == RenderDriver::OpenGL)
        {
            window_title = "Iceshard 0.1-Alpha : Win32, SDL2, OpenGL";
            creation_flags |= SDL_WINDOW_OPENGL;
        }

        _window = SDL_CreateWindow(
            ice::string::data(window_title),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1080, 756,
            creation_flags
        );
    }

    SDL2_RenderSurface::~SDL2_RenderSurface() noexcept
    {
        SDL_DestroyWindow(_window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    bool SDL2_RenderSurface::query_details(SurfaceDetails* details) const noexcept
    {
        if (details != nullptr && details->system == ice::build::System::Windows)
        {
            ICE_ASSERT(
                details->size == sizeof(SurfaceDetails_Win32),
                "Invalid structure while querying surface details!"
            );

            SDL_SysWMinfo wm_info{};
            SDL_GetWindowWMInfo(_window, &wm_info);

            SurfaceDetails_Win32* win32_surface = static_cast<SurfaceDetails_Win32*>(details);
            win32_surface->hinstance = wm_info.info.win.hinstance;
            win32_surface->hwnd = wm_info.info.win.window;
            return true;
        }
        return false;
    }

    auto SDL2_RenderSurface::render_driver() const noexcept -> ice::platform::RenderDriver
    {
        return ice::platform::RenderDriver::Vulkan;
    }

    auto SDL2_RenderSurface::dimensions() const noexcept -> ice::vec2u
    {
        return { 1080, 756 };
    }

} // namespace ice::platform
