/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
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
        , _widget_manager{ _allocator }
    {
        ice::array::push_back(_builtin_widgets, create_allocator_tree_widget(_allocator));

        // Register all built-in's
        _widget_manager.add_widget(&_widget_manager); // Add self...
        for (ice::UniquePtr<ice::DevUIWidget> const& widget : _builtin_widgets)
        {
            _widget_manager.add_widget(widget.get());
        }
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
    }

    void ImGuiSystem::register_widget(ice::DevUIWidget* widget) noexcept
    {
        _widget_manager.add_widget(widget);
    }

    void ImGuiSystem::unregister_widget(ice::DevUIWidget* widget) noexcept
    {
        _widget_manager.remove_widget(widget);
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
            "Help"
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
                        for (ImGuiDevUIWidget& runtime : _widget_manager.widgets())
                        {
                            ice::DevUIWidgetInfo const& info = runtime.widget->info;
                            if (ice::string::starts_with(info.category, category) && runtime.widget->build_mainmenu(runtime))
                            {
                                _widget_frame.mainmenu(info, runtime);
                            }
                        }

                        // Special case for ImGui Demo
                        if (category == "Help")
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

            for (ImGuiDevUIWidget& runtime : _widget_manager.widgets())
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
