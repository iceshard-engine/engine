#include "iceshard_world_service_provider.hxx"
#include <iceshard/component/component_system.hxx>

namespace iceshard
{

    IceshardWorldServiceProvider::IceshardWorldServiceProvider(
        core::allocator& allocator,
        iceshard::ServiceProvider& engine_service_provider,
        core::pod::Hash<ComponentSystem*>& systems
    ) noexcept
        : ServiceProvider{ }
        , _allocator{ allocator }
        , _engine_service_provider{ engine_service_provider }
        , _world_component_systems{ systems }
    { }

    IceshardWorldServiceProvider::~IceshardWorldServiceProvider() noexcept
    {
    }

    auto IceshardWorldServiceProvider::entity_manager() noexcept -> EntityManager*
    {
        return _engine_service_provider.entity_manager();
    }

    auto IceshardWorldServiceProvider::entity_manager() const noexcept -> const EntityManager*
    {
        return _engine_service_provider.entity_manager();
    }

    auto IceshardWorldServiceProvider::entity_index() noexcept -> EntityIndex*
    {
        return nullptr;
    }

    auto IceshardWorldServiceProvider::entity_index() const noexcept -> EntityIndex const*
    {
        return nullptr;
    }

    auto IceshardWorldServiceProvider::archetype_index() noexcept -> iceshard::ecs::ArchetypeIndex*
    {
        return _engine_service_provider.archetype_index();
    }

    auto IceshardWorldServiceProvider::archetype_index() const noexcept -> iceshard::ecs::ArchetypeIndex const*
    {
        return _engine_service_provider.archetype_index();
    }

    auto IceshardWorldServiceProvider::component_block_allocator() noexcept -> ComponentBlockAllocator*
    {
        return _engine_service_provider.component_block_allocator();
    }

    bool IceshardWorldServiceProvider::has_component_system(
        core::stringid_arg_type component_system_name
    ) const noexcept
    {
        return core::pod::hash::has(_world_component_systems, static_cast<uint64_t>(component_system_name.hash_value))
            || _engine_service_provider.has_component_system(component_system_name);
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
            component_system_ptr = _engine_service_provider.component_system(component_system_name);
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
            component_system_ptr = _engine_service_provider.component_system(component_system_name);
        }

        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);
        return component_system_ptr;
    }

    void IceshardWorldServiceProvider::add_system(core::stringid_arg_type name, ComponentSystem* system) noexcept
    {
        auto name_hash = core::hash(name);
        IS_ASSERT(
            core::pod::hash::has(_world_component_systems, name_hash) == false,
            "Component system with this name `{}` already exists",
            name
        );

        core::pod::hash::set(_world_component_systems, name_hash, system);
    }

    auto IceshardWorldServiceProvider::remove_system(core::stringid_arg_type name) noexcept -> ComponentSystem*
    {
        ComponentSystem* result = nullptr;
        if (auto name_hash = core::hash(name); core::pod::hash::has(_world_component_systems, name_hash))
        {
            result = core::pod::hash::get(_world_component_systems, name_hash, nullptr);
            core::pod::hash::remove(_world_component_systems, name_hash);
        }
        return result;
    }

} // namespace iceshard
