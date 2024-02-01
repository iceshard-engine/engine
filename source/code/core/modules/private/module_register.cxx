/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/module_register.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>
#include <ice/profiler.hxx>

#include "module_globals.hxx"

namespace ice
{

#if ISP_WINDOWS

    class Win32ModuleRegister;

    struct Win32ModuleEntry
    {
        ice::StringID_Hash name;
        ice::Allocator* module_allocator;
        ice::FnModuleSelectAPI* select_api;
        ice::FnModuleUnload* unload_proc;
    };

    struct ModuleNegotiatorAPIContext
    {
        Win32ModuleRegister* module_register;
        Win32ModuleEntry current_module;

        static bool get_module_api(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::u32, ice::ModuleAPI*) noexcept;
        static bool get_module_apis(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::u32, ice::ModuleAPI*, ice::u32*) noexcept;
        static bool register_module(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::FnModuleSelectAPI*) noexcept;
    };

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
            ice::FnModuleLoad* load_fn,
            ice::FnModuleUnload* unload_fn
        ) noexcept override;

        auto api_count(
            ice::StringID_Arg name,
            ice::u32 version
        ) const noexcept -> ice::ucount;

        bool query_apis(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::ModuleAPI* out_array,
            ice::ucount* inout_array_size
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
        IPT_ZONE_SCOPED;
        IPT_ZONE_TEXT_STR(path);

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
                        reinterpret_cast<ice::FnModuleLoad*>(load_proc),
                        reinterpret_cast<ice::FnModuleUnload*>(unload_proc)
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
        ice::FnModuleLoad* load_fn,
        ice::FnModuleUnload* unload_fn
    ) noexcept
    {
        Win32ModuleEntry module_entry{
            .module_allocator = &alloc,
            .select_api = nullptr,
            .unload_proc = unload_fn,
        };

        ModuleNegotiatorAPIContext negotiator_context{
            .module_register = this,
            .current_module = module_entry,
        };

        ModuleNegotiatorAPI negotiator{
            .fn_select_apis = ModuleNegotiatorAPIContext::get_module_apis,
            .fn_register_api = ModuleNegotiatorAPIContext::register_module,
        };

        load_fn(
            module_entry.module_allocator,
            &negotiator_context,
            &negotiator
        );
        return true;
    }

    auto Win32ModuleRegister::api_count(
        ice::StringID_Arg api_name,
        ice::u32 version
    ) const noexcept -> ice::ucount
    {
        ice::ucount result = 0;
        auto it = ice::multi_hashmap::find_first(_modules, ice::hash(api_name));
        while (it != nullptr)
        {
            ice::ModuleAPI api_ptr;
            if (it.value().select_api(ice::stringid_hash(api_name), version, &api_ptr))
            {
                result += 1;
            }
            it = ice::multi_hashmap::find_next(_modules, it);
        }
        return result;
    }

    bool Win32ModuleRegister::query_apis(
        ice::StringID_Arg api_name,
        ice::u32 version,
        ice::ModuleAPI* out_array,
        ice::ucount* inout_array_size
    ) const noexcept
    {
        if (out_array == nullptr)
        {
            if (inout_array_size == nullptr)
            {
                return false;
            }

            *inout_array_size = this->api_count(api_name, version);
            return *inout_array_size > 0;
        }

        ice::u32 idx = 0;
        auto it = ice::multi_hashmap::find_first(_modules, ice::hash(api_name));

        ice::ucount const array_size = *inout_array_size;
        while (it != nullptr && idx < array_size)
        {
            ice::ModuleAPI api_ptr;
            if (it.value().select_api(ice::stringid_hash(api_name), version, &api_ptr))
            {
                out_array[idx] = api_ptr;
                idx += 1;
            }
            it = ice::multi_hashmap::find_next(_modules, it);
        }
        return idx > 0;
    }

    ///////////////////////////////////////////////////////////////////////////

    bool Win32ModuleRegister::register_module(
        ice::Win32ModuleEntry const& entry
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(entry.name);
        ice::multi_hashmap::insert(_modules, name_hash, entry);
        return true;
    }

    bool ModuleNegotiatorAPIContext::get_module_apis(
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::StringID_Hash api_name,
        ice::u32 version,
        ice::ModuleAPI* api_ptr,
        ice::u32* array_size
    ) noexcept
    {
        return ctx->module_register->query_apis(ice::StringID{ api_name }, version, api_ptr, array_size);
    }

    bool ModuleNegotiatorAPIContext::register_module(
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::StringID_Hash api_name,
        ice::FnModuleSelectAPI* api_select_proc
    ) noexcept
    {
        if (api_name != ice::stringid_hash(ice::StringID_Invalid) && api_select_proc != nullptr)
        {
            ctx->current_module.name = api_name;
            ctx->current_module.select_api = api_select_proc;
            return ctx->module_register->register_module(ctx->current_module);
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////

    auto create_default_module_register(ice::Allocator& alloc, bool load_global_modules /*= true*/) noexcept -> ice::UniquePtr<ModuleRegister>
    {
        ice::UniquePtr<ModuleRegister> result = ice::make_unique<Win32ModuleRegister>(alloc, alloc);
        if (result != nullptr && load_global_modules)
        {
            ice::load_global_modules(alloc, *result);
        }
        return result;
    }

#else

    class UnixModuleRegister;

    struct UnixModuleEntry
    {
        ice::StringID_Hash name;
        ice::Allocator* module_allocator;
        ice::FnModuleSelectAPI* select_api;
        ice::FnModuleUnload* unload_proc;
    };

    struct ModuleNegotiatorAPIContext
    {
        UnixModuleRegister* module_register;
        UnixModuleEntry current_module;

        static bool get_module_api(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::u32, ice::ModuleAPI*) noexcept;
        static bool get_module_apis(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::u32, ice::ModuleAPI*, ice::ucount*) noexcept;
        static bool register_module(ModuleNegotiatorAPIContext*, ice::StringID_Hash, FnModuleSelectAPI*) noexcept;
    };

    class UnixModuleRegister final : public ModuleRegister
    {
    public:
        UnixModuleRegister(ice::Allocator& alloc) noexcept;
        ~UnixModuleRegister() noexcept override;

        bool load_module(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept override;

        bool load_module(
            ice::Allocator& alloc,
            ice::FnModuleLoad* load_fn,
            ice::FnModuleUnload* unload_fn
        ) noexcept override;

        auto api_count(
            ice::StringID_Arg name,
            ice::u32 version
        ) const noexcept -> ice::ucount;

        bool query_apis(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::ModuleAPI* out_array,
            ice::ucount* inout_array_size
        ) const noexcept override;

        bool register_module(
            ice::UnixModuleEntry const& entry
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<UnixModuleEntry> _modules;
        ice::Array<void*> _module_handles;
    };

    UnixModuleRegister::UnixModuleRegister(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _modules{ _allocator }
        , _module_handles{ _allocator }
    { }

    UnixModuleRegister::~UnixModuleRegister() noexcept
    {
        for (UnixModuleEntry const& entry : ice::hashmap::values(_modules))
        {
            entry.unload_proc(entry.module_allocator);
        }
        for (void* handle : _module_handles)
        {
            dlclose(handle);
        }
    }

    bool UnixModuleRegister::load_module(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_ZONE_TEXT_STR(path);

        ice::HeapString<> utf8_path{ _allocator, path };
        {
            void* module_handle{ dlopen(ice::string::begin(utf8_path), 0) };
            if (module_handle)
            {
                void* const load_proc = dlsym(module_handle, "ice_module_load");
                void* const unload_proc = dlsym(module_handle, "ice_module_unload");

                if (load_proc != nullptr && unload_proc != nullptr)
                {
                    load_module(
                        alloc,
                        reinterpret_cast<ice::FnModuleLoad*>(load_proc),
                        reinterpret_cast<ice::FnModuleUnload*>(unload_proc)
                    );

                    ice::array::push_back(_module_handles, module_handle);
                }
            }
            return module_handle;
        }
        return { };
    }

    bool UnixModuleRegister::load_module(
        ice::Allocator& alloc,
        ice::FnModuleLoad* load_fn,
        ice::FnModuleUnload* unload_fn
    ) noexcept
    {
        UnixModuleEntry module_entry{
            .module_allocator = &alloc,
            .select_api = nullptr,
            .unload_proc = unload_fn,
        };

        ModuleNegotiatorAPIContext negotiator_context{
            .module_register = this,
            .current_module = module_entry,
        };

        ModuleNegotiatorAPI negotiator{
            .fn_select_apis = ModuleNegotiatorAPIContext::get_module_apis,
            .fn_register_api = ModuleNegotiatorAPIContext::register_module,
        };

        load_fn(
            module_entry.module_allocator,
            &negotiator_context,
            &negotiator
        );
        return true;
    }

    auto UnixModuleRegister::api_count(
        ice::StringID_Arg api_name,
        ice::u32 version
    ) const noexcept -> ice::ucount
    {
        ice::ucount result = 0;
        auto it = ice::multi_hashmap::find_first(_modules, ice::hash(api_name));
        while (it != nullptr)
        {
            ice::ModuleAPI api_ptr;
            if (it.value().select_api(ice::stringid_hash(api_name), version, &api_ptr))
            {
                result += 1;
            }
            it = ice::multi_hashmap::find_next(_modules, it);
        }
        return result;
    }

    bool UnixModuleRegister::query_apis(
        ice::StringID_Arg api_name,
        ice::u32 version,
        ice::ModuleAPI* out_array,
        ice::ucount* inout_array_size
    ) const noexcept
    {
        if (out_array == nullptr)
        {
            if (inout_array_size == nullptr)
            {
                return false;
            }

            *inout_array_size = this->api_count(api_name, version);
            return *inout_array_size > 0;
        }

        ice::u32 idx = 0;
        auto it = ice::multi_hashmap::find_first(_modules, ice::hash(api_name));

        ice::ucount const array_size = *inout_array_size;
        while (it != nullptr && idx < array_size)
        {
            ice::ModuleAPI api_ptr;
            if (it.value().select_api(ice::stringid_hash(api_name), version, &api_ptr))
            {
                out_array[idx] = api_ptr;
                idx += 1;
            }
            it = ice::multi_hashmap::find_next(_modules, it);
        }
        return idx > 0;
    }

    bool UnixModuleRegister::register_module(
        ice::UnixModuleEntry const& entry
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(entry.name);
        ice::multi_hashmap::insert(_modules, name_hash, entry);
        return true;
    }

    bool ModuleNegotiatorAPIContext::get_module_apis(
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::StringID_Hash api_name,
        ice::u32 version,
        ice::ModuleAPI* out_api,
        ice::ucount* inout_array_size
    ) noexcept
    {
        return ctx->module_register->query_apis(ice::StringID{ api_name }, version, out_api, inout_array_size);
    }

    bool ModuleNegotiatorAPIContext::register_module(
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::StringID_Hash api_name,
        ice::FnModuleSelectAPI* fn_api_select
    ) noexcept
    {
        if (api_name != ice::stringid_hash(ice::StringID_Invalid) && fn_api_select != nullptr)
        {
            ctx->current_module.name = api_name;
            ctx->current_module.select_api = fn_api_select;
            return ctx->module_register->register_module(ctx->current_module);
        }
        return false;
    }

    auto create_default_module_register(
        ice::Allocator& alloc,
        bool load_global_modules /*= true*/
    ) noexcept -> ice::UniquePtr<ModuleRegister>
    {
        ice::UniquePtr<ModuleRegister> result = ice::make_unique<UnixModuleRegister>(alloc, alloc);
        if (result != nullptr && load_global_modules)
        {
            ice::load_global_modules(alloc, *result);
        }
        return result;
    }

#endif // #if ISP_WINDOWS

} // namespace ice
