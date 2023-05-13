/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world_tracker.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/container/hashmap.hxx>

namespace ice
{

    IceshardWorldTracker::IceshardWorldTracker(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _worlds{ _allocator }
    {
        ice::hashmap::reserve(_worlds, 20);
    }

    IceshardWorldTracker::~IceshardWorldTracker() noexcept
    {
    }

    void IceshardWorldTracker::set_managed_world(
        ice::IceshardWorld* world
    ) noexcept
    {
        Entry entry = ice::hashmap::get(
            _worlds,
            ice::hash_from_ptr(world),
            Entry{ .world = world, .current_state = WorldState::Managed }
        );

        entry.current_state = WorldState::Managed;
        ice::hashmap::set(_worlds, ice::hash_from_ptr(world), entry);

        world->set_state(entry.current_state);
    }

    void IceshardWorldTracker::unset_manager_world(
        ice::IceshardWorld* world
    ) noexcept
    {
        Entry entry = ice::hashmap::get(
            _worlds,
            ice::hash_from_ptr(world),
            Entry{ .world = world, .current_state = WorldState::Idle }
        );

        if (entry.current_state == WorldState::Managed)
        {
            ice::hashmap::remove(_worlds, ice::hash_from_ptr(world));

            world->set_state(WorldState::Idle);
        }
    }

    void IceshardWorldTracker::activate_world(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::IceshardWorld* world
    ) noexcept
    {
        Entry entry = ice::hashmap::get(
            _worlds,
            ice::hash_from_ptr(world),
            Entry{ .world = world, .current_state = WorldState::Idle }
        );

        if (entry.current_state == WorldState::Idle)
        {
            entry.world->activate(engine, runner);
            entry.current_state = WorldState::Active;
            world->set_state(entry.current_state);
        }

        ice::hashmap::set(_worlds, ice::hash_from_ptr(world), entry);
    }

    void IceshardWorldTracker::deactivate_world(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::IceshardWorld* world
    ) noexcept
    {
        Entry entry = ice::hashmap::get(
            _worlds,
            ice::hash_from_ptr(world),
            Entry{ .world = world, .current_state = WorldState::Idle }
        );

        if (entry.current_state == WorldState::Active)
        {
            entry.world->deactivate(engine, runner);
            ice::hashmap::remove(_worlds, ice::hash_from_ptr(world));
            world->set_state(WorldState::Idle);
        }
    }

    void IceshardWorldTracker::update_active_worlds(
        ice::EngineRunner& runner
    ) noexcept
    {
        for (Entry const& entry : _worlds)
        {
            if (entry.current_state == WorldState::Active)
            {
                entry.world->update(runner);
            }
        }
    }

} // namespace ice
