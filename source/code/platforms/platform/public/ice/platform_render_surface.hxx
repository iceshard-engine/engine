/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/string/string.hxx>
#include <ice/platform.hxx>
#include <ice/render/render_driver.hxx>

namespace ice::platform
{

    struct RenderSurfaceParams
    {
        ice::render::RenderDriverAPI driver;
        ice::vec2u dimensions;
        ice::String window_title;
    };

    struct RenderSurface
    {
        virtual auto create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result = 0;
        virtual auto get_dimensions() const noexcept -> ice::vec2u = 0;
        virtual bool get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept = 0;
        virtual void destroy() noexcept = 0;
    };


    static constexpr ice::ErrorCode E_RenderSurfaceNotAvailable{ "E.0110:App:No Render Surface Available" };
    static constexpr ice::ErrorCode E_RenderSurfaceAlreadyExisting{ "E.0111:App:Render Surface Already Existing" };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::RenderSurface> = FeatureFlags::RenderSurface;

} // namespace ice::platform
