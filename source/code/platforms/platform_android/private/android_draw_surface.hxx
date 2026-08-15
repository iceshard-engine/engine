/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_draw_surface.hxx>
#include <android/native_window.h>

namespace ice::platform::android
{

    class AndroidDrawSurface final : public ice::platform::DrawSurface
        , public ice::render::NativeSurface
    {
    public:
        AndroidDrawSurface() noexcept;

        void set_native_window(ANativeWindow* window) noexcept;

    public:
        auto create(ice::platform::DrawSurfaceParams const& params) noexcept -> ice::Result override;
        void destroy() noexcept override;

        [[nodiscard]]
        auto render_driver() const noexcept -> ice::render::RenderDriverAPI override;

        [[nodiscard]]
        auto native_surface() const noexcept -> ice::render::NativeSurface const* override;

        [[nodiscard]]
        auto dimensions() const noexcept -> ice::vec2u override;

    public:
        [[nodiscard]]
        bool is_valid() const noexcept override { return _native_window != nullptr; }

        [[nodiscard]]
        auto surface_type() const noexcept -> ice::render::SurfaceType override;

        void query_surface_info(ice::render::NativeSurfaceInfo& out_surface_info) const noexcept override;

    private:
        ANativeWindow* _native_window;
    };

} // namespace ice::platform::android
