/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/string_types.hxx>
#include <core/message/operations.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::debug
{

    enum class DebugContextHandle : uintptr_t;

    class DebugSystem;

    class DebugModule
    {
    public:
        virtual ~DebugModule() noexcept = default;

        virtual void on_register(DebugContextHandle handle) noexcept;

        virtual void on_initialize(DebugSystem& system) noexcept = 0;

        virtual void on_deinitialize(DebugSystem& system) noexcept = 0;
    };

    auto load_debug_module_from_handle(
        core::allocator& alloc,
        core::ModuleHandle handle,
        DebugSystem& debug_system
    ) noexcept -> core::memory::unique_pointer<DebugModule>;

} // namespace iceshard::debug
