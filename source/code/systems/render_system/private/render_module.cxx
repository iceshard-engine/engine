#include <iceshard/renderer/render_module.hxx>
#include <iceshard/renderer/render_api.hxx>
#include <core/platform/windows.hxx>

#include <filesystem>

namespace iceshard::renderer
{

    auto RenderModule::render_system() -> RenderSystem*
    {
        api::render_module_api = render_module_interface();
        return reinterpret_cast<RenderSystem*>(api::render_module_api->internal_data);
    }

    auto RenderModule::render_system() const -> RenderSystem const*
    {
        api::render_module_api = render_module_interface();
        return reinterpret_cast<RenderSystem*>(api::render_module_api->internal_data);
    }

    namespace detail
    {
        using RenderModuleCreateFunc = auto(core::allocator&, core::cexpr::stringid_hash_type) -> RenderModule*;
        using RenderModuleReleaseFunc = void(core::allocator&, RenderModule*);

        //! \brief A dynamic loaded engine DLL module.
        class RenderSystemDynamicModule : public iceshard::renderer::RenderModuleInstance
        {
        public:
            RenderSystemDynamicModule(
                core::allocator& alloc,
                HMODULE handle,
                RenderModule* instance,
                RenderModuleReleaseFunc* release_func
            ) noexcept
                : _allocator{ alloc }
                , _handle{ handle }
                , _instance{ instance }
                , _release_func{ release_func }
            {
            }

            ~RenderSystemDynamicModule() override
            {
                _release_func(_allocator, _instance);
                FreeLibrary(_handle);
            }

            auto render_module() noexcept -> iceshard::renderer::RenderModule* override
            {
                return _instance;
            }

            auto render_module() const noexcept -> iceshard::renderer::RenderModule  const* override
            {
                return _instance;
            }

        private:
            core::allocator& _allocator;

            //! \brief Loaded module handle.
            const HMODULE _handle;

            //! \brief Loaded render module instance.
            iceshard::renderer::RenderModule* const _instance;

            //! \brief Engine release procedure.
            RenderModuleReleaseFunc* const _release_func;
        };

    } // namespace detail

    auto load_render_system_module(core::allocator& alloc, core::StringView path) noexcept -> core::memory::unique_pointer<iceshard::renderer::RenderModuleInstance>
    {
        auto module_path = std::filesystem::canonical(path);

        // The result object
        core::memory::unique_pointer<iceshard::renderer::RenderModuleInstance> result{ nullptr, { alloc } };

        // Try to load the module.
        HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
        if (module_handle != nullptr)
        {
            void* create_addr = GetProcAddress(module_handle, "create_render_module");
            void* release_addr = GetProcAddress(module_handle, "release_render_module");

            // Check both functions.
            if (create_addr && release_addr)
            {
                auto create_func = reinterpret_cast<detail::RenderModuleCreateFunc*>(create_addr);
                auto release_func = reinterpret_cast<detail::RenderModuleReleaseFunc*>(release_addr);

                auto* const module_instace = create_func(alloc, iceshard::renderer::api::version_name.hash_value);
                if (module_instace != nullptr)
                {
                    result = { alloc.make<detail::RenderSystemDynamicModule>(alloc, module_handle, module_instace, release_func), alloc };
                }
                else
                {
                    FreeLibrary(module_handle);
                }
            }
        }

        return result;
    }

} // namespace render
