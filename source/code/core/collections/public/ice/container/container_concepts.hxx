#pragma once
#include <ice/types/ncount.hxx>
#include <ice/types/nindex.hxx>

namespace ice::concepts
{

    template<typename T>
    concept ContainerType = requires(T t) {
        typename std::remove_reference_t<T>::SizeType;
        typename std::remove_reference_t<T>::ValueType;
        typename std::remove_reference_t<T>::Iterator;
        typename std::remove_reference_t<T>::ReverseIterator;
        typename std::remove_reference_t<T>::ConstIterator;
        typename std::remove_reference_t<T>::ConstReverseIterator;
    };

    template<typename T>
    concept ContiguousContainer = ContainerType<T> && requires(T t) {
        { t.size() } -> std::convertible_to<ice::ncount>;
        { t.data() } -> std::convertible_to<typename std::remove_reference_t<T>::ValueType const*>;
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
    using ContainerType = typename std::remove_reference_t<ContainerT>;

    template<ice::concepts::ContainerType ContainerT>
    using SpanType = ice::Span<typename std::remove_reference_t<ContainerT>::ValueType>;

    template<ice::concepts::ContainerType ContainerT>
    //using ValueRef = ice::const_correct_t<std::remove_reference_t<ContainerT>, typename ContainerT::ValueType>&;
    using ValueRef = typename std::remove_reference_t<ContainerT>::ValueType&;

    template<ice::concepts::ContainerType ContainerT>
    //using ValuePtr = ice::const_correct_t<std::remove_reference_t<ContainerT>, typename ContainerT::ValueType>*;
    using ValuePtr = typename std::remove_reference_t<ContainerT>::ValueType*;

    template<ice::concepts::ContainerType ContainerT>
    using Iterator = typename std::remove_reference_t<ContainerT>::Iterator;

    template<ice::concepts::ContainerType ContainerT>
    using ReverseIterator = typename std::remove_reference_t<ContainerT>::ReverseIterator;

} // namespace ice::container
