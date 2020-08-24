#include "iceshard_input_actions.hxx"

#include <iceshard/frame.hxx>
#include <iceshard/input/input_event.hxx>

#include <core/message/operations.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{

    InputActionsTracker::InputActionsTracker(core::allocator& alloc, core::Clock& clock) noexcept
        : _allocator{ alloc }
        , _clock{ clock }
        , _defined_actions{ alloc }
        , _trigger_definitions{ alloc }
        , _action_states{ alloc }
    {
    }

    InputActionsTracker::~InputActionsTracker() noexcept
    {
    }

    void InputActionsTracker::define_action(InputAction input_action) noexcept
    {
        auto const action_name = input_action.name;

        _defined_actions.emplace(
            action_name.hash_value,
            core::memory::make_unique<InputAction>(_allocator, std::move(input_action))
        );

        InputAction const* action_info = _defined_actions.at(action_name.hash_value).get();

        core::pod::array::push_back(_action_states,
            InputActionState{
                .action_name = action_name.hash_value,
                .action_info = action_info,
                .action_timeline = core::timeline::create_timeline(_clock),
                .current_stage = 0,
                .current_success_trigger = action_info->stages[0].initial_success_trigger,
                .current_fail_trigger = action_info->stages[0].initial_fail_trigger,
                .current_reset_trigger = action_info->stages[0].reset_trigger,
                .is_success = false,
                .is_fail = false,
            }
        );
    }

    void InputActionsTracker::create_action(core::stringid_arg_type name, ActionDefinition action_definition) noexcept
    {
        core::pod::Array<InputActionTrigger> triggers{ _allocator };

        uint32_t trigger_role_index[3] = { 0, 0, 0 };

        auto create_triggers = [this](auto& target, auto const& trigger_array) noexcept
        {
            auto const starting_index = core::pod::array::size(target);

            for (ActionTrigger const& trigger : trigger_array)
            {
                auto const trigger_definition = get_trigger_definition(trigger.trigger_name);

                core::pod::array::push_back(target,
                    InputActionTrigger{
                        .event = trigger_definition.event,
                        .func = trigger_definition.func,
                        .user_data = trigger.trigger_userdata
                    }
                );
            }

            return starting_index;
        };

        trigger_role_index[0] = create_triggers(triggers, action_definition.success_triggers);
        trigger_role_index[1] = create_triggers(triggers, action_definition.failure_triggers);
        trigger_role_index[2] = create_triggers(triggers, action_definition.reset_triggers);

        core::pod::Array<InputActionStage> stages{ _allocator };
        for (ActionStage const& stage_info : action_definition.stages)
        {
            core::pod::array::push_back(stages,
                InputActionStage
                {
                    .initial_success_trigger = trigger_role_index[0] + stage_info.initial_success_trigger,
                    .final_success_trigger = trigger_role_index[0] + stage_info.initial_success_trigger + (stage_info.num_success_triggers - 1), // the valid final index
                    .initial_fail_trigger = trigger_role_index[1] + stage_info.initial_failure_trigger,
                    .final_fail_trigger = trigger_role_index[1] + stage_info.initial_failure_trigger + (stage_info.num_failure_triggers - 1),
                    .reset_trigger = trigger_role_index[2] + stage_info.reset_trigger,
                }
            );
        }

        define_action(
            InputAction{
                name,
                std::move(stages),
                std::move(triggers)
            }
        );
    }

    void InputActionsTracker::remove_action(core::stringid_arg_type name) noexcept
    {
        _defined_actions.erase(name.hash_value);
    }

    void InputActionsTracker::update_actions(Frame const& frame, core::pod::Array<core::stringid_type>& out_actions) noexcept
    {
        static auto handle_reset_trigger = [this](InputActionTrigger const& trigger, InputActionState& state, void const* data) noexcept
        {
            if (trigger.func(trigger.user_data, core::timeline::elapsed(state.action_timeline), data))
            {
                state.action_timeline = core::timeline::create_timeline(_clock);
                state.current_stage = 0;
                state.current_success_trigger = state.action_info->stages[0].initial_success_trigger;
                state.current_fail_trigger = state.action_info->stages[0].initial_fail_trigger;
                state.current_reset_trigger = state.action_info->stages[0].reset_trigger;
                state.is_success = false;
                state.is_fail = false;
            }
        };

        static auto handle_fail_trigger = [this](
            InputActionTrigger const& trigger,
            InputActionStage const& stage,
            InputActionState& state,
            void const* data
        ) noexcept
        {
            if (trigger.func(trigger.user_data, core::timeline::elapsed(state.action_timeline), data))
            {
                if (state.current_fail_trigger == stage.final_fail_trigger)
                {
                    state.action_timeline = core::timeline::create_timeline(_clock);
                    state.is_fail = true;
                }
                else
                {
                    state.current_fail_trigger += 1;
                    state.action_timeline = core::timeline::create_timeline(_clock);
                }
            }
        };

        static auto handle_success_trigger = [this](
            InputAction const& action,
            InputActionTrigger const& trigger,
            InputActionStage const& stage,
            InputActionState& state,
            void const* data
        ) noexcept
        {
            if (trigger.func(trigger.user_data, core::timeline::elapsed(state.action_timeline), data))
            {
                if (state.current_success_trigger == stage.final_success_trigger)
                {
                    state.current_stage += 1;
                    if (state.current_stage == core::pod::array::size(action.stages))
                    {
                        state.action_timeline = core::timeline::create_timeline(_clock);
                        state.is_success = true;
                    }
                    else // Move to the next action stage
                    {
                        InputActionStage const& next_stage = action.stages[state.current_stage];
                        state.action_timeline = core::timeline::create_timeline(_clock);
                        state.current_success_trigger = next_stage.initial_success_trigger;
                        state.current_fail_trigger = next_stage.initial_fail_trigger;
                        state.current_reset_trigger = next_stage.reset_trigger;
                    }
                }
                else
                {
                    state.current_success_trigger += 1;
                    state.action_timeline = core::timeline::create_timeline(_clock);
                }
            }
        };

        for (auto& state : _action_states)
        {
            InputAction const& action = *state.action_info;

            if (state.is_fail == false && state.is_success == false)
            {
                InputActionStage const& stage = action.stages[state.current_stage];

                {
                    InputActionTrigger const& fail_trigger = action.triggers[state.current_fail_trigger];

                    if (fail_trigger.event == ActionTriggerEvent::InputEvent)
                    {
                        for (auto const& input_event : frame.input_events())
                        {
                            handle_fail_trigger(fail_trigger, stage, state, &input_event);
                        }
                    }

                    if (fail_trigger.event == ActionTriggerEvent::ActionEvent)
                    {
                        for (auto const& action_state : _action_states)
                        {
                            if (action_state.is_success)
                            {
                                handle_fail_trigger(fail_trigger, stage, state, std::addressof(action_state.action_name));
                            }
                        }
                    }

                    if (fail_trigger.event == ActionTriggerEvent::FrameEvent)
                    {
                        handle_fail_trigger(fail_trigger, stage, state, &frame);
                    }
                }

                if (state.is_fail == false)
                {
                    InputActionTrigger const& success_trigger = action.triggers[state.current_success_trigger];

                    if (success_trigger.event == ActionTriggerEvent::InputEvent)
                    {
                        for (auto const& input_event : frame.input_events())
                        {
                            handle_success_trigger(action, success_trigger, stage, state, &input_event);
                        }
                    }

                    if (success_trigger.event == ActionTriggerEvent::ActionEvent)
                    {
                        for (auto const& action_state : _action_states)
                        {
                            if (action_state.is_success)
                            {
                                handle_success_trigger(action, success_trigger, stage, state, std::addressof(action_state.action_name));
                            }
                        }
                    }

                    if (success_trigger.event == ActionTriggerEvent::FrameEvent)
                    {
                        handle_success_trigger(action, success_trigger, stage, state, &frame);
                    }
                }

                if (state.is_success)
                {
                    core::pod::array::push_back(out_actions, action.name);
                }
            }

            if (state.is_fail || state.is_success)
            {
                InputActionTrigger const& reset_trigger = action.triggers[state.current_reset_trigger];

                if (reset_trigger.event == ActionTriggerEvent::InputEvent)
                {
                    for (auto const& input_event : frame.input_events())
                    {
                        handle_reset_trigger(reset_trigger, state, &input_event);
                    }
                }

                if (reset_trigger.event == ActionTriggerEvent::ActionEvent)
                {
                    for (auto const& action_state : _action_states)
                    {
                        if (action_state.is_success)
                        {
                            handle_reset_trigger(reset_trigger, state, std::addressof(action_state.action_name));
                        }
                    }
                }

                if (reset_trigger.event == ActionTriggerEvent::FrameEvent)
                {
                    handle_reset_trigger(reset_trigger, state, &frame);
                }
            }
        }
    }

    void InputActionsTracker::add_trigger_definition(
        core::stringid_arg_type name,
        ActionTriggerDefinition definition
    ) noexcept
    {
        auto const name_hash = core::hash(name);
        IS_ASSERT(
            core::pod::hash::has(_trigger_definitions, name_hash) == false,
            "Setting existing trigger definition {}"
        );

        core::pod::hash::set(_trigger_definitions, name_hash, std::move(definition));
    }

    auto InputActionsTracker::get_trigger_definition(
        core::stringid_arg_type name
    ) const noexcept -> ActionTriggerDefinition
    {
        static ActionTriggerDefinition invalid_trigger_definition{
            .event = ActionTriggerEvent::Invalid,
            .func = nullptr
        };
        return core::pod::hash::get(_trigger_definitions, core::hash(name), invalid_trigger_definition);
    }

} // namespace iceshard
