#include "imgui_system.hxx"

#include <ice/assert.hxx>
#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    namespace detail
    {

        void create_alloc_tree_widget(ice::devui::ImGuiSystem& /*sys*/, ice::BaseAllocator& /*base_alloc*/) noexcept
        {
        }

        void create_alloc_tree_widget(ice::devui::ImGuiSystem& sys, ice::TrackedAllocator& tracked_alloc) noexcept
        {
            ice::TrackedAllocator const* top_alloc = &tracked_alloc;
            while (top_alloc->parent_allocator() != nullptr)
            {
                top_alloc = top_alloc->parent_allocator();
            }

            sys.set_alloc_tree_widget(tracked_alloc.make<ImGui_AllocatorTreeWidget>(*top_alloc));
        }

    } // namespace detail

    ImGuiSystem::ImGuiSystem(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _render_trait{ alloc }
        , _widgets{ alloc }
        , _widget_alloc_tree{ nullptr }
    {
        ice::pod::array::reserve(_widgets, 100);
        detail::create_alloc_tree_widget(*this, _allocator);
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
        _allocator.destroy(_widget_alloc_tree);
    }

    auto ImGuiSystem::world_trait() noexcept -> ice::devui::DevUITrait*
    {
        return &_render_trait;
    }

    void ImGuiSystem::register_widget(ice::devui::DevUIWidget* widget) noexcept
    {
        widget->on_prepare(_render_trait.imgui_context());
        ice::pod::array::push_back(_widgets, widget);
    }

    void ImGuiSystem::unregister_widget(ice::devui::DevUIWidget* widget) noexcept
    {
        ice::u32 const count = ice::pod::array::size(_widgets);
        if (count == 0)
        {
            return;
        }

        ice::u32 idx = 0;
        for (; idx < count; ++idx)
        {
            if (_widgets[idx] == widget)
            {
                break;
            }
        }

        if (idx < (count - 1))
        {
            _widgets[idx] = _widgets[count - 1];
        }

        ice::pod::array::pop_back(_widgets);
    }

    void ImGuiSystem::set_alloc_tree_widget(ice::devui::ImGui_AllocatorTreeWidget* widget) noexcept
    {
        _widget_alloc_tree = widget;
        register_widget(_widget_alloc_tree);
    }

    void ImGuiSystem::internal_set_key(ice::devui::DevUIExecutionKey new_execution_key) noexcept
    {
        _execution_key = new_execution_key;
    }

    void ImGuiSystem::internal_build_widgets(
        ice::EngineFrame& frame,
        ice::devui::DevUIExecutionKey execution_key
    ) noexcept
    {
        if (_render_trait.start_frame())
        {
            ImGui::ShowDemoWindow();

            for (ice::devui::DevUIWidget* widget : _widgets)
            {
                widget->on_draw();
            }
        }
        _render_trait.end_frame(frame);
        ICE_ASSERT(
            _execution_key == execution_key,
            "Method 'internal_build_widgets' was executed from an invalid context!"
        );
    }

} // namespace ice::devui
