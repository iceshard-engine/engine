#include "iceshard_world.hxx"
#include <ice/world/world_trait.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_shards.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

#include "iceshard_world_portal.hxx"

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::EntityStorage* entity_storage
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
        ICE_ASSERT(
            ice::pod::array::empty(_portals._data),
            "Not all traits where removed from this World before destruction!"
        );
    }

    auto IceshardWorld::allocator() noexcept -> ice::Allocator&
    {
        return _allocator;
    }

    auto IceshardWorld::entity_storage() noexcept -> ice::EntityStorage&
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
            ice::pod::hash::has(_portals, name_hash) == false,
            "World already contains a trait of name {}",
            ice::stringid_hint(name)
        );

        ice::pod::array::push_back(
            _traits,
            trait
        );

        ice::pod::hash::set(
            _portals,
            name_hash,
            _allocator.make<IceshardWorldPortal>(
                _allocator,
                *this,
                trait,
                *_entity_storage
            )
        );
    }

    void IceshardWorld::remove_trait(
        ice::StringID_Arg name
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_portals, name_hash) == true,
            "World does not contain a trait with name {}",
            ice::stringid_hint(name)
        );

        ice::IceshardWorldPortal* const portal = ice::pod::hash::get(_portals, name_hash, nullptr);

        auto* it = ice::pod::array::begin(_traits);
        auto* const end = ice::pod::array::end(_traits);
        while (it != end && (*it) != portal->trait())
        {
            it += 1;
        }

        ICE_ASSERT(it != end, "Couldnt find location of the world trait to be removed!");
        *it = ice::pod::array::back(_traits);
        ice::pod::array::pop_back(_traits);

        ice::pod::hash::remove(
            _portals,
            ice::hash(name_hash)
        );

        _allocator.destroy(portal);
    }

    void IceshardWorld::activate(
        ice::Engine& engine,
        ice::EngineRunner& runner
    ) noexcept
    {
        for (auto& entry : _portals)
        {
            entry.value->trait()->on_activate(
                engine, runner, *entry.value
            );
        }
    }

    void IceshardWorld::deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner
    ) noexcept
    {
        for (auto& entry : _portals)
        {
            entry.value->trait()->on_deactivate(
                engine, runner, *entry.value
            );
        }
    }

    void IceshardWorld::update(
        ice::EngineRunner& runner
    ) noexcept
    {
        ice::EngineFrame& current_frame = runner.current_frame();

        ice::shards::inspect_each<ice::Entity>(current_frame.shards(), Shard_EntityDestroyed,
            [this](ice::Entity entity) noexcept
            {
                _entity_storage->erase_data(entity);
            }
        );

        for (auto& entry : _portals)
        {
            entry.value->remove_finished_tasks();
            entry.value->trait()->on_update(
                runner.current_frame(),
                runner,
                *entry.value
            );
        }
    }

    auto IceshardWorld::traits() noexcept -> ice::pod::Array<ice::WorldTrait*>&
    {
        return _traits;
    }

} // namespace ice
