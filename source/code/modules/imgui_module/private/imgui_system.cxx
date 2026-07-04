/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"
#include "imgui_trait.hxx"
#include "widgets/imgui_allocator_tree.hxx"

#include <ice/assert.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/devui_widgets.hxx>
#include <ice/static_string.hxx>
#include <ice/colors.hxx>

#if ISP_WINDOWS
#include <imguizmo/ImGuizmo.h>
#endif

namespace ice::devui
{

    namespace detail
    {

        void build_mainmenu(ice::StaticString<32>& temp, ice::String path, ice::String name, bool& state) noexcept
        {
            ice::nindex const separator_pos = path.find_first_of('/');
            if (separator_pos != ice::nindex_none)
            {
                temp = path.substr(0, separator_pos);

                if (ImGui::BeginMenu(temp.begin()))
                {
                    build_mainmenu(temp, path.substr(separator_pos + 1), name, state);
                    ImGui::EndMenu();
                }
            }
            else
            {
                if (ImGui::BeginMenu(path.begin()))
                {
                    ImGui::MenuItem(name.begin(), nullptr, &state);
                    ImGui::EndMenu();
                }
            }
        }

    } // namespace detail


    void ImGuiWidgetFrame::mainmenu(ice::DevUIWidget const& widget, ice::DevUIWidgetState& state) noexcept
    {
        ice::nindex const separator_pos = widget.category().find_first_of('/');
        if (separator_pos == ice::nindex_none)
        {
            ImGui::MenuItem(widget.name().begin(), nullptr, &state.active);
            return;
        }

        ice::StaticString<32> helper;
        detail::build_mainmenu(helper, widget.category().substr(separator_pos + 1), widget.name(), state.active);
    }

    bool ImGuiWidgetFrame::begin(ice::DevUIWidget const& widget, ice::DevUIWidgetState& state) noexcept
    {
        return ImGui::Begin(widget.name().begin(), &state.active);
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
        , _widget_colorpicker{ }
        //, _widget_style{ _allocator }
    {
        _builtin_widgets.push_back(create_allocator_tree_widget(_allocator));
        // ice::array::push_back(_builtin_widgets, (ice::UniquePtr<ice::DevUIWidget>) ice::make_unique<ImGuiLogger>(_allocator, _allocator));

        // Register all built-in's
        _widget_manager.add_widget(&_widget_manager); // Add self...
        _widget_manager.add_widget(&_widget_logger); // Add logger
        _widget_manager.add_widget(&_widget_colorpicker);
        //_widget_manager.add_widget(&_widget_style); // Add style editor
        for (ice::UniquePtr<ice::DevUIWidget> const& widget : _builtin_widgets)
        {
            _widget_manager.add_widget(widget.get());
        }

        // Setup default main-menu categories
        ice::I18NReference categories[]
        {
            "builtin.devui.strings/menu.category.file|File"_i18n,
            "builtin.devui.strings/menu.category.settings|Settings"_i18n,
            "builtin.devui.strings/menu.category.utility|Utility"_i18n,
            "builtin.devui.strings/menu.category.engine|Engine"_i18n,
            "builtin.devui.strings/menu.category.tools|Tools"_i18n,
            "builtin.devui.strings/menu.category.help|Help"_i18n
        };
        setup_mainmenu(categories);
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
    }

    void ImGuiSystem::setup_mainmenu(ice::Span<ice::I18NReference> categories) noexcept
    {
        _menu_categories.clear();
        for (ice::I18NReference category : categories)
        {
            _menu_categories.push_back(category);
        }
    }

    void ImGuiSystem::register_widget(
        ice::DevUIWidget* widget,
        ice::DevUIWidget* owning_widget
    ) noexcept
    {
        _widget_manager.add_widget(widget, owning_widget);
    }

    void ImGuiSystem::unregister_widget(ice::DevUIWidget* widget) noexcept
    {
        _widget_manager.remove_widget(widget);
    }

    void ImGuiSystem::update_widgets() noexcept
    {
        ImGuiIO const& imgui_io = ImGui::GetIO();

        // If display size is not set we return quickly
        if (imgui_io.DisplaySize.x <= 0.F || imgui_io.DisplaySize.y <= 0.F)
        {
            return;
        }

        static bool show_demo = false;

        ImGui::NewFrame();
#if ISP_WINDOWS
        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, imgui_io.DisplaySize.x, imgui_io.DisplaySize.y);
#endif

        {
            for (ice::I18NString& category : _menu_categories)
            {
                category.resolve();
            }

            if (ImGui::BeginMainMenuBar())
            {
                for (ice::String category : _menu_categories)
                {
                    if (ImGui::BeginMenu(category.begin()))
                    {
                        for (auto const& runtime : _widget_manager.widgets())
                        {
                            ice::String const widget_category = runtime->widget->category();
                            if (category.starts_with(widget_category) && runtime->widget->build_mainmenu(runtime->state))
                            {
                                _widget_frame.mainmenu(*runtime->widget, runtime->state);
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

            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            for (auto const& runtime : _widget_manager.widgets())
            {
                DevUIWidgetState const* const owner_state = runtime->state.owner;

                runtime->widget->update_state(runtime->state);
                if (runtime->state.active)
                {
                    ICE_ASSERT(runtime->state.owner == owner_state, "It's prohibited to change the owner pointer!");
                    runtime->widget->build_widget(_widget_frame, runtime->state);
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
        ImGui::TextT("Widgets: {}", _widget_manager.widgets().size());

        ImGui::NewLine(); ImGui::Separator();
        ImGui::TextT("Draw calls: {} ({:p})", stats.draw_calls, stats.draw_datasize);
        ImGui::SameLine(size.x * 0.5f);
        ImGui::TextT("Time: {:.3d}", stats.draw_processtime);

        ImGui::TextT("Vertices: {}", stats.draw_vertices);
        ImGui::SameLine(size.x * 0.5f);
        ImGui::TextT("Indices: {}", stats.draw_indices);
    }

    ImGui_ColorPicker_OkLCH::ImGui_ColorPicker_OkLCH() noexcept
        : DevUIWidget{ DevUIWidgetInfo{
            .category = "builtin.devui.strings/menu.category.utility|Utility"_i18n,
            .name = "builtin.devui.strings/widget.color-picker-oklch.name|OkLCH Color Picker"_i18n
        } }
    {
    }

    void ImGui_ColorPicker_OkLCH::build_content() noexcept
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::ColorPickerOkLCH(_widget_info.name, newcolor, ImGui::OkLCHPickerFlags::Chroma_ClipToMax, &color); ImGui::IsItemHovered() == false)
        {
            color = newcolor;
        }
    }

    void ImGui_ColorPicker_OkLCH::update_state(ice::DevUIWidgetState& state) noexcept
    {
        _widget_info.category.resolve();
        _widget_info.name.resolve();
    }
} // namespace ice::devui
