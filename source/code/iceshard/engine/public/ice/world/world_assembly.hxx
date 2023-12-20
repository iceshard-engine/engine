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
        bool is_initially_active = true;
    };

    struct WorldAssembly
    {
        virtual ~WorldAssembly() noexcept = default;

        virtual auto create_world(
            ice::WorldTemplate const& world_template
        ) noexcept -> ice::World* = 0;

        virtual auto find_world(
            ice::StringID_Arg name
        ) noexcept -> ice::World* = 0;

        virtual void destroy_world(
            ice::StringID_Arg name
        ) noexcept = 0;

        virtual void query_worlds(
            ice::Array<ice::StringID>& out_worlds,
            bool only_active = true
        ) const noexcept = 0;
    };

} // namespace ice
