#pragma once
#include <ice/ui_types.hxx>
#include <ice/span.hxx>
#include <ice/math.hxx>

namespace ice::ui
{

    struct DrawData
    {
        ice::Span<ice::vec2f> vertices;
        ice::Span<ice::vec4f> colors;

        ice::u32 vertice_count;
    };

} // namespace ice::ui
