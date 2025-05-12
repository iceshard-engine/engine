/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_concepts.hxx>

namespace ice::ecs
{

    namespace concepts
    {

        //! \brief A components needs to have all required members \see `ComponentTypeMembers` and be trivially copyable.
        //!
        //! \note The current entity storage only copies data using `memcpy` so only trivially copyable components are allowed.
        template<typename T>
        concept Component = ice::ecs::concepts::ComponentTypeMembers<T>
            && std::is_trivially_copyable_v<ice::clear_type_t<T>>;

        //! \brief A tag is an empty component, there is no data stored for tags just information of their existances.
        //!
        //! \note Archetypes and without tags are separate instances!
        template<typename T>
        concept ComponentTag = ice::ecs::concepts::Component<T> && std::is_empty_v<T>;

    } // namespace concepts

    using ice::ecs::concepts::Component;
    using ice::ecs::concepts::ComponentTag;

    //! \brief Quick access to the identifier of a specific component, tag or the entity type.
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
