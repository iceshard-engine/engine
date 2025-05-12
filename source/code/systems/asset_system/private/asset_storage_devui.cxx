/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
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
#if ICE_DEBUG || ICE_DEVELOP
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

        if (ImGui::CollapsingHeader("Asset Categories"))
        {
            if (ImGui::BeginTable("DefaultAssetStorage:AssetCategories", 3, flags))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Extensions");
                ImGui::TableSetupColumn("Compiler");
                ImGui::TableHeadersRow();

                for (ice::AssetCategory_Arg category : _storage._asset_archive->categories())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(ice::string::begin(category.name), ice::string::end(category.name));

                    ice::AssetCategoryDefinition const& def = _storage._asset_archive->find_definition(category);
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
                            _storage._asset_archive->find_compiler(category)
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
#endif // #if ICE_DEBUG || ICE_DEVELOP
    }

} // namespace ice
