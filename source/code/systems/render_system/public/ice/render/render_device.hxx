#pragma once
#include <ice/render/render_queue.hxx>
#include <ice/render/render_pass.hxx>
#include <ice/render/render_framebuffer.hxx>

namespace ice::render
{

    class RenderSurface;

    class RenderSwapchain;

    class RenderDevice
    {
    protected:
        virtual ~RenderDevice() noexcept = default;

    public:
        //virtual auto active_queue_count() const noexcept -> ice::u32 = 0;
        virtual auto create_swapchain(
            ice::render::RenderSurface* surface
        ) noexcept -> ice::render::RenderSwapchain* = 0;

        virtual void destroy_swapchain(
            ice::render::RenderSwapchain* swapchain
        ) noexcept = 0;

        virtual auto create_renderpass(
            ice::render::RenderPassInfo const& info
        ) noexcept -> ice::render::RenderPass = 0;

        virtual void destroy_renderpass(
            ice::render::RenderPass render_pass
        ) noexcept = 0;

        virtual auto create_framebuffer(
            ice::vec2u extent,
            ice::render::RenderPass renderpass,
            ice::Span<ice::render::Image> images
        ) noexcept -> ice::render::Framebuffer = 0;

        virtual void destroy_framebuffer(
            ice::render::Framebuffer framebuffer
        ) noexcept = 0;

        virtual auto create_image(
            ice::render::ImageInfo image_info,
            ice::Data data
        ) noexcept -> ice::render::Image = 0;

        virtual void destroy_image(
            ice::render::Image image
        ) noexcept = 0;

        virtual auto create_queue(
            ice::render::QueueID queue_id,
            ice::u32 queue_index,
            ice::u32 command_pools
        ) const noexcept -> ice::render::RenderQueue* = 0;

        virtual void destroy_queue(
            ice::render::RenderQueue* queue
        ) const noexcept = 0;

        virtual auto get_commands() noexcept -> ice::render::RenderCommands& = 0;
    };

} // namespace ice::render
