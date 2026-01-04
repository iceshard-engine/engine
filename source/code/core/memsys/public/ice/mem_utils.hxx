/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_arithmetic.hxx>
#include <type_traits>

namespace ice
{

    constexpr auto mem_max_capacity(ice::usize element_size, ice::usize memory_space) noexcept -> ice::u64;

    template <typename T>
    constexpr auto mem_max_capacity(ice::usize memory_space) noexcept -> ice::u64;

    constexpr auto mem_max_capacity(ice::usize element_size, ice::usize memory_space) noexcept -> ice::u64
    {
        return ice::u64{ memory_space.value / element_size.value };
    }

    template <typename T>
    constexpr auto mem_max_capacity(ice::usize memory_space) noexcept -> ice::u64
    {
        return ice::mem_max_capacity(ice::size_of<T>, memory_space);
    }

} // namespace ice
