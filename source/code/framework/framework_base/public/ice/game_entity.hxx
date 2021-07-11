#pragma once
#include <ice/stringid.hxx>
#include <ice/entity/entity.hxx>

namespace ice
{

    struct Parent
    {
        static constexpr ice::StringID Identifier = "ice.component.parent"_sid;

        ice::Entity parent;
    };

    struct TransformStatic
    {
        static constexpr ice::StringID Identifier = "ice.component.xform-static"_sid;

        ice::vec3f position{ 0.f };
        ice::vec3f rotation{ 0.f };
        ice::vec3f scale{ 1.f };
    };

    struct TransformDynamic
    {
        static constexpr ice::StringID Identifier = "ice.component.xform-dynamic"_sid;

        ice::vec3f position{ 0.f };
        ice::vec3f rotation{ 0.f };
        ice::vec3f scale{ 1.f };
    };

    struct Transform2DStatic
    {
        static constexpr ice::StringID Identifier = "ice.component.xform-2d-static"_sid;

        ice::vec3f position{ 0.f };
        ice::vec2f scale{ 1.f };
        ice::rad rotation{ 0.f };
    };

} // namespace ice
