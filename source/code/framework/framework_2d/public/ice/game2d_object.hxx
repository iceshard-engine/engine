#pragma once
#include <ice/render/render_image.hxx>
#include <ice/entity/entity_component.hxx>

namespace ice
{

    struct Obj2dShapeDefinition
    {
        ice::StringID shape_name;
        ice::vec2f const* vertices;
        ice::vec2f const* uvs;
        ice::u32 vertex_count;
    };

    struct Obj2dShape
    {
        static constexpr ice::StringID Identifier = "ice.object-2d.shape"_sid;

        ice::Obj2dShapeDefinition const* shape_definition;
    };

    struct Obj2dMaterial
    {
        static constexpr ice::StringID Identifier = "ice.object-2d.material"_sid;

        ice::StringID name;
        ice::i32 texture_index;
        ice::render::Image texture_image;
    };

    struct Obj2dTransform
    {
        static constexpr ice::StringID Identifier = "ice.object-2d.transform"_sid;

        ice::vec3f position;
        ice::f32 rotation;
    };

    using Obj2dXform = Obj2dTransform;

} // namespace ice
