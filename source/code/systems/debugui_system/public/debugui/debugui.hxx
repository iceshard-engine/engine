#pragma once
#include <core/allocator.hxx>

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

        virtual void on_draw() noexcept = 0;
    };

} // namespace debugui
