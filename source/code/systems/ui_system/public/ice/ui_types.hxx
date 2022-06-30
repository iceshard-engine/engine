#pragma once
#include <ice/base.hxx>

namespace ice::ui
{

    struct UIData;
    struct PageInfo;
    struct ElementInfo;
    struct ButtonInfo;

    struct Page;
    struct Element;
    struct Button;
    struct Action;

    struct ElementDrawData;

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

    constexpr auto rect_size(ice::ui::Rect rect) noexcept -> ice::ui::Size
    {
        return { rect.right - rect.left, rect.bottom - rect.top };
    }

    constexpr auto rect_position(ice::ui::Rect rect) noexcept -> ice::ui::Position
    {
        return { rect.left, rect.top };
    }

    constexpr auto operator-(ice::ui::RectOffset offset) noexcept -> ice::ui::RectOffset
    {
        return { -offset.left, -offset.top, -offset.right, -offset.bottom };
    }

    constexpr auto operator+(ice::ui::Rect rect, ice::ui::RectOffset offset) noexcept -> ice::ui::Rect
    {
        return { rect.left - offset.left, rect.top - offset.top, rect.right + offset.right, rect.bottom + offset.bottom };
    }

    constexpr auto operator-(ice::ui::Rect rect, ice::ui::RectOffset offset) noexcept -> ice::ui::Rect
    {
        return { rect.left + offset.left, rect.top + offset.top, rect.right - offset.right, rect.bottom - offset.bottom };
    }

    static constexpr Rect t{ 10, 10, 210, 40 };
    static constexpr RectOffset o1{ 2, 2, 2, 2 };

    static constexpr Rect r1 = t - o1;
    static constexpr Rect r2 = t + o1;

} // namespace ice::ui
