#pragma once
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    class GfxStage
    {
    public:
        virtual ~GfxStage() noexcept = default;

        virtual void record_commands(
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) noexcept = 0;
    };

} // namespace ice::gfx
