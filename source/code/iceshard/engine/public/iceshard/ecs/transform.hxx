#pragma once
#include <core/math/matrix.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct Transform
    {
        static constexpr auto identifier = "isc.transform"_sid;

        //! \brief The mesh name
        core::math::mat4x4 xform;
    };

} // namespace iceshard::ecs
