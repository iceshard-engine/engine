#include <ice/input_action_executor.hxx>
#include <ice/input_action_definitions.hxx>

namespace ice
{

    bool InputActionExecutor::execute_condition(
        ice::InputActionCondition condition,
        ice::InputActionSource const& val,
        ice::f32 param
    ) const noexcept
    {
        switch (condition)
        {
            using enum InputActionCondition;
        case Pressed: return val.event == InputActionSourceEvent::KeyPress;
        case Released: return val.event == InputActionSourceEvent::KeyRelease;
        case Trigger: return val.event == InputActionSourceEvent::Trigger;
        case Axis: return val.event == InputActionSourceEvent::Axis;
        case AxisDeadzone: return val.event == InputActionSourceEvent::AxisDeadzone;
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
            case Max: action_value = ice::min(action_value, param); break;
            case Div:
                ICE_ASSERT_CORE(param != 0.0f);
                action_value /= param;
                break;
            default:
                break;
        }
    }

} // namespace ice
