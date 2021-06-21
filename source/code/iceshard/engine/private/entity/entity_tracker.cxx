#include <ice/entity/entity_tracker.hxx>
#include <ice/assert.hxx>

namespace ice
{

    EntityTracker::EntityTracker(
        ice::Allocator& alloc,
        ice::EntityIndex& index
    ) noexcept
        : _entity_index{ index }
        , _named_entities{ alloc }
    {
    }

    EntityTracker::~EntityTracker() noexcept
    {
        for (auto const& entry : _named_entities)
        {
            _entity_index.destroy(entry.value);
        }
    }

    bool EntityTracker::create_entity(
        ice::StringID_Arg name,
        ice::Entity& entity_out
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);

        bool can_create = true;
        if (ice::pod::hash::has(_named_entities, name_hash))
        {
            Entity const result = ice::pod::hash::get(_named_entities, name_hash, Entity{ });
            can_create = _entity_index.is_alive(result) == false;
        }


        if (can_create)
        {
            entity_out =  _entity_index.create();
            ice::pod::hash::set(
                _named_entities,
                name_hash,
                entity_out
            );
        }
        else
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Engine,
                "An entity with this name `{}` already exist!",
                ice::stringid_hint(name)
            );
        }

        return can_create;
    }

    bool EntityTracker::find_entity(
        ice::StringID_Arg name,
        ice::Entity& entity_out
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        if (ice::pod::hash::has(_named_entities, name_hash))
        {
            Entity const result = ice::pod::hash::get(_named_entities, name_hash, Entity{ });
            if (_entity_index.is_alive(result))
            {
                entity_out = result;
                return true;
            }

            ice::pod::hash::remove(
                _named_entities,
                name_hash
            );
        }
        return false;
    }

    bool EntityTracker::destroy_entity(ice::StringID_Arg name) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);

        bool destroyed = false;
        if (ice::pod::hash::has(_named_entities, name_hash))
        {
            Entity const tracked_entity = ice::pod::hash::get(_named_entities, name_hash, Entity{});
            if (_entity_index.is_alive(tracked_entity))
            {
                _entity_index.destroy(tracked_entity);
                destroyed = true;
            }

            ice::pod::hash::remove(
                _named_entities,
                name_hash
            );
        }
        return destroyed;
    }

} // namespace ice
