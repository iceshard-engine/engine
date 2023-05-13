/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
//#include <ice/unique_ptr.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/world/world_trait_archive.hxx>

namespace ice
{

    class EntityStorage;

    struct WorldTemplate
    {
        ice::StringID name;
        ice::Span<ice::StringID const> traits;
        ice::ecs::EntityStorage* entity_storage;
    };

    class WorldAssembly
    {
    public:
        virtual ~WorldAssembly() noexcept = default;

        virtual auto create_world(
            ice::Allocator& alloc,
            ice::WorldTemplate const& world_template
        ) const noexcept -> ice::World* = 0;
    };

} // namespace ice
