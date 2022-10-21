#include <ice/action/action_trigger.hxx>
#include <ice/engine_shards.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_controller.hxx>
#include <ice/container/hashmap.hxx>

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

    class SimpleTriggerDatabase : public ice::action::ActionTriggerDatabase
    {
    public:
        SimpleTriggerDatabase(ice::Allocator& alloc) noexcept;

        void add_trigger(
            ice::StringID_Arg name,
            ice::action::ActionTriggerDefinition trigger_info
        ) noexcept override;

        auto get_trigger(
            ice::StringID_Arg name
        ) const noexcept -> ice::action::ActionTriggerDefinition override;

    private:
        ice::HashMap<ice::action::ActionTriggerDefinition> _triggers;
    };

    SimpleTriggerDatabase::SimpleTriggerDatabase(ice::Allocator& alloc) noexcept
        : _triggers{ alloc }
    {
    }

    void SimpleTriggerDatabase::add_trigger(ice::StringID_Arg name, ice::action::ActionTriggerDefinition trigger_info) noexcept
    {
        ice::hashmap::set(_triggers, ice::hash(name), trigger_info);
    }

    auto SimpleTriggerDatabase::get_trigger(ice::StringID_Arg name) const noexcept -> ice::action::ActionTriggerDefinition
    {
        return ice::hashmap::get(_triggers, ice::hash(name), ActionTriggerDefinition{ ice::Shard_Invalid.id });
    }

    auto create_trigger_database(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::action::ActionTriggerDatabase>
    {
        return ice::make_unique<SimpleTriggerDatabase>(alloc, alloc);
    }

    void setup_common_triggers(
        ice::action::ActionTriggerDatabase& database
    ) noexcept
    {
        database.add_trigger(
            "trigger.success"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = ice::shardid(ice::Shard_FrameTick),
                .trigger_handler = detail::trigger_success
            }
        );
        database.add_trigger(
            "trigger.failure"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = ice::shardid(ice::Shard_FrameTick),
                .trigger_handler = detail::trigger_failure
            }
        );
        database.add_trigger(
            "trigger.elapsed-time"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = ice::shardid(ice::Shard_FrameTick),
                .trigger_handler = detail::trigger_time_elapsed
            }
        );
        database.add_trigger(
            "trigger.action-success"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = "event/action/success"_shardid,
                .trigger_handler = detail::trigger_check_action
            }
        );
        database.add_trigger(
            "trigger.action-no-success"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = "event/action/no-success"_shardid,
                .trigger_handler = detail::trigger_check_action
            }
        );
        database.add_trigger(
            "trigger.action-input-button"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = ice::shardid(ice::Shard_InputEventButton),
                .trigger_handler = detail::trigger_input_button
            }
        );
        database.add_trigger(
            "trigger.action-input-axis-above"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = ice::shardid(ice::Shard_InputEventAxis),
                .trigger_handler = detail::trigger_input_axis_above
            }
        );
        database.add_trigger(
            "trigger.action-input-axis-below"_sid,
            ActionTriggerDefinition{
                .trigger_shardid = ice::shardid(ice::Shard_InputEventAxis),
                .trigger_handler = detail::trigger_input_axis_below
            }
        );

    }

} // namespace ice::action
