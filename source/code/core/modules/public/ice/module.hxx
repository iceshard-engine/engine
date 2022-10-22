/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    struct ModuleNegotiator;
    struct ModuleNegotiatorContext;

    using ModuleProcLoad = void (ice::Allocator*, ice::ModuleNegotiatorContext*, ice::ModuleNegotiator*);
    using ModuleProcUnload = void (ice::Allocator*);
    using ModuleProcGetAPI = bool (ice::StringID_Hash, ice::u32, void**);

    struct ModuleNegotiator
    {
        bool (*fn_get_module_api)(ice::ModuleNegotiatorContext*, ice::StringID_Hash, ice::u32, void**) noexcept;
        bool (*fn_register_module)(ice::ModuleNegotiatorContext*, ice::StringID_Hash, ice::ModuleProcGetAPI*) noexcept;
    };

} // namespace ice
