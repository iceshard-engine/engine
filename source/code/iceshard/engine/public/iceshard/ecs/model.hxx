#pragma once
#include <core/math/vector.hxx>
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

        core::math::vec4 color;
    };

} // namespace iceshard::ecs
