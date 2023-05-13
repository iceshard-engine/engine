/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <iceshard/debug/debug_system.hxx>
#include <core/platform/windows.hxx>
#include <filesystem>

namespace iceshard::debug
{

    namespace detail
    {

        using DebugSystemCreateFunc = auto (core::allocator& alloc, iceshard::Engine&) -> DebugSystem*;
        using DebugSystemDestroyFunc = void (core::allocator& alloc, DebugSystem*);

        class IceDebugSystemModule : public DebugSystemModule
        {
        public:
            IceDebugSystemModule(
                core::allocator& alloc,
                HMODULE module_handle,
                DebugSystem* object,
                DebugSystemDestroyFunc* release_func
            ) noexcept
                : DebugSystemModule{}
                , _allocator{ alloc }
                , _module{ module_handle }
                , _object{ object }
                , _release_func{ release_func }
            {
            }

            ~IceDebugSystemModule() noexcept
            {
                _release_func(_allocator, _object);
                FreeLibrary(_module);
            }

            auto debug_system() noexcept -> DebugSystem& override
            {
                return *_object;
            }

        private:
            core::allocator& _allocator;
            HMODULE _module;
            DebugSystem* _object;
            DebugSystemDestroyFunc* _release_func;
        };

    } // namespace detail

    auto load_debug_system_module(
        core::allocator& alloc,
        core::StringView path,
        iceshard::Engine& engine
    ) noexcept -> core::memory::unique_pointer<DebugSystemModule>
    {
        auto module_path = std::filesystem::canonical(path);

        // The result object
        core::memory::unique_pointer<DebugSystemModule> result{ nullptr, { alloc } };

        // Try to load the module.
        HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
        if (module_handle != nullptr)
        {
            void* create_debug_sys_addr = GetProcAddress(module_handle, "create_debug_system");
            void* release_debug_sys_addr = GetProcAddress(module_handle, "release_debug_system");

            // Check both functions.
            if (create_debug_sys_addr && release_debug_sys_addr)
            {
                auto create_func = reinterpret_cast<detail::DebugSystemCreateFunc*>(create_debug_sys_addr);
                auto release_func = reinterpret_cast<detail::DebugSystemDestroyFunc*>(release_debug_sys_addr);

                if (auto* system = create_func(alloc, engine); system != nullptr)
                {
                    result = core::memory::make_unique<DebugSystemModule, detail::IceDebugSystemModule>(
                        alloc, alloc, module_handle, system, release_func
                    );
                }
            }
        }

        return result;
    }

} // iceshard::debug
