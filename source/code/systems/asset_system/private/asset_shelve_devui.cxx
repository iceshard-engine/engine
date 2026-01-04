/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_shelve_devui.hxx"

#include <ice/resource.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/devui_imgui.hxx>

namespace ice
{

    AssetShelve::DevUI::DevUI(ice::AssetShelve& shelve) noexcept
        : DevUIWidget{ DevUIWidgetInfo{ .category = "Tools",  } }
        , _shelve{ shelve }
    {
    }

    AssetShelve::DevUI::~DevUI() noexcept = default;

    void AssetShelve::DevUI::build_content() noexcept
    {
#if ICE_DEBUG || ICE_DEVELOP
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

        if (ImGui::BeginTable("AssetShelve:Assets", 3, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Resource", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();

            for (ice::AssetEntry const* entry : ice::hashmap::values(_shelve._asset_resources))
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->debug_name.cbegin(), entry->debug_name.cend());

                if (ImGui::TableNextColumn()) // Status
                {
                    static constexpr ice::String Constant_StateNames[]{
                        "Invalid", "Unknown", "Exists", "Raw", "Baked", "Loaded", "Runtime"
                    };

                    ImGui::TextUnformatted(Constant_StateNames[static_cast<ice::u32>(entry->state())]);
                }
                if (ImGui::TableNextColumn()) // Resource
                {
                    ice::ResourceHandle const handle = ice::asset_data_resource(entry->_data);
                    ice::String const origin = handle != nullptr ? handle->origin() : "???";
                    ImGui::TextUnformatted(origin);
                }
            }

            ImGui::EndTable();
        }
#endif // #if ICE_DEBUG || ICE_DEVELOP
    }

} // namespace ice
