#pragma once
#include <core/allocator.hxx>
#include <core/message/types.hxx>
#include <core/pointer.hxx>

namespace iceshard::debug
{

    class DebugWindow
    {
    public:
        virtual ~DebugWindow() noexcept = default;

        virtual void update(core::MessageBuffer const& messages) noexcept { }

        virtual void begin_frame() noexcept { }

        virtual void end_frame() noexcept { }
    };

} // namespace debugui
