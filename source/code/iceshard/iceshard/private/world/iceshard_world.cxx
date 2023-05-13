/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_shards.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

#include "iceshard_world_portal.hxx"

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::ecs::EntityStorage* entity_storage
    ) noexcept
        : _allocator{ alloc }
        , _entity_storage{ entity_storage }
        , _state{ WorldState::Idle }
        , _traits{ _allocator }
        , _portals{ _allocator }
    {
    }

    IceshardWorld::~IceshardWorld() noexcept
    {
        for (ice::IceshardWorldPortal* portal : _portals)
        {
            ICE_ASSERT(
                portal->is_owning() == true,
                "Not all external traits where removed from this World before destruction!"
            );

            if (portal->is_owning())
            {
                _allocator.destroy(portal->trait());
                _allocator.destroy(portal);
            }
        }
    }

    auto IceshardWorld::allocator() noexcept -> ice::Allocator&
    {
        return _allocator;
    }

    auto IceshardWorld::entity_storage() noexcept -> ice::ecs::EntityStorage&
    {
        return *_entity_storage;
    }

    auto IceshardWorld::state_hint() const noexcept -> ice::WorldState
    {
        return _state;
    }

    void IceshardWorld::set_state(
        ice::WorldState state
    ) noexcept
    {
        _state = state;
    }

    void IceshardWorld::add_trait(
        ice::StringID_Arg name,
        ice::WorldTrait* trait
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_portals, name_hash) == false,
            "World already contains a trait of name {}",
            ice::stringid_hint(name)
        );

        ice::array::push_back(
            _traits,
            trait
        );

        ice::hashmap::set(
            _portals,
            name_hash,
            _allocator.create<IceshardWorldPortal>(
                _allocator,
                *this,
                trait,
                *_entity_storage,
                false
            )
        );
    }

    void IceshardWorld::add_owning_trait(
        ice::StringID_Arg name,
        ice::WorldTrait* trait
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_portals, name_hash) == false,
            "World already contains a trait of name {}",
            ice::stringid_hint(name)
        );

        ice::array::push_back(
            _traits,
            trait
        );

        ice::hashmap::set(
            _portals,
            name_hash,
            _allocator.create<IceshardWorldPortal>(
                _allocator,
                *this,
                trait,
                *_entity_storage,
                true
            )
        );
    }

    void IceshardWorld::remove_trait(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_portals, name_hash) == true,
            "World does not contain a trait with name {}",
            ice::stringid_hint(name)
        );

        ice::IceshardWorldPortal* const portal = ice::hashmap::get(_portals, name_hash, nullptr);

        auto* it = ice::array::begin(_traits);
        auto* const end = ice::array::end(_traits);
        while (it != end && (*it) != portal->trait())
        {
            it += 1;
        }

        ICE_ASSERT(it != end, "Couldnt find location of the world trait to be removed!");
        *it = ice::array::back(_traits);
        ice::array::pop_back(_traits);

        ice::hashmap::remove(
            _portals,
            ice::hash(name_hash)
        );

        _allocator.destroy(portal);
    }

    auto IceshardWorld::find_trait(
        ice::StringID_Arg name
    ) const noexcept -> ice::WorldTrait*
    {
        ice::u64 const name_hash = ice::hash(name);

        ice::IceshardWorldPortal* const portal = ice::hashmap::get(_portals, name_hash, nullptr);
        return portal == nullptr ? nullptr : portal->trait();
    }

    void IceshardWorld::activate(
        ice::Engine& engine,
        ice::EngineRunner& runner
    ) noexcept
    {
        for (ice::IceshardWorldPortal* portal : _portals)
        {
            portal->trait()->on_activate(
                engine, runner, *portal
            );
        }
    }

    void IceshardWorld::deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner
    ) noexcept
    {
        ice::Span<ice::IceshardWorldPortal*> portals = ice::hashmap::values(_portals);
        auto it = ice::span::rbegin(portals);
        auto const end = ice::span::rend(portals);

        while (it != end)
        {
            (*it)->trait()->on_deactivate(
                engine, runner, *(*it)
            );
            it += 1;
        }
    }

    void IceshardWorld::update(
        ice::EngineRunner& runner
    ) noexcept
    {
        ice::EngineFrame& current_frame = runner.current_frame();

        // #todo: This needs to change, we need to make worlds responsible for: EntityStorage, EntityOperations and probably QueryScheduling
        _entity_storage->execute_operations(
            runner.previous_frame().entity_operations(),
            current_frame.shards()
        );

        for (ice::IceshardWorldPortal* portal : _portals)
        {
            portal->remove_finished_tasks();
            portal->trait()->on_update(
                runner.current_frame(),
                runner,
                *portal
            );
        }
    }

    auto IceshardWorld::traits() noexcept -> ice::Span<ice::WorldTrait*>
    {
        return _traits;
    }

} // namespace ice
