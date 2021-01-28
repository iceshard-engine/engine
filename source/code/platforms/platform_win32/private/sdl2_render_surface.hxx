#pragma once
#include <ice/platform_render_surface.hxx>
#include <SDL.h>

namespace ice::platform
{

    class SDL2_RenderSurface final : public ice::platform::RenderSurface
    {
    public:
        SDL2_RenderSurface(
            ice::Allocator& alloc,
            ice::platform::RenderDriver render_driver
        ) noexcept;
        ~SDL2_RenderSurface() noexcept override;

        bool query_details(SurfaceDetails*) const noexcept override;

        auto render_driver() const noexcept -> ice::platform::RenderDriver override;

        auto dimensions() const noexcept -> ice::vec2u override;

    private:
        ice::Allocator& _allocator;
        ice::platform::RenderDriver const _render_driver;

        SDL_Window* _window;
    };

} // namespace ice::platform
