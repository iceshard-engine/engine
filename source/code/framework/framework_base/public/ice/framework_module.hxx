/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module.hxx>

namespace ice::framework
{

    struct FrameworkModule
    {
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept;
        static void on_unload(ice::Allocator& alloc) noexcept;
    };

} // namespace ice::framework
