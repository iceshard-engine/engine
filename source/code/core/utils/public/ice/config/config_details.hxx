/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/expected.hxx>
#include <ice/string/string.hxx>

namespace ice
{

    static constexpr ice::ErrorCode E_ConfigIsInvalid{ "E.2210:Config:Config is invalid." };
    static constexpr ice::ErrorCode E_ConfigKeyNotFound{ "E.2211:Config:Config key was not found." };
    static constexpr ice::ErrorCode E_ConfigIndexOutOfBounds{ "E.2211:Config:Config index was out-of-bounds." };
    static constexpr ice::ErrorCode E_ConfigValueInvalid{ "E.2215:Config:Config value is not valid." };
    static constexpr ice::ErrorCode E_ConfigValueNotAnTable{ "E.2216:Config:Config value is not a table." };
    static constexpr ice::ErrorCode E_ConfigValueNotAnObject{ "E.2217:Config:Config value is not an object." };
    static constexpr ice::ErrorCode E_ConfigValueTypeMissmatch{ "E.2218:Config:Config value type does not match the requested type." };

    namespace concepts
    {

        template<typename T>
        concept ConfigKeyType = std::convertible_to<T, ice::String> || std::convertible_to<T, ice::u32>;

        template<typename T>
        concept ConfigValueType = std::convertible_to<T, ice::String> || std::is_arithmetic_v<T>;

    } // namespace concepts

    namespace config::detail
    {

        struct ConfigKey;
        struct ConfigValue;
        struct ConfigBuilderEntry;

        enum KeyType : ice::u8;
        enum ValType : ice::u8;

    } // namespace detail

} // namespace ice
