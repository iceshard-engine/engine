#pragma once
#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>

namespace ice::gfx
{

    class IceGfxFrame;

    class IceGfxPassGroup;

    class IceGfxDevice final : public ice::gfx::GfxDevice
    {
    public:
        IceGfxDevice(
            ice::Allocator& alloc,
            ice::render::RenderDriver* driver,
            ice::render::RenderSurface* render_surface,
            ice::render::RenderDevice* render_device,
            ice::pod::Array<ice::gfx::IceGfxPassGroup*> graphics_passes
        ) noexcept;
        ~IceGfxDevice() noexcept override;

        auto device() noexcept -> ice::render::RenderDevice& override;
        auto swapchain() noexcept -> ice::render::RenderSwapchain const& override;

        auto default_queue() noexcept -> ice::render::RenderQueue&;

        auto next_frame(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::gfx::IceGfxFrame>;

    private:
        ice::Allocator& _allocator;
        ice::render::RenderDriver* const _render_driver;
        ice::render::RenderSurface* const _render_surface;
        ice::render::RenderDevice* const _render_device;

        ice::render::RenderSwapchain* _render_swapchain;

        ice::pod::Array<ice::gfx::IceGfxPassGroup*> _graphics_passes;
    };

    auto create_graphics_device(
        ice::Allocator& alloc,
        ice::gfx::GfxDeviceCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::gfx::IceGfxDevice>;

} // namespace ice::gfx
