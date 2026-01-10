/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_types.hxx>
#include <ice/mem_initializers.hxx>

namespace ice::array
{

    template<typename Type, ice::ContainerLogic Logic>
        requires std::copy_constructible<Type>
    inline void push_back(ice::Array<Type, Logic>& arr, ice::Array<Type, Logic> const& items) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::copy_constructible<Type>
    inline void push_back(ice::Array<Type, Logic>& arr, ice::Span<Type const> items) noexcept;

    template<typename Type, ice::ContainerLogic Logic, typename Source>
        requires std::copy_constructible<Type> && (std::is_same_v<Type, Source> == false)
    inline void push_back(ice::Array<Type, Logic>& arr, ice::Span<Source const> items, Type(*fn)(Source const&) noexcept) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void pop_back(ice::Array<Type, Logic>& arr, ice::u32 count = 1) noexcept;

    template<typename Type>
    inline auto memset(ice::Array<Type, ice::ContainerLogic::Trivial>& arr, ice::u8 value) noexcept -> ice::Memory;

} // namespace ice::array

namespace ice
{

} // namespace ice

#include "impl/array_impl.inl"
