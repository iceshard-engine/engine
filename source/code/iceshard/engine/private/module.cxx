#include <iceshard/module.hxx>
#include <core/string_view.hxx>
#include <core/platform/windows.hxx>
#include <debugui/debugui.hxx>

#include <filesystem>

namespace iceshard
{
    namespace detail
    {
        using EngineCreateFunc = Engine*(core::allocator&, resource::ResourceSystem&);
        using EngineReleaseFunc = void(core::allocator&, Engine*);
        using EngineCreateDebugUiFunc = debugui::DebugUI*(core::allocator&, debugui::debugui_context_handle);
        using EngineReleaseDebugUiFunc = void(core::allocator&, debugui::DebugUI*);

        //! \brief A dynamic loaded engine DLL module.
        class EngineDynamicModule : public EngineModule
        {
        public:
            EngineDynamicModule(core::allocator& alloc, HMODULE handle, Engine* instance, EngineReleaseFunc* release_func) noexcept
                : _allocator{ alloc }
                , _handle{ handle }
                , _instance{ instance }
                , _release_func{ release_func }
            { }

            ~EngineDynamicModule() override
            {
                if (_debugui_instance != nullptr)
                {
                    IS_ASSERT(_debugui_release_func != nullptr, "Error while trying to release DebugUI object!");
                    _debugui_release_func(_allocator, _debugui_instance);
                }

                _release_func(_allocator, _instance);
                FreeLibrary(_handle);
            }

            auto load_debugui(debugui::DebugUIContext& debugui) noexcept -> debugui::DebugUI*
            {
                void* const create_debugui_addr = GetProcAddress(_handle, "create_debugui");
                void* const release_debugui_addr = GetProcAddress(_handle, "release_debugui");
                if (create_debugui_addr != nullptr && release_debugui_addr != nullptr)
                {
                    auto create_debugui = reinterpret_cast<EngineCreateDebugUiFunc*>(create_debugui_addr);
                    _debugui_instance = create_debugui(_allocator, debugui.context_handle());
                    if (_debugui_instance != nullptr)
                    {
                        _debugui_release_func = reinterpret_cast<EngineReleaseDebugUiFunc*>(release_debugui_addr);
                        debugui.register_ui(_debugui_instance);
                    }
                }
                return _debugui_instance;
            }

            auto engine() noexcept -> Engine* override
            {
                return _instance;
            }

            auto engine() const noexcept -> const Engine* override
            {
                return _instance;
            }

        private:
            core::allocator& _allocator;

            //! \brief Loaded module handle.
            const HMODULE _handle;

            //! \brief Loaded engine instance.
            Engine* const _instance;

            //! \brief Engine release procedure.
            EngineReleaseFunc* const _release_func;

            // Debug UI objects
            debugui::DebugUI* _debugui_instance = nullptr;
            EngineReleaseDebugUiFunc* _debugui_release_func = nullptr;
        };

        auto load_engine_module(
            core::allocator& alloc,
            core::StringView path,
            resource::ResourceSystem& resources
        ) noexcept -> EngineDynamicModule*
        {
            auto module_path = std::filesystem::canonical(path);

            // The result object
            EngineDynamicModule* result = nullptr;

            // Try to load the module.
            HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
            if (module_handle != nullptr)
            {
                void* create_engine_addr = GetProcAddress(module_handle, "create_engine");
                void* release_engine_addr = GetProcAddress(module_handle, "release_engine");

                // Check both functions.
                if (create_engine_addr && release_engine_addr)
                {
                    auto create_func = reinterpret_cast<detail::EngineCreateFunc*>(create_engine_addr);
                    auto release_func = reinterpret_cast<detail::EngineReleaseFunc*>(release_engine_addr);

                    result = alloc.make<detail::EngineDynamicModule>(alloc, module_handle, create_func(alloc, resources), release_func);
                }
            }

            return result;
        }

    } // namespace detail

    auto load_engine_module(
        core::allocator& alloc,
        core::StringView path,
        resource::ResourceSystem& resources
    ) noexcept -> core::memory::unique_pointer<EngineModule>
    {
        return { detail::load_engine_module(alloc, path, resources), { alloc } };
    }

} // namespace iceshard
