/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_forward.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/stringid.hxx>
#include <ice/log_formatters.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        inline constexpr ice::Data Constant_FileHeaderData_ConfigFile{
            .location = ice::string::begin(Constant_FileHeader_ConfigFile),
            .size = { ice::string::size(Constant_FileHeader_ConfigFile) },
            .alignment = ice::ualign::b_1
        };

        inline bool get_entry(
            ice::Config const& config,
            ice::StringID_Arg key,
            ice::detail::ConfigEntryType expected_type,
            ice::detail::ConfigEntry& entry_out
        ) noexcept
        {
            entry_out = ice::hashmap::get(
                config._config_entries,
                ice::hash(key),
                ConfigEntry{ .data_type = ConfigEntryType::Invalid }
            );
            return entry_out.data_type == expected_type;
        }

        void deserialize_json_config(ice::Data data, ice::MutableConfig& config) noexcept;

    } // namespace detail

} // namespace ice
