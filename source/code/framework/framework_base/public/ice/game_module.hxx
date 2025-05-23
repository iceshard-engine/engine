/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void load_game_module(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::ModuleNegotiatorAPI* api
    ) noexcept;

    void unload_game_module(
        ice::Allocator* alloc
    ) noexcept;

} // namespace ice
