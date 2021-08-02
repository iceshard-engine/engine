#pragma once
#include <ice/engine.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>

#include "iceshard_gfx_resource_tracker.hxx"

namespace ice::gfx
{

    class IceGfxQueueGroup;

    class IceGfxDevice final : public ice::gfx::GfxDevice
    {
    public:
        IceGfxDevice(
            ice::Allocator& alloc,
            ice::render::RenderDriver& driver,
            ice::render::RenderSurface& render_surface,
            ice::render::RenderDevice* render_device,
            ice::pod::Array<ice::gfx::IceGfxQueueGroup*> graphics_passes
        ) noexcept;
        ~IceGfxDevice() noexcept override;

        auto device() noexcept -> ice::render::RenderDevice& override;
        auto swapchain() noexcept -> ice::render::RenderSwapchain const& override;

        void recreate_swapchain() noexcept override;

        auto resource_tracker() noexcept -> ice::gfx::GfxResourceTracker& override;

        auto next_frame() noexcept -> ice::u32;

        auto queue_group(ice::u32 image_index) noexcept -> ice::gfx::IceGfxQueueGroup&;

        void present(ice::u32 image_index) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::render::RenderDriver& _render_driver;
        ice::render::RenderSurface& _render_surface;
        ice::render::RenderDevice* const _render_device;

        ice::render::RenderSwapchain* _render_swapchain;

        ice::pod::Array<ice::gfx::IceGfxQueueGroup*> _graphics_queues;
        ice::gfx::IceGfxResourceTracker _resource_tracker;
    };

    auto create_graphics_device(
        ice::Allocator& alloc,
        ice::render::RenderDriver& render_driver,
        ice::render::RenderSurface& render_surface,
        ice::Span<ice::RenderQueueDefinition const> render_queues
    ) noexcept -> ice::UniquePtr<ice::gfx::IceGfxDevice>;

} // namespace ice::gfx
