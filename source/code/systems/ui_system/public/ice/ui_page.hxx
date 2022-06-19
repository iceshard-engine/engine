#pragma once
#include <ice/ui_types.hxx>
#include <ice/allocator.hxx>
#include <ice/span.hxx>

namespace ice::ui
{

    struct PageInfo
    {
        ice::ui::UIData const* uidata;
    };

    struct Page
    {
        //ice::ui::PageInfo page_info;

        //ice::vec4f orientation;
        ice::vec2f canvas;
        ice::vec3f position;
        ice::vec2f size;

        ice::Span<ice::ui::Element> elements;
    };

    auto page_create(
        ice::Allocator& alloc,
        ice::ui::UIData const* uidata,
        ice::vec2f canvas_size
    ) noexcept -> ice::ui::Page*;

    auto page_destroy(
        ice::Allocator& alloc,
        ice::ui::Page* page
    ) noexcept;

    void page_update_canvas(
        ice::ui::Page& page,
        ice::vec2f new_size
    ) noexcept;

    //struct Page
    //{
    //    ice::ui::ISUIData const* definition;
    //    //ice::ui::Size resolution;
    //    ice::ui::Size size; // resolution

    //    ice::ui::Element* elements;
    //    ice::u32 elements_count;
    //};

} // namespace ice::ui
