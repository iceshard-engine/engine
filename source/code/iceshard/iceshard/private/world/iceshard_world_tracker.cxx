#include "iceshard_world_tracker.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/pod/hash.hxx>

namespace ice
{

    IceshardWorldTracker::IceshardWorldTracker(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _worlds{ _allocator }
    {
        ice::pod::hash::reserve(_worlds, 20);
    }

    IceshardWorldTracker::~IceshardWorldTracker() noexcept
    {
    }

    void IceshardWorldTracker::activate_world(
        ice::EngineRunner& runner,
        ice::IceshardWorld* world
    ) noexcept
    {
        Entry entry = ice::pod::hash::get(
            _worlds,
            ice::hash_from_ptr(world),
            Entry{ .world = world, .current_state = WorldState::Idle }
        );

        if (entry.current_state != WorldState::Active)
        {
            for (auto const& trait_entry : entry.world->traits())
            {
                trait_entry.value->on_activate(runner, *world);
            }
            entry.current_state = WorldState::Active;
        }

        ice::pod::hash::set(_worlds, ice::hash_from_ptr(world), entry);
    }

    void IceshardWorldTracker::deactivate_world(
        ice::EngineRunner& runner,
        ice::IceshardWorld* world
    ) noexcept
    {
        Entry entry = ice::pod::hash::get(
            _worlds,
            ice::hash_from_ptr(world),
            Entry{ .world = world, .current_state = WorldState::Idle }
        );

        if (entry.current_state != WorldState::Idle)
        {
            for (auto const& trait_entry : entry.world->traits())
            {
                trait_entry.value->on_deactivate(runner, *world);
            }

            ice::pod::hash::remove(_worlds, ice::hash_from_ptr(world));
        }
    }

    void IceshardWorldTracker::update_active_worlds(
        ice::EngineRunner& runner
    ) noexcept
    {
        for (auto const& entry : _worlds)
        {
            if (entry.value.current_state == WorldState::Active)
            {
                entry.value.world->update(runner, WorldUpdateKey{ });
            }
        }
    }

} // namespace ice
