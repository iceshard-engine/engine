/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "config_builder_types.hxx"
#include "config_builder_utils.hxx"

namespace ice::config::detail
{

    struct CBKeyString
    {
        ice::String value;
        ice::u32 index;
    };

    struct CBDataBuffer
    {
        ice::Memory memory;
        ice::u32 offset;
        ice::u32 offset_strings;
    };

    auto cb_find_keystr_idx(
        ice::HashMap<CBKeyString> const& keystrings,
        ice::String keystr
    ) noexcept -> ice::u32;

    auto cb_calculate_key_size(
        ice::HashMap<CBKeyString>& keystrings,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::config::detail::ConfigBuilderEntry const* entry
    ) noexcept -> ice::usize;

    auto cb_calculate_final_size(
        ice::HashMap<CBKeyString>& keystrings,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::usize& out_data_size,
        ice::u32& out_count
    ) noexcept -> ice::usize;

    auto cb_calculate_final_size(
        ice::HashMap<CBKeyString>& keystrings,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::u32& out_count
    ) noexcept -> ice::usize;

    auto cb_finalize_store_keysvalues(
        ice::HashMap<CBKeyString>& keystrings,
        ice::Span<ice::u32 const> keystringoffsets,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::config::detail::ConfigKey* out_keylist,
        ice::config::detail::ConfigValue* out_vallist,
        ice::config::detail::CBDataBuffer& out_data
    ) noexcept -> ice::u32;

} // namespace ice
