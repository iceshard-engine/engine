/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/hash.hxx>
#include <ice/stringid.hxx>
#include <ice/types/ncount.hxx>
#include <ice/types/nindex.hxx>
#include <ice/container_logic.hxx>

namespace ice::concepts
{

    template<typename T>
    concept ContainerType = requires(T t) {
        typename std::remove_reference_t<T>::SizeType;
        typename std::remove_reference_t<T>::ValueType;
        typename std::remove_reference_t<T>::ConstContainerValueType;
    };

    template<typename T>
    concept AssociativeContainerType = ContainerType<T> && requires(T t) {
        typename std::remove_reference_t<T>::KeyType;
        typename std::remove_reference_t<T>::EntryType;
    };

    template<typename T>
    concept Container = ContainerType<T> && requires(T t) {
        { t.size() } -> std::convertible_to<ice::ncount>;
    };

    template<typename T>
    concept AssociativeContainer = Container<T> && AssociativeContainerType<T>
        && requires(T t, typename std::remove_reference_t<T>::KeyType key, typename std::remove_reference_t<T>::ValueType&& val) {
        { t.size() } -> std::convertible_to<ice::ncount>;
        { t.find(key) } -> std::convertible_to<typename std::remove_reference_t<T>::ValueType const*>;
    };

    template<typename T>
    concept ResizableContainer = Container<T> && requires(T t, ice::ncount size) {
        { t.capacity() } -> std::convertible_to<ice::ncount>;
        { t.set_capacity(size) } -> std::convertible_to<void>;
        { t.clear() } -> std::convertible_to<void>;
    };

    template<typename T>
    concept AssociativeResizableContainer = AssociativeContainer<T> && ResizableContainer<T> && requires(
        T t,
        typename std::remove_reference_t<T>::KeyType key,
        typename std::remove_reference_t<T>::ValueType&& val)
    {
        { t.store(key, val) } -> std::convertible_to<typename std::remove_reference_t<T>::ValueType&>;
        { t.remove(key) } -> std::convertible_to<bool>;
    };

    struct ContiguousContainerTag{ };

    template<typename T>
    concept ContiguousContainer = Container<T> && requires(T t) {
        typename std::remove_reference_t<T>::Iterator;
        typename std::remove_reference_t<T>::ReverseIterator;
        typename std::remove_reference_t<T>::ConstIterator;
        typename std::remove_reference_t<T>::ConstReverseIterator;
        std::is_same_v<typename std::remove_reference_t<T>::ContainerTag, ContiguousContainerTag>;
        { t.data() } -> std::convertible_to<typename std::remove_reference_t<T>::ValueType const*>;
        { t.data_view() } -> std::convertible_to<ice::Data>;
    };

    template<typename T>
    concept ContiguousResizableContainer = ResizableContainer<T> && ContiguousContainer<T> && requires(T t) {
        { t.memory_view() } -> std::convertible_to<ice::Memory>;
    };

    template<typename T>
    concept TrivialContainerLogic = ContainerType<T>
        && TrivialContainerLogicAllowed<typename std::remove_reference_t<T>::ValueType>;

    template<typename T>
    concept RegularContainerLogic = ContainerType<T>
        && not TrivialContainerLogicAllowed<typename std::remove_reference_t<T>::ValueType>;

    template<typename T>
    concept HashableKeyType = requires(T t) {
        { ice::hash(t) } -> std::convertible_to<ice::u64>;
    };

} // namespace ice::concepts

namespace ice
{

    template<typename T>
    struct Span;

} // namespace ice

namespace ice::container
{

    template<ice::concepts::ContainerType ContainerT>
    using ConstCorrectContainerValueType = std::conditional_t<
        std::is_const_v<typename std::remove_reference_t<ContainerT>>,
        typename std::remove_reference_t<ContainerT>::ConstContainerValueType,
        typename std::remove_reference_t<ContainerT>::ValueType
    >;

    template<ice::concepts::ContainerType ContainerT>
    using ConstCorrectContainerIterator = std::conditional_t<
        std::is_const_v<typename std::remove_reference_t<ContainerT>>,
        typename std::remove_reference_t<ContainerT>::ConstIterator,
        typename std::remove_reference_t<ContainerT>::Iterator
    >;

    template<ice::concepts::ContainerType ContainerT>
    using ConstCorrectContainerReverseIterator = std::conditional_t<
        std::is_const_v<typename std::remove_reference_t<ContainerT>>,
        typename std::remove_reference_t<ContainerT>::ConstReverseIterator,
        typename std::remove_reference_t<ContainerT>::ReverseIterator
    >;

    template<ice::concepts::ContainerType ContainerT>
    using KeyType = typename std::remove_reference_t<ContainerT>::KeyType;

    template<ice::concepts::ContainerType ContainerT>
    using KeyTypeArg = std::conditional_t<
        sizeof(ice::container::KeyType<ContainerT>) <= 16 && std::is_trivially_copyable_v<ice::container::KeyType<ContainerT>>,
        ice::container::KeyType<ContainerT>,
        ice::container::KeyType<ContainerT> const&
    >;


    template<ice::concepts::ContainerType ContainerT>
    using ValueType = ConstCorrectContainerValueType<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using ValueRef = ValueType<ContainerT>&;

    template<ice::concepts::ContainerType ContainerT>
    using ValueRVal = ValueType<ContainerT>&&;

    template<ice::concepts::ContainerType ContainerT>
    using ValuePtr = ValueType<ContainerT>*;

    template<ice::concepts::ContainerType ContainerT>
    using Iterator = ConstCorrectContainerIterator<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using ReverseIterator = ConstCorrectContainerReverseIterator<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using ContainerType = typename std::remove_reference_t<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using SpanType = ice::Span<ice::container::ConstCorrectContainerValueType<ContainerT>>;

} // namespace ice::container

namespace ice::concepts
{

    template<typename TargetT, typename SourceContainerT>
    concept CompatibleContainer = std::convertible_to<
        ice::container::ValueType<SourceContainerT>,
        TargetT
    >;// && std::is_constructible_v<Type, ice::container::ValueType<ContainerT>>)


    template<typename T>
    concept IterableContainer = ice::concepts::Container<T> && requires(T t) {
        { t.begin() } -> std::convertible_to<ice::container::Iterator<T>>;
        { t.end() } -> std::convertible_to<ice::container::Iterator<T>>;
    };

    template<typename T>
    concept ReverseIterableContainer = requires(T t) {
        { t.rbegin() } -> std::convertible_to<ice::container::ReverseIterator<T>>;
        { t.rend() } -> std::convertible_to<ice::container::ReverseIterator<T>>;
    };

} // namespace ice::concepts
