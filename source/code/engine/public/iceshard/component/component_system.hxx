#pragma once
#include <core/cexpr/stringid.hxx>
#include <iceshard/entity/entity.hxx>

namespace iceshard
{


    //! \brief A regular interface for component systems.
    class ComponentSystem
    {
    public:
        virtual ~ComponentSystem() noexcept = default;

        //! \brief The name of the component system.
        virtual auto name() const noexcept -> core::cexpr::stringid_type = 0;

        //! \brief Creates a new component instance for the given entity and name.
        virtual void create(entity_handle_type entity, core::cexpr::stringid_argument_type name) noexcept = 0;

        //! \brief Creates a new component instance for the given entity and name.
        virtual void remove(entity_handle_type entity, core::cexpr::stringid_argument_type name) noexcept = 0;
    };


} // namespace iceshard
