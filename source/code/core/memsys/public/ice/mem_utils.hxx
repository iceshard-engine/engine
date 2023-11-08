/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_arithmetic.hxx>
#include <type_traits>

namespace ice
{

    constexpr auto mem_max_capacity(ice::usize element_size, ice::usize memory_space) noexcept -> ice::ucount;

    template<typename T>
    constexpr auto mem_max_capacity(ice::usize memory_space) noexcept -> ice::ucount;


    constexpr auto mem_max_capacity(ice::usize element_size, ice::usize memory_space) noexcept -> ice::ucount
    {
        return static_cast<ice::ucount>(memory_space.value / element_size.value);
    }

    template<typename T>
    constexpr auto mem_max_capacity(ice::usize memory_space) noexcept -> ice::ucount
    {
        return ice::mem_max_capacity(ice::size_of<T>, memory_space);
    }

} // namespace ice
