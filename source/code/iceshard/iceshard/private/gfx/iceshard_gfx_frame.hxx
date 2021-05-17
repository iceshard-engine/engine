#pragma once
#include <ice/gfx/gfx_frame.hxx>
#include <ice/render/render_device.hxx>
#include <ice/engine_frame.hxx>


#include <ice/memory/proxy_allocator.hxx>
#include <ice/collections.hxx>
#include <ice/task.hxx>

#include "../iceshard_task_executor.hxx"
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

        auto aquire_task_commands(ice::StringID_Arg) noexcept -> GfxFrame::GfxAwaitCommandsOperation override;

        auto frame_end() noexcept -> GfxAwaitFrameEnd override;

        void execute_task(ice::Task<void> task) noexcept override;
        void start_all() noexcept;
        void wait_ready() noexcept;

        void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept override;

        void prepare_frame(
            ice::gfx::IceGfxQueueGroup& queue_group
        ) noexcept;

        void execute_passes(
            ice::EngineFrame const& frame,
            ice::gfx::IceGfxQueueGroup& queue_group
        ) noexcept;

        auto task_commands(
            ice::StringID_Arg queue_name
        ) noexcept -> ice::gfx::GfxTaskCommands& override;

    private:
        ice::memory::ProxyAllocator _allocator;

        ice::pod::Hash<ice::gfx::GfxPass*> _enqueued_passes;
        ice::Vector<ice::Task<>> _frame_tasks;

        // #todo
        ice::gfx::IceGfxQueueGroup* _queue_group;
        ice::IceshardTaskExecutor _task_executor;
    };

} // namespace ice::gfx
