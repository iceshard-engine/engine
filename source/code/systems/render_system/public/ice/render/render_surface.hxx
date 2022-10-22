/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
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
        Win32_Window,
        UWP_Window,
    };

    struct SurfaceInfo
    {
        SurfaceType type;
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
                void* reserved[2];
            } wayland;
        };
    };

} // namespace ice::render
