/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"
#include "widgets/imgui_allocator_tree.hxx"

#include <ice/assert.hxx>
#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    void ImGuiWidgetFrame::mainmenu(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept
    {
        ImGui::MenuItem(ice::string::begin(widget.name), nullptr, &state.active);
    }

    bool ImGuiWidgetFrame::begin(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept
    {
        return ImGui::Begin(ice::string::begin(widget.name), &state.active);
    }

    void ImGuiWidgetFrame::end() noexcept
    {
        ImGui::End();
    }

    ImGuiSystem::ImGuiSystem(ice::Allocator& alloc) noexcept
        : _allocator{ alloc, "ImGUI-System" }
        , _builtin_widgets{ alloc }
        , _widgets{ alloc }
    {
        ice::array::reserve(_widgets, 100);
        ice::array::push_back(_builtin_widgets, create_allocator_tree_widget(_allocator));

        // Register all built-in's
        for (ice::UniquePtr<ice::DevUIWidget> const& widget : _builtin_widgets)
        {
            register_widget(widget.get());
        }
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
    }

    void ImGuiSystem::register_widget(ice::DevUIWidget* widget) noexcept
    {
        ice::array::push_back(_widgets, { .widget = widget });
    }

    void ImGuiSystem::unregister_widget(ice::DevUIWidget* widget) noexcept
    {
        // TODO: ice::array::remove_at

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

        _widgets[idx] = _widgets[count - 1];
        ice::array::pop_back(_widgets);
    }

    void ImGuiSystem::update_widgets() noexcept
    {
        ImGuiIO const& io = ImGui::GetIO();

        // If display size is not set we return quickly
        if (io.DisplaySize.x <= 0.f || io.DisplaySize.y <= 0.f)
        {
            return;
        }

        ice::String categories[]{
            "File",
            "Settings",
            "Tools",
            "Test",
            "ImGui"
        };

        static bool show_demo = false;

        ImGui::NewFrame();
        {
            if (ImGui::BeginMainMenuBar())
            {
                for (ice::String category : categories)
                {
                    if (ImGui::BeginMenu(ice::string::begin(category)))
                    {
                        for (WidgetRuntimeInfo& runtime : _widgets)
                        {
                            ice::DevUIWidgetInfo const& info = runtime.widget->info;
                            if (category == info.category && runtime.widget->build_mainmenu())
                            {
                                _widget_frame.mainmenu(info, runtime);
                            }
                        }

                        // Special case for ImGui Demo
                        if (category == "ImGui")
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

            for (WidgetRuntimeInfo& runtime : _widgets)
            {
                if (runtime.active)
                {
                    runtime.widget->build_widget(_widget_frame, runtime);
                }
            }
        }

        ImGui::EndFrame();
    }

} // namespace ice::devui
