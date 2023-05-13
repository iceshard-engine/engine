/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_memory.hxx>
#include <type_traits>

namespace ice
{

    inline auto ptr_add(void* ptr, ice::usize offset) noexcept -> void*;
    inline auto ptr_add(void const* ptr, ice::usize offset) noexcept -> void const*;
    inline auto ptr_add(ice::Memory mem, ice::usize offset) noexcept -> ice::Memory;

    inline auto ptr_sub(void* ptr, ice::usize offset) noexcept -> void*;
    inline auto ptr_sub(void const* ptr, ice::usize offset) noexcept -> void const*;

    inline auto ptr_distance(void const* ptr_from, void const* ptr_to) noexcept -> ice::usize;

    constexpr auto mem_max_capacity(ice::usize element_size, ice::usize memory_space) noexcept -> ice::ucount;
    template<typename T>
    constexpr auto mem_max_capacity(ice::usize memory_space) noexcept -> ice::ucount;


    inline auto ptr_add(void* ptr, ice::usize offset) noexcept -> void*
    {
        return reinterpret_cast<char*>(ptr) + offset.value;
    }

    inline auto ptr_add(void const* ptr, ice::usize offset) noexcept -> void const*
    {
        return reinterpret_cast<char const*>(ptr) + offset.value;
    }

    inline auto ptr_sub(void* ptr, ice::usize offset) noexcept -> void*
    {
        return reinterpret_cast<char*>(ptr) - offset.value;
    }

    inline auto ptr_sub(void const* ptr, ice::usize offset) noexcept -> void const*
    {
        return reinterpret_cast<char const*>(ptr) - offset.value;
    }

    inline auto ptr_add(ice::Memory mem, ice::usize offset) noexcept -> ice::Memory
    {
        ICE_ASSERT_CORE(mem.size >= offset);
        return Memory{
            .location = ice::ptr_add(mem.location, offset),
            .size = { mem.size.value - offset.value },
            .alignment = mem.alignment
        };
    }

    inline auto ptr_distance(void const* ptr_from, void const* ptr_to) noexcept -> ice::usize
    {
        ICE_ASSERT_CORE(ptr_from <= ptr_to);
        return { ice::usize::base_type(reinterpret_cast<char const*>(ptr_to) - reinterpret_cast<char const*>(ptr_from)) };
    }

    inline auto ptr_offset(void const* ptr_from, void const* ptr_to) noexcept -> ice::isize
    {
        return { ice::isize::base_type(reinterpret_cast<char const*>(ptr_to) - reinterpret_cast<char const*>(ptr_from)) };
    }

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
