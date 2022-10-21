#include <ice/module_register.hxx>
#include <ice/module.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/os/windows.hxx>

namespace ice
{

    class Win32ModuleRegister;

    struct Win32ModuleEntry
    {
        ice::StringID_Hash name;
        ice::Allocator* module_allocator;
        ice::ModuleProcGetAPI* lookup_api;
        ice::ModuleProcUnload* unload_proc;
    };

    struct ModuleNegotiatorContext
    {
        Win32ModuleRegister* module_register;
        Win32ModuleEntry current_module;

        static bool get_module_api(ModuleNegotiatorContext*, ice::StringID_Hash, ice::u32, void**) noexcept;
        static bool register_module(ModuleNegotiatorContext*, ice::StringID_Hash, ModuleProcGetAPI*) noexcept;
    };

#if ISP_WINDOWS

    bool utf8_to_wide_append_module(ice::String path, ice::HeapString<ice::wchar>& out_str) noexcept
    {
        ice::i32 const required_size = MultiByteToWideChar(CP_UTF8, 0, ice::string::begin(path), ice::string::size(path), NULL,  0);

        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            [[maybe_unused]]
            ice::i32 const chars_written = MultiByteToWideChar(
                CP_UTF8,
                0,
                ice::string::begin(path),
                ice::string::size(path),
                ice::string::begin(out_str) + current_size,
                ice::string::size(out_str) - current_size
            );
        }

        return required_size != 0;
    }

    class Win32ModuleRegister final : public ModuleRegister
    {
    public:
        Win32ModuleRegister(ice::Allocator& alloc) noexcept;
        ~Win32ModuleRegister() noexcept override;

        bool load_module(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept override;

        bool load_module(
            ice::Allocator& alloc,
            ice::ModuleProcLoad* load_fn,
            ice::ModuleProcUnload* unload_fn
        ) noexcept override;

        bool find_module_api(
            ice::StringID_Arg api_name,
            ice::u32 version,
            void** api_ptr
        ) const noexcept override;

        bool find_module_apis(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::Array<void*>& api_ptrs_out
        ) const noexcept override;

        bool register_module(
            ice::Win32ModuleEntry const& entry
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<Win32ModuleEntry> _modules;
        ice::Array<HMODULE> _module_handles;
    };

    Win32ModuleRegister::Win32ModuleRegister(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _modules{ _allocator }
        , _module_handles{ _allocator }
    { }

    Win32ModuleRegister::~Win32ModuleRegister() noexcept
    {
        for (Win32ModuleEntry const& entry : ice::hashmap::values(_modules))
        {
            entry.unload_proc(entry.module_allocator);
        }
    }

    bool Win32ModuleRegister::load_module(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept
    {
        ice::HeapString<ice::wchar> wide_path{ _allocator };
        if (utf8_to_wide_append_module(path, wide_path))
        {
            HMODULE module_handle{ LoadLibraryExW(ice::string::begin(wide_path), NULL, NULL) };
            if (module_handle)
            {
                void* const load_proc = GetProcAddress(module_handle, "ice_module_load");
                void* const unload_proc = GetProcAddress(module_handle, "ice_module_unload");

                if (load_proc != nullptr && unload_proc != nullptr)
                {
                    load_module(
                        alloc,
                        reinterpret_cast<ice::ModuleProcLoad*>(load_proc),
                        reinterpret_cast<ice::ModuleProcUnload*>(unload_proc)
                    );

                    ice::array::push_back(_module_handles, module_handle);
                }
            }
            return module_handle;
        }
        return { };
    }

    bool Win32ModuleRegister::load_module(
        ice::Allocator& alloc,
        ice::ModuleProcLoad* load_fn,
        ice::ModuleProcUnload* unload_fn
    ) noexcept
    {
        Win32ModuleEntry module_entry{
            .module_allocator = &alloc,
            .lookup_api = nullptr,
            .unload_proc = unload_fn,
        };

        ModuleNegotiatorContext negotiator_context{
            .module_register = this,
            .current_module = module_entry,
        };

        ModuleNegotiator negotiator{
            .fn_get_module_api = ModuleNegotiatorContext::get_module_api,
            .fn_register_module = ModuleNegotiatorContext::register_module,
        };

        load_fn(
            module_entry.module_allocator,
            &negotiator_context,
            &negotiator
        );
        return true;
    }

    bool Win32ModuleRegister::find_module_api(
        ice::StringID_Arg api_name,
        ice::u32 version,
        void** api_ptr
    ) const noexcept
    {
        if (api_ptr == nullptr)
        {
            return false;
        }

        bool api_found = false;

        auto it = ice::multi_hashmap::find_first(_modules, ice::hash(api_name));
        while (it != nullptr && api_found == false)
        {
            api_found = it.value().lookup_api(ice::stringid_hash(api_name), version, api_ptr);
            it = ice::multi_hashmap::find_next(_modules, it);
        }

        return api_found;
    }

    bool Win32ModuleRegister::find_module_apis(
        ice::StringID_Arg api_name,
        ice::u32 version,
        ice::Array<void*>& api_ptrs_out
    ) const noexcept
    {
        auto it = ice::multi_hashmap::find_first(_modules, ice::hash(api_name));
        while (it != nullptr)
        {
            void* api_ptr;
            if (it.value().lookup_api(ice::stringid_hash(api_name), version, &api_ptr))
            {
                ice::array::push_back(api_ptrs_out, api_ptr);
            }
            it = ice::multi_hashmap::find_next(_modules, it);
        }
        return ice::array::any(api_ptrs_out);
    }

    bool Win32ModuleRegister::register_module(
        ice::Win32ModuleEntry const& entry
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(entry.name);
        ice::multi_hashmap::insert(_modules, name_hash, entry);
        return true;
    }

    bool ModuleNegotiatorContext::get_module_api(
        ice::ModuleNegotiatorContext* ctx,
        ice::StringID_Hash api_name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        return ctx->module_register->find_module_api(ice::StringID{ api_name }, version, api_ptr);
    }

    bool ModuleNegotiatorContext::register_module(
        ice::ModuleNegotiatorContext* ctx,
        ice::StringID_Hash api_name,
        ice::ModuleProcGetAPI* api_lookup_proc
    ) noexcept
    {
        if (api_name != ice::stringid_hash(ice::StringID_Invalid) && api_lookup_proc != nullptr)
        {
            ctx->current_module.name = api_name;
            ctx->current_module.lookup_api = api_lookup_proc;
            return ctx->module_register->register_module(ctx->current_module);
        }
        return false;
    }

    auto create_default_module_register(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ModuleRegister>
    {
        return ice::make_unique<Win32ModuleRegister>(alloc, alloc);
    }

#else

    // #TODO: https://github.com/iceshard-engine/engine/issues/89
    auto create_default_module_register(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ModuleRegister>
    {
        return { };
    }

#endif // #if ISP_WINDOWS

} // namespace ice
