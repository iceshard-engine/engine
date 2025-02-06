#include "iceshard_world_manager_devui.hxx"
#include "iceshard_world_devui.hxx"

#include <ice/devui_imgui.hxx>
#include <ice/engine_shards.hxx>

namespace ice
{

    auto IceshardWorldManager::create_devui() noexcept -> ice::UniquePtr<IceshardWorldManager::DevUI>
    {
        if (ice::devui_available())
        {
            static const ice::DevUIWidgetInfo info{.category = "Engine", .name = "World Manager"};
            return ice::make_unique<DevUI>(_allocator, _allocator, info, *this);
        }
        return {};
    }

    IceshardWorldManager::DevUI::DevUI(
        ice::Allocator& alloc,
        ice::DevUIWidgetInfo const& info,
        ice::IceshardWorldManager& manager
    ) noexcept
        : DevUIWidget{ info }
        , _allocator{ alloc }
        , _manager{ manager }
        , _entries{ _allocator }
        , _selected{ 0 }
    {
        ice::devui_register_widget(this);
    }

    void IceshardWorldManager::DevUI::build_content() noexcept
    {
        ice::Color<ice::u8> constexpr TextColors[]{
            0xFF'888888_argb, // grayed-out
            0xFF'57C747_argb, // green
        };

        // Always ensure same size

        ice::array::resize(_entries, ice::hashmap::count(_manager._worlds));

        [[maybe_unused]]
        ImVec2 const avail = ImGui::GetContentRegionAvail();
        if (ImGui::BeginTable("##details", 2))
        {
            ImGui::TableSetupColumn("##first", ImGuiTableColumnFlags_WidthFixed, 200.f);
            ImGui::TableSetupColumn("##second", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ice::u32 idx = 0;
            ice::Span const entries = ice::hashmap::values(_manager._worlds);
            for (IceshardWorldManager::Entry const& entry : entries)
            {
                ice::IceshardWorld const& world = *entry.world;

                if (ImGui::BeginLargeButton(ice::stringid_hint(world.worldID), _entries[idx].status, {0,0}))
                {
                    if (_entries[idx].status & 1)
                    {
                        _selected = idx;
                    }

                    ImGui::SameLine();
                    ImGui::TextRightColoredT(TextColors[entry.is_active], "{}", entry.is_active ? "Active" : "Inactive");
                }
                ImGui::EndLargeButton();

                idx += 1;
            }
            ImGui::TableNextColumn();
            if (ImGui::BeginChild("##world-details", {}, ImGuiChildFlags_Border))
            {
                ice::IceshardWorldManager::Entry& world_entry = entries[_selected];
                ice::IceshardWorld& world = *world_entry.world;

                if (ImGui::Button(world_entry.is_active ? "Deactivate" : "Activate"))
                {
                    if (world_entry.is_active)
                    {
                        world.devui().world_operation = ice::shard(ice::ShardID_WorldDeactivate, world.worldID.value);
                    }
                    else
                    {
                        world.devui().world_operation = ice::shard(ice::ShardID_WorldActivate, world.worldID.value);
                    }
                }

                if (ImGui::BeginChild("##stats", {}, ImGuiChildFlags_AutoResizeY))
                {
                    world.devui().build_content();
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }

} // namespace ice
