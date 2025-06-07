/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>

namespace ice::render
{

    class RenderSurface
    {
    protected:
        virtual ~RenderSurface() noexcept = default;
    };

    enum class SurfaceType
    {
        Unknown,
        Win32_Window,
        UWP_Window,
        Wayland_Window,
        X11_Window,
        Android_NativeWindow,
        HTML5_DOMCanvas
    };

    struct SurfaceInfo
    {
        SurfaceType type = SurfaceType::Unknown;
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
