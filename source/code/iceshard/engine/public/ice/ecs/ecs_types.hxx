/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/ecs/ecs_component.hxx>

namespace ice::ecs
{

    enum class Archetype : ice::u64;

    template<ice::ecs::concepts::Component... Components>
    struct ArchetypeDefinition;
    struct ArchetypeInfo;

    class ArchetypeIndex;


    enum class Entity : ice::u32;

    struct EntityInfo;
    struct EntityDataSlot;

    class EntityIndex;
    class EntityStorage;
    class EntityOperations;


    enum class QueryType
    {
        Unchecked,
        Synchronized,

        Default = Unchecked,
    };

    template<ice::ecs::QueryType Type, typename... Parts>
    struct Query;

    class QueryStorage;
    struct QueryProvider;
    struct QueryAccessTracker;

    struct QueryView;

    namespace detail
    {

        struct ArchetypeInstanceInfo;

    } // namespace detail

} // namespace ice::ecs
