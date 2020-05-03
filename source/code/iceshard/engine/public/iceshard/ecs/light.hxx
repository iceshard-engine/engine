#pragma once
#include <core/math/vector.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct Light
    {
        static constexpr auto identifier = "isc.light"_sid;

        //! \brief The lights position.
        core::math::vec3 position;
    };

} // namespace iceshard::ecs
