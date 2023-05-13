/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"

#include <ice/assert.hxx>
#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    namespace detail
    {

        void create_alloc_tree_widget(ice::devui::ImGuiSystem& sys, ice::Allocator& tracked_alloc) noexcept
        {
            if constexpr (ice::Allocator::HasDebugInformation)
            {
                ice::AllocatorDebugInfo const* top_alloc = &tracked_alloc.debug_info();
                while (top_alloc->parent_allocator() != nullptr)
                {
                    top_alloc = top_alloc->parent_allocator();
                }

                sys.set_alloc_tree_widget(tracked_alloc.create<ImGui_AllocatorTreeWidget>(*top_alloc));
            }
        }

        auto create_imgui_trait(
            void* userdata,
            ice::Allocator& alloc,
            ice::WorldTraitTracker const& tracker
        ) noexcept -> ice::WorldTrait*
        {
            ice::devui::ImGuiSystem* system = reinterpret_cast<ImGuiSystem*>(userdata);
            ice::devui::ImGuiTrait* trait = alloc.create<ice::devui::ImGuiTrait>(alloc);
            system->set_trait(trait);
            return trait;
        }

    } // namespace detail

    ImGuiSystem::ImGuiSystem(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _execution_key{ }
        , _render_trait{ nullptr }
        , _widget_alloc_tree{ nullptr }
        , _widgets{ alloc }
        , _inactive_widgets{ alloc }
    {
        ice::array::reserve(_widgets, 100);
        detail::create_alloc_tree_widget(*this, _allocator);
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
        for (WidgetRuntimeInfo& info : _widgets)
        {
            _allocator.destroy(info.state);
        }

        _allocator.destroy(_widget_alloc_tree);
    }

    void ImGuiSystem::set_trait(ice::devui::ImGuiTrait* trait) noexcept
    {
        ICE_ASSERT(
            _render_trait == nullptr
            || trait == nullptr,
            "Setting ImGui trait failed!"
        );
        _render_trait = trait;

        if (_render_trait != nullptr)
        {
            for (ice::devui::DevUIWidget* widget : _inactive_widgets)
            {
                ice::array::push_back(_widgets, { .widget = widget, .state = _allocator.create<WidgetState>() });
            }
            ice::array::clear(_inactive_widgets);

            for (WidgetRuntimeInfo& info : _widgets)
            {
                info.widget->on_prepare(_render_trait->imgui_context(), *info.state);
            }
        }
    }

    void ImGuiSystem::register_trait(
        ice::WorldTraitArchive& archive
    ) noexcept
    {
        archive.register_trait(
            ice::Constant_TraitName_DevUI,
            ice::WorldTraitDescription
            {
                .factory = ice::devui::detail::create_imgui_trait,
                .factory_userdata = this,
            }
        );
    }

    void ImGuiSystem::register_widget(ice::devui::DevUIWidget* widget) noexcept
    {
        if (_render_trait != nullptr)
        {
            WidgetState* state = _allocator.create<WidgetState>();
            widget->on_prepare(_render_trait->imgui_context(), *state);
            ice::array::push_back(_widgets, { .widget = widget, .state = state });
        }
        else
        {
            ice::array::push_back(_inactive_widgets, widget);
        }
    }

    void ImGuiSystem::unregister_widget(ice::devui::DevUIWidget* widget) noexcept
    {
        if (_render_trait == nullptr)
        {
            return;
        }

        ice::u32 const count = ice::array::count(_widgets);
        if (count == 0)
        {
            return;
        }

        ice::u32 idx = 0;
        for (; idx < count; ++idx)
        {
            if (_widgets[idx].widget == widget)
            {
                break;
            }
        }

        WidgetState* state = _widgets[idx].state;
        if (idx < (count - 1))
        {
            _widgets[idx] = _widgets[count - 1];
        }
        _allocator.destroy(state);

        ice::array::pop_back(_widgets);
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
        ICE_ASSERT(
            _execution_key == execution_key,
            "Method 'internal_build_widgets' was executed from an invalid context!"
        );

        if (_render_trait == nullptr)
        {
            return;
        }

        ice::String categories[]{
            "Tools",
            "Uncategorized"
        };

        static bool show_demo = false;

        if (_render_trait->start_frame())
        {
            if (ImGui::BeginMainMenuBar())
            {
                for (ice::String category : categories)
                {
                    if (ImGui::BeginMenu(ice::string::begin(category)))
                    {
                        for (WidgetRuntimeInfo& info : _widgets)
                        {
                            WidgetState& state = *info.state;
                            WidgetSettings const& settings = info.widget->settings();

                            if (settings.menu_category == category && ice::string::any(settings.menu_text))
                            {
                                ImGui::MenuItem(ice::string::begin(settings.menu_text), nullptr, &state.is_visible);
                            }
                        }

                        // Special case for ImGui Demo
                        if (category == "Uncategorized")
                        {
                            ImGui::MenuItem("ImGui Demo Window", nullptr, &show_demo);
                        }
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMainMenuBar();

            if (show_demo)
            {
                ImGui::ShowDemoWindow(&show_demo);
            }

            for (WidgetRuntimeInfo& info : _widgets)
            {
                if (info.state->is_visible)
                {
                    info.widget->on_draw();
                }
            }
        }
        _render_trait->end_frame(frame);
    }

} // namespace ice::devui
