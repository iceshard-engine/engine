#pragma once
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    class GfxStage;

    class GfxPassResourceRequester
    {
    public:
    };

    class GfxPass
    {
    public:
        virtual ~GfxPass() noexcept = default;

        virtual void add_stage(
            ice::gfx::GfxStage* stage
        ) noexcept = 0;
    };

} // namespace ice::gfx
