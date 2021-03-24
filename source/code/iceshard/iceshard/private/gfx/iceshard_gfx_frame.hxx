#pragma once
#include <ice/render/render_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include "iceshard_gfx_queue_group.hxx"

namespace ice::gfx
{

    class IceGfxFrame final : public ice::gfx::GfxFrame
    {
    public:
        IceGfxFrame(
            ice::render::RenderDevice* device,
            ice::render::RenderSwapchain* swapchain,
            ice::gfx::IceGfxQueueGroup* pass_group
        ) noexcept;

        ~IceGfxFrame() noexcept override;

        void present() noexcept;

        auto get_pass(
            ice::StringID_Arg name
        ) noexcept -> GfxQueue* override;

    private:
        ice::render::RenderDevice* _render_device;
        ice::render::RenderSwapchain* _render_swapchain;
        ice::gfx::IceGfxQueueGroup* _pass_group;
    };

} // namespace ice::gfx
