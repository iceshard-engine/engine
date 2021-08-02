#pragma once
#include <ice/engine_frame.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    class GfxDevice;
    class GfxContext;

    class GfxContextStage
    {
    public:
        virtual ~GfxContextStage() noexcept = default;

        virtual void prepare_context(
            ice::gfx::GfxContext& context,
            ice::gfx::GfxDevice& device
        ) const noexcept { }

        virtual void clear_context(
            ice::gfx::GfxContext& context,
            ice::gfx::GfxDevice& device
        ) const noexcept { }

        virtual void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept = 0;
    };

    class GfxFrameStage
    {
    public:
        virtual ~GfxFrameStage() noexcept = default;

        virtual void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept = 0;
    };

} // namespace ice::gfx
