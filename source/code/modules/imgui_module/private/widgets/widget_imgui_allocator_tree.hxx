#pragma once
#include <ice/allocator.hxx>
#include <ice/devui/devui_widget.hxx>

namespace ice::devui
{

    class ImGui_AllocatorTreeWidget : public ice::devui::DevUIWidget
    {
    public:
        ImGui_AllocatorTreeWidget(ice::TrackedAllocator const& alloc) noexcept;
        ~ImGui_AllocatorTreeWidget() noexcept override = default;

        void on_draw() noexcept override;

    private:
        ice::TrackedAllocator const& _root_tracked_allocator;
        bool _open = true;
    };

} // namespace ice::devui
