#pragma once
#include <ice/render/render_device.hxx>
#include <ice/engine_frame.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/collections.hxx>
#include <ice/task.hxx>

#include "iceshard_gfx_queue_group.hxx"

namespace ice::gfx
{

    class IceGfxFrame final : public ice::gfx::GfxFrame
    {
    public:
        IceGfxFrame(
            ice::Allocator& alloc
        ) noexcept;

        ~IceGfxFrame() noexcept override;

        void execute_task(ice::Task<void> task) noexcept override;
        void wait_ready() noexcept;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept override;

        void execute_passes(
            ice::EngineFrame const& frame,
            ice::gfx::IceGfxQueueGroup& queue_group
        ) noexcept;

    private:
        ice::memory::ProxyAllocator _allocator;
        ice::render::RenderDevice* _render_device;

        ice::pod::Hash<ice::gfx::GfxPass*> _enqueued_passes;
        ice::Vector<ice::Task<>> _frame_tasks;
    };

} // namespace ice::gfx
