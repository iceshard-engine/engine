#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard
{


    //! \brief A regular interface for component systems.
    class ComponentSystem
    {
    public:
        virtual ~ComponentSystem() noexcept = default;

        //! \brief The name of the component system.
        virtual auto name() const noexcept -> core::cexpr::stringid_type = 0;
    };


} // namespace iceshard
