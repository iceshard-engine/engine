/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_allocator_tree.hxx"
#include <ice/log_formatters.hxx>
#include <ice/string/string.hxx>
#include <ice/devui_imgui.hxx>

#include <imgui/imgui.h>

namespace ice::devui
{

    namespace detail
    {

        void build_table_view(ice::AllocatorDebugInfo const& allocator, std::string_view filter, ice::i32 default_expanded = 2) noexcept
        {
            ice::AllocatorDebugInfo const* child_alloc = allocator.child_allocator();

            std::string_view alloc_name = allocator.name();
            if (alloc_name.empty())
            {
                alloc_name = "<unnamed_allocator>";
            }

            bool const filtered_out = filter.empty() == false
                && alloc_name.find(filter) == std::string_view::npos;

            bool open = child_alloc != nullptr;
            if (filtered_out == false)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (default_expanded > 0)
                {
                    node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
                }
                if (child_alloc == nullptr)
                {
                    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
                }

                open &= ImGui::TreeNodeEx(&allocator, node_flags, "%s", alloc_name.data());
            }

            // We push a tree even when filtered out, so we can access child trees during filtering
            if (open)
            {
                ImGui::TreePush(&allocator);
            }

            if (filtered_out == false)
            {
                if (ImGui::TableNextColumn())
                {
                    ice::ucount const current_count = allocator.allocation_count();
                    ImGui::Text(current_count == Allocator::CountNotTracked ? "- not tracked -" : "%d", current_count);
                }

                if (ImGui::TableNextColumn())
                {
                    ice::ucount const total_count = allocator.allocation_total_count();
                    ImGui::Text(total_count == Allocator::CountNotTracked ? "- not tracked -" : "%d", total_count);
                }

                // InUse
                if (ImGui::TableNextColumn())
                {
                    ice::usize const size_allocated = allocator.allocation_size_inuse();
                    if (size_allocated == Allocator::SizeNotTracked)
                    {
                        ImGui::TextUnformatted("- not tracked -");
                    }
                    else
                    {
                        ImGui::TextT("{0:P} ({0:i} bytes)", size_allocated);
                    }
                }

                // Watermark
                if (ImGui::TableNextColumn())
                {
                    ice::usize const size_allocated = allocator.allocation_size_watermark();
                    if (size_allocated == Allocator::SizeNotTracked)
                    {
                        ImGui::TextUnformatted("- not tracked -");
                    }
                    else
                    {
                        ImGui::TextT("{0:P} ({0:i} bytes)", size_allocated);
                    }
                }

                if (ImGui::TableNextColumn())
                {
                    ImGui::TextUnformatted(allocator.location().function_name());
                }

                if (ImGui::TableNextColumn())
                {
                    ImGui::Text("%s(%u)", allocator.location().file_name(), allocator.location().line());
                }
            }

            if (open)
            {
                while (child_alloc != nullptr)
                {
                    build_table_view(*child_alloc, filter, default_expanded - 1);
                    child_alloc = child_alloc->next_sibling();
                }
                ImGui::TreePop();
            }
        }

    } // namespace detail

    static constexpr DevUIWidgetInfo Constant_WidgetInfo{
        .category = "Tools",
        .name = "Allocator Tree",
    };

    ImGui_AllocatorTreeWidget::ImGui_AllocatorTreeWidget(
        ice::AllocatorDebugInfo const& alloc
    ) noexcept
        : DevUIWidget{ Constant_WidgetInfo }
        , _root_tracked_allocator{ alloc }
        , _expanded{ false }
    {
    }

    void ImGui_AllocatorTreeWidget::build_widget(ice::DevUIFrame&, ice::DevUIWidgetState& state) noexcept
    {
        ImGui::SetWindowSize({ 600, 300 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Allocators", &state.active, 0))
        {
            build_content();
        }
        ImGui::End();
    }

    void ice::devui::ImGui_AllocatorTreeWidget::build_content() noexcept
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

        ImGui::InputText("Filter", _filter, sizeof(_filter), ImGuiInputTextFlags_AutoSelectAll);
        ImGui::SameLine();
        ImGui::Checkbox("Expand all", &_expanded);

        if (ImGui::BeginTable("Allocators", 7, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Count (current)");
            ImGui::TableSetupColumn("Count (total)");
            ImGui::TableSetupColumn("Current Size");
            ImGui::TableSetupColumn("Watermark Size");
            ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();

            detail::build_table_view(_root_tracked_allocator, { _filter, strlen(_filter) },  _expanded ? 64 : 2);
            ImGui::EndTable();
        }
    }

    auto create_allocator_tree_widget(
        ice::Allocator& allocator
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>
    {
        ice::UniquePtr<ice::DevUIWidget> result;
        if constexpr (ice::Allocator::HasDebugInformation)
        {
            ice::AllocatorDebugInfo const* top_alloc = &allocator.debug_info();
            while (top_alloc->parent_allocator() != nullptr)
            {
                top_alloc = top_alloc->parent_allocator();
            }

            result = ice::make_unique<ImGui_AllocatorTreeWidget>(allocator, *top_alloc);
        }
        return result;
    }

} // namespace ice::devui

#undef IMGUI_SIZE_FMT
