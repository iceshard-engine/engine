/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_arithmetic.hxx>
#include <type_traits>

namespace ice
{

    //! \return 'pointer' value advanced by the number of 'offset' bytes and aligned to the given 'alignment'
    inline auto ptr_adv(void* pointer, ice::usize offset, ice::ualign align) noexcept -> void*;

    //! \return 'pointer' value advanced by the number of 'offset' bytes and aligned to the given 'alignment'
    inline auto ptr_adv(void const* pointer, ice::usize offset, ice::ualign align) noexcept -> void const*;

    //! \return 'pointer' value advanced by the number of 'offset' bytes.
    inline auto ptr_add(void* pointer, ice::usize offset) noexcept -> void*;

    //! \return 'pointer' value advanced by the number of 'offset' bytes.
    inline auto ptr_add(void const* pointer, ice::usize offset) noexcept -> void const*;


    //! \return 'pointer' value retracted by the number of 'offset' bytes.
    inline auto ptr_sub(void* pointer, ice::usize offset) noexcept -> void*;

    //! \return 'pointer' value retracted by the number of 'offset' bytes.
    inline auto ptr_sub(void const* pointer, ice::usize offset) noexcept -> void const*;

    //! \return 'distance' between the given pointers.
    inline auto ptr_distance(void const* ptr_from, void const* ptr_to) noexcept -> ice::usize;


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

    inline auto ptr_distance(void const* ptr_from, void const* ptr_to) noexcept -> ice::usize
    {
        ICE_ASSERT_CORE(ptr_from <= ptr_to);
        return { ice::usize::base_type(reinterpret_cast<char const*>(ptr_to) - reinterpret_cast<char const*>(ptr_from)) };
    }

    inline auto ptr_offset(void const* ptr_from, void const* ptr_to) noexcept -> ice::isize
    {
        return { ice::isize::base_type(reinterpret_cast<char const*>(ptr_to) - reinterpret_cast<char const*>(ptr_from)) };
    }

} // namespace ice
