#pragma once
#include <ice/platform_render_surface.hxx>
#include <android/native_window.h>

namespace ice::platform::android
{

    class AndroidRenderSurface final : public ice::platform::RenderSurface
    {
    public:
        AndroidRenderSurface() noexcept;

        bool valid() const noexcept { return _native_window != nullptr; }

        void set_native_window(ANativeWindow* window) noexcept;

        auto create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result override;
        auto get_dimensions() const noexcept -> ice::vec2u override;
        bool get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept override;
        void destroy() noexcept override;

    private:
        ANativeWindow* _native_window;
    };

} // namespace ice::platform::android
