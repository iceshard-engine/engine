#include "iceshard_debug_input_action.hxx"
#include "../iceshard_execution_instance.hxx"
#include "../frame.hxx"

#include <imgui/imgui.h>

namespace iceshard::debug
{

    ActionsDebugWindow::ActionsDebugWindow(core::allocator& alloc) noexcept
        : DebugWindow{}
        , _actions{ alloc }
        , _action_states{ alloc }
    {
    }

    void ActionsDebugWindow::update(iceshard::Frame const& frame) noexcept
    {
        auto const& execution_instance = static_cast<MemoryFrame const&>(frame).execution_instance();

        core::pod::array::clear(_actions);

        for (InputActionState const& state : execution_instance.input_actions().action_states())
        {
            core::pod::array::push_back(_actions, state.action_info);
            core::pod::hash::set(_action_states, core::hash(state.action_name), &state);
        }
    }

    void ActionsDebugWindow::end_frame() noexcept
    {
        if (ImGui::Begin("Actions", &_visible))
        {
            for (InputAction const* action_info : _actions)
            {
                if (ImGui::TreeNode(action_info, "ActionID: %llX (%s)", core::hash(action_info->name), core::origin(action_info->name).data()))
                {
                    ImGui::Columns(3);
                    auto const* const action_state = core::pod::hash::get(_action_states, core::hash(action_info->name), nullptr);
                    IS_ASSERT(action_state != nullptr, "Acction state cannot be nullptr!");

                    if (action_state->is_success)
                    {
                        ImGui::TextColored(ImVec4{ 0.2f, 0.8f, 0.2f, 1.0f }, "Successful");
                    }
                    else if (action_state->is_fail)
                    {
                        ImGui::TextColored(ImVec4{ 0.8f, 0.2f, 0.2f, 1.0f }, "Failed");
                    }
                    else
                    {
                        ImGui::Text("Active");
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Stages: %d", core::pod::array::size(action_info->stages));
                    ImGui::NextColumn();
                    ImGui::Text("Triggers: %d", core::pod::array::size(action_info->triggers));
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::Text("Time since last trigger: %f", core::timeline::elapsed(action_state->action_timeline));
                    ImGui::Separator();

                    uint32_t stage_index = 0;
                    for (auto const& stage : action_info->stages)
                    {
                        core::StringView stage_text = "Stage";
                        if (stage_index == action_state->current_stage)
                        {
                            stage_text = "Stage %d (current)";
                            ImGui::SetNextTreeNodeOpen(true);
                        }

                        if (ImGui::TreeNode(action_state, stage_text.data(), stage_index))
                        {
                            ImGui::TreePop();
                        }

                        stage_index += 1;
                    }

                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    void ActionsDebugWindow::show() noexcept
    {
        _visible = true;
    }

} // namespace iceshard::debug
