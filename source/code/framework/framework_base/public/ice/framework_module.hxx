/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module.hxx>

namespace ice::framework
{

    class FrameworkModule : public ice::Module<FrameworkModule>
    {
    public:
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiatorTagged<FrameworkModule> const& negotiator) noexcept;

        IS_WORKAROUND_MODULE_INITIALIZATION(FrameworkModule);
    };

} // namespace ice::framework
