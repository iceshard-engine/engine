/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/module.hxx>

namespace ice
{

    struct LogModule
    {
        static void init(ice::Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept;
        static void on_unload(ice::Allocator& alloc) noexcept;
    };

} // namespace ice
