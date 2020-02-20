#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>

namespace iceshard
{

    class EntityManager;

    class EntityIndex;

    //! \brief Special interface for providing entity related services.
    class EntityServiceProvider
    {
    public:
        virtual ~EntityServiceProvider() noexcept = default;

        virtual auto entity_manager() noexcept -> EntityManager* = 0;
        virtual auto entity_manager() const noexcept -> EntityManager const* = 0;

        virtual auto entity_index() noexcept -> EntityIndex* = 0;
        virtual auto entity_index() const noexcept -> EntityIndex const* = 0;
    };

    class ComponentSystem;

    class ComponentBlockAllocator;

    //! \brief Special interface for providing component related services.
    class ComponentServiceProvider
    {
    public:
        virtual ~ComponentServiceProvider() noexcept = default;

        virtual auto component_block_allocator() noexcept -> ComponentBlockAllocator* = 0;

        //! \brief Checks if the given component system exists.
        virtual bool has_component_system(core::stringid_arg_type component_system_name) const noexcept = 0;

        //! \brief A component system with the given name.
        //!
        //! \remarks If the given component system does not exist the engine will abort the process.
        virtual auto component_system(core::stringid_arg_type component_system_name) noexcept -> ComponentSystem* = 0;
        virtual auto component_system(core::stringid_arg_type component_system_name) const noexcept -> ComponentSystem const* = 0;
    };

    //! \brief Special class that allows provides access to component managers.
    class ServiceProvider : public EntityServiceProvider, public ComponentServiceProvider
    {
    public:
        virtual ~ServiceProvider() noexcept = default;
    };


} // namespace iceshard::component
