/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/string_types.hxx>
#include <core/message/operations.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard
{

    class Engine;

} // namespace iceshard

namespace iceshard::debug
{

    class DebugModule;

    class DebugWindow;

    enum class DebugContextHandle : uintptr_t
    {
        Invalid = 0x0
    };

    class DebugSystem
    {
    public:
        virtual ~DebugSystem() noexcept = default;

        virtual void register_module(DebugModule& module) noexcept = 0;

        virtual void unregister_module(DebugModule& module) noexcept = 0;

        virtual void register_window(
            core::stringid_arg_type name,
            DebugWindow& window
        ) noexcept = 0;

        virtual void unregister_window(
            core::stringid_arg_type name
        ) noexcept = 0;
    };

    class DebugSystemModule
    {
    public:
        virtual ~DebugSystemModule() noexcept = default;

        virtual auto debug_system() noexcept -> DebugSystem& = 0;
    };

    auto load_debug_system_module(
        core::allocator& alloc,
        core::StringView path,
        iceshard::Engine& engine
    ) noexcept -> core::memory::unique_pointer<DebugSystemModule>;

} // namespace iceshard::debug
