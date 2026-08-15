/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "android_draw_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/assert.hxx>

namespace ice::render {
    class NativeSurface;
}

namespace ice::platform::android
{

    AndroidDrawSurface::AndroidDrawSurface() noexcept
        : _native_window{ nullptr }
    {
    }

    void AndroidDrawSurface::set_native_window(ANativeWindow* window) noexcept
    {
        _native_window = window;
    }

    auto AndroidDrawSurface::create(ice::platform::DrawSurfaceParams const& draw_params) noexcept -> ice::Result
    {
        // This should only be called after Android created a native window object.
        if (_native_window == nullptr)
        {
            return ice::platform::E_DrawSurfaceAlreadyExisting;
        }
        return S_Success;
    }

    void AndroidDrawSurface::destroy() noexcept
    {
    }

    auto AndroidDrawSurface::render_driver() const noexcept -> ice::render::RenderDriverAPI
    {
        return render::RenderDriverAPI::Vulkan;
    }

    auto AndroidDrawSurface::native_surface() const noexcept -> ice::render::NativeSurface const*
    {
        if (_native_window != nullptr)
        {
            return this;
        }
        return nullptr;
    }

    auto AndroidDrawSurface::dimensions() const noexcept -> ice::vec2u
    {
        ice::vec2u result{ 0, 0 };
        if (_native_window != nullptr)
        {
            result.x = (ice::u32) ANativeWindow_getWidth(_native_window);
            result.y = (ice::u32) ANativeWindow_getHeight(_native_window);
        }
        return result;
    }

    auto AndroidDrawSurface::surface_type() const noexcept -> ice::render::SurfaceType
    {
        return render::SurfaceType::Android_NativeWindow;
    }

    void AndroidDrawSurface::query_surface_info(
        render::NativeSurfaceInfo& out_surface_info
    ) const noexcept
    {
        out_surface_info.android.native_window = _native_window;
    }

} // namespace ice::platform::android
