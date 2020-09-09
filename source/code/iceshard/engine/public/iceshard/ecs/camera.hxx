#pragma once
#include <iceshard/math.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct Camera
    {
        static constexpr auto identifier = "isc.camera"_sid;

        ism::vec3f position;

        ism::vec3f front;

        ism::deg yaw;
        ism::deg pitch;
    };

    struct ProjectionPerspective
    {
        static constexpr auto identifier = "isc.projection.perspective"_sid;

        ism::deg fovy;
    };

    struct ProjectionOrtographic
    {
        static constexpr auto identifier = "isc.projection.ortographic"_sid;

        ism::vec2f left_right;
        ism::vec2f top_bottom;
        ism::vec2f near_far;
    };

} // namespace iceshard::ecs
