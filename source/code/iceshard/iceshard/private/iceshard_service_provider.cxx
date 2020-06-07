#include "iceshard_service_provider.hxx"
#include <core/pod/hash.hxx>
#include <iceshard/component/component_archetype_index.hxx>

namespace iceshard
{

    IceshardServiceProvider::IceshardServiceProvider(
        core::allocator& alloc,
        iceshard::EntityManager* entity_manager_ptr
    ) noexcept
        : ServiceProvider{ }
        , _allocator{ alloc }
        , _entity_manager{ entity_manager_ptr }
        , _component_block_allocator{ alloc }
        , _component_systems{ _allocator }
        , _archetype_index{ iceshard::ecs::create_default_index(alloc, &_component_block_allocator) }
    {
    }

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

    auto IceshardServiceProvider::archetype_index() noexcept -> iceshard::ecs::ArchetypeIndex*
    {
        return _archetype_index.get();
    }

    auto IceshardServiceProvider::archetype_index() const noexcept -> iceshard::ecs::ArchetypeIndex const*
    {
        return _archetype_index.get();
    }

    auto IceshardServiceProvider::component_block_allocator() noexcept -> ComponentBlockAllocator*
    {
        return &_component_block_allocator;
    }

    bool IceshardServiceProvider::has_component_system(core::stringid_arg_type component_system_name) const noexcept
    {
        return core::pod::hash::has(
            _component_systems,
            core::hash(component_system_name)
        );
    }

    auto IceshardServiceProvider::component_system(core::stringid_arg_type component_system_name) noexcept -> ComponentSystem*
    {
        const auto component_system_hash = core::hash(component_system_name);

        IS_ASSERT(
            has_component_system(component_system_name)
            , "Unrecognized component system name: {}!"
            , component_system_name
        );

        auto* component_system_ptr = core::pod::hash::get<ComponentSystem*>(
            _component_systems,
            component_system_hash,
            nullptr
        );
        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);

        return component_system_ptr;
    }

    auto IceshardServiceProvider::component_system(core::stringid_arg_type component_system_name) const noexcept -> const ComponentSystem*
    {
        const auto component_system_hash = core::hash(component_system_name);

        IS_ASSERT(
            has_component_system(component_system_name)
            , "Unrecognized component system name: {}!"
            , component_system_name
        );

        const auto* component_system_ptr = core::pod::hash::get<ComponentSystem*>(
            _component_systems,
            component_system_hash,
            nullptr
        );
        IS_ASSERT(component_system_ptr != nullptr, "Invalid component system pointer for name {}!", component_system_name);

        return component_system_ptr;
    }

    void IceshardServiceProvider::add_system(core::stringid_arg_type name, ComponentSystem* system) noexcept
    {
        auto name_hash = core::hash(name);
        IS_ASSERT(
            core::pod::hash::has(_component_systems, name_hash) == false,
            "Component system with this name `{}` already exists",
            name
        );

        core::pod::hash::set(_component_systems, name_hash, system);
    }

    auto IceshardServiceProvider::remove_system(core::stringid_arg_type name) noexcept -> ComponentSystem*
    {
        ComponentSystem* result = nullptr;
        if (auto name_hash = core::hash(name); core::pod::hash::has(_component_systems, name_hash))
        {
            result = core::pod::hash::get(_component_systems, name_hash, nullptr);
            core::pod::hash::remove(_component_systems, name_hash);
        }
        return result;
    }

} // namespace iceshard
