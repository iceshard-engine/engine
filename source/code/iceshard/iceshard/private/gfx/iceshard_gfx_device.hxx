#pragma once
#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>

namespace ice::gfx
{

    class IceGfxFrame;

    class IceGfxDevice final : public ice::gfx::GfxDevice
    {
    public:
        IceGfxDevice(
            ice::Allocator& alloc,
            ice::render::RenderDriver* driver,
            ice::render::RenderSurface* render_surface
        ) noexcept;
        ~IceGfxDevice() noexcept override;

        auto device() noexcept -> ice::render::RenderDevice&;
        auto swapchain() noexcept -> ice::render::RenderSwapchain&;

        auto default_queue() noexcept -> ice::render::RenderQueue&;

        auto next_frame(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::gfx::IceGfxFrame>;

    private:
        void create_temporary_resources() noexcept;

    private:
        ice::render::RenderDriver* const _render_driver;
        ice::render::RenderSurface* const _render_surface;
        ice::render::QueueID const _default_queue_id;

        ice::render::RenderDevice* _render_device;
        ice::render::RenderSwapchain* _render_swapchain;
        ice::pod::Array<ice::render::RenderQueue*> _render_queues;

        // Temporary here
        ice::render::RenderPass _render_pass;
        ice::render::Image _depth_stencil_image;
        ice::render::Framebuffer _render_framebuffers[2];
    };

} // namespace ice::gfx
