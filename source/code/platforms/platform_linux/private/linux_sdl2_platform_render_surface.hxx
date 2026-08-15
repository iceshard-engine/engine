/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_draw_surface.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/render/render_surface.hxx>

#include "linux_sdl2.hxx"

namespace ice::platform::linux::sdl2
{

    class UnixWindow_X11WaylandSDL2 final
        : public ice::platform::DrawSurface
        , public ice::render::NativeSurface
    {
    public:
        UnixWindow_X11WaylandSDL2() noexcept;
        ~UnixWindow_X11WaylandSDL2() noexcept override;

        auto create(ice::platform::DrawSurfaceParams const& surface_params) noexcept -> ice::Result override;

        [[nodiscard]]
        auto render_driver() const noexcept -> ice::render::RenderDriverAPI override;

        [[nodiscard]]
        auto native_surface() const noexcept -> ice::render::NativeSurface const* override;

        [[nodiscard]]
        auto dimensions() const noexcept -> ice::vec2u override;

        void destroy() noexcept override;

        [[nodiscard]]
        bool is_valid() const noexcept override;

        [[nodiscard]]
        auto surface_type() const noexcept -> ice::render::SurfaceType override;

        void query_surface_info(ice::render::NativeSurfaceInfo& out_surface_info) const noexcept override;

    private:
        ice::render::RenderDriverAPI _render_driver = ice::render::RenderDriverAPI::None;
        SDL_Window* _window = nullptr;
    };

} // namespace ice::platform::win32::sdl2
