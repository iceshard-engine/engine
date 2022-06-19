#pragma once
#include <ice/ui_types.hxx>

namespace ice::ui
{

    struct DrawData
    {
        ice::f32* vertices;
        ice::f32* colors;
        ice::f32* uvs;

        ice::u32 vertice_count;
    };

} // namespace ice::ui
