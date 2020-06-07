#pragma once
#include <iceshard/service_provider.hxx>
#include <core/allocator.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{

    class IceshardWorldServiceProvider : public iceshard::ServiceProvider
    {
    public:
        IceshardWorldServiceProvider(
            core::allocator& allocator,
            iceshard::ServiceProvider& engine_service_provider,
            core::pod::Hash<ComponentSystem*>& systems
        ) noexcept;

        ~IceshardWorldServiceProvider() noexcept override;

        //! \brief Engine entity manager.
        auto entity_manager() noexcept -> EntityManager* override;
        auto entity_manager() const noexcept -> const EntityManager* override;

        auto entity_index() noexcept -> EntityIndex* override;
        auto entity_index() const noexcept -> EntityIndex const* override;

        auto archetype_index() noexcept -> iceshard::ecs::ArchetypeIndex* override;
        auto archetype_index() const noexcept -> iceshard::ecs::ArchetypeIndex const* override;

        auto component_block_allocator() noexcept -> ComponentBlockAllocator* override;

        //! \brief Checks if the given component system exists.
        bool has_component_system(core::stringid_arg_type component_system_name) const noexcept override;

        //! \brief A component system with the given name.
        //!
        //! \remarks If the given component system does not exist the engine will abort the process.
        auto component_system(core::stringid_arg_type component_system_name) noexcept -> ComponentSystem* override;
        auto component_system(core::stringid_arg_type component_system_name) const noexcept -> const ComponentSystem* override;

        void add_system(
            core::stringid_arg_type name,
            ComponentSystem* system
        ) noexcept override;

        auto remove_system(
            core::stringid_arg_type name
        ) noexcept -> ComponentSystem* override;

    private:
        core::allocator& _allocator;

        iceshard::ServiceProvider& _engine_service_provider;

        core::pod::Hash<ComponentSystem*>& _world_component_systems;
    };

} // namespace iceshard
