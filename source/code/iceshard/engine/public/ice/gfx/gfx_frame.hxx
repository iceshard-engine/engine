#pragma once
#include <ice/stringid.hxx>
#include <ice/task.hxx>
#include <ice/task_operations.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_task_operation.hxx>

namespace ice::gfx
{

    class GfxFrame
    {
    public:
        virtual void add_task(ice::Task<> task) noexcept = 0;

        virtual void set_stage_slot(
            ice::StringID_Arg stage_name,
            ice::gfx::GfxContextStage const* stage
        ) noexcept = 0;

        virtual void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::StringID_Arg pass_name,
            ice::gfx::GfxPass const* pass
        ) noexcept = 0;

        [[nodiscard]]
        auto frame_begin() noexcept -> ice::gfx::GfxAwaitBeginFrame;

        [[nodiscard]]
        auto frame_commands(ice::gfx::GfxFrameStage const* stage) noexcept -> ice::gfx::GfxAwaitExecuteStage;

        [[nodiscard]]
        auto frame_end() noexcept -> ice::gfx::GfxAwaitEndFrame;

    protected:
        virtual ~GfxFrame() noexcept = default;

    private:
        friend GfxAwaitBeginFrame;
        friend GfxAwaitExecuteStage;
        friend GfxAwaitEndFrame;

        virtual void schedule_internal(
            ice::gfx::GfxAwaitBeginFrameData& operation
        ) noexcept = 0;

        virtual void schedule_internal(
            ice::gfx::GfxAwaitExecuteStageData& operation
        ) noexcept = 0;

        virtual void schedule_internal(
            ice::gfx::GfxAwaitEndFrameData& operation
        ) noexcept = 0;
    };

} // namespace ice::gfx
