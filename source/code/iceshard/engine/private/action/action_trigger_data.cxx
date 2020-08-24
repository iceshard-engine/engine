#include <iceshard/action/action_trigger_data.hxx>

namespace iceshard::trigger
{

    auto create_trigger_userdata(
        iceshard::input::InputID input
    ) noexcept -> void*
    {
        union
        {
            TriggerData_Input input_var;
            void* pointer_var;
        } helper{ .input_var = TriggerData_Input{ .input = input } };

        static_assert(sizeof(helper) == sizeof(void*));
        return helper.pointer_var;
    }

    auto create_trigger_userdata(float delay) noexcept -> void*
    {
        union
        {
            TriggerData_Time time_var;
            void* pointer_var;
        } helper{ .time_var = TriggerData_Time{ .delay = delay } };

        static_assert(sizeof(helper) == sizeof(void*));
        return helper.pointer_var;
    }

    auto create_trigger_userdata(core::stringid_arg_type action) noexcept -> void*
    {
        union
        {
            TriggerData_Action action_var;
            void* pointer_var;
        } helper{ .action_var = TriggerData_Action{ .expected_action = action.hash_value } };

        static_assert(sizeof(helper) == sizeof(void*));
        return helper.pointer_var;
    }

    template<>
    auto get_trigger_userdata(void* userdata) noexcept -> TriggerData_Input
    {
        union
        {
            void* pointer_var;
            TriggerData_Input input_var;
        } helper{ .pointer_var = userdata };

        static_assert(sizeof(helper) == sizeof(void*));
        return helper.input_var;
    }

    template<>
    auto get_trigger_userdata(void* userdata) noexcept -> TriggerData_Time
    {
        union
        {
            void* pointer_var;
            TriggerData_Time time_var;
        } helper{ .pointer_var = userdata };

        static_assert(sizeof(helper) == sizeof(void*));
        return helper.time_var;
    }

    template<>
    auto get_trigger_userdata(void* userdata) noexcept -> TriggerData_Action
    {
        union
        {
            void* pointer_var;
            TriggerData_Action action_var;
        } helper{ .pointer_var = userdata };

        static_assert(sizeof(helper) == sizeof(void*));
        return helper.action_var;
    }

} // namespace iceshard
