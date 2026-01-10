#pragma once
#include <ice/types/ncount.hxx>
#include <ice/types/nindex.hxx>

namespace ice::concepts
{

    template<typename T>
    concept ContainerType = requires(T t) {
        typename std::remove_reference_t<T>::SizeType;
        typename std::remove_reference_t<T>::ValueType;
        typename std::remove_reference_t<T>::ConstContainerValueType;
        typename std::remove_reference_t<T>::Iterator;
        typename std::remove_reference_t<T>::ReverseIterator;
        typename std::remove_reference_t<T>::ConstIterator;
        typename std::remove_reference_t<T>::ConstReverseIterator;
    };

    template<typename T>
    concept Container = ContainerType<T> && requires(T t) {
        { t.size() } -> std::convertible_to<ice::ncount>;
        { t.data() } -> std::convertible_to<typename std::remove_reference_t<T>::ValueType const*>;
    };

    template<typename T>
    concept ResizableContainer = Container<T> && requires(T t, ice::ncount size) {
        { t.data() } -> std::convertible_to<typename std::remove_reference_t<T>::ValueType*>;
        { t.resize(size) } -> std::convertible_to<void>;
        { t.capacity() } -> std::convertible_to<ice::ncount>;
        { t.set_capacity(size) } -> std::convertible_to<void>;
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
        typename std::remove_reference_t<ContainerT>::ReverseConstIterator,
        typename std::remove_reference_t<ContainerT>::ReverseIterator
    >;

    template<ice::concepts::ContainerType ContainerT>
    using ValueRef = ConstCorrectContainerValueType<ContainerT>&;

    template<ice::concepts::ContainerType ContainerT>
    using ValuePtr = ConstCorrectContainerValueType<ContainerT>*;

    template<ice::concepts::ContainerType ContainerT>
    using Iterator = ConstCorrectContainerIterator<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using ReverseIterator = ConstCorrectContainerReverseIterator<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using ContainerType = typename std::remove_reference_t<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using SpanType = ice::Span<ice::container::ConstCorrectContainerValueType<ContainerT>>;

} // namespace ice::container
