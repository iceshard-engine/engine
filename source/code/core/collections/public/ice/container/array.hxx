/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_types.hxx>
#include <ice/mem_initializers.hxx>

namespace ice::array
{

    template<typename Type, ice::ContainerLogic Logic>
    inline void set_capacity(ice::Array<Type, Logic>& arr, ice::ucount new_capacity) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void reserve(ice::Array<Type, Logic>& arr, ice::ucount min_capacity) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void grow(ice::Array<Type, Logic>& arr, ice::ucount min_capacity = 0) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void resize(ice::Array<Type, Logic>& arr, ice::ucount new_size) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void shrink(ice::Array<Type, Logic>& arr) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void clear(ice::Array<Type, Logic>& arr) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto slice(
        ice::Array<Type, Logic>& arr,
        ice::ucount from_idx = 0,
        ice::ucount count = ice::ucount_max
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

    template<typename Type, ice::ContainerLogic Logic>
    inline void pop_back(ice::Array<Type, Logic>& arr, ice::ucount count = 1) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto begin(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::Iterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto end(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::Iterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto rbegin(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::ReverseIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto rend(ice::Array<Type, Logic>& arr) noexcept -> typename ice::Array<Type, Logic>::ReverseIterator;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto front(ice::Array<Type, Logic>& arr) noexcept -> Type&;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto back(ice::Array<Type, Logic>& arr) noexcept -> Type&;



    template<typename Type, ice::ContainerLogic Logic>
    inline auto count(ice::Array<Type, Logic> const& arr) noexcept -> ice::ucount;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto capacity(ice::Array<Type, Logic> const& arr) noexcept -> ice::ucount;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto size_bytes(ice::Array<Type, Logic> const& arr) noexcept -> ice::usize;

    template<typename Type, ice::ContainerLogic Logic>
    inline bool any(ice::Array<Type, Logic> const& arr) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline bool empty(ice::Array<Type, Logic> const& arr) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto slice(
        ice::Array<Type, Logic> const& arr,
        ice::ucount from_idx = 0,
        ice::ucount count = ice::ucount_max
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
    inline auto front(ice::Array<Type, Logic> const& arr) noexcept -> Type const&;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto back(ice::Array<Type, Logic> const& arr) noexcept -> Type const&;



    template<typename Type, ice::ContainerLogic Logic>
    inline auto data_view(ice::Array<Type, Logic> const& arr) noexcept -> ice::Data;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto memory(ice::Array<Type, Logic>& arr) noexcept -> ice::Memory;

} // namespace ice::array

namespace ice
{

    using ice::array::count;
    using ice::array::begin;
    using ice::array::end;

} // namespace ice

#include "impl/array_impl.inl"
