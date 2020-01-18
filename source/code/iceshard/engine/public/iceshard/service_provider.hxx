#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>

namespace iceshard
{

    class EntityManager;

    class WorldManager;

    class ComponentSystem;

    //! \brief Special class that allows provides access to component managers.
    class ServiceProvider
    {
    public:
        virtual ~ServiceProvider() noexcept = default;

        //! \brief Engine entity manager.
        virtual auto entity_manager() noexcept -> EntityManager* = 0;
        virtual auto entity_manager() const noexcept -> const EntityManager* = 0;

        //! \brief Checks if the given component system exists.
        virtual bool has_component_system(core::stringid_arg_type component_system_name) const noexcept = 0;

        //! \brief A component system with the given name.
        //!
        //! \remarks If the given component system does not exist the engine will abort the process.
        virtual auto component_system(core::stringid_arg_type component_system_name) noexcept -> ComponentSystem* = 0;
        virtual auto component_system(core::stringid_arg_type component_system_name) const noexcept -> const ComponentSystem* = 0;
    };


} // namespace iceshard::component
