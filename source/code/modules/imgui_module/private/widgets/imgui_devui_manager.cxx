/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_devui_manager.hxx"

#include <imgui/imgui.h>
#undef assert

namespace ImGui
{
    void StringUnformatted(ice::String str) noexcept
    {
        TextUnformatted(ice::string::begin(str), ice::string::end(str));
    }
} // namespace ImGui

namespace ice::devui
{

    ImGuiDevUIManager::ImGuiDevUIManager(ice::Allocator& alloc) noexcept
        : DevUIWidget{ DevUIWidgetInfo{ .category = "Help", .name = "Widget Manager" } }
        , _widgets{ alloc }
    {
        ice::array::reserve(_widgets, 100);
    }

    ImGuiDevUIManager::~ImGuiDevUIManager() noexcept
    {
    }

    void ImGuiDevUIManager::add_widget(ice::DevUIWidget *widget) noexcept
    {
        ice::array::push_back(_widgets, { .widget = widget });
    }

    void ImGuiDevUIManager::remove_widget(ice::DevUIWidget *widget) noexcept
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

    void ImGuiDevUIManager::build_content() noexcept
    {
        ImGuiTableFlags const flags = ImGuiTableFlags_None
            // Functional
            | ImGuiTableFlags_ContextMenuInBody
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_Hideable
            // Visual
            | ImGuiTableFlags_SizingStretchSame
            | ImGuiTableFlags_HighlightHoveredColumn
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_BordersOuterH
            | ImGuiTableFlags_BordersV
            | ImGuiTableFlags_RowBg;

        if (ImGui::BeginTable("DevUI:DevUIWidgetManager", 3, flags))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Category");
            ImGui::TableSetupColumn("Visible");
            ImGui::TableHeadersRow();

            for (ImGuiDevUIWidget& widget : _widgets)
            {
                ImGui::TableNextRow();

                ImGui::PushID(widget.widget);
                if (ImGui::TableNextColumn()) // Name
                {
                    ImGui::StringUnformatted(widget.widget->widget_info.name);
                }
                if (ImGui::TableNextColumn()) // Category
                {
                    ImGui::StringUnformatted(widget.widget->widget_info.category);
                }
                if (ImGui::TableNextColumn()) // Visible
                {
                    ImGui::Checkbox("", &widget.active);
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

} // namespace ice::devui
