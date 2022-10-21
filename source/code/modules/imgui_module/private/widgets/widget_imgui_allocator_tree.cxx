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
                ImGui::NextColumn(); // We start from the first column again

                ImGui::Text("%s | %s(%u)", allocator.location().function_name(), allocator.location().file_name(), allocator.location().line());
                ImGui::NextColumn();
            }
            else
            {
                bool show_childs = ImGui::TreeNode(&allocator, "%s", alloc_name.data());
                ImGui::NextColumn();
                build_info_column(allocator);

                ImGui::NextColumn(); // We start from the first column again

                ImGui::Text("%s | %s(%u)", allocator.location().function_name(), allocator.location().file_name(), allocator.location().line());
                ImGui::NextColumn();

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

            //ImGui::NextColumn();
            ////if (ImGui::TreeNode(&allocator, "Details"))
            //{
            //    //ImGui::TreePop();
            //}

            //if (child_alloc != nullptr)
            //{
            //    if (ImGui::TreeNode(&allocator, "Child allocators"))
            //    {
            //        while (child_alloc != nullptr)
            //        {
            //            build_tree_view(*child_alloc);
            //            child_alloc = child_alloc->next_sibling();
            //        }

            //        ImGui::TreePop();
            //    }
            //}
            //else
            //{
            //    // ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
            //    //ImGui::NewLine();
            //}
        }

    } // namespace detail

    ImGui_AllocatorTreeWidget::ImGui_AllocatorTreeWidget(
        ice::AllocatorDebugInfo const& alloc
    ) noexcept
        : _root_tracked_allocator{ alloc }
    {
    }

    void ice::devui::ImGui_AllocatorTreeWidget::on_draw() noexcept
    {
        if (ImGui::Begin("Allocator tree", &_open))
        {
            ImGui::Columns(4, "allocator_tree_table", true);

            ImGui::Text("Allocator name");
            ImGui::NextColumn();
            ImGui::Text("Allocation count");
            ImGui::NextColumn();
            ImGui::Text("Allocated bytes");
            ImGui::NextColumn();
            ImGui::Text("Function | file(line)");
            ImGui::NextColumn();

            ImGui::Separator();

            detail::build_tree_view(_root_tracked_allocator);
        }
        ImGui::End();

        //{
        //    ICE_LOG(
        //        ice::LogSeverity::Debug, ice::LogTag::Game,
        //        "{}# Allocator `{}` total: {} [{:.3} MiB]",
        //        offset,
        //        ta->name(),
        //        ta->total_allocated(),
        //        ice::f32(ta->total_allocated()) / (1024.f * 1024.f)
        //    );
        //    if (ta->child_allocators())
        //    {
        //        ICE_LOG(
        //            ice::LogSeverity::Debug, ice::LogTag::Game,
        //            "{}> Sub-Allocators...",
        //            offset
        //        );
        //        ice::string::push_back(offset, "| ");
        //        list_allocator_allocations(offset, ta->child_allocators());
        //        ice::string::pop_back(offset, 2);
        //    }
        //    ta = ta->next_sibling();
        //}
    }

} // namespace ice::devui
