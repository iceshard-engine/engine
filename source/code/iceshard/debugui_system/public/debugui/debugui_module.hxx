#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/string_types.hxx>
#include <core/message/operations.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard
{

    class Engine;

    namespace debug
    {

        enum class debugui_context_handle : uintptr_t;

        class DebugUI;

        class DebugUIContext : public ComponentSystem
        {
        public:
            virtual ~DebugUIContext() noexcept = default;

            virtual void register_ui(DebugUI* ui_object) noexcept = 0;

            virtual auto context_handle() const noexcept -> debugui_context_handle = 0;
        };

        class DebugUIModule
        {
        public:
            virtual ~DebugUIModule() noexcept = default;

            virtual auto context() noexcept -> DebugUIContext & = 0;

            virtual auto context_handle() noexcept -> debugui_context_handle = 0;
        };

        auto load_module(
            core::allocator& alloc,
            core::StringView path,
            iceshard::Engine& engine
        ) noexcept -> core::memory::unique_pointer<DebugUIModule>;

    } // namespace debug

} // namespace iceshard
