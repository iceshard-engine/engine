/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/module_register.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>
#include <ice/profiler.hxx>

#include "module_globals.hxx"
#include "module_native.hxx"

namespace ice
{

    class DefaultModuleRegister;

    struct DefaultModuleEntry
    {
        ice::StringID_Hash name;
        ice::Allocator* module_allocator;
        ice::FnModuleSelectAPI* select_api;
        ice::FnModuleUnload* unload_proc;
    };

    struct ModuleNegotiatorAPIContext
    {
        DefaultModuleRegister* module_register;
        DefaultModuleEntry current_module;
        bool in_app_context;

        static bool from_app(ModuleNegotiatorAPIContext*) noexcept;
        static bool get_module_api(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::u32, ice::ModuleAPI*) noexcept;
        static bool get_module_apis(ModuleNegotiatorAPIContext*, ice::StringID_Hash, ice::u32, ice::ModuleAPI*, ice::u32*) noexcept;
        static bool register_module(ModuleNegotiatorAPIContext*, ice::StringID_Hash, FnModuleSelectAPI*) noexcept;
    };

    class DefaultModuleRegister final : public ModuleRegister
    {
    public:
        DefaultModuleRegister(ice::Allocator& alloc) noexcept;
        ~DefaultModuleRegister() noexcept override;

        bool load_module(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept override;

        bool load_module(
            ice::Allocator& alloc,
            ice::FnModuleLoad* load_fn,
            ice::FnModuleUnload* unload_fn,
            bool from_shared_library
        ) noexcept override;

        auto api_count(
            ice::StringID_Arg name,
            ice::u32 version
        ) const noexcept -> ice::u32;

        bool query_apis(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::ModuleAPI* out_array,
            ice::u32* inout_array_size
        ) const noexcept override;

        bool register_module(
            ice::DefaultModuleEntry const& entry
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<DefaultModuleEntry> _modules;
        ice::Array<ice::native_module::ModuleHandle> _module_handles;
    };

    DefaultModuleRegister::DefaultModuleRegister(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _modules{ _allocator }
        , _module_handles{ _allocator }
    { }

    DefaultModuleRegister::~DefaultModuleRegister() noexcept
    {
        ice::FnModuleUnload* fn_unload_prev = nullptr;
        for (DefaultModuleEntry const& entry : ice::hashmap::values(_modules))
        {
            if (fn_unload_prev != entry.unload_proc)
            {
                entry.unload_proc(entry.module_allocator);
                fn_unload_prev = entry.unload_proc;
            }
        }
        for (ice::native_module::ModuleHandle& handle : _module_handles)
        {
            ice::native_module::module_close(ice::move(handle));
        }
    }

    bool DefaultModuleRegister::load_module(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_ZONE_TEXT_STR(path);

        ice::HeapString<> utf8_path{ _allocator, path };
        ice::native_module::ModuleHandle module_handle = ice::native_module::module_open(utf8_path);
        if (module_handle)
        {
            void* const load_proc = ice::native_module::module_find_address(module_handle, "ice_module_load");
            void* const unload_proc = ice::native_module::module_find_address(module_handle, "ice_module_unload");

            if (load_proc != nullptr && unload_proc != nullptr)
            {
                load_module(
                    alloc,
                    reinterpret_cast<ice::FnModuleLoad*>(load_proc),
                    reinterpret_cast<ice::FnModuleUnload*>(unload_proc),
                    /* is_app_context */ false
                );

                ice::array::push_back(_module_handles, ice::move(module_handle));
                return true;
            }
        }
        return false;
    }

    bool DefaultModuleRegister::load_module(
        ice::Allocator& alloc,
        ice::FnModuleLoad* load_fn,
        ice::FnModuleUnload* unload_fn,
        bool from_shared_library
    ) noexcept
    {
        DefaultModuleEntry module_entry{
            .module_allocator = &alloc,
            .select_api = nullptr,
            .unload_proc = unload_fn,
        };

        ModuleNegotiatorAPIContext negotiator_context{
            .module_register = this,
            .current_module = module_entry,
            .in_app_context = from_shared_library
        };

        ModuleNegotiatorAPI negotiator{
            .fn_is_app_context = ModuleNegotiatorAPIContext::from_app,
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

    auto DefaultModuleRegister::api_count(
        ice::StringID_Arg api_name,
        ice::u32 version
    ) const noexcept -> ice::u32
    {
        ice::u32 result = 0;
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

    bool DefaultModuleRegister::query_apis(
        ice::StringID_Arg api_name,
        ice::u32 version,
        ice::ModuleAPI* out_array,
        ice::u32* inout_array_size
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

        ice::u32 const array_size = *inout_array_size;
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

    bool DefaultModuleRegister::register_module(
        ice::DefaultModuleEntry const& entry
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(entry.name);
        ice::multi_hashmap::insert(_modules, name_hash, entry);
        return true;
    }

    bool ModuleNegotiatorAPIContext::from_app(ModuleNegotiatorAPIContext* ctx) noexcept
    {
        return ctx->in_app_context;
    }

    bool ModuleNegotiatorAPIContext::get_module_apis(
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::StringID_Hash api_name,
        ice::u32 version,
        ice::ModuleAPI* out_api,
        ice::u32* inout_array_size
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
        ice::UniquePtr<ModuleRegister> result = ice::make_unique<DefaultModuleRegister>(alloc, alloc);
        if (result != nullptr && load_global_modules)
        {
            ice::load_global_modules(alloc, *result);
        }
        return result;
    }

} // namespace ice
