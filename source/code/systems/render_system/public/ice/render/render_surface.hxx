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
        };
    };

} // namespace ice::render
