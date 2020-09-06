#pragma once
#include <iceshard/math.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct ModelName
    {
        static constexpr auto identifier = "isc.model"_sid;

        core::stringid_type name;
    };

    struct Material
    {
        static constexpr auto identifier = "isc.material"_sid;

        core::stringid_type material_name;
    };

} // namespace iceshard::ecs
