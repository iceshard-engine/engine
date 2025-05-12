/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_concepts.hxx>

namespace ice::ecs
{

    namespace concepts
    {

        //! \brief A component needs to define specified members and be trivially copyable.
        //!
        //! \see `ice::ecs::concepts::ComponentTypeMembers` for more information on which members are required.
        //! \see `std::is_trivially_copyable_v`
        //!
        //! \note The current `EntityStorage` only moves data using `ice::memcpy` so only trivially copyable components are allowed.
        template<typename T>
        concept Component = ice::ecs::concepts::ComponentTypeMembers<T>
            && std::is_trivially_copyable_v<ice::clear_type_t<T>>;

        //! \brief A tag is an empty component, there is no data stored for tags just information of their existances.
        //!
        //! \note Currently there is no way to define tags using a `ArchetypeDefinition`.
        template<typename T>
        concept ComponentTag = ice::ecs::concepts::Component<T> && std::is_empty_v<T>;

    } // namespace concepts

    using ice::ecs::concepts::Component;
    using ice::ecs::concepts::ComponentTag;

    //! \brief Quick access to the `identifier` of a specific component, tag or the entity type.
    template<typename T> requires ice::ecs::Component<T> || ice::ecs::concepts::Entity<T>
    constexpr inline ice::StringID Constant_ComponentIdentifier = T::Identifier;

    template<>
    constexpr inline ice::StringID Constant_ComponentIdentifier<ice::ecs::Entity> = "ice.__ecs_entity__"_sid;

    //! \brief Quick access to the `size` of a component, tag or or the entity type.
    template<typename T> requires ice::ecs::Component<T> || ice::ecs::concepts::Entity<T>
    constexpr ice::u32 Constant_ComponentSize = ice::ecs::ComponentTag<T> ? 0 : sizeof(T);

    //! \brief Quick access to the `alignment` of a component, tag or or the entity type.
    template<typename T> requires ice::ecs::Component<T> || ice::ecs::concepts::Entity<T>
    constexpr ice::u32 Constant_ComponentAlignment = ice::ecs::ComponentTag<T> ? 0 : alignof(T);

} // namespace ice::ecs
