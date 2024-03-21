/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "widget_imgui_allocator_tree.hxx"
#include <ice/string/string.hxx>
#include <imgui/imgui.h>

#if ISP_WEBAPP || ISP_ANDROID
#define IMGUI_SIZE_FMT "%lu"
#else
#define IMGUI_SIZE_FMT "%llu"
#endif

namespace ice::devui
{

    namespace detail
    {

        void build_table_view(ice::AllocatorDebugInfo const& allocator) noexcept
        {
            ice::AllocatorDebugInfo const* child_alloc = allocator.child_allocator();

            std::string_view alloc_name = allocator.name();
            if (alloc_name.empty())
            {
                alloc_name = "<unnamed_allocator>";
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_SpanAllColumns;
            if (child_alloc == nullptr)
            {
                node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }

            bool const open = ImGui::TreeNodeEx(&allocator, node_flags, "%s", alloc_name.data());

            // Parent
            if (ImGui::TableSetColumnIndex(1))
            {
                ice::ucount const total_count = allocator.allocation_total_count();
                char const* const format = total_count == Allocator::CountNotTracked
                    ? "- not tracked -" : "%d";

                ImGui::Text(format, total_count);
            }

            if (ImGui::TableSetColumnIndex(2))
            {
                ice::usize const size_allocated = allocator.allocation_size_inuse();
                if (size_allocated == Allocator::SizeNotTracked)
                {
                    ImGui::TextUnformatted("- not tracked -");
                }
                else
                {
                    bool const shows_mibs = size_allocated > 1_MiB;
                    bool const shows_kibs = size_allocated > 1_KiB;

                    ice::usize const mibs = size_allocated / 1_MiB;
                    ice::usize const kibs = ((size_allocated / 1_KiB) - (mibs * 1_KiB)).to_usize();

                    if (shows_mibs)
                    {
                        ImGui::Text(IMGUI_SIZE_FMT " MiB " IMGUI_SIZE_FMT " KiB (" IMGUI_SIZE_FMT " bytes)", mibs.value, kibs.value, size_allocated.value);
                    }
                    else if (shows_kibs)
                    {
                        ImGui::Text(IMGUI_SIZE_FMT " KiB (" IMGUI_SIZE_FMT " bytes)", (size_allocated / 1_KiB).value, size_allocated.value);
                    }
                    else
                    {
                        ImGui::Text(IMGUI_SIZE_FMT, size_allocated.value);
                    }
                }
            }

            if (ImGui::TableSetColumnIndex(3))
            {
                ImGui::TextUnformatted(allocator.location().function_name());
            }

            if (ImGui::TableSetColumnIndex(4))
            {
                ImGui::Text("%s(%u)", allocator.location().file_name(), allocator.location().line());
            }

            if (open && child_alloc != nullptr)
            {
                while (child_alloc != nullptr)
                {
                    build_table_view(*child_alloc);
                    child_alloc = child_alloc->next_sibling();
                }
                ImGui::TreePop();
            }
        }

    } // namespace detail

    ImGui_AllocatorTreeWidget::ImGui_AllocatorTreeWidget(
        ice::AllocatorDebugInfo const& alloc
    ) noexcept
        : _root_tracked_allocator{ alloc }
    {
    }

    auto ImGui_AllocatorTreeWidget::settings() const noexcept -> ice::devui::WidgetSettings const&
    {
        static devui::WidgetSettings settings{
            .menu_text = "Allocator Tree",
            .menu_category = "Tools",
        };
        return settings;
    }

    void ImGui_AllocatorTreeWidget::on_prepare(void*, ice::devui::WidgetState& state) noexcept
    {
        _state = &state;
    }

    void ice::devui::ImGui_AllocatorTreeWidget::on_draw() noexcept
    {
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

        if (ImGui::BeginTable("Allocators", 5, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Count (total)");
            ImGui::TableSetupColumn("Size (current)");
            ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();

            detail::build_table_view(_root_tracked_allocator);
            ImGui::EndTable();
        }
    }

} // namespace ice::devui

#undef IMGUI_SIZE_FMT
