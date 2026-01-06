#pragma once
#include <ice/types/ncount.hxx>
#include <ice/types/nindex.hxx>

namespace ice::concepts
{

    template<typename T>
    concept ContiguousContainer = requires(T t) {
        { t.size() } -> std::convertible_to<ice::ncount>;
        { t.data() } -> std::convertible_to<typename T::ValueType const*>;
    };

} // namespace ice::concepts

namespace ice::container
{

    template<typename ContainerT>
    using ValueRef = ice::const_correct_t<std::remove_reference_t<ContainerT>, typename ContainerT::ValueType>&;

    template<typename ContainerT>
    using ValuePtr = ice::const_correct_t<std::remove_reference_t<ContainerT>, typename ContainerT::ValueType>*;

} // namespace ice::container
