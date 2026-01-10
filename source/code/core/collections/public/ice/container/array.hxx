/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_types.hxx>
#include <ice/mem_initializers.hxx>

namespace ice::array
{

    template<typename Type, ice::ContainerLogic Logic>
    inline auto slice(
        ice::Array<Type, Logic>& arr,
        ice::u32 from_idx = 0,
        ice::u32 count = ice::u32_max
    ) noexcept -> ice::Span<Type>;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::move_constructible<Type>
    inline void push_back(ice::Array<Type, Logic>& arr, Type&& item) noexcept;

    template<typename Type, ice::ContainerLogic Logic, typename Value = Type>
        requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
    inline void push_back(ice::Array<Type, Logic>& arr, Value const& item) noexcept;

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

    template<typename Type, ice::ContainerLogic Logic>
    inline auto begin(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::Iterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto end(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::Iterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto rbegin(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::ReverseIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto rend(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::ReverseIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto size_bytes(ice::Array<Type, Logic> const& arr) noexcept -> ice::usize;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto slice(
        ice::Array<Type, Logic> const& arr,
        ice::u32 from_idx = 0,
        ice::u32 count = ice::u32_max
    )noexcept -> ice::Span<Type const>;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto begin(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto end(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto rbegin(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstReverseIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto rend(ice::Array<Type, Logic> const& arr) noexcept -> typename ice::Array<Type, Logic>::ConstReverseIterator;



    template<typename Type, ice::ContainerLogic Logic>
    inline auto data_view(ice::Array<Type, Logic> const& arr) noexcept -> ice::Data;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto memory(ice::Array<Type, Logic>& arr) noexcept -> ice::Memory;

    template<typename Type>
    inline auto memset(ice::Array<Type, ice::ContainerLogic::Trivial>& arr, ice::u8 value) noexcept -> ice::Memory;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto meminfo(ice::Array<Type, Logic> const& arr) noexcept -> ice::meminfo;

} // namespace ice::array

namespace ice
{

    using ice::array::begin;
    using ice::array::end;

} // namespace ice

#include "impl/array_impl.inl"
