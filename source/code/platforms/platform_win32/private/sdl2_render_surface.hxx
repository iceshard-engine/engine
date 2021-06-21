#pragma once
#include <ice/platform_window_surface.hxx>
#include <SDL.h>

namespace ice::platform
{

    class SDL2_WindowSurface final : public ice::platform::WindowSurface
    {
    public:
        SDL2_WindowSurface(
            ice::Allocator& alloc,
            ice::render::RenderDriverAPI driver_api
        ) noexcept;
        ~SDL2_WindowSurface() noexcept override;

        bool query_details(
            ice::render::SurfaceInfo& surface_info_out
        ) const noexcept override;

        auto render_driver() const noexcept -> ice::render::RenderDriverAPI override;

        auto dimensions() const noexcept -> ice::vec2u override;

    private:
        ice::Allocator& _allocator;
        ice::render::RenderDriverAPI _render_driver;

        SDL_Window* _window;
    };

} // namespace ice::platform
