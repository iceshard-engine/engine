#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>

namespace ice::gfx
{

    class GfxStage;

    class GfxPass
    {
    public:
        virtual ~GfxPass() noexcept = default;

        virtual bool presenting() const noexcept = 0;
        virtual void set_presenting(bool is_presenting) noexcept = 0;

        virtual auto add_stage(
            ice::StringID_Arg name,
            ice::Span<ice::gfx::GfxStage*> fence_wait
        ) noexcept -> ice::gfx::GfxStage* = 0;
    };

} // namespace ice::gfx
