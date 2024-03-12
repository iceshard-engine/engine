/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "android_render_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/assert.hxx>

namespace ice::platform::android
{

    AndroidRenderSurface::AndroidRenderSurface() noexcept
        : _native_window{ nullptr }
    {
    }

    void AndroidRenderSurface::set_native_window(ANativeWindow* window) noexcept
    {
        _native_window = window;
    }

    auto AndroidRenderSurface::create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result
    {
        // This should onlt be called after Android created a native window object.
        if (_native_window == nullptr)
        {
            return ice::platform::E_RenderSurfaceNotAvailable;
        }
        return ice::Res::Success;
    }

    auto AndroidRenderSurface::get_dimensions() const noexcept -> ice::vec2u
    {
        ice::vec2u result{ 0, 0 };
        if (_native_window != nullptr)
        {
            result.x = (ice::u32) ANativeWindow_getWidth(_native_window);
            result.y = (ice::u32) ANativeWindow_getHeight(_native_window);
        }
        return result;
    }

    bool AndroidRenderSurface::get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept
    {
        if (_native_window != nullptr)
        {
            out_surface_info.type = ice::render::SurfaceType::Android_NativeWindow;
            out_surface_info.android.native_window = _native_window;
        }
        return _native_window != nullptr;
    }

    void AndroidRenderSurface::destroy() noexcept
    {
    }

} // namespace ice::platform::android
