/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/mem_utils.hxx>
#include <ice/container_logic.hxx>
#include <ice/stringid.hxx>

#include <ice/string.hxx>
#include <ice/heap_string.hxx>
#include <ice/static_string.hxx>
#include <ice/varstring.hxx>

namespace ice
{

} // namespace ice

namespace ice
{

    constexpr auto hash(ice::String value) noexcept -> ice::u64
    {
        return ice::hash(std::string_view{ value });
    }

    constexpr auto hash32(ice::String value) noexcept -> ice::u32
    {
        return ice::hash32(std::string_view{ value });
    }

    constexpr auto stringid(ice::String value) noexcept -> ice::StringID
    {
        return ice::stringid(value.data(), value.size().u64());
    }

    template<ice::u32 Capacity = 12>
    constexpr auto stringid(ice::StaticString<Capacity, char> value) noexcept -> ice::StringID
    {
        return ice::stringid(value._data, value._size);
    }

} // namespace ice
