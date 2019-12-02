#pragma once
#include <iceshard/service_provider.hxx>
#include <iceshard/engine.hxx>

#include <core/allocator.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{


    //! \brief Service provider for the Engine instance.
    class IceshardServiceProvider : public iceshard::ServiceProvider
    {
    public:
        IceshardServiceProvider(core::allocator& alloc, iceshard::EntityManager* entity_manager_ptr) noexcept;
        ~IceshardServiceProvider() noexcept override = default;

        //! \brief Engine entity manager.
        auto entity_manager() noexcept -> EntityManager* override;
        auto entity_manager() const noexcept -> const EntityManager* override;

        //! \brief Checks if the given component system exists.
        bool has_component_system(core::cexpr::stringid_argument_type component_system_name) const noexcept override;

        //! \brief A component system with the given name.
        //!
        //! \remarks If the given component system does not exist the engine will abort the process.
        auto component_system(core::cexpr::stringid_argument_type component_system_name) noexcept -> ComponentSystem* override;
        auto component_system(core::cexpr::stringid_argument_type component_system_name) const noexcept -> const ComponentSystem* override;

    private:
        iceshard::EntityManager* const _entity_manager;

        core::pod::Hash<ComponentSystem*> _world_component_systems;
    };


} // namespace iceshard