#include <iceshard/action/action_trigger.hxx>
#include <iceshard/action/action_trigger_data.hxx>
#include <iceshard/input/device/input_device.hxx>

namespace iceshard
{

    namespace detail
    {

        bool trigger_always(void*, float, void const*) noexcept
        {
            return true;
        }

        bool trigger_never(void*, float, void const*) noexcept
        {
            return false;
        }

        bool trigger_time_elapsed(void* userdata, float current_elapsed, void const*) noexcept
        {
            auto const time_data = trigger::get_trigger_userdata<trigger::TriggerData_Time>(userdata);
            return time_data.delay < current_elapsed;
        }

        bool trigger_action_success(void* userdata, float current_elapsed, void const* data) noexcept
        {
            auto const action_data = trigger::get_trigger_userdata<trigger::TriggerData_Action>(userdata);
            auto const& action_event = *reinterpret_cast<core::stringid_type const*>(data);
            return action_event.hash_value == action_data.expected_action;
        }

        bool trigger_button_pressed(void* userdata, float current_elapsed, void const* data) noexcept
        {
            auto const input_data = trigger::get_trigger_userdata<trigger::TriggerData_Input>(userdata);
            auto const* event_data = reinterpret_cast<input::InputEvent const*>(data);
            return input_data.input == event_data->identifier
                && event_data->value.button.state.pressed;
        }

        bool trigger_button_released(void* userdata, float current_elapsed, void const* data) noexcept
        {
            auto const input_data = trigger::get_trigger_userdata<trigger::TriggerData_Input>(userdata);
            auto const* event_data = reinterpret_cast<input::InputEvent const*>(data);
            return input_data.input == event_data->identifier
                && event_data->value.button.state.released;
        }

        bool trigger_button_clicked(void* userdata, float current_elapsed, void const* data) noexcept
        {
            auto const input_data = trigger::get_trigger_userdata<trigger::TriggerData_Input>(userdata);
            auto const* event_data = reinterpret_cast<input::InputEvent const*>(data);
            return input_data.input == event_data->identifier
                && event_data->value.button.state.clicked;
        }

        bool trigger_button_hold(void* userdata, float current_elapsed, void const* data) noexcept
        {
            auto const input_data = trigger::get_trigger_userdata<trigger::TriggerData_Input>(userdata);
            auto const* event_data = reinterpret_cast<input::InputEvent const*>(data);
            return input_data.input == event_data->identifier
                && event_data->value.button.state.hold;
        }

    } // namespace detail

    void register_common_triggers(ActionTriggerDatabase& database) noexcept
    {
        database.add_trigger_definition("trigger.always"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::FrameEvent,
                .func = detail::trigger_always
            }
        );

        database.add_trigger_definition("trigger.never"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::FrameEvent,
                .func = detail::trigger_never
            }
        );

        database.add_trigger_definition("trigger.elapsed-time"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::FrameEvent,
                .func = detail::trigger_time_elapsed
            }
        );

        database.add_trigger_definition("trigger.action-success"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::ActionEvent,
                .func = detail::trigger_action_success
            }
        );

        database.add_trigger_definition("trigger.button-pressed"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::InputEvent,
                .func = detail::trigger_button_pressed
            }
        );

        database.add_trigger_definition("trigger.button-released"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::InputEvent,
                .func = detail::trigger_button_released
            }
        );

        database.add_trigger_definition("trigger.button-clicked"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::InputEvent,
                .func = detail::trigger_button_clicked
            }
        );

        database.add_trigger_definition("trigger.button-hold"_sid,
            ActionTriggerDefinition{
                .event = ActionTriggerEvent::InputEvent,
                .func = detail::trigger_button_hold
            }
        );
    }

} // namespace iceshard
