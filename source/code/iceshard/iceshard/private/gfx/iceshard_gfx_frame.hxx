#pragma once
#include <ice/render/render_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include "iceshard_gfx_queue_group.hxx"

namespace ice::gfx
{

    class IceGfxBaseFrame : public ice::gfx::GfxFrame
    {
    public:
        IceGfxBaseFrame() noexcept = default;
        ~IceGfxBaseFrame() noexcept override = default;

        virtual void present() noexcept { }

        virtual auto get_queue(
            ice::StringID_Arg name
        ) noexcept -> GfxQueue* override
        {
            return nullptr;
        }
    };

    class IceGfxFrame final : public IceGfxBaseFrame
    {
    public:
        IceGfxFrame(
            ice::render::RenderDevice* device,
            ice::render::RenderSwapchain* swapchain,
            ice::gfx::IceGfxQueueGroup* pass_group
        ) noexcept;

        ~IceGfxFrame() noexcept override;

        void present() noexcept;

        auto get_queue(
            ice::StringID_Arg name
        ) noexcept -> GfxQueue* override;

    private:
        ice::render::RenderDevice* _render_device;
        ice::render::RenderSwapchain* _render_swapchain;
        ice::gfx::IceGfxQueueGroup* _queue_group;
    };

} // namespace ice::gfx
