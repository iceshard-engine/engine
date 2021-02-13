#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/math/vector.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/render/render_surface.hxx>

namespace ice::platform
{

    class WindowSurface
    {
    public:
        virtual ~WindowSurface() noexcept = default;

        virtual bool query_details(
            ice::render::SurfaceInfo& surface_info_out
        ) const noexcept = 0;

        virtual auto render_driver() const noexcept -> ice::render::RenderDriverAPI = 0;

        virtual auto dimensions() const noexcept -> ice::vec2u = 0;
    };

    auto create_window_surface(
        ice::Allocator& alloc,
        ice::render::RenderDriverAPI driver_api
    ) noexcept -> ice::UniquePtr<ice::platform::WindowSurface>;

} // namespace ice::platform
