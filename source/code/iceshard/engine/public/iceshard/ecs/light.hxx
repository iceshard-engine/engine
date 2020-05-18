#pragma once
#include <iceshard/math.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct Light
    {
        static constexpr auto identifier = "isc.light"_sid;

        //! \brief The lights position.
        ism::vec3f position;
    };

} // namespace iceshard::ecs
