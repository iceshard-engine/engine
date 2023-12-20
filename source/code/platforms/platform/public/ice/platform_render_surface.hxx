#pragma once
#include <ice/math.hxx>
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
        virtual bool get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept = 0;
        virtual void destroy() noexcept = 0;
    };


    static constexpr ice::ResultCode E_RenderSurfaceAlreadyExisting = ice::ResCode::create(ice::ResultSeverity::Error, "Render Surface Already Existing");

    template<>
    constexpr ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::RenderSurface> = FeatureFlags::RenderSurface;

} // namespace ice::platform