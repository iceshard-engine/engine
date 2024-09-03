/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>
#include <ice/config/config_details.hxx>

namespace ice::config::detail
{

    enum KeyType : ice::u8
    {
        CONFIG_KEYTYPE_NONE = 0,
        CONFIG_KEYTYPE_STRING = 1,
    };

    enum ValType : ice::u8
    {
        CONFIG_VALTYPE_NONE = 0x00,

        CONFIG_VALTYPE_8B_BIT = 0x01,
        CONFIG_VALTYPE_16B_BIT = 0x02,
        CONFIG_VALTYPE_32B_BIT = 0x03,
        CONFIG_VALTYPE_SIGN_BIT = 0x04,
        CONFIG_VALTYPE_FP_BIT = 0x08,

        CONFIG_VALTYPE_LARGE = 0x10,
        CONFIG_VALTYPE_64B_BIT = CONFIG_VALTYPE_LARGE,

        // Other types that should be checked directly
        CONFIG_VALTYPE_OTHER = 0x20,
        CONFIG_VALTYPE_STRING = CONFIG_VALTYPE_OTHER,

        // User provided types
        CONFIG_VALTYPE_USER = 0x30,

        CONFIG_VALTYPE_CONTAINER = 0x3C, // Last 4 values 60, 61, 62, 63
        CONFIG_VALTYPE_TABLE = CONFIG_VALTYPE_CONTAINER + 0,
        CONFIG_VALTYPE_OBJECT = CONFIG_VALTYPE_CONTAINER + 1,
        CONFIG_VALTYPE_ROOT = CONFIG_VALTYPE_CONTAINER + 3,

        // Only here to enable flag support
        None = CONFIG_VALTYPE_NONE,
    };

    struct ConfigKey
    {
        ice::u32 next : 1;
        ice::u32 type : 1; // 1=Strings, 0=None (Used By Arrays)
        ice::u32 vtype : 6;

        ice::u32 offset : 16;
        ice::u32 size : 8;
    };

    struct ConfigValue
    {
        ice::u32 internal;
    };

} // namespace ice::config::detail
