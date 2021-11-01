#pragma once
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_component.hxx>

namespace ice::ecs::detail
{

    struct Query_TypeInfo
    {
        bool const is_writable = false;
        bool const is_optional = false;
    };


    template<typename T>
    concept Query_Type = ice::ecs::IsEntityHandle<T>
        || (ice::ecs::Component<T> && (std::is_reference_v<T> || std::is_pointer_v<T>));

    template<Query_Type T>
    constexpr bool Query_TypeIsWritable = ice::ecs::IsEntityHandle<T> == false
        && std::is_const_v<ice::clear_type_t<T>> == false;

    template<Query_Type T>
    constexpr bool Query_TypeIsOptional = ice::ecs::IsEntityHandle<T> == false
        && std::is_pointer_v<T>;


    template<Query_Type T>
    struct Query_ComponentTypeInfo
    {
        bool const is_writable = ice::ecs::detail::Query_TypeIsWritable<T>;
        bool const is_optional = ice::ecs::detail::Query_TypeIsOptional<T>;

        constexpr operator Query_TypeInfo() const noexcept;
    };

    template<Query_Type T>
    constexpr Query_ComponentTypeInfo<T>::operator Query_TypeInfo() const noexcept
    {
        return Query_TypeInfo{ .is_writable = is_writable, .is_optional = is_optional };
    }

} // namespace ice::ecs::detail
