/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>

namespace ice::ecs
{

    enum class Entity : ice::u32;

    struct ArchetypeInfo;

    class ArchetypeIndex;
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
