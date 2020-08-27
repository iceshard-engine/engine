#pragma once
#include <iceshard/frame.hxx>

namespace iceshard::debug
{

    class DebugWindow
    {
    public:
        virtual ~DebugWindow() noexcept = default;

        virtual void update(Frame const& inputs) noexcept { }

        virtual void begin_frame() noexcept { }

        virtual void end_frame() noexcept { }
    };

} // namespace debugui
