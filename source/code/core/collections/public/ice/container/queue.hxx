/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container_types.hxx>
#include <ice/mem_initializers.hxx>

namespace ice::queue
{

    // TODO: Move to details (we don't need this, also makes no sense for this queue)
    template<typename Type, ice::ContainerLogic Logic>
    inline void set_capacity(ice::Queue<Type, Logic>& queue, ice::ucount new_capacity) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void reserve(ice::Queue<Type, Logic>& queue, ice::ucount min_capacity) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void grow(ice::Queue<Type, Logic>& queue, ice::ucount min_capacity = 0) noexcept;

    // TODO: Move to details (we don't need this, also makes no sense for this queue)
    template<typename Type, ice::ContainerLogic Logic>
    inline void resize(ice::Queue<Type, Logic>& queue, ice::ucount new_count) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void shrink(ice::Queue<Type, Logic>& queue) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void clear(ice::Queue<Type, Logic>& queue) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::move_constructible<Type>
    inline void push_back(ice::Queue<Type, Logic>& queue, Type&& item) noexcept;

    template<typename Type, ice::ContainerLogic Logic, typename Value = Type>
        requires std::copy_constructible<Type>&& std::convertible_to<Value, Type>
    inline void push_back(ice::Queue<Type, Logic>& queue, Value const& item) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::copy_constructible<Type>
    inline void push_back(ice::Queue<Type, Logic>& queue, ice::Queue<Type, Logic> const& items) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::copy_constructible<Type>
    inline void push_back(ice::Queue<Type, Logic>& queue, ice::Span<Type const> items) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void pop_back(ice::Queue<Type, Logic>& queue, ice::ucount count = 1) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::move_constructible<Type>
    inline void push_front(ice::Queue<Type, Logic>& queue, Type&& item) noexcept;

    template<typename Type, ice::ContainerLogic Logic, typename Value = Type>
        requires std::copy_constructible<Type> && std::convertible_to<Value, Type>
    inline void push_front(ice::Queue<Type, Logic>& queue, Value const& item) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::copy_constructible<Type>
    inline void push_front(ice::Queue<Type, Logic>& queue, ice::Queue<Type, Logic> const& items) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
        requires std::copy_constructible<Type>
    inline void push_front(ice::Queue<Type, Logic>& queue, ice::Span<Type const> items) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline void pop_front(ice::Queue<Type, Logic>& queue, ice::ucount count = 1) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto front(ice::Queue<Type, Logic>& queue) noexcept -> Type&;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto back(ice::Queue<Type, Logic>& queue) noexcept -> Type&;


    template<typename Type, ice::ContainerLogic Logic>
    inline auto count(ice::Queue<Type, Logic> const& queue) noexcept -> ice::ucount;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto capacity(ice::Queue<Type, Logic> const& queue) noexcept -> ice::ucount;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto size_bytes(ice::Queue<Type, Logic> const& queue) noexcept -> ice::usize;

    template<typename Type, ice::ContainerLogic Logic>
    inline bool any(ice::Queue<Type, Logic> const& queue) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline bool empty(ice::Queue<Type, Logic> const& queue) noexcept;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto front(ice::Queue<Type, Logic> const& queue) noexcept -> Type const&;

    template<typename Type, ice::ContainerLogic Logic>
    inline auto back(ice::Queue<Type, Logic> const& queue) noexcept -> Type const&;

    template<typename Type, ice::ContainerLogic Logic, typename Fn>
    inline void for_each(ice::Queue<Type, Logic> const& queue, Fn&& fn) noexcept;


    template<typename Type, ice::ContainerLogic Logic>
    inline auto memory(ice::Queue<Type, Logic>& queue) noexcept -> ice::Memory;

} // namespace ice::queue

namespace ice
{

    using ice::queue::count;

} // namespace ice

#include "impl/queue_impl.inl"
