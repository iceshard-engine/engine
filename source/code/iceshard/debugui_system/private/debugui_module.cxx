#pragma once
#include <core/allocator.hxx>
#include <debugui/debugui_module.hxx>

#include <filesystem>

#include <core/platform/windows.hxx>

namespace iceshard::debug
{

    namespace detail
    {

        using DebugUICreateFunc = DebugUIContext* (core::allocator&, iceshard::Engine&);
        using DebugUIReleaseFunc = void(core::allocator&, DebugUIContext*);

        class DebugImGuiModule : public DebugUIModule
        {
        public:
            DebugImGuiModule(core::allocator& alloc, HMODULE module_handle, DebugUIContext* context, DebugUIReleaseFunc* release_func) noexcept
                : _allocator{ alloc }
                , _module_handle{ module_handle }
                , _context{ context }
                , _release_func( release_func )
            {
            }

            ~DebugImGuiModule() noexcept
            {
                _release_func(_allocator, _context);
                FreeLibrary(_module_handle);
            }

            auto context() noexcept -> DebugUIContext& override
            {
                return *_context;
            }

            auto context_handle() noexcept -> debugui_context_handle override { return _context->context_handle(); }

        private:
            core::allocator& _allocator;
            HMODULE _module_handle;
            DebugUIContext* _context;
            DebugUIReleaseFunc* _release_func;
        };

    } // namespace detail


    auto load_module(
        core::allocator& alloc,
        core::StringView path,
        iceshard::Engine& engine
    ) noexcept -> core::memory::unique_pointer<DebugUIModule>
    {
        auto module_path = std::filesystem::canonical(path);

        // The result object
        core::memory::unique_pointer<DebugUIModule> result{ nullptr, { alloc } };

        // Try to load the module.
        HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
        if (module_handle != nullptr)
        {
            void* create_debugui_addr = GetProcAddress(module_handle, "create_debugui");
            void* release_debugui_addr = GetProcAddress(module_handle, "release_debugui");

            // Check both functions.
            if (create_debugui_addr && release_debugui_addr)
            {
                auto create_func = reinterpret_cast<detail::DebugUICreateFunc*>(create_debugui_addr);
                auto release_func = reinterpret_cast<detail::DebugUIReleaseFunc*>(release_debugui_addr);

                result = { alloc.make<detail::DebugImGuiModule>(alloc, module_handle, create_func(alloc, engine), release_func), alloc };
            }
        }

        return result;
    }

} // namespace debugui
