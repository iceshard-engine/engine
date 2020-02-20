#include "iceshard_service_provider.hxx"

namespace iceshard
{

    IceshardServiceProvider::IceshardServiceProvider(core::allocator& alloc, iceshard::EntityManager* entity_manager_ptr) noexcept
        : ServiceProvider{ }
        , _entity_manager{ entity_manager_ptr }
        , _world_component_systems{ alloc }
    { }

    auto IceshardServiceProvider::entity_manager() noexcept -> EntityManager*
    {
        return _entity_manager;
    }

    auto IceshardServiceProvider::entity_manager() const noexcept -> const EntityManager*
    {
        return _entity_manager;
    }

    auto IceshardServiceProvider::entity_index() noexcept -> EntityIndex*
    {
        return nullptr;
    }

    auto IceshardServiceProvider::entity_index() const noexcept -> EntityIndex const*
    {
        return nullptr;
    }

    auto IceshardServiceProvider::component_block_allocator() noexcept -> ComponentBlockAllocator*
    {
        return nullptr;
    }

    bool IceshardServiceProvider::has_component_system(core::stringid_arg_type component_system_name) const noexcept
    {
        return core::pod::hash::has(_world_component_systems, static_cast<uint64_t>(component_system_name.hash_value));
    }

    auto IceshardServiceProvider::component_system(core::stringid_arg_type component_system_name) noexcept -> ComponentSystem*
    {
        const auto component_system_hash = static_cast<uint64_t>(component_system_name.hash_value);

        IS_ASSERT(
            has_component_system(component_system_name)
            , "Unrecognized component system name: {}!"
            , component_system_name
        );

        auto* component_system_ptr = core::pod::hash::get<ComponentSystem*>(_world_component_systems, component_system_hash, nullptr);
        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);

        return component_system_ptr;
    }

    auto IceshardServiceProvider::component_system(core::stringid_arg_type component_system_name) const noexcept -> const ComponentSystem*
    {
        const auto component_system_hash = static_cast<uint64_t>(component_system_name.hash_value);

        IS_ASSERT(
            has_component_system(component_system_name)
            , "Unrecognized component system name: {}!"
            , component_system_name
        );

        const auto* component_system_ptr = core::pod::hash::get<ComponentSystem*>(_world_component_systems, component_system_hash, nullptr);
        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);

        return component_system_ptr;
    }

} // namespace iceshard
