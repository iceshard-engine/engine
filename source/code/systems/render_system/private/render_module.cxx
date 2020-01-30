#include <render_system/render_module.hxx>
#include <render_system/render_api.hxx>
#include <core/platform/windows.hxx>

#include <filesystem>

namespace iceshard::renderer
{

    namespace detail
    {
        using RenderSystemCreateFunc = render::RenderSystem*(core::allocator&, core::cexpr::stringid_hash_type, void*);
        using RenderSystemReleaseFunc = void(core::allocator&, render::RenderSystem*);

        //! \brief A dynamic loaded engine DLL module.
        class RenderSystemDynamicModule : public iceshard::renderer::RenderSystemModule
        {
        public:
            RenderSystemDynamicModule(
                core::allocator& alloc,
                HMODULE handle,
                render::RenderSystem* instance,
                api::RenderInterface render_api,
                RenderSystemReleaseFunc* release_func) noexcept
                : _allocator{ alloc }
                , _handle{ handle }
                , _instance{ instance }
                , _render_api{ std::move(render_api) }
                , _release_func{ release_func }
            {
            }

            ~RenderSystemDynamicModule() override
            {
                _release_func(_allocator, _instance);
                FreeLibrary(_handle);
            }

            auto render_system() noexcept -> render::RenderSystem* override
            {
                return _instance;
            }

            auto render_system() const noexcept -> const render::RenderSystem* override
            {
                return _instance;
            }

            auto render_api() noexcept -> api::RenderInterface* override
            {
                return &_render_api;
            }

        private:
            core::allocator& _allocator;

            //! \brief Loaded module handle.
            const HMODULE _handle;

            //! \brief Loaded render system instance.
            render::RenderSystem* const _instance;

            //! \brief Loaded render api.
            iceshard::renderer::api::RenderInterface _render_api;

            //! \brief Engine release procedure.
            RenderSystemReleaseFunc* const _release_func;
        };

    } // namespace detail

    auto load_render_system_module(core::allocator& alloc, core::StringView path) noexcept -> core::memory::unique_pointer<iceshard::renderer::RenderSystemModule>
    {
        auto module_path = std::filesystem::canonical(path);

        // The result object
        core::memory::unique_pointer<iceshard::renderer::RenderSystemModule> result{ nullptr, { alloc } };

        // Try to load the module.
        HMODULE module_handle = LoadLibraryEx(module_path.generic_string().c_str(), NULL, NULL);
        if (module_handle != nullptr)
        {
            void* create_engine_addr = GetProcAddress(module_handle, "create_render_system");
            void* release_engine_addr = GetProcAddress(module_handle, "release_render_system");

            // Check both functions.
            if (create_engine_addr && release_engine_addr)
            {
                auto create_func = reinterpret_cast<detail::RenderSystemCreateFunc*>(create_engine_addr);
                auto release_func = reinterpret_cast<detail::RenderSystemReleaseFunc*>(release_engine_addr);

                iceshard::renderer::api::RenderInterface render_api{};
                auto* const module_instace = create_func(alloc, iceshard::renderer::api::version_name.hash_value, &render_api);
                if (module_instace != nullptr)
                {
                    result = { alloc.make<detail::RenderSystemDynamicModule>(alloc, module_handle, module_instace, render_api, release_func), alloc };
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
