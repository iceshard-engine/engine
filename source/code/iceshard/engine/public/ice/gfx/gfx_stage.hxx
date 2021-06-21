#pragma once
#include <ice/task.hxx>
#include <ice/engine_frame.hxx>
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    class GfxDevice;

    class GfxStage
    {
    public:
        virtual ~GfxStage() noexcept = default;

        virtual void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) noexcept = 0;
    };

    class GfxUpdateStage
    {
    public:
        virtual ~GfxUpdateStage() noexcept = default;

        virtual void prepare_data(
            ice::gfx::GfxDevice& device
        ) noexcept { }

        virtual auto record_commands(
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) noexcept -> ice::Task<void> { co_return; }

        virtual auto final_task() noexcept -> ice::Task<void> { co_return; }
    };

} // namespace ice::gfx
