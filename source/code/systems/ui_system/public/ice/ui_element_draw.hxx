#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    struct DrawData
    {
        ice::f32* vertices;
        ice::u32 vertices_count;
        ice::f32* uvs;
        ice::u32 uvs_count;
    };

} // namespace ice::ui
