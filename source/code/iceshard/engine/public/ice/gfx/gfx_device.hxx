#pragma once
#include <ice/stringid.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/render/render_driver.hxx>

namespace ice::gfx
{

    struct GfxPassCreateInfo
    {
        ice::StringID name;
        ice::render::QueueFlags queue_flags;
    };

    struct GfxDeviceCreateInfo
    {
        ice::render::RenderDriver* render_driver;
        ice::render::RenderSurface* render_surface;
        ice::Span<ice::gfx::GfxPassCreateInfo> pass_list;
    };

    class GfxDevice
    {
    protected:
        virtual ~GfxDevice() noexcept = default;

    public:
        virtual auto device() noexcept -> ice::render::RenderDevice& = 0;
        virtual auto swapchain() noexcept -> ice::render::RenderSwapchain const& = 0;
    };

} // namespace ice::gfx
