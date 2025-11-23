/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/input_action_executor.hxx>
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_info.hxx>
#include <ice/clock.hxx>

namespace ice
{

    auto constant_at(ice::InputActionConstant id, ice::Span<ice::f32 const> values) noexcept -> ice::f32
    {
        return values[ice::u32(id)];
    }

    bool InputActionExecutor::execute_condition(
        ice::InputActionCondition condition,
        ice::InputActionRuntime const& action,
        ice::f32 param
    ) const noexcept
    {
        switch (condition)
        {
            using enum InputActionCondition;
        case ActionToggleActive: return action.toggle_enabled;
        case ActionToggleInactive: return action.toggle_enabled == false;
        case AlwaysTrue: return true;
        default: ICE_ASSERT_CORE(false); return false;
        }
    }

    void InputActionExecutor::prepare_constants(
        ice::InputActionLayer const& layer
    ) noexcept
    {
        layer.load_constants(_constants);
    }

    bool InputActionExecutor::execute_condition(
        ice::InputActionCondition condition,
        ice::InputActionSource const& val,
        ice::f32 param
    ) const noexcept
    {
        ice::f32 const deadzone_cutoff = constant_at(InputActionConstant::ControllerAxisDeadzone, _constants);

        switch (condition)
        {
            using enum InputActionCondition;
            // Source conditions
        case Active: return val.event != InputActionSourceEvent::None;
        case Pressed: return val.event == InputActionSourceEvent::KeyPress;
        case Released: return val.event == InputActionSourceEvent::KeyRelease;
        case Trigger: return val.event == InputActionSourceEvent::Trigger;
            // Deadzone comparisons
        case Axis: return val.value >= deadzone_cutoff;
        case AxisDeadzone: return val.value < deadzone_cutoff;
            // Source and Parameter comparisons
        case Equal: return val.value == param;
        case NotEqual: return val.value != param;
        case Greater: return val.value > param;
        case GreaterOrEqual: return val.value >= param;
        case Lower: return val.value < param;
        case LowerOrEqual: return val.value <= param;
            // Action conditions

        default:
            break;
        }
        ICE_ASSERT_CORE(false);
        return false;
    }

    void InputActionExecutor::execute_step(
        ice::InputActionStep step,
        ice::InputActionRuntime& runtime
    ) const noexcept
    {
        switch (step)
        {
            using enum InputActionStep;
        case Activate:
            runtime.state = runtime.state * 2 + 1;
            runtime.active = true;
            break;
        case Deactivate:
            runtime.state = 0;
            runtime.active = false;
            runtime.toggle_enabled = false;
            break;
        case Toggle:
            runtime.state = runtime.state * 2 + 1;
            if (runtime.state == 1)
            {
                runtime.toggle_enabled = !runtime.toggle_enabled;
            }
            break;
        case Reset:
            runtime.value = ice::vec2f{};
            runtime.raw_value = ice::vec3f{};
            break;
        case Time:
            runtime.raw_value.x = (float) Ts(ice::clock::elapsed(runtime.timestamp, ice::clock::now())).value;
            break;
        default:
            break;
        }
    }

    void InputActionExecutor::execute_step(
        ice::InputActionStep step,
        ice::InputActionSource const& source_value,
        ice::f32& dst
    ) const noexcept
    {
        ice::f32 const src = source_value.value;

        switch (step)
        {
            using enum InputActionStep;
        case Set: dst = src; break;
        case Add: dst += src; break;
        case Sub: dst -= src; break;
        default:
            break;
        }
    }

    void InputActionExecutor::execute_modifier(
        ice::InputActionModifier modifier,
        ice::f32& action_value,
        ice::f32 param
    ) const noexcept
    {
        switch(modifier)
        {
            using enum InputActionModifier;
        case Add: action_value += param;
        case Sub: action_value -= param;
        case Mul: action_value *= param;
        case Div:
            ICE_ASSERT_CORE(param != 0.0f);
            action_value /= param;
            break;
        case MaxOf: action_value = ice::max(action_value, param); break;
        case MinOf: action_value = ice::min(action_value, param); break;
        default:
            break;
        }
    }

} // namespace ice
