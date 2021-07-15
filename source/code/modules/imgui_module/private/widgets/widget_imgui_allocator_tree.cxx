#include "widget_imgui_allocator_tree.hxx"
#include <ice/string.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    namespace detail
    {

        void build_tree_view(ice::TrackedAllocator const& allocator)
        {
            ice::TrackedAllocator const* child_alloc = allocator.child_allocators();
            ice::String alloc_name = allocator.name();
            if (ice::string::empty(alloc_name))
            {
                alloc_name = "<unnamed_allocator>";
            }

            ImGui::Text("Allocator: %s", alloc_name.data());
            //if (ImGui::TreeNode(&allocator, "Details"))
            {
                ice::u32 const current_total_allocation = allocator.total_allocated();
                ice::u32 const current_total_count = allocator.allocation_count();

                if (current_total_count == Allocator::Constant_SizeNotTracked)
                {
                    ImGui::Text("> Allocation count: <not tracked>");
                }
                else
                {
                    ImGui::Text("> Allocation count: %d", current_total_count);
                }
                if (current_total_allocation == Allocator::Constant_SizeNotTracked)
                {
                    ImGui::Text("> Allocated bytes: <not tracked>");
                }
                else
                {
                    bool const shows_mibs = current_total_allocation > (1024 * 1024);
                    bool const shows_kibs = current_total_allocation > (1024);

                    if (shows_mibs)
                    {
                        ice::u32 const mibs = current_total_allocation / (1024 * 1024);
                        ice::u32 const kibs = (current_total_allocation / 1024) - (mibs * 1024);
                        ImGui::Text("> Allocated bytes: %d (%d MiB %d KiB)", current_total_allocation, mibs, kibs);
                    }
                    else if (shows_kibs)
                    {
                        ImGui::Text("> Allocated bytes: %d (%d KiB)", current_total_allocation, (current_total_allocation / 1024));
                    }
                    else
                    {
                        ImGui::Text("> Allocated bytes: %d", current_total_allocation);
                    }
                }
                //ImGui::TreePop();
            }

            if (child_alloc != nullptr)
            {
                if (ImGui::TreeNode(&allocator, "Child allocators"))
                {
                    while (child_alloc != nullptr)
                    {
                        build_tree_view(*child_alloc);
                        child_alloc = child_alloc->next_sibling();
                    }

                    ImGui::TreePop();
                }
            }
            else
            {
                // ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
                //ImGui::NewLine();
            }
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
