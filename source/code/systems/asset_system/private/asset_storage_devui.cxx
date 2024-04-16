/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_entry.hxx"
#include "asset_storage_devui.hxx"
#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice
{

    auto create_asset_storage_devui(
        ice::Allocator& alloc,
        ice::DefaultAssetStorage& storage,
        ice::Array<ice::UniquePtr<ice::AssetShelve::DevUI>> shelves
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>
    {
        if (ice::devui_available())
        {
            return ice::make_unique<DefaultAssetStorage::DevUI>(alloc, alloc, storage, ice::move(shelves));
        }
        return {};
    }

    DefaultAssetStorage::DevUI::DevUI(
        ice::Allocator& alloc,
        ice::DefaultAssetStorage& storage,
        ice::Array<ice::UniquePtr<ice::AssetShelve::DevUI>> shelves
    ) noexcept
        : DevUIWidget{ DevUIWidgetInfo{ .category = "Tools", .name = "Assets" } }
        , _storage{ storage }
        , _shelves{ ice::move(shelves) }
    {
        ice::devui_register_widget(this);
    }

    DefaultAssetStorage::DevUI::~DevUI() noexcept
    {
        ice::devui_remove_widget(this);
    }

    void DefaultAssetStorage::DevUI::build_content() noexcept
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

        if (ImGui::CollapsingHeader("Asset Types"))
        {
            if (ImGui::BeginTable("DefaultAssetStorage:AssetTypes", 3, flags))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Extensions");
                ImGui::TableSetupColumn("Compiler");
                ImGui::TableHeadersRow();

                for (ice::AssetType_Arg& asset_type : _storage._asset_archive->asset_types())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(ice::string::begin(asset_type.name), ice::string::end(asset_type.name));

                    ice::AssetTypeDefinition const& def = _storage._asset_archive->find_definition(asset_type);
                    if (ImGui::TableNextColumn())
                    {
                        for (ice::String ext : def.resource_extensions)
                        {
                            ImGui::TextUnformatted(ice::string::begin(ext), ice::string::end(ext)); ImGui::SameLine();
                        }
                    }

                    if (ImGui::TableNextColumn())
                    {
                        ImGui::TextUnformatted(
                            _storage._asset_archive->find_compiler(asset_type)
                            ? "Available"
                            : "-"
                        );
                    }
                }

                ImGui::EndTable();
            }
        }

        for (ice::UniquePtr<AssetShelve::DevUI> const& shelve : _shelves)
        {
            shelve->build_content();
        }
    }

} // namespace ice
