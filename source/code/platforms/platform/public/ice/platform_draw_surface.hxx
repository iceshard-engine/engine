/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_driver.hxx>
#include <ice/render/render_surface.hxx>
#include <ice/math.hxx>
#include <ice/platform.hxx>

namespace ice::platform
{

    struct DrawSurfaceParams
    {
        ice::render::RenderDriverAPI driver;
        ice::vec2u dimensions;
        ice::String surface_name;
    };

    class DrawSurface
    {
    public:
        virtual ~DrawSurface() noexcept = default;

        virtual auto create(ice::platform::DrawSurfaceParams const& params) noexcept -> ice::Result = 0;
        virtual void destroy() noexcept = 0;

        virtual auto render_driver() const noexcept -> ice::render::RenderDriverAPI = 0;
        virtual auto native_surface() const noexcept -> ice::render::NativeSurface const* = 0;

        virtual auto dimensions() const noexcept -> ice::vec2u = 0;
    };

    static constexpr ice::ErrorCode E_DrawSurfaceNotAvailable{ "E.0110:App:No Draw Surface Available" };
    static constexpr ice::ErrorCode E_DrawSurfaceAlreadyExisting{ "E.0111:App:Draw Surface Already Existing" };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::DrawSurface> = FeatureFlags::DrawSurface;

} // namespace ice::platform
