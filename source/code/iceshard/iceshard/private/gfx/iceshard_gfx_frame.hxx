#pragma once
#include <ice/render/render_device.hxx>
#include <ice/gfx/gfx_frame.hxx>

namespace ice::gfx
{

    class IceGfxFrame final : public ice::gfx::GfxFrame
    {
    public:
        IceGfxFrame(
            ice::render::RenderDevice* device,
            ice::render::RenderSwapchain* swapchain,
            ice::render::RenderPass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::render::RenderQueue* queue,
            ice::u32 queue_pool_idx
        ) noexcept;

        ~IceGfxFrame() noexcept override;

        void present() noexcept;

        auto get_pass(
            ice::StringID_Arg name
        ) noexcept -> GfxPass* override
        {
            return nullptr;
        }

    private:
        ice::render::RenderDevice* _render_device;
        ice::render::RenderSwapchain* _render_swapchain;
        ice::render::RenderQueue* _render_queue;
        ice::render::CommandBuffer _cmd_buffer;

        bool _quitting = false;
    };

} // namespace ice::gfx
