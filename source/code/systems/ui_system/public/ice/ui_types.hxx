#pragma once
#include <ice/base.hxx>

namespace ice::ui
{

    struct ElementInfo;
    struct ElementDrawData;
    struct ButtonInfo;

    enum class ElementFlags : ice::u16;
    enum class ElementType : ice::u8;

    struct Size
    {
        ice::f32 width;
        ice::f32 height;
    };

    struct Position
    {
        ice::f32 x;
        ice::f32 y;
    };

    struct Rect
    {
        ice::f32 left;
        ice::f32 top;
        ice::f32 right;
        ice::f32 bottom;
    };

    struct RectOffset
    {
        ice::f32 left;
        ice::f32 top;
        ice::f32 right;
        ice::f32 bottom;
    };

} // namespace ice::ui
