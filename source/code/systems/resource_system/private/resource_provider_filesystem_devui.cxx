/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_filesystem_devui.hxx"
#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_imgui.hxx>

namespace ice
{

    FileSystemResourceProvider::DevUI::DevUI(ice::HashMap<ice::FileSystemResource*> const& resources) noexcept
        : DevUIWidget{ DevUIWidgetInfo{ .category = "Tools", .name = "Files" } }
        , _resources{ resources }
        , _filter{ }
    {
        ice::devui_register_widget(this);
    }
    FileSystemResourceProvider::DevUI::~DevUI() noexcept
    {
        ice::devui_remove_widget(this);
    }

    bool FileSystemResourceProvider::DevUI::build_mainmenu(ice::DevUIWidgetState& state) noexcept
    {
        if (ImGui::BeginMenu("Resource Providers", true))
        {
            ImGui::MenuItem(ice::string::begin(info.name), nullptr, &state.active);
            ImGui::EndMenu();
        }
        return false;
    }

    void FileSystemResourceProvider::DevUI::build_content() noexcept
    {
        IPT_ZONE_SCOPED;

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
            | ImGuiTableFlags_RowBg
            // Scrolling
            | ImGuiTableFlags_ScrollY;

        ice::u32 const entry_count = 30;
        ice::u32 const entry_size = static_cast<ice::u32>(ImGui::GetTextLineHeightWithSpacing());
        ice::u32 const table_size = entry_size * entry_count;

        ImGui::InputTextWithHint("Filter", "Filter...", _filter, ice::count(_filter) - 1);

        if (ImGui::BeginTable("FileSystemResourceProvider:Resources", 4, flags, { 0, (ice::f32) table_size }))
        {
            ice::u32 const scroll_value = (ice::u32) ImGui::GetScrollY();
            ice::u32 const scroll_idx = scroll_value / entry_size;

            // Adjust the scroll position to always show full
            if (ice::u32 const scroll_offset = scroll_value % entry_size; scroll_offset > 0)
            {
                ImGui::SetScrollY(ImGui::GetScrollY() + ice::f32(entry_size - scroll_offset));
            }

            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Origin");
            ImGui::TableSetupColumn("URI", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();

            this->build_resources_table(scroll_idx, scroll_idx + entry_count);
            ImGui::EndTable();
        }
    }

    void FileSystemResourceProvider::DevUI::build_resources_table(ice::u32 idx_start, ice::u32 idx_end) noexcept
    {
        ice::u32 idx = 1; // We start with '1' since the first entry are the headers.
        for (ice::FileSystemResource* const res : ice::hashmap::values(_resources))
        {
            if (strstr(ice::string::begin(res->name()), _filter) == nullptr)
            {
                continue;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // Skip over invisible rows
            if (idx < idx_start || idx > idx_end)
            {
                ImGui::Text(" ");
                idx += 1;
                continue;
            }

            ImGui::TextUnformatted(ice::string::begin(res->name()), ice::string::end(res->name()));

            if (ImGui::TableNextColumn())
            {
                ImGui::TextUnformatted(ice::string::begin(res->origin()), ice::string::end(res->origin()));
            }

            if (ImGui::TableNextColumn())
            {
                ImGui::TextUnformatted(res->uri()._uri);
            }

            if (ImGui::TableNextColumn())
            {
                ImGui::TextT("{:p}", res->size());
            }

            idx += 1;
        }
    }

    auto create_filesystem_provider_devui(
        ice::Allocator& alloc,
        ice::HashMap<ice::FileSystemResource*> const& resources
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>
    {
        if (ice::devui_available())
        {
            return ice::make_unique<FileSystemResourceProvider::DevUI>(alloc, resources);
        }
        return {};
    }

} // namespace ice
