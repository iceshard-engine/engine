/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_tracker_devui.hxx"
#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>
#include <ice/string/static_string.hxx>

#include <imgui/imgui.h>
#undef assert

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
            for (ice::ResourceHandle& handle : _tracker._handles)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ice::string::begin(handle.resource->name()), ice::string::end(handle.resource->name()));

                if (ImGui::TableNextColumn())
                {
                    detail::status_flags_string(handle.status, temp_str);
                    ImGui::Text(ice::string::begin(temp_str), ice::string::end(temp_str));
                }
            }

            ImGui::EndTable();
        }
    }

    void detail::status_flags_string(ice::ResourceStatus flags, ice::StaticString<64>& out_str) noexcept
    {
        using enum ice::ResourceStatus;

        ice::string::clear(out_str);
        if (ice::has_all(flags, Invalid))
        {
            ice::string::push_back(out_str, "Invalid | ");
        }
        if (ice::has_all(flags, Available))
        {
            ice::string::push_back(out_str, "Available | ");
        }
        if (ice::has_all(flags, Loading))
        {
            ice::string::push_back(out_str, "Loading | ");
        }
        if (ice::has_all(flags, Loaded))
        {
            ice::string::push_back(out_str, "Loaded | ");
        }
        if (ice::has_all(flags, Unloading))
        {
            ice::string::push_back(out_str, "Unloading | ");
        }
        ice::string::pop_back(out_str, 3);
    }

} // namespace ice
