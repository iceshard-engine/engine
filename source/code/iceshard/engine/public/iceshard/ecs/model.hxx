#pragma once
#include <iceshard/math.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct ModelName
    {
        static constexpr auto identifier = "isc.model"_sid;

        //! \brief The mesh name
        core::stringid_type name;
    };

    struct ModelMaterial
    {
        static constexpr auto identifier = "isc.model-color"_sid;

        ism::vec4f color;
    };

} // namespace iceshard::ecs
