/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice
{

    static constexpr ice::String ArchetypeName_OrtographicCamera = "ice/archetype/ortographic-camera";
    static constexpr ice::String ArchetypeName_PerspectiveCamera = "ice/archetype/perspective-camera";

    static constexpr ice::StringID TraitID_CameraManager = "iceshard/trait/camera-manager"_sid;

    struct Camera
    {
        static constexpr ice::StringID Identifier = "ice.component.camera-lookat"_sid;

        ice::StringID name;
        ice::vec3f position;
        ice::vec3f front;
    };

    struct CameraOrtho
    {
        static constexpr ice::StringID Identifier = "ice.component.camera-ortographic"_sid;

        ice::vec2f left_right;
        ice::vec2f bottom_top;
        ice::vec2f near_far;
    };

    struct CameraPerspective
    {
        static constexpr ice::StringID Identifier = "ice.component.camera-perspective"_sid;

        ice::rad field_of_view;
        ice::f32 aspect_ration;
        ice::vec2f near_far;
    };

    struct CameraData
    {
        ice::mat4x4 view;
        ice::mat4x4 projection;
    };

} // namespace ice
