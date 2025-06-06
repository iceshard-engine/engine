/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    template<typename Type>
    class Module;
    class ModuleRegister;
    class ModuleNegotiatorBase;
    template<typename Type>
    class ModuleNegotiatorTagged;

    struct ModuleQuery;
    struct ModuleAPI;
    struct ModuleNegotiatorAPI;
    struct ModuleNegotiatorAPIContext;

    using FnModuleLoad = void (ice::Allocator*, ice::ModuleNegotiatorAPIContext*, ice::ModuleNegotiatorAPI*);
    using FnModuleUnload = void (ice::Allocator*);
    using FnModuleSelectAPI = bool (ice::StringID_Hash, ice::u32, ice::ModuleAPI*);

    template <typename T>
    using ProcAPIQuickRegisterFunc = void(*)(T& out_api) noexcept;

} // namespace ice
