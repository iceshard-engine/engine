#pragma once
#include <ice/stringid.hxx>
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    class GfxStage;

    class GfxPass
    {
    public:
        virtual ~GfxPass() noexcept = default;

        virtual void add_stage(
            ice::StringID_Arg name,
            ice::gfx::GfxStage* stage
        ) noexcept = 0;

        virtual void add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::StringID const> dependencies,
            ice::gfx::GfxStage* stage
        ) noexcept = 0;
    };

} // namespace ice::gfx
