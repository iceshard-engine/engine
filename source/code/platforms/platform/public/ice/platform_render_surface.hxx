#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/math/vector.hxx>

namespace ice::platform
{

    struct SurfaceDetails
    {
        ice::build::System system;
        ice::u32 size;
    };

    struct SurfaceDetails_Win32 : SurfaceDetails
    {
        void* hinstance;
        void* hwnd;
    };

    enum class RenderDriver
    {
        None = 0x0,
        DirectX11,
        DirectX12,
        Vulkan,
        Metal,
        OpenGL,
    };

    class RenderSurface
    {
    public:
        virtual ~RenderSurface() noexcept = default;

        virtual bool query_details(SurfaceDetails*) const noexcept = 0;

        virtual auto render_driver() const noexcept -> ice::platform::RenderDriver = 0;

        virtual auto dimensions() const noexcept -> ice::vec2u = 0;

    };

} // namespace ice::platform
