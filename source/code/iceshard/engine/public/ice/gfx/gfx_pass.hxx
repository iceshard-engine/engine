#pragma once
#include <ice/stringid.hxx>

namespace ice::gfx
{

    class GfxStage;

    class GfxPass
    {
    public:
        virtual ~GfxPass() noexcept = 0;

        virtual auto get_stage(
            ice::StringID_Arg name
        ) noexcept -> GfxStage* = 0;
    };

} // namespace ice::gfx
