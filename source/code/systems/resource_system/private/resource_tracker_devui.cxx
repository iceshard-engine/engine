/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_internal.hxx"
#include "resource_tracker_devui.hxx"

#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_imgui.hxx>

namespace ice
{

    namespace detail
    {
        void status_flags_string(ice::ResourceStatus flags, ice::StaticString<64>& out_str) noexcept;
    } // namespace detail

    auto create_tracker_devui(
        ice::Allocator& alloc,
        ice::ResourceTrackerImplementation& tracker
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>
    {
        if (ice::devui_available())
        {
            return ice::make_unique<ResourceTrackerImplementation::DevUI>(alloc, alloc, tracker);
        }
        return {};
    }

    ResourceTrackerImplementation::DevUI::DevUI(
        ice::Allocator& alloc,
        ice::ResourceTrackerImplementation& tracker
    ) noexcept
        : DevUIWidget{ DevUIWidgetInfo{ .category = "Tools", .name = "Resources" } }
        , _tracker{ tracker }
    {
        ice::devui_register_widget(this);
    }

    ResourceTrackerImplementation::DevUI::~DevUI() noexcept
    {
        ice::devui_remove_widget(this);
    }

    void ResourceTrackerImplementation::DevUI::build_content() noexcept
    {
        build_resource_view();
    }

    void ResourceTrackerImplementation::DevUI::build_resource_view() noexcept
    {
        IPT_ZONE_SCOPED;

        ImGuiTableFlags const flags = ImGuiTableFlags_None
            // Functional
            | ImGuiTableFlags_ContextMenuInBody
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_Hideable
            // Visual
            | ImGuiTableFlags_HighlightHoveredColumn
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_BordersOuterH
            | ImGuiTableFlags_BordersV
            | ImGuiTableFlags_RowBg;

        if (ImGui::BeginTable("ResourceTrackerImplementation:Resources", 2, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();

            ice::StaticString<64> temp_str{};
            for (ice::Resource* handle : _tracker._resources)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(handle->name());

                if (ImGui::TableNextColumn())
                {
                    detail::status_flags_string(ice::internal_status(handle), temp_str);
                    ImGui::TextUnformatted(temp_str);
                }
            }

            ImGui::EndTable();
        }
    }

    void detail::status_flags_string(ice::ResourceStatus flags, ice::StaticString<64>& out_str) noexcept
    {
        using enum ice::ResourceStatus;

        out_str.clear();
        if (ice::has_all(flags, Available))
        {
            out_str.push_back("Available | ");
        }
        if (ice::has_all(flags, Loading))
        {
            out_str.push_back("Loading | ");
        }
        if (ice::has_all(flags, Loaded))
        {
            out_str.push_back("Loaded | ");
        }
        if (ice::has_all(flags, Unloading))
        {
            out_str.push_back("Unloading | ");
        }
        if (flags == Invalid)
        {
            out_str = ice::String{ "Invalid | " };
        }
        out_str.pop_back(3);
    }

} // namespace ice
