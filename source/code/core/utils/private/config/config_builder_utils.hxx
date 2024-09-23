/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "config_builder_types.hxx"

namespace ice::config::detail
{

    auto cb_getkey(
        ice::String stringpack,
        ice::config::detail::ConfigBuilderEntry const& entry
    ) noexcept -> ice::String;

    void cb_assign_value_type(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::config::detail::ValType vtype
    ) noexcept;

    void cb_assign_value_type(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::i32 vtype
    ) noexcept;

    void cb_clear_value_type(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry* entry
    ) noexcept;

    auto cb_get_entry(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::String key,
        bool clean
    ) noexcept -> ice::config::detail::ConfigBuilderEntry*;

    auto cb_get_entry(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::u32 idx,
        bool clean
    ) noexcept -> ice::config::detail::ConfigBuilderEntry*;

    auto cb_get_or_set_container(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::config::detail::ValType vtype
    ) noexcept -> ice::ConfigBuilderValue;

} // namespace ice
