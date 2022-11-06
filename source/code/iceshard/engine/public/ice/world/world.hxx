/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/data_storage.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

namespace ice
{

    class WorldTrait;
    class WorldPortal;

    enum class WorldState
    {
        Idle,
        Active,
        Managed,
    };

    class World
    {
    public:
        virtual auto allocator() noexcept -> ice::Allocator& = 0;
        virtual auto entity_storage() noexcept -> ice::ecs::EntityStorage& = 0;

        virtual auto state_hint() const noexcept -> ice::WorldState = 0;

        virtual void add_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept = 0;

        virtual void remove_trait(
            ice::StringID_Arg name
        ) noexcept = 0;

    protected:
        virtual ~World() noexcept = default;
    };

} // namespace ice
