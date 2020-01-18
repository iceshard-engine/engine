#include "iceshard_world_service_provider.hxx"

namespace iceshard
{

    IceshardWorldServiceProvider::IceshardWorldServiceProvider(
        core::allocator& allocator,
        iceshard::ServiceProvider* engine_service_provider
    ) noexcept
        : ServiceProvider{ }
        , _engine_service_provider{ engine_service_provider }
        , _world_component_systems{ allocator }
    { }

    auto IceshardWorldServiceProvider::entity_manager() noexcept -> EntityManager*
    {
        return _engine_service_provider->entity_manager();
    }

    auto IceshardWorldServiceProvider::entity_manager() const noexcept -> const EntityManager*
    {
        return _engine_service_provider->entity_manager();
    }

    bool IceshardWorldServiceProvider::has_component_system(
        core::stringid_arg_type component_system_name
    ) const noexcept
    {
        return core::pod::hash::has(_world_component_systems, static_cast<uint64_t>(component_system_name.hash_value))
            || _engine_service_provider->has_component_system(component_system_name);
    }

    auto IceshardWorldServiceProvider::component_system(
        core::stringid_arg_type component_system_name
    ) noexcept -> ComponentSystem*
    {
        const auto component_system_hash = static_cast<uint64_t>(component_system_name.hash_value);

        IS_ASSERT(
            has_component_system(component_system_name)
            , "Unrecognized component system name: {}!"
            , component_system_name
        );

        auto* component_system_ptr = core::pod::hash::get<ComponentSystem*>(_world_component_systems, component_system_hash, nullptr);
        if (component_system_ptr == nullptr)
        {
            component_system_ptr = _engine_service_provider->component_system(component_system_name);
        }

        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);
        return component_system_ptr;
    }

    auto IceshardWorldServiceProvider::component_system(
        core::stringid_arg_type component_system_name
    ) const noexcept -> const ComponentSystem*
    {
        const auto component_system_hash = static_cast<uint64_t>(component_system_name.hash_value);

        IS_ASSERT(
            has_component_system(component_system_name)
            , "Unrecognized component system name: {}!"
            , component_system_name
        );

        const auto* component_system_ptr = core::pod::hash::get<ComponentSystem*>(_world_component_systems, component_system_hash, nullptr);
        if (component_system_ptr == nullptr)
        {
            component_system_ptr = _engine_service_provider->component_system(component_system_name);
        }

        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);
        return component_system_ptr;
    }

} // namespace iceshard
