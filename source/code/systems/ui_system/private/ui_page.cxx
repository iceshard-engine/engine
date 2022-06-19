#include <ice/ui_page.hxx>

namespace ice::ui
{

    auto page_create(
        ice::Allocator& alloc,
        ice::ui::UIData const* uidata,
        ice::vec2f canvas_size
    ) noexcept -> ice::ui::Page*
    {
        return alloc.make<Page>();
    }

    auto page_destroy(
        ice::Allocator& alloc,
        ice::ui::Page* page
    ) noexcept
    {
        alloc.deallocate(page->elements.data());
        alloc.destroy(page);
    }

    void page_update_canvas(
        ice::ui::Page& page,
        ice::vec2f new_size
    ) noexcept
    {
    }

} // namespace ice::ui
