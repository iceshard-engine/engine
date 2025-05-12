/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <core/allocator.hxx>
#include <core/platform/windows.hxx>
#include <iceshard/debug/debug_module.hxx>
#include <iceshard/debug/debug_system.hxx>

#include <filesystem>
#include <imgui/imgui.h>

namespace iceshard::debug
{
    namespace detail
    {

        void initialize_imgui_context(DebugContextHandle handle) noexcept
        {
            ImGuiContext* const context = reinterpret_cast<ImGuiContext*>(handle);
            bool const is_same_context = ImGui::GetCurrentContext() == context;
            bool const is_null_context = ImGui::GetCurrentContext() == nullptr;

            IS_ASSERT(is_same_context || is_null_context, "Unexpected ImGui context value!");
            if (is_null_context)
            {
                ImGui::SetCurrentContext(context);
            }
        }

        using DebugModuleCreateFunc = auto (core::allocator&, iceshard::debug::DebugSystem&) -> DebugModule*;
        using DebugModuleReleaseFunc = void (core::allocator&, DebugModule*);

        class DebugModuleWrapper final : public DebugModule
        {
        public:
            DebugModuleWrapper(
                core::allocator& alloc,
                DebugSystem& debug_system,
                DebugModule* wrapped_object,
                DebugModuleReleaseFunc* release_func
            ) noexcept
                : _allocator{ alloc }
                , _debug_system{ debug_system }
                , _wrapped_object{ wrapped_object }
                , _release_func{ release_func }
            {
                _debug_system.register_module(*_wrapped_object);
            }

            ~DebugModuleWrapper() noexcept override
            {
                _debug_system.unregister_module(*_wrapped_object);
                _release_func(_allocator,  _wrapped_object);
            }

            void on_initialize(DebugSystem& system) noexcept override
            {
                _wrapped_object->on_initialize(system);
            }

            void on_deinitialize(DebugSystem& system) noexcept override
            {
                _wrapped_object->on_deinitialize(system);
            }

        private:
            core::allocator& _allocator;
            DebugSystem& _debug_system;
            DebugModule* _wrapped_object;
            DebugModuleReleaseFunc* _release_func;
        };

    } // namespace detail

    void DebugModule::on_register(DebugContextHandle handle) noexcept
    {
        detail::initialize_imgui_context(handle);
    }

    auto load_debug_module_from_handle(
        core::allocator& alloc,
        core::ModuleHandle handle,
        DebugSystem& debug_system
    ) noexcept -> core::memory::unique_pointer<DebugModule>
    {
        core::memory::unique_pointer<DebugModule> result{ nullptr, { alloc } };

        if (handle != core::ModuleHandle::Invalid)
        {
            HMODULE module_handle = reinterpret_cast<HMODULE>(handle);
            void* create_func_ptr = GetProcAddress(module_handle, "create_debug_module");
            void* release_func_ptr = GetProcAddress(module_handle, "release_debug_module");

            if (create_func_ptr != nullptr && release_func_ptr != nullptr)
            {
                auto* create_func = reinterpret_cast<detail::DebugModuleCreateFunc*>(create_func_ptr);
                auto* release_func = reinterpret_cast<detail::DebugModuleReleaseFunc*>(release_func_ptr);

                result = core::memory::make_unique<DebugModule, detail::DebugModuleWrapper>(
                    alloc, alloc, debug_system, create_func(alloc, debug_system), release_func
                );
            }
        }

        return result;
    }

} // namespace debugui
