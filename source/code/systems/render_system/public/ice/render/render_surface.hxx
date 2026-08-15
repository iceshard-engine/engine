/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::render
{

    class RenderSurface
    {
    protected:
        virtual ~RenderSurface() noexcept = default;
    };

    enum class SurfaceType : ice::u8
    {
        Unknown,
        Win32_Window,
        UWP_Window,
        Wayland_Window,
        X11_Window,
        Android_NativeWindow,
        HTML5_DOMCanvas
    };

    class NativeSurface
    {
    protected:
        virtual ~NativeSurface() noexcept = default;

    public:
        virtual bool is_valid() const noexcept = 0;

        virtual auto surface_type() const noexcept -> ice::render::SurfaceType = 0;
        virtual void query_surface_info(ice::render::NativeSurfaceInfo& out_surface_info) const noexcept = 0;
        virtual auto dimensions() const noexcept -> ice::vec2u = 0;
    };

    struct NativeSurfaceInfo
    {
        union
        {
            struct
            {
                void* hinstance;
                void* hwn;
            } win32;

            struct
            {
                void* reserved[2];
            } uwp;

            struct
            {
                char const* selector;
                void* internal;
            } webgpu;

            struct
            {
                void* native_window;
                void* reserved[1];
            } android;

            struct
            {
                void* surface;
                void* display;
            } wayland;

            struct
            {
                unsigned long window;
                void* display;
            } x11;
        };
    };

} // namespace ice::render
