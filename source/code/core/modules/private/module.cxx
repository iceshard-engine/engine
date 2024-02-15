/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/module.hxx>
#include <ice/assert_core.hxx>
#include "module_globals.hxx"

namespace ice
{

    ModulesEntry const* Global_ModulesList = nullptr;

    ModulesEntry::ModulesEntry(
        ice::FnModuleLoad* fn_load,
        ice::FnModuleUnload* fn_unload
    ) noexcept
        : ModuleInfo{ fn_load, fn_unload }
        , next{ ice::exchange(Global_ModulesList, this) }
    {
        ICE_ASSERT_CORE(fn_load != nullptr);
        ICE_ASSERT_CORE(fn_unload != nullptr);
    }

} // namespace ice
