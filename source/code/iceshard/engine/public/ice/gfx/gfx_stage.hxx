#pragma once
#include <ice/task.hxx>
#include <ice/engine_frame.hxx>
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    class GfxStage;

    enum class GfxStageType
    {
        InitialStage,
        TransferStage,
        FinalStage,
        DrawStage,
        CustomStage,
    };

    struct GfxStageInfo
    {
        ice::StringID name;
        ice::Span<ice::StringID const> dependencies;
        ice::gfx::GfxStageType type = GfxStageType::DrawStage;
    };

    struct GfxStageSlot
    {
        ice::StringID name;
        ice::gfx::GfxStage* stage;
    };

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

} // namespace ice::gfx
