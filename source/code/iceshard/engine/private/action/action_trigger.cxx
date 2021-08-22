#include <ice/action/action_trigger.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_controller.hxx>

namespace ice::action
{

    namespace detail
    {

        bool trigger_success(ice::Shard const&, ice::Shard const&, ice::f32) noexcept
        {
            return true;
        }

        bool trigger_failure(ice::Shard const&, ice::Shard const&, ice::f32) noexcept
        {
            return false;
        }

        bool trigger_time_elapsed(ice::Shard const& user_shard, ice::Shard const&, ice::f32 stage_time_elapsed) noexcept
        {
            ice::f32 time_data;
            return ice::shard_inspect(user_shard, time_data) && time_data < stage_time_elapsed;
        }

        bool trigger_check_action(ice::Shard const& user_shard, ice::Shard const& event_shard, ice::f32) noexcept
        {
            ice::StringID_Hash expected_action;
            ice::StringID_Hash incoming_action;

            if (ice::shard_inspect(user_shard, expected_action) && ice::shard_inspect(event_shard, incoming_action))
            {
                return expected_action == incoming_action;
            }
            return false;
        }

        bool trigger_input_button(ice::Shard const& user_shard, ice::Shard const& event_shard, ice::f32) noexcept
        {
            ice::input::InputEvent expected_input;
            ice::input::InputEvent incoming_input;

            if (ice::shard_inspect(user_shard, expected_input) && ice::shard_inspect(event_shard, incoming_input) && expected_input.value_type == incoming_input.value_type)
            {
                return expected_input.identifier == incoming_input.identifier &&
                    (expected_input.value.button.value_i32 & incoming_input.value.button.value_i32) != 0;
            }
            return false;
        }

        bool trigger_input_axis_above(ice::Shard const& user_shard, ice::Shard const& event_shard, ice::f32) noexcept
        {
            ice::input::InputEvent expected_input;
            ice::input::InputEvent incoming_input;

            if (ice::shard_inspect(user_shard, expected_input) && ice::shard_inspect(event_shard, incoming_input) && expected_input.value_type == incoming_input.value_type)
            {
                return expected_input.identifier == incoming_input.identifier &&
                    (expected_input.value.axis.value_f32 < incoming_input.value.axis.value_f32) != 0;
            }
            return false;
        }

        bool trigger_input_axis_below(ice::Shard const& user_shard, ice::Shard const& event_shard, ice::f32) noexcept
        {
            ice::input::InputEvent expected_input;
            ice::input::InputEvent incoming_input;

            if (ice::shard_inspect(user_shard, expected_input) && ice::shard_inspect(event_shard, incoming_input) && expected_input.value_type == incoming_input.value_type)
            {
                return expected_input.identifier == incoming_input.identifier &&
                    (expected_input.value.axis.value_f32 > incoming_input.value.axis.value_f32) != 0;
            }
            return false;
        }

    } // namespace detail

    void setup_common_triggers(
        ice::action::ActionTriggerDatabase& database
    ) noexcept
    {
        database.add_trigger(
            "trigger.success"_sid,
            ActionTrigger{
                .trigger_shardid = ice::ShardID_Invalid,
                .trigger_handler = detail::trigger_success
            }
        );
        database.add_trigger(
            "trigger.failure"_sid,
            ActionTrigger{
                .trigger_shardid = ice::ShardID_Invalid,
                .trigger_handler = detail::trigger_failure
            }
        );
        database.add_trigger(
            "trigger.elapsed-time"_sid,
            ActionTrigger{
                .trigger_shardid = "event/frame/new-frame"_shardid,
                .trigger_handler = detail::trigger_time_elapsed
            }
        );
        database.add_trigger(
            "trigger.action-success"_sid,
            ActionTrigger{
                .trigger_shardid = "event/action/success"_shardid,
                .trigger_handler = detail::trigger_check_action
            }
        );
        database.add_trigger(
            "trigger.action-no-success"_sid,
            ActionTrigger{
                .trigger_shardid = "event/action/no-success"_shardid,
                .trigger_handler = detail::trigger_check_action
            }
        );
        database.add_trigger(
            "trigger.action-input-button"_sid,
            ActionTrigger{
                .trigger_shardid = "event/input/button"_shardid,
                .trigger_handler = detail::trigger_input_button
            }
        );
        database.add_trigger(
            "trigger.action-input-axis-above"_sid,
            ActionTrigger{
                .trigger_shardid = "event/input/axis-above"_shardid,
                .trigger_handler = detail::trigger_input_axis_above
            }
        );
        database.add_trigger(
            "trigger.action-input-axis-below"_sid,
            ActionTrigger{
                .trigger_shardid = "event/input/axis-below"_shardid,
                .trigger_handler = detail::trigger_input_axis_below
            }
        );

        //database.add_trigger_definition("trigger.action-success"_sid,
        //    ActionTriggerDefinition{
        //        .event = ActionTriggerEvent::ActionEvent,
        //        .func = detail::trigger_action_success
        //    }
        //);

        //database.add_trigger_definition("trigger.action-not-success"_sid,
        //    ActionTriggerDefinition{
        //        .event = ActionTriggerEvent::ActionEvent,
        //        .func = detail::trigger_action_not_success
        //    }
        //);

        //database.add_trigger_definition("trigger.button-pressed"_sid,
        //    ActionTriggerDefinition{
        //        .event = ActionTriggerEvent::InputEvent,
        //        .func = detail::trigger_button_pressed
        //    }
        //);

        //database.add_trigger_definition("trigger.button-released"_sid,
        //    ActionTriggerDefinition{
        //        .event = ActionTriggerEvent::InputEvent,
        //        .func = detail::trigger_button_released
        //    }
        //);

        //database.add_trigger_definition("trigger.button-clicked"_sid,
        //    ActionTriggerDefinition{
        //        .event = ActionTriggerEvent::InputEvent,
        //        .func = detail::trigger_button_clicked
        //    }
        //);

        //database.add_trigger_definition("trigger.button-hold"_sid,
        //    ActionTriggerDefinition{
        //        .event = ActionTriggerEvent::InputEvent,
        //        .func = detail::trigger_button_hold
        //    }
        //);
    }

} // namespace ice::action
