/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

namespace ice
{

    class World;

    class WorldManager : public ice::WorldAssembly
    {
    public:
        using WorldAssembly::create_world;

        virtual auto create_world(
            ice::WorldTemplate const& world_template
        ) noexcept -> World* = 0;

        virtual auto find_world(
            ice::StringID_Arg name
        ) noexcept -> World* = 0;

        virtual void destroy_world(
            ice::StringID_Arg world
        ) noexcept = 0;

    protected:
        virtual ~WorldManager() noexcept = default;
    };

} // namespace ice
