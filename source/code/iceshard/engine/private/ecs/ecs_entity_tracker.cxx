#include <ice/ecs/ecs_entity_tracker.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    EntityTracker::EntityTracker(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _tracked_entities{ _allocator }
        , _named_entities{ _allocator }
    {
        ice::pod::hash::reserve(_tracked_entities, 1024);
        ice::pod::hash::reserve(_named_entities, 256);
    }

    auto EntityTracker::find_handle(ice::ecs::Entity entity) const noexcept -> ice::ecs::EntityHandle
    {
        return ice::pod::hash::get(_tracked_entities, ice::hash(entity), ice::ecs::EntityHandle::Invalid);
    }

    auto EntityTracker::find_handle(ice::StringID name) const noexcept -> ice::ecs::EntityHandle
    {
        ice::u64 const name_hash = ice::hash(name);

        EntityHandle result = EntityHandle::Invalid;
        if (ice::pod::hash::has(_named_entities, name_hash))
        {
            Entity const entity = ice::pod::hash::get(_named_entities, name_hash, Entity{ });
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
            ice::pod::hash::has(_named_entities, name_hash) == false,
            "An entity with this name '{}' is already tracked! [ tracked_entity:{} ]",
            ice::stringid_hint(name),
            ice::pod::hash::get(_named_entities, name_hash, Entity{ })
        );

        ice::pod::hash::set(
            _named_entities,
            name_hash,
            entity
        );
    }

    void EntityTracker::forget_entity(ice::ecs::Entity entity) noexcept
    {
        ice::pod::hash::remove(_tracked_entities, ice::hash(entity));
    }

    void EntityTracker::forget_entity(ice::StringID name) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);

        if (ice::pod::hash::has(_named_entities, name_hash))
        {
            Entity const entity = ice::pod::hash::get(_named_entities, name_hash, Entity{ });
            ice::pod::hash::remove(_tracked_entities, ice::hash(entity));
            ice::pod::hash::remove(_named_entities, name_hash);
        }
    }

    void EntityTracker::refresh_handles(ice::ShardContainer const& shards) noexcept
    {
        for (Shard const shard : shards)
        {
            if (shard == ice::ecs::Shard_EntityHandleChanged)
            {
                EntityHandle const entity_handle = ice::shard_shatter<EntityHandle>(shard);
                EntityHandleInfo const handle_info = ice::ecs::entity_handle_info(entity_handle);
                u64 const entity_hash = ice::hash(handle_info.entity);

                // If that entity exists, update the tracked handle
                if (ice::pod::hash::has(_tracked_entities, entity_hash))
                {
                    ice::pod::hash::set(_tracked_entities, entity_hash, entity_handle);
                }
            }

            if constexpr (ice::build::is_debug || ice::build::is_develop)
            {
                if (shard == ice::ecs::Shard_EntityDestroyed)
                {
                    Entity const entity = ice::shard_shatter<Entity>(shard);
                    ICE_ASSERT(
                        ice::pod::hash::has(_tracked_entities, ice::hash(entity)) == false,
                        "Don't rely on the tracker to remove tracked entities! You are responsible to remove the tracked entity once a destroy operation was queued."
                    );
                }
            }
        }
    }



} // namespace ice::ecs
