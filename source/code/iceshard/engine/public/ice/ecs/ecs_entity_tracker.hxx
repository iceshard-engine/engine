/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/container_types.hxx>
#include <ice/shard_container.hxx>

#include <ice/ecs/ecs_entity.hxx>

namespace ice::ecs
{

#if 0
    class EntityTracker
    {
    public:
        EntityTracker(
            ice::Allocator& alloc
        ) noexcept;

        ~EntityTracker() noexcept = default;

        auto find_handle(ice::ecs::Entity entity) const noexcept -> ice::ecs::EntityHandle;
        auto find_handle(ice::StringID name) const noexcept -> ice::ecs::EntityHandle;

        void track_entity(
            ice::ecs::Entity entity,
            ice::StringID name = ice::StringID_Invalid
        ) noexcept;

        void forget_entity(ice::ecs::Entity entity) noexcept;
        void forget_entity(ice::StringID name) noexcept;

        void refresh_handles(ice::ShardContainer const& shards) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::ecs::EntityHandle> _tracked_entities;
        ice::HashMap<ice::ecs::Entity> _named_entities;
    };
#endif

} // namespace ice::ecs
