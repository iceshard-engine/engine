#include "widget_imgui_allocator_tree.hxx"
#include <ice/string.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    namespace detail
    {

        void build_info_column(ice::TrackedAllocator const& allocator) noexcept
        {
            ice::u32 const current_total_allocation = allocator.total_allocated();
            ice::u32 const current_total_count = allocator.allocation_count();

            if (current_total_count == Allocator::Constant_SizeNotTracked)
            {
                ImGui::Text("<not tracked>");
            }
            else
            {
                ImGui::Text("%d", current_total_count);
            }

            ImGui::NextColumn();

            if (current_total_allocation == Allocator::Constant_SizeNotTracked)
            {
                ImGui::Text("<not tracked>");
            }
            else
            {
                bool const shows_mibs = current_total_allocation > (1024 * 1024);
                bool const shows_kibs = current_total_allocation > (1024);

                if (shows_mibs)
                {
                    ice::u32 const mibs = current_total_allocation / (1024 * 1024);
                    ice::u32 const kibs = (current_total_allocation / 1024) - (mibs * 1024);
                    ImGui::Text("%d MiB %d KiB (%d bytes)", mibs, kibs, current_total_allocation);
                }
                else if (shows_kibs)
                {
                    ImGui::Text("%d KiB (%d bytes)", (current_total_allocation / 1024), current_total_allocation);
                }
                else
                {
                    ImGui::Text("%d", current_total_allocation);
                }
            }
        }

        void build_tree_view(ice::TrackedAllocator const& allocator) noexcept
        {
            ice::TrackedAllocator const* child_alloc = allocator.child_allocators();
            ice::String alloc_name = allocator.name();
            if (ice::string::empty(alloc_name))
            {
                alloc_name = "<unnamed_allocator>";
            }

            if (child_alloc == nullptr)
            {
                ImGui::Text(alloc_name.data());
                ImGui::NextColumn();
                build_info_column(allocator);
                ImGui::NextColumn(); // We start from the first column again
            }
            else
            {
                bool show_childs = ImGui::TreeNode(&allocator, alloc_name.data());
                ImGui::NextColumn();
                build_info_column(allocator);

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
        ice::TrackedAllocator const& alloc
    ) noexcept
        : _root_tracked_allocator{ alloc }
    {
    }

    void ice::devui::ImGui_AllocatorTreeWidget::on_draw() noexcept
    {
        if (ImGui::Begin("Allocator tree", &_open))
        {
            ImGui::Columns(3, "allocator_tree_table", true);

            ImGui::Text("Allocator name");
            ImGui::NextColumn();
            ImGui::Text("Allocation count");
            ImGui::NextColumn();
            ImGui::Text("Allocated bytes");
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
