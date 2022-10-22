/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void load_game_module(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* api
    ) noexcept;

    void unload_game_module(
        ice::Allocator* alloc
    ) noexcept;

} // namespace ice
