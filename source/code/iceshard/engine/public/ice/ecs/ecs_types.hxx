/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::ecs
{

    enum class Entity : ice::u32;
    enum class EntityHandle : ice::u64;

    struct ArchetypeInfo;

    class ArchetypeIndex;
    class EntityIndex;
    class EntityStorage;
    class EntityOperations;

    struct QueryProvider;
    struct QueryAccessTracker;

    template<typename Definition>
    struct Query;

    struct QueryView;

} // namespace ice::ecs
