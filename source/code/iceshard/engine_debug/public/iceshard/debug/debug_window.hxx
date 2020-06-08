#pragma once
#include <core/allocator.hxx>
#include <core/message/types.hxx>
#include <core/pointer.hxx>

namespace iceshard::debug
{

    enum class debugui_context_handle : uintptr_t
    {
        Invalid = 0x0
    };

    class DebugWindow
    {
    public:
        DebugWindow(debugui_context_handle context_handle) noexcept;

        virtual ~DebugWindow() noexcept = default;

        virtual void update([[maybe_unused]] core::MessageBuffer const& messages) noexcept { }

        virtual void begin_frame() noexcept { };

        virtual void end_frame() noexcept = 0;
    };

    auto load_debugui_from_module(
        core::allocator& alloc,
        core::ModuleHandle module,
        debugui_context_handle context
    ) noexcept -> core::memory::unique_pointer<DebugWindow>;

} // namespace debugui
