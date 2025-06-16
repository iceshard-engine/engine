/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"
#include "widgets/imgui_allocator_tree.hxx"

#include <ice/assert.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/string/static_string.hxx>
#include <ice/string/heap_string.hxx>

#if ISP_WINDOWS
#include <imguizmo/ImGuizmo.h>
#endif

namespace ice::devui
{

    namespace detail
    {

        void build_mainmenu(ice::StaticString<32>& temp, ice::String path, ice::String name, bool& state) noexcept
        {
            ice::ucount const separator_pos = ice::string::find_first_of(path, '/');
            if (separator_pos != ice::String_NPos)
            {
                temp = ice::string::substr(path, 0, separator_pos);

                if (ImGui::BeginMenu(ice::string::begin(temp)))
                {
                    build_mainmenu(temp, ice::string::substr(path, separator_pos + 1), name, state);
                    ImGui::EndMenu();
                }
            }
            else
            {
                if (ImGui::BeginMenu(ice::string::begin(path)))
                {
                    ImGui::MenuItem(ice::string::begin(name), nullptr, &state);
                    ImGui::EndMenu();
                }
            }
        }

    } // namespace detail


    void ImGuiWidgetFrame::mainmenu(ice::DevUIWidgetInfo const& widget, ice::DevUIWidgetState& state) noexcept
    {
        ice::ucount const separator_pos = ice::string::find_first_of(widget.category, '/');
        if (separator_pos == ice::String_NPos)
        {
            ImGui::MenuItem(ice::string::begin(widget.name), nullptr, &state.active);
            return;
        }

        ice::StaticString<32> helper;
        detail::build_mainmenu(helper, ice::string::substr(widget.category, separator_pos + 1), widget.name, state.active);
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
        , _menu_categories{ _allocator }
        , _widget_manager{ _allocator }
        , _widget_frame{ }
        , _widget_logger{ _allocator }
    {
        ice::array::push_back(_builtin_widgets, create_allocator_tree_widget(_allocator));
        // ice::array::push_back(_builtin_widgets, (ice::UniquePtr<ice::DevUIWidget>) ice::make_unique<ImGuiLogger>(_allocator, _allocator));

        // Register all built-in's
        _widget_manager.add_widget(&_widget_manager); // Add self...
        _widget_manager.add_widget(&_widget_logger); // Add logger
        for (ice::UniquePtr<ice::DevUIWidget> const& widget : _builtin_widgets)
        {
            _widget_manager.add_widget(widget.get());
        }

        // Setup default main-menu categories
        ice::String categories[]{ "File", "Settings", "Engine", "Tools", "Help" };
        setup_mainmenu(categories);
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
    }

    void ImGuiSystem::setup_mainmenu(ice::Span<ice::String> categories) noexcept
    {
        ice::array::clear(_menu_categories);
        for (ice::String category : categories)
        {
            ice::array::push_back(_menu_categories, ice::HeapString<>{ _allocator, category });
        }
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

        static bool show_demo = false;

        ImGui::NewFrame();
#       if ISP_WINDOWS
        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
#       endif

        {
            if (ImGui::BeginMainMenuBar())
            {
                for (ice::String category : _menu_categories)
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

            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            for (ImGuiDevUIWidget& runtime : _widget_manager.widgets())
            {
                runtime.widget->update_state(runtime);
                if (runtime.active)
                {
                    runtime.widget->build_widget(_widget_frame, runtime);
                }
            }
        }

        ImGui::EndFrame();
    }

    void ImGuiSystem::devui_draw(ice::devui::ImGuiStats const& stats) noexcept
    {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 const size = ImGui::GetContentRegionAvail();

        // ImGui::Separator();//Text("Status");
        ImGui::TextT("Display: {} x {}", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::SameLine(size.x * 0.5f, 1.0f);
        ImGui::TextT("FPS: {} ({:m})", io.Framerate, ice::Ts{io.DeltaTime});

        ImGui::NewLine();
        ImGui::Text("Log file: %s", io.LogFilename);
        ImGui::SameLine(size.x * 0.5f);
        ImGui::Text("Config file: %s", io.IniFilename);

        // ImGui::NewLine(); ImGui::SeparatorText("Info");
        ImGui::NewLine(); ImGui::Separator();
        ImGui::TextT("Widgets: {}", ice::count(_widget_manager.widgets()));

        ImGui::NewLine(); ImGui::Separator();
        ImGui::TextT("Draw calls: {} ({:p})", stats.draw_calls, stats.draw_datasize);
        ImGui::SameLine(size.x * 0.5f);
        ImGui::TextT("Time: {:.3d}", stats.draw_processtime);

        ImGui::TextT("Vertices: {}", stats.draw_vertices);
        ImGui::SameLine(size.x * 0.5f);
        ImGui::TextT("Indices: {}", stats.draw_indices);
    }

} // namespace ice::devui
