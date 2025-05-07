#pragma once
#include <ice/ecs/ecs_component.hxx>

namespace ice::ecs
{

    namespace detail
    {

        template<typename T>
        concept QueryType = ice::ecs::IsEntity<T>
            || (ice::ecs::Component<T> && !ice::ecs::ComponentTag<T> && (std::is_reference_v<T> || std::is_pointer_v<T>));

        template<QueryType T>
        constexpr bool Constant_QueryTypeIsWritable = ice::ecs::IsEntity<T> == false
            && std::is_const_v<ice::clear_type_t<T>> == false;

        template<QueryType T>
        constexpr bool Constant_QueryTypeIsOptional = ice::ecs::IsEntity<T> == false
            && std::is_pointer_v<T>;

        struct QueryTypeInfo
        {
            ice::StringID identifier;

            bool is_writable = false;
            bool is_optional = false;
        };

        constexpr bool operator<(
            ice::ecs::detail::QueryTypeInfo const& left,
            ice::ecs::detail::QueryTypeInfo const& right
        ) noexcept
        {
            return ice::hash(left.identifier) < ice::hash(right.identifier);
        }


        template<QueryType T>
        struct QueryComponentTypeInfo
        {
            ice::StringID const identifier = ice::ecs::Constant_ComponentIdentifier<ice::clear_type_t<T>>;

            bool const is_writable = ice::ecs::detail::Constant_QueryTypeIsWritable<T>;
            bool const is_optional = ice::ecs::detail::Constant_QueryTypeIsOptional<T>;

            constexpr operator QueryTypeInfo() const noexcept;
        };

        template<QueryType T>
        constexpr QueryComponentTypeInfo<T>::operator QueryTypeInfo() const noexcept
        {
            return QueryTypeInfo{ .identifier = identifier, .is_writable = is_writable, .is_optional = is_optional };
        }

    } // namespace detail

    using ice::ecs::detail::QueryType;

    template<ice::ecs::QueryType... QueryComponents>
    struct Query;

    template<typename... QueryParts>
    struct Query_v2;

    template<ice::u32 CompIdx, ice::ecs::QueryType... QueryComponents>
    struct QueryPart;

} // namespace ice::ecs
