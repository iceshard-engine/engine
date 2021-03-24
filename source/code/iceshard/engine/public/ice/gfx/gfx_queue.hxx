#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>

namespace ice::gfx
{

    class GfxStage;

    class GfxQueue
    {
    public:
        virtual ~GfxQueue() noexcept = default;

        virtual bool presenting() const noexcept = 0;
        virtual void set_presenting(bool is_presenting) noexcept = 0;

        virtual void add_stage(
            ice::StringID_Arg name,
            ice::gfx::GfxStage* stage,
            ice::Span<ice::gfx::GfxStage*> fence_wait
        ) noexcept = 0;
    };

} // namespace ice::gfx
