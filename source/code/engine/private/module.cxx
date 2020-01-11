#include <iceshard/module.hxx>
#include <core/string_view.hxx>
#include <core/platform/windows.hxx>

#include <filesystem>

namespace iceshard
{
    namespace detail
    {
        using EngineCreateFunc = Engine*(core::allocator&, resource::ResourceSystem&);
        using EngineReleaseFunc = void(core::allocator&, Engine*);

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
                _release_func(_allocator, _instance);
                FreeLibrary(_handle);
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
        };

    } // namespace detail

    auto load_engine_module(core::allocator& alloc, core::StringView path, resource::ResourceSystem& resources) noexcept -> core::memory::unique_pointer<EngineModule>
    {
        auto module_path = std::filesystem::canonical(path);

        // The result object
        core::memory::unique_pointer<EngineModule> result{ nullptr, { alloc } };

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

                result = { alloc.make<detail::EngineDynamicModule>(alloc, module_handle, create_func(alloc, resources), release_func), alloc };
            }
        }

        return result;
    }

} // namespace iceshard
