#pragma once

namespace ice::render
{

    class RenderSurface
    {
    protected:
        virtual ~RenderSurface() noexcept = default;

    public:
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
