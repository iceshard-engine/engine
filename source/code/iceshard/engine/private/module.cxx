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
            EngineDynamicModule(
                core::allocator& alloc,
                HMODULE handle,
                EngineCreateFunc* create_func,
                EngineReleaseFunc* release_func
            ) noexcept
                : _allocator{ alloc }
                , _handle{ handle }
                , _create_func{ create_func }
                , _release_func{ release_func }
            { }

            ~EngineDynamicModule() override
            {
                FreeLibrary(_handle);
            }

            auto create_instance(
                core::allocator& alloc,
                resource::ResourceSystem& resources
            ) noexcept -> EnginePtr override
            {
                return EnginePtr {
                    _create_func(alloc, resources),
                    EnginePtr::deleter_type{ alloc, _release_func }
                };
            }

            auto native_handle() noexcept -> core::ModuleHandle
            {
                return core::ModuleHandle{ reinterpret_cast<uintptr_t>(_handle) };
            }

        private:
            core::allocator& _allocator;

            //! \brief Loaded module handle.
            const HMODULE _handle;

            //! \brief Engine release procedure.
            EngineCreateFunc* const _create_func;
            EngineReleaseFunc* const _release_func;
        };

        auto load_engine_module(
            core::allocator& alloc,
            core::StringView path
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

                    result = alloc.make<detail::EngineDynamicModule>(alloc, module_handle, create_func, release_func);
                }
            }

            return result;
        }

    } // namespace detail

    auto load_engine_module(
        core::allocator& alloc,
        core::StringView path
    ) noexcept -> core::memory::unique_pointer<EngineModule>
    {
        return { detail::load_engine_module(alloc, path), { alloc } };
    }

} // namespace iceshard
