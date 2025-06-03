/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/module_register.hxx>
#include "module_globals.hxx"

namespace ice
{

    auto load_global_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister& modules_register
    ) noexcept -> ice::ucount
    {
        ice::ucount loaded_modules = 0;

        ice::ModulesEntry const* module_entry = Global_ModulesList;
        while(module_entry != nullptr)
        {
            modules_register.load_module(alloc, *module_entry);
            module_entry = module_entry->next;
        }

        return loaded_modules;
    }

} // namespace ice

#if ISP_WINDOWS

extern "C"
{

    __declspec(dllexport) bool ice_module_load(
       ice::Allocator* alloc,
       ice::ModuleNegotiatorAPIContext* ctx,
       ice::ModuleNegotiatorAPI* negotiator
    )
    {
       ice::ModulesEntry const* module_entry = ice::Global_ModulesList;
       while(module_entry != nullptr)
       {
           module_entry->fn_load(alloc, ctx, negotiator);
           module_entry = module_entry->next;
       }
       return true;
    }

    __declspec(dllexport) bool ice_module_unload(
       ice::Allocator* alloc
    )
    {
       ice::ModulesEntry const* module_entry = ice::Global_ModulesList;
       while(module_entry != nullptr)
       {
           module_entry->fn_unload(alloc);
           module_entry = module_entry->next;
       }
       return true;
    }

} // extern "C"

#elif ISP_ANDROID || ISP_LINUX

extern "C"
{

    bool ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::ModuleNegotiatorAPI* negotiator
    )
    {
        ice::ModulesEntry const* module_entry = ice::Global_ModulesList;
        while(module_entry != nullptr)
        {
            module_entry->fn_load(alloc, ctx, negotiator);
            module_entry = module_entry->next;
        }
        return true;
    }

    bool ice_module_unload(
        ice::Allocator* alloc
    )
    {
        ice::ModulesEntry const* module_entry = ice::Global_ModulesList;
        while(module_entry != nullptr)
        {
            module_entry->fn_unload(alloc);
            module_entry = module_entry->next;
        }
        return true;
    }

} // extern "C"

#elif ISP_WEBAPP
// Nothing needed here. WebApps are better as Monolythic executables.

#else
#error Not implemented

#endif
