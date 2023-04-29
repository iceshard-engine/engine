/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_component.hxx>
#include <ice/ecs/ecs_entity.hxx>

namespace ice
{

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

    struct Transform2DDynamic
    {
        static constexpr ice::StringID Identifier = "ice.component.xform-2d-dynamic"_sid;

        ice::vec3f position{ 0.f };
        ice::vec2f scale{ 1.f };
        ice::rad rotation{ 0.f };
    };

} // namespace ice
