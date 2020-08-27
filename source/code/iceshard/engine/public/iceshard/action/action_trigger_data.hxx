#pragma once
#include <iceshard/action/action_trigger.hxx>
#include <iceshard/input/input_event.hxx>

namespace iceshard::trigger
{

    struct TriggerData_Input
    {
        iceshard::input::InputID input;
        uint32_t unused;
    };

    static_assert(sizeof(TriggerData_Input) == sizeof(void*));

    struct TriggerData_Time
    {
        float delay;
    };

    struct TriggerData_Action
    {
        core::stringid_hash_type expected_action;
    };

    struct TriggerEvent_ActionState
    {
        core::stringid_hash_type event_action;
        uint32_t state; // 0 - reset, 1 - success, 2 - failed
    };

    auto create_trigger_userdata(
        iceshard::input::InputID input
    ) noexcept -> void*;

    auto create_trigger_userdata(float delay) noexcept -> void*;

    auto create_trigger_userdata(core::stringid_arg_type action) noexcept -> void*;

    template<typename T>
    auto get_trigger_userdata(void* userdata) noexcept -> T;

    template<>
    auto get_trigger_userdata<TriggerData_Input>(void* userdata) noexcept -> TriggerData_Input;

    template<>
    auto get_trigger_userdata<TriggerData_Time>(void* userdata) noexcept -> TriggerData_Time;

    template<>
    auto get_trigger_userdata<TriggerData_Action>(void* userdata) noexcept -> TriggerData_Action;

} // namespace iceshard
