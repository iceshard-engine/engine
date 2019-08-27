#include "iceshard_world_manager.hxx"
#include <core/pod/hash.hxx>

namespace iceshard::world
{

    IceshardWorldManager::IceshardWorldManager(
        core::allocator& alloc,
        iceshard::entity::EntityManager* entity_manager
    ) noexcept
        : WorldManager{ }
        , _allocator{ alloc }
        , _entity_manager{ entity_manager }
        , _worlds{ _allocator }
    {
        core::pod::hash::reserve(_worlds, 5);
    }

    auto IceshardWorldManager::get_world(core::cexpr::stringid_argument_type world_name) noexcept -> World*
    {
        return core::pod::hash::get<IceshardWorld*>(_worlds, static_cast<uint64_t>(world_name.hash_value), nullptr);
    }

    auto IceshardWorldManager::create_world(core::cexpr::stringid_argument_type world_name) noexcept -> World*
    {
        const auto world_hash_value = static_cast<uint64_t>(world_name.hash_value);
        IS_ASSERT(core::pod::hash::has(_worlds, world_hash_value) == false, "World with the given name already exist! [ name: {} ]", world_name);

        // Creates a new world object and saves it in the map
        auto* world_instance = _allocator.make<IceshardWorld>(_allocator, world_name, _entity_manager->create(this));
        core::pod::hash::set(_worlds, world_hash_value, world_instance);

        return world_instance;
    }

    void IceshardWorldManager::destroy_world(core::cexpr::stringid_argument_type world_name) noexcept
    {
        const auto world_hash_value = static_cast<uint64_t>(world_name.hash_value);
        IS_ASSERT(core::pod::hash::has(_worlds, world_hash_value) == true, "World with the given name does not exist! [ name: {} ]", world_name);

        // Get the world instance and remove it from the map
        auto* world_instance = core::pod::hash::get<IceshardWorld*>(_worlds, world_hash_value, nullptr);
        core::pod::hash::remove(_worlds, world_hash_value);

        // Destroy the entity object
        _entity_manager->destroy(world_instance->entity());

        // Destroys the world object
        _allocator.destroy(world_instance);
    }

} // namespace iceshard::world
