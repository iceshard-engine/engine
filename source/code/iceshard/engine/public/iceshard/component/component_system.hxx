#pragma once
#include <core/cexpr/stringid.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_block.hxx>

namespace iceshard
{

    //! \brief A regular interface for component systems.
    class ComponentSystem
    {
    public:
        virtual ~ComponentSystem() noexcept = default;

        //! \brief The name of the component system.
        virtual auto name() const noexcept -> core::stringid_type = 0;

        //! \brief Creates a new component instance for the given entity and name.
        virtual void create(Entity entity, core::stringid_arg_type name) noexcept = 0;

        //! \brief Returns the component instance id and index for the given entity and component name.
        virtual auto lookup(Entity entity, core::stringid_arg_type name) const noexcept -> ComponentInstance = 0;

        //! \brief Creates a new component instance for the given entity and name.
        virtual void remove(Entity entity, core::stringid_arg_type name) noexcept = 0;
    };

} // namespace iceshard
