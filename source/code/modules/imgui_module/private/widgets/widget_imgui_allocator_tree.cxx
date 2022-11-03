/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "widget_imgui_allocator_tree.hxx"
#include <ice/string/string.hxx>
#include <imgui/imgui.h>

namespace ice::devui
{

    namespace detail
    {

        void build_info_column(ice::AllocatorDebugInfo const& allocator) noexcept
        {
            ice::usize const current_total_allocation = allocator.allocation_size_inuse();
            ice::ucount const current_total_count = allocator.allocation_total_count();

            if (current_total_count == Allocator::CountNotTracked)
            {
                ImGui::Text("<not tracked>");
            }
            else
            {
                ImGui::Text("%d", current_total_count);
            }

            ImGui::NextColumn();

            if (current_total_allocation == Allocator::SizeNotTracked)
            {
                ImGui::Text("<not tracked>");
            }
            else
            {
                bool const shows_mibs = current_total_allocation > 1_MiB;
                bool const shows_kibs = current_total_allocation > 1_KiB;

                if (shows_mibs)
                {
                    ice::usize const mibs = current_total_allocation / 1_MiB;
                    ice::isize const kibs = (current_total_allocation / 1_KiB) - (mibs * 1_KiB);
                    ImGui::Text("%lu MiB %lu KiB (%lu bytes)", mibs.value, kibs.value, current_total_allocation.value);
                }
                else if (shows_kibs)
                {
                    ImGui::Text("%lu KiB (%lu bytes)", (current_total_allocation / 1_KiB).value, current_total_allocation.value);
                }
                else
                {
                    ImGui::Text("%lu", current_total_allocation.value);
                }
            }
        }

        void build_tree_view(ice::AllocatorDebugInfo const& allocator) noexcept
        {
            ice::AllocatorDebugInfo const* child_alloc = allocator.child_allocator();
            std::string_view alloc_name = allocator.name();
            if (alloc_name.empty())
            {
                alloc_name = "<unnamed_allocator>";
            }

            if (child_alloc == nullptr)
            {
                ImGui::Text("%s", alloc_name.data());
                ImGui::NextColumn();
                build_info_column(allocator);

                ImGui::NextColumn();
                ImGui::Text("%s", allocator.location().function_name());
                ImGui::NextColumn();
                ImGui::Text("%s(%u)", allocator.location().file_name(), allocator.location().line());

                ImGui::NextColumn(); // We start from the first column again
            }
            else
            {
                bool show_childs = ImGui::TreeNode(&allocator, "%s", alloc_name.data());
                ImGui::NextColumn();
                build_info_column(allocator);

                ImGui::NextColumn();
                ImGui::Text("%s", allocator.location().function_name());
                ImGui::NextColumn();
                ImGui::Text("%s(%u)", allocator.location().file_name(), allocator.location().line());

                ImGui::NextColumn(); // We start from the first column again

                if (show_childs)
                {
                    while (child_alloc != nullptr)
                    {
                        build_tree_view(*child_alloc);
                        child_alloc = child_alloc->next_sibling();
                    }
                    ImGui::TreePop();
                }
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
        if (ImGui::Begin("Allocator tree", &_state->is_visible))
        {
            ImGui::Columns(5, "allocator_tree_table", true);

            ImGui::Text("Allocator name");
            ImGui::NextColumn();
            ImGui::Text("Allocation count");
            ImGui::NextColumn();
            ImGui::Text("Allocated bytes");
            ImGui::NextColumn();
            ImGui::Text("Function");
            ImGui::NextColumn();
            ImGui::Text("File (line)");
            ImGui::NextColumn();

            ImGui::Separator();

            detail::build_tree_view(_root_tracked_allocator);
        }
        ImGui::End();
    }

} // namespace ice::devui
