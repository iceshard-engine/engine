/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/ecs/ecs_component.hxx>

namespace ice::ecs
{

    //! \brief Opaque handle identyfing a single archetype. This value is calculated from a sorted list of components defining that archetype.
    //!
    //! \remark If two archetypes are defined with the same component but in different order, the hash will still be the same.
    //!   See the `ice::ecs::static_validation` section in this file for explicit compiletime checks and invariants.
    //! \remark The 'Archetype::Invalid' value is referencing a special 'Null' archetype that cannot have entities and does not contain any data.
    //!
    //! \headerfile <ice/ecs/ecs_archetype.hxx>
    enum class Archetype : ice::u64;

    //! \brief Compile-time definition of an archetypes set of components it will store for each entity.
    //! \tparam ...Components A set of types that each entity will have access to.
    //!
    //! \headerfile <ice/ecs/ecs_archetype.hxx>
    template<ice::ecs::concepts::Component... Components>
    struct ArchetypeDefinition;

    //! \brief Provides the same information as `ArchetypeDefinition` however it's not templated, allowing for type-erased access.
    //!
    //! \headerfile <ice/ecs/ecs_archetype.hxx>
    struct ArchetypeInfo;

    //! \headerfile <ice/ecs/ecs_archetype_index.hxx>
    class ArchetypeIndex;

    //! \brief Primary identifier for entities in IceShard. Used to update data using operations or access components with queries.
    //!
    //! \details An entity handle is opaque and does not provide any functionality out of the box. It's mainly used as an argument in other ECS related
    //!   systems. For examples, you can check the entities 'is_alive' status with 'EntityIndex' or query a specific component with 'Query' objects.
    enum class Entity : ice::u32;

    //! \brief Provides easy access to the specific parts making up the 'Entity' handle value.
    struct EntityInfo;

    //! \brief Describes the location where the entity data is stored for a specific EntityStorage implementation. There shouldn't be any case where
    //!   users of the API will need to directly access this data.
    //!
    //! \remarks (2025.05) The current storage implementation, does not grant access to all components of a specific entity with just the
    //!   data available in this structure. You will also need the Archetype description to access the right locations.
    struct EntityDataSlot;


    class EntityIndex;
    class EntityStorage;
    class EntityOperations;


    enum class QueryPolicy
    {
        Unchecked,
        Synchronized,

        Default = Unchecked,
    };

    template<ice::ecs::QueryPolicy Policy, typename... Parts>
    struct Query;

    class QueryStorage;
    struct QueryProvider;
    struct QueryAccessTracker;

    struct QueryView;

} // namespace ice::ecs
