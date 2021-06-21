#pragma once
#include <ice/entity/entity_component.hxx>

namespace ice::phx
{

    struct Phx2dBodyDefinition
    {
        ice::StringID shape_name;
    };

    struct Phx2dBody
    {
        static constexpr ice::StringID Identifier = "ice.phx-2d.body"_sid;

        ice::phx::Phx2dBodyDefinition* const body_definition;
    };

    struct Phx2dVelocity
    {
        static constexpr ice::StringID Identifier = "ice.phx-2d.velocity"_sid;

        ice::vec2f velocity;
        ice::f32 angular_velocity;
    };

} // namespace ice::phx
