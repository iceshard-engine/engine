/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ecs/ecs_entity_tracker.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    EntityTracker::EntityTracker(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _tracked_entities{ _allocator }
        , _named_entities{ _allocator }
    {
        ice::hashmap::reserve(_tracked_entities, 1024);
        ice::hashmap::reserve(_named_entities, 256);
    }

    auto EntityTracker::find_handle(ice::ecs::Entity entity) const noexcept -> ice::ecs::EntityHandle
    {
        return ice::hashmap::get(_tracked_entities, ice::hash(entity), ice::ecs::EntityHandle::Invalid);
    }

    auto EntityTracker::find_handle(ice::StringID name) const noexcept -> ice::ecs::EntityHandle
    {
        ice::u64 const name_hash = ice::hash(name);

        EntityHandle result = EntityHandle::Invalid;
        if (ice::hashmap::has(_named_entities, name_hash))
        {
            Entity const entity = ice::hashmap::get(_named_entities, name_hash, Entity{ });
            result = this->find_handle(entity);
        }
        return result;
    }

    void EntityTracker::track_entity(
        ice::ecs::Entity entity,
        ice::StringID name /*= ice::stringid_invalid*/
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_named_entities, name_hash) == false,
            "An entity with this name '{}' is already tracked! [ tracked_entity:{} ]",
            ice::stringid_hint(name),
            ice::hashmap::get(_named_entities, name_hash, Entity{ })
        );

        ice::hashmap::set(
            _named_entities,
            name_hash,
            entity
        );
    }

    void EntityTracker::forget_entity(ice::ecs::Entity entity) noexcept
    {
        ice::hashmap::remove(_tracked_entities, ice::hash(entity));
    }

    void EntityTracker::forget_entity(ice::StringID name) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);

        if (ice::hashmap::has(_named_entities, name_hash))
        {
            Entity const entity = ice::hashmap::get(_named_entities, name_hash, Entity{ });
            ice::hashmap::remove(_tracked_entities, ice::hash(entity));
            ice::hashmap::remove(_named_entities, name_hash);
        }
    }

    void EntityTracker::refresh_handles(ice::ShardContainer const& shards) noexcept
    {
        for (Shard const shard : shards)
        {
            if (shard == ice::ecs::Shard_EntityHandleChanged)
            {
                EntityHandle const entity_handle = ice::shard_shatter<EntityHandle>(shard, EntityHandle::Invalid);
                EntityHandleInfo const handle_info = ice::ecs::entity_handle_info(entity_handle);
                u64 const entity_hash = ice::hash(handle_info.entity);

                // If that entity exists, update the tracked handle
                if (ice::hashmap::has(_tracked_entities, entity_hash))
                {
                    ice::hashmap::set(_tracked_entities, entity_hash, entity_handle);
                }
            }

            if constexpr (ice::build::is_debug || ice::build::is_develop)
            {
                if (shard == ice::ecs::Shard_EntityDestroyed)
                {
                    Entity const entity = ice::shard_shatter<Entity>(shard, Entity{ });
                    ICE_ASSERT(
                        ice::hashmap::has(_tracked_entities, ice::hash(entity)) == false,
                        "Don't rely on the tracker to remove tracked entities! You are responsible to remove the tracked entity once a destroy operation was queued."
                    );
                }
            }
        }
    }



} // namespace ice::ecs
