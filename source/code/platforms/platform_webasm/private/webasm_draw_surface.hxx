/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log.hxx>
#include <ice/expected.hxx>
#include <ice/platform_draw_surface.hxx>

#include "webasm_include.hxx"

namespace ice::platform::webasm
{

    class WebASM_DrawSurface : public ice::platform::DrawSurface, public ice::render::NativeSurface
    {
    public:
        auto create(ice::platform::DrawSurfaceParams const& surface_params) noexcept -> ice::Result override;

        [[nodiscard]]
        auto render_driver() const noexcept -> ice::render::RenderDriverAPI override { return ice::render::RenderDriverAPI::WebGPU; }

        [[nodiscard]]
        auto native_surface() const noexcept -> ice::render::NativeSurface const* override;

        [[nodiscard]]
        auto dimensions() const noexcept -> ice::vec2u override;

        void destroy() noexcept override;

    public: // Implements: NativeSurface
        [[nodiscard]]
        bool is_valid() const noexcept override { return _initialized; }
        [[nodiscard]]
        auto surface_type() const noexcept -> ice::render::SurfaceType  override { return ice::render::SurfaceType::HTML5_DOMCanvas; }
        void query_surface_info(ice::render::NativeSurfaceInfo& out_surface_info) const noexcept override;

    private:
        bool _initialized = false;
    };

} // namespace ice::platform::webasm
