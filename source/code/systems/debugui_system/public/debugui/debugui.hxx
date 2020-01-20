#pragma once
#include <core/allocator.hxx>
#include <core/message/types.hxx>

namespace debugui
{

    enum class debugui_context_handle : uintptr_t
    {
        Invalid = 0x0
    };

    class DebugUI
    {
    public:
        DebugUI(debugui_context_handle context_handle) noexcept;
        ~DebugUI() noexcept;

        virtual void update([[maybe_unused]] core::MessageBuffer const& messages) noexcept { }

        virtual void begin_frame() noexcept { };

        virtual void end_frame() noexcept = 0;
    };

} // namespace debugui
