#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/render/render_driver.hxx>

namespace ice::gfx
{

    struct GfxQueueCreateInfo
    {
        ice::StringID name;
        ice::render::QueueFlags queue_flags;
    };

    struct GfxDeviceCreateInfo
    {
        ice::render::RenderDriver* render_driver;
        ice::render::RenderSurface* render_surface;
        ice::Span<ice::gfx::GfxQueueCreateInfo> queue_list;
    };

    class GfxPass;

    class GfxResourceTracker;

    class GfxDevice
    {
    protected:
        virtual ~GfxDevice() noexcept = default;

    public:
        virtual auto device() noexcept -> ice::render::RenderDevice& = 0;
        virtual auto swapchain() noexcept -> ice::render::RenderSwapchain const& = 0;

        // virtual auto aquire_pass(ice::StringID_Arg name) noexcept -> ice::gfx::GfxPass& = 0;

        virtual auto resource_tracker() noexcept -> ice::gfx::GfxResourceTracker& = 0;
    };

} // namespace ice::gfx
