#pragma once
#include <ice/render/render_device.hxx>
#include <ice/engine_frame.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include "iceshard_gfx_queue_group.hxx"

namespace ice::gfx
{

    class IceGfxBaseFrame : public ice::gfx::GfxFrame
    {
    public:
        IceGfxBaseFrame(ice::gfx::IceGfxQueueGroup* queue_group) noexcept;
        ~IceGfxBaseFrame() noexcept override = default;

        virtual void present() noexcept { }

        auto get_queue(
            ice::StringID_Arg name
        ) noexcept -> GfxQueue* override final;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept override { }

        virtual void execute_passes(
            ice::EngineFrame const& frame
        ) noexcept { }

    protected:
        ice::gfx::IceGfxQueueGroup* _queue_group;
    };

    class IceGfxFrame final : public IceGfxBaseFrame
    {
    public:
        IceGfxFrame(
            ice::Allocator& alloc,
            ice::render::RenderDevice* device,
            ice::render::RenderSwapchain* swapchain,
            ice::gfx::IceGfxQueueGroup* pass_group
        ) noexcept;

        ~IceGfxFrame() noexcept override;

        void present() noexcept;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept override;

        void execute_passes(
            ice::EngineFrame const& frame
        ) noexcept override;

    private:
        ice::render::RenderDevice* _render_device;
        ice::render::RenderSwapchain* _render_swapchain;

        ice::pod::Hash<ice::gfx::GfxPass*> _enqueued_passes;
    };

} // namespace ice::gfx
