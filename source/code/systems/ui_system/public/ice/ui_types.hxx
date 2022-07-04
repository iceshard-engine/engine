#pragma once
#include <ice/base.hxx>

namespace ice::ui
{

    struct PageInfo;
    struct UIResourceData;

    struct PageInfo;
    struct FontInfo;
    struct ElementInfo;
    struct LayoutInfo;
    struct ButtonInfo;
    struct LabelInfo;
    struct ConstantInfo;
    struct ResourceInfo;
    struct ActionInfo;
    struct ShardInfo;

    struct Page;
    struct Element;
    struct Layout;
    struct Button;
    struct Label;

    enum class ResourceType : ice::u32;
    enum class ElementFlags : ice::u32;
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

} // namespace ice::ui
