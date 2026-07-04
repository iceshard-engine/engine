/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_devui_manager.hxx"

#include <ice/engine_devui.hxx>
#include <ice/sort.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ImGui
{
    void StringUnformatted(ice::String str) noexcept
    {
        TextUnformatted(str.begin(), str.end());
    }
} // namespace ImGui

namespace ice::devui
{

    ImGuiDevUIManager::ImGuiDevUIManager(ice::Allocator& alloc) noexcept
        : DevUIWidget{ DevUIWidgetInfo{
            .category = "builtin.devui.strings/menu.category.help"_i18n,
            .name = "builtin.devui.strings/widget.widget-manager.name|Widget Manager"_i18n
        } }
        , _allocator{ alloc }
        , _widgets{ alloc }
    {
        _widgets.reserve(100);
        _widgets.push_back(ice::make_unique<ImGuiDevUIWidget>(_allocator));
    }

    ImGuiDevUIManager::~ImGuiDevUIManager() noexcept
    {
    }

    void ImGuiDevUIManager::add_widget(
        ice::DevUIWidget* widget,
        ice::DevUIWidget* owning_widget
    ) noexcept
    {
        static auto fn_compare = [](ice::UniquePtr<ImGuiDevUIWidget> const& entry, ice::DevUIWidget const* needle) noexcept
            {
                return entry->widget == needle;
            };

        ice::DevUIWidgetState const* owner_state = nullptr;
        ice::u32 owner_idx = 0;
        if (ice::search(ice::Span{ _widgets }, owning_widget, fn_compare, owner_idx))
        {
            owner_state = ice::addressof(_widgets[owner_idx]->state);
        }

        _widgets.push_back(
            ice::make_unique<ImGuiDevUIWidget>(_allocator,
                ImGuiDevUIWidget{
                    .state = {.owner = owner_state },
                    .widget = widget,
                    .owner_index = owner_idx,
                }
            )
        );
    }

    void ImGuiDevUIManager::remove_widget(ice::DevUIWidget* widget) noexcept
    {
        ice::nindex const widget_idx = _widgets.index_of(
            [](ice::UniquePtr<ImGuiDevUIWidget> const& entry, ice::DevUIWidget const* item) noexcept
            {
                return entry->widget == item;
            }, widget
        );
        _widgets.remove_at(widget_idx);
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

            for (auto const& widget : _widgets.tailspan())
            {
                ImGui::TableNextRow();

                ImGui::PushID(widget->widget);
                if (ImGui::TableNextColumn()) // Name
                {
                    ImGui::StringUnformatted(widget->widget->name());
                }
                if (ImGui::TableNextColumn()) // Category
                {
                    ImGui::StringUnformatted(widget->widget->category());
                }
                if (ImGui::TableNextColumn()) // Visible
                {
                    ImGui::Checkbox("", &widget->state.active);
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void ImGuiDevUIManager::update_state(ice::DevUIWidgetState& state) noexcept
    {
        _widget_info.name.resolve();
        _widget_info.category.resolve();
    }

} // namespace ice::devui
