#include "iceshard_input_actions.hxx"

#include <iceshard/frame.hxx>
#include <iceshard/input/input_event.hxx>

#include <core/message/operations.hxx>
#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{

    //static constexpr float default_input_action_reset_time = 0.25f; // 0.25s

    //bool default_success_trigger_func(void* userdata, core::Timeline const&, void const* event_data) noexcept
    //{
    //    input::InputID const input_id = static_cast<input::InputID>(reinterpret_cast<uintptr_t>(userdata));
    //    input::InputEvent const& input = *reinterpret_cast<input::InputEvent const*>(event_data);
    //    return input.identifier == input_id && input.value.button.state.pressed;
    //}

    //bool default_reset_trigger_func(void*, core::Timeline const& timeline, const void*) noexcept
    //{
    //    return core::timeline::elapsed(timeline) > default_input_action_reset_time;
    //}

    //bool immediate_reset_trigger_func(void*, core::Timeline const&, const void*) noexcept
    //{
    //    return true;
    //}

    //static constexpr InputActionTrigger default_success_trigger
    //{
    //    .event = InputActionTriggerEvent::InputEvent,
    //    .handler_func = default_success_trigger_func,
    //};

    //static constexpr InputActionTrigger default_fail_trigger
    //{
    //    .event = InputActionTriggerEvent::Unused,
    //    .handler_func = nullptr,
    //};

    //static constexpr InputActionTrigger default_reset_trigger
    //{
    //    .event = InputActionTriggerEvent::TimeEvent,
    //    .handler_func = immediate_reset_trigger_func,
    //};

    //static constexpr InputActionStage default_input_stage =
    //{
    //    .initial_success_trigger = 0,
    //    .final_success_trigger = 0,
    //    .initial_fail_trigger = 0,
    //    .final_fail_trigger = 0,
    //    .reset_trigger = 0,
    //};

    //struct MoveInfo
    //{
    //    using HashType = core::pod::Hash<InputActionState*>;
    //    HashType* hash_target;
    //    HashType::Entry const* hash_entry;
    //};

    //using HandleTriggerFunc = bool(InputAction const&, InputActionStage const&, void const*, InputActionState&) noexcept;

    //bool handle_fail_trigger(
    //    InputAction const& action,
    //    InputActionStage const& stage,
    //    void const* event_data,
    //    InputActionState& state
    //) noexcept
    //{
    //    bool result = false;
    //    if (state.is_fail == false && state.is_success == false)
    //    {
    //        InputActionTrigger const& fail_trigger = action.fail_triggers[state.current_fail_trigger];
    //        if (fail_trigger.handler_func(fail_trigger.user_data, state.action_timeline, event_data))
    //        {
    //            if (state.current_fail_trigger == stage.final_fail_trigger)
    //            {
    //                state.is_fail = true;
    //                result = true;
    //                return true;
    //            }
    //            else
    //            {
    //                state.current_fail_trigger += 1;
    //            }
    //        }
    //    }
    //    return result;
    //}

    //bool handle_success_trigger(
    //    InputAction const& action,
    //    InputActionStage const& stage,
    //    void const* event_data,
    //    InputActionState& state
    //) noexcept
    //{
    //    bool result = false;
    //    if (state.is_fail == false && state.is_success == false)
    //    {
    //        InputActionTrigger const& success_trigger = action.success_triggers[state.current_success_trigger];
    //        if (success_trigger.handler_func(success_trigger.user_data, state.action_timeline, event_data))
    //        {
    //            if (state.current_success_trigger == stage.final_success_trigger)
    //            {
    //                state.current_stage += 1;
    //                if (state.current_stage == core::pod::array::size(action.stages))
    //                {
    //                    state.is_success = true;
    //                }
    //                else // Move to the next action stage
    //                {
    //                    InputActionStage const& next_stage = action.stages[state.current_stage];
    //                    state.current_success_trigger = next_stage.initial_success_trigger;
    //                    state.current_fail_trigger = next_stage.initial_fail_trigger;
    //                    state.current_reset_trigger = next_stage.reset_trigger;
    //                }
    //                result = true;
    //            }
    //            else
    //            {
    //                state.current_success_trigger += 1;
    //            }
    //        }
    //    }
    //    return result;
    //}

    //bool handle_reset_trigger(
    //    InputAction const& action,
    //    InputActionStage const& stage,
    //    void const* event_data,
    //    InputActionState& state
    //) noexcept
    //{
    //    bool result = false;
    //    if (state.is_fail || state.is_success)
    //    {
    //        InputActionTrigger const& reset_trigger = action.reset_triggers[state.current_reset_trigger];
    //        if (reset_trigger.handler_func(reset_trigger.user_data, state.action_timeline, event_data))
    //        {
    //            state.is_success = false;
    //            state.is_fail = false;
    //            state.current_stage = 0;
    //            state.current_success_trigger = 0;
    //            state.current_fail_trigger = 0;
    //            state.current_reset_trigger = 0;
    //            result = true;
    //        }
    //    }
    //    return result;
    //}

    //void handle_action_states(
    //    InputActionTriggerEvent event_type,
    //    const void* event_data,
    //    core::pod::Hash<InputActionState*>& action_states,
    //    core::pod::Array<MoveInfo>& entries_to_move,
    //    HandleTriggerFunc* trigger_handle
    //) noexcept
    //{
    //    auto const* entry = core::pod::multi_hash::find_first(
    //        action_states,
    //        core::hash(event_type)
    //    );

    //    while (entry != nullptr)
    //    {
    //        InputActionState* action_state = entry->value;
    //        InputAction const* action_definition = entry->value->action_info;

    //        InputActionStage const& stage = action_definition->stages[action_state->current_stage];

    //        if (trigger_handle(*action_definition, stage, event_data, *action_state))
    //        {
    //            core::pod::array::push_back(entries_to_move, MoveInfo{ &action_states, entry });
    //        }

    //        entry = core::pod::multi_hash::find_next(action_states, entry);
    //    }
    //}

    //void handle_fail_action_state_for_data(
    //    InputActionTriggerEvent event_type,
    //    const void* event_data,
    //    core::pod::Hash<InputActionState*> const& action_states,
    //    core::pod::Array<core::pod::Hash<InputActionState*>::Entry const*>& entries_to_move
    //) noexcept
    //{
    //    auto const* entry = core::pod::multi_hash::find_first(
    //        action_states,
    //        core::hash(event_type)
    //    );

    //    while (entry != nullptr)
    //    {
    //        InputActionState* action_state = entry->value;
    //        InputAction const* action_definition = entry->value->action_info;

    //        InputActionStage const& stage = action_definition->stages[action_state->current_stage];

    //        if (action_state->is_fail == false && action_state->is_success == false)
    //        {
    //            InputActionTrigger const& fail_trigger = action_definition->fail_triggers[action_state->current_fail_trigger];
    //            if (fail_trigger.handler_func(fail_trigger.user_data, action_state->action_timeline, event_data))
    //            {
    //                if (action_state->current_fail_trigger == stage.final_fail_trigger)
    //                {
    //                    action_state->is_fail = true;
    //                    core::pod::array::push_back(entries_to_move, entry);
    //                }
    //                else
    //                {
    //                    action_state->current_fail_trigger += 1;
    //                }
    //            }
    //        }

    //        entry = core::pod::multi_hash::find_next(action_states, entry);
    //    }
    //}

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

    //void InputActionsTracker::define_quick_action(core::stringid_type name, iceshard::input::InputID input) noexcept
    //{
    //    core::pod::Array<InputActionStage> stages{ _allocator };
    //    core::pod::array::reserve(stages, 1);
    //    core::pod::array::push_back(stages, default_input_stage);

    //    core::pod::Array<InputActionTrigger> fail_triggers{ _allocator };
    //    core::pod::array::reserve(fail_triggers, 1);
    //    core::pod::array::push_back(fail_triggers, default_fail_trigger);

    //    core::pod::Array<InputActionTrigger> success_triggers{ _allocator };
    //    auto success_trigger = default_success_trigger;
    //    success_trigger.user_data = reinterpret_cast<void*>(static_cast<uintptr_t>(input));
    //    core::pod::array::reserve(success_triggers, 1);
    //    core::pod::array::push_back(success_triggers, success_trigger);

    //    core::pod::Array<InputActionTrigger> reset_triggers{ _allocator };
    //    core::pod::array::reserve(reset_triggers, 1);
    //    core::pod::array::push_back(reset_triggers, default_reset_trigger);

    //    auto input_action = core::memory::make_unique<InputAction>(_allocator,
    //        InputAction{
    //            .name = name,
    //            .stages = std::move(stages),
    //            .success_triggers = std::move(success_triggers),
    //            .fail_triggers = std::move(fail_triggers),
    //            .reset_triggers = std::move(reset_triggers),
    //        }
    //    );

    //    define_action(std::move(input_action));
    //}

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
                        fmt::print("Next stage for {}\n", state.action_name);
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
