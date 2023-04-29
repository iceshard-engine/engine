/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/ecs/ecs_types.hxx>

namespace ice
{

    class DataStorage;
    class World;

    class WorldPortal
    {
    public:
        virtual ~WorldPortal() noexcept = default;

        virtual auto world() const noexcept -> ice::World const& = 0;

        virtual auto allocator() noexcept -> ice::Allocator& = 0;

        virtual auto storage() noexcept -> ice::DataStorage& = 0;
        virtual auto entity_storage() noexcept -> ice::ecs::EntityStorage& = 0;

        virtual void execute(ice::Task<void> task) noexcept = 0;
    };

} // namespace ice
