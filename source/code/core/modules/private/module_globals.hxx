/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/module.hxx>

namespace ice
{

    //! \brief This is the global modules list that will be defined for each dynamic module and the actuall app.
    //!   This list is used to load all available modules when a dynamic library is loaded via ModuleRegister.
    extern ModulesEntry const* Global_ModulesList;

} // namespace ice
