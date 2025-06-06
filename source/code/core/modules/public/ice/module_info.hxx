/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/module_types.hxx>

namespace ice
{

    //! \brief Stores information of module load and unload functions.
    struct ModuleInfo
    {
        ice::FnModuleLoad* const fn_load;
        ice::FnModuleUnload* const fn_unload;
    };

    //! \brief Helper type to register modules globally.
    //! \details Such registered modules are available to be loaded later.
    struct ModulesEntry : ModuleInfo
    {
        ModulesEntry(
            ice::FnModuleLoad* fn_load,
            ice::FnModuleUnload* fn_unload
        ) noexcept;

        //! \brief Allows to setup a list of modules without allocation before entering the "main" routine.
        ModulesEntry const* const next;
    };

    //! \brief Stores information about a single API entry.
    //! \details Modules may provide multiple API entries.
    struct ModuleAPI
    {
        //! \brief Pointer to an API structure holding functions to be called.
        void* api_ptr;

        //! \brief Version of the API.
        ice::u32 version;

        //! \brief Arbitrary priority of the API. Can be used to wrap existing APIs with additional functionality.
        ice::u32 priority;
    };

} // namespace ice
