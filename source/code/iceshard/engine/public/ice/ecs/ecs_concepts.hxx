#pragma once
#include <ice/stringid.hxx>

namespace ice::ecs
{

    enum class Entity : ice::u32;

} // namespace ice::ecs

namespace ice::ecs::concepts
{

    //! \brief Concept to check if a type is an entity handle.
    //!
    //! \details We use a concept instead the `std::is_same_v` check directly due to the possibility the types will change and grow over time.
    //!
    //! \see ice::ecs::Entity
    template<typename T>
    concept Entity = std::is_same_v<ice::ecs::Entity, T>;

    //! \brief Concept checking that a type has all necessary members and definitions to be considered a component type.
    //!
    //! \details This concepts is currently very simple but once C++ static reflection are available this might change.
    //!
    //! \see ice::ecs::concepts::Component
    template<typename T>
    concept ComponentTypeMembers = requires(T t) {
        { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
    };

    //! \brief Concept checking that the referenced component type meets requirements to be used in the `ice::ecs::QueryBuilder::with` method.
    //!
    //! \see ice::ecs::concepts::Component
    //! \see ice::ecs::QueryBuilder
    template<typename T>
    concept ComponentRefTypeMembers = ice::ecs::concepts::ComponentTypeMembers<T> && requires(T t) {
        { t.entity } -> ice::ecs::concepts::Entity;
    };

} // namespace ice::ecs
