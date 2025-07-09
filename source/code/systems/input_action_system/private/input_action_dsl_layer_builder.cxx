#include "input_action_dsl_grammar.hxx"
#include "input_action_dsl_layer_builder.hxx"

#include <ice/string_utils.hxx>
#include <ice/input_action_info.hxx>
#include <ice/log.hxx>

namespace ice
{

    namespace detail
    {

        auto key_from_dsl(arctic::String value) noexcept -> ice::input::KeyboardKey;
        auto mouse_from_dsl(arctic::String value) noexcept -> ice::input::MouseInput;

        auto datatype_from_dsl(arctic::Token token) noexcept -> ice::InputActionDataType;
        auto condition_from_dsl(arctic::Token token, bool action_condition) noexcept -> ice::InputActionCondition;
        auto step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep;
        auto modifier_from_dsl(arctic::Token token) noexcept -> ice::InputActionModifier;

    } // namespace detail

    InputActionDSLLayerBuilder::InputActionDSLLayerBuilder(
        ice::UniquePtr<ice::InputActionLayerBuilder> builder
    ) noexcept
        : _builder{ ice::move(builder) }
    {
    }

    void InputActionDSLLayerBuilder::visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept
    {
        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Layer: {}", node.data().name);

        arctic::SyntaxNode<> child = node.child();
        while(child != false)
        {
            if (child.type() == ice::syntax::SyntaxEntity_LayerSource)
            {
                visit(child.to<ice::syntax::LayerSource>());
            }
            else if (child.type() == ice::syntax::SyntaxEntity_LayerAction)
            {
                visit(child.to<ice::syntax::LayerAction>());
            }
            child = child.sibling();
        }
    }

    void InputActionDSLLayerBuilder::visit(arctic::SyntaxNode<ice::syntax::LayerSource> node) noexcept
    {
        ice::InputActionSourceType type = InputActionSourceType::Key;
        switch(node.data().type.type)
        {
        case ice::grammar::UCT_InputTypeButton: type = InputActionSourceType::Button; break;
        case ice::grammar::UCT_InputTypeAxis1D: type = InputActionSourceType::Axis1d; break;
        case ice::grammar::UCT_InputTypeAxis2D: type = InputActionSourceType::Axis2d; break;
        }

        ice::InputActionLayerBuilder::SourceBuilder source = _builder->define_source(node.data().name, type);
        if (arctic::SyntaxNode const binding = node.child<ice::syntax::LayerInputBinding>(); binding)
        {
            using ice::input::DeviceType;
            using ice::input::KeyboardKey;
            using ice::input::MouseInput;

            if (binding.data().device.type == ice::grammar::UCT_InputBindingKeyboard)
            {
                ICE_ASSERT_CORE(type == InputActionSourceType::Key || type == InputActionSourceType::Button);

                KeyboardKey const key = detail::key_from_dsl(binding.data().source);
                ICE_ASSERT_CORE(key != KeyboardKey::Unknown);

                source.add_key(key);
            }
            else if (binding.data().device.type == ice::grammar::UCT_InputBindingMouse)
            {
                MouseInput const mouseinput = detail::mouse_from_dsl(binding.data().source);
                ICE_ASSERT_CORE(
                    (type == InputActionSourceType::Key || type == InputActionSourceType::Button)
                    || mouseinput == MouseInput::PositionX
                );

                if (type == InputActionSourceType::Axis2d)
                {
                    source.add_axis(mouseinput);
                }
                else
                {
                    source.add_button(mouseinput);
                }
            }
        }

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Source: {}", node.data().name);
    }

    void InputActionDSLLayerBuilder::visit(arctic::SyntaxNode<ice::syntax::LayerAction> node) noexcept
    {
        ice::syntax::LayerAction const& action_info = node.data();
        ice::InputActionDataType const action_datatype = detail::datatype_from_dsl(action_info.type);

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Action: {}", action_info.name);
        ice::InputActionLayerBuilder::ActionBuilder action = _builder->define_action(action_info.name, action_datatype);
        ice::InputActionBehavior behavior = InputActionBehavior::Default;
        if (action_info.flag_once)
        {
            behavior = InputActionBehavior::ActiveOnce;
        }
        else if (action_info.flag_toggled)
        {
            behavior = InputActionBehavior::Toggled;
        }
        else if (action_info.flag_accumulated)
        {
            behavior = InputActionBehavior::Accumulated;
        }
        action.set_behavior(behavior);

        arctic::SyntaxNode<> child = node.child();
        while(child != false)
        {
            if (child.type() == ice::syntax::SyntaxEntity_LayerActionCondition)
            {
                visit(action, child.to<ice::syntax::LayerActionWhen>());

                arctic::SyntaxNode<> steps = child.child();
                while (steps != false)
                {
                    if (steps.type() == ice::syntax::SyntaxEntity_LayerActionStep)
                    {
                        visit(action, steps.to<ice::syntax::LayerActionStep>());
                    }
                    steps = steps.sibling();
                }
            }
            else if (child.type() == ice::syntax::SyntaxEntity_LayerActionModifier)
            {
                visit(action, child.to<ice::syntax::LayerActionModifier>());
            }
            child = child.sibling();
        }
    }

    void InputActionDSLLayerBuilder::visit(
        ice::InputActionLayerBuilder::ActionBuilder& action,
        arctic::SyntaxNode<ice::syntax::LayerActionWhen> node
    ) noexcept
    {
        using enum ice::InputActionCondition;

        ice::syntax::LayerActionWhen const& cond = node.to<ice::syntax::LayerActionWhen>().data();
        ICE_LOG(LogSeverity::Debug, LogTag::Tool, "- {} {}.{} {} {}", cond.type.value, cond.source_type.value, cond.source_name, cond.condition.value, cond.param.value);

        bool const from_action = cond.source_type.type == grammar::UCT_Action
            || (cond.source_type.type == arctic::TokenType::Invalid && cond.source_name.empty()); // "self"
        ice::InputActionCondition const condition = detail::condition_from_dsl(cond.condition, from_action);
        ice::InputActionConditionFlags flags = InputActionConditionFlags::None;

        // No siblings or next sibling: "when" == "final" condition.
        if (auto sib = node.sibling<ice::syntax::LayerActionWhen>(); !sib || sib.data().type.type == grammar::UCT_When)
        {
            flags |= InputActionConditionFlags::Final;
        }
        if (node.child<syntax::LayerActionStep>()) // Only steps can introduce children.
        {
            flags |= InputActionConditionFlags::RunSteps;
        }
        if (cond.flag_series)
        {
            flags |= InputActionConditionFlags::SeriesCheck;
        }
        if (cond.type.type == grammar::UCT_WhenAnd) // Only steps can introduce children.
        {
            flags |= InputActionConditionFlags::SeriesAnd;
        }
        else // if (cond.type.type == grammar::UCT_WhenOr) // Only steps can introduce children.
        {
            flags |= InputActionConditionFlags::SeriesOr;
        }

        // TODO: SeriesCheck
        // TODO: non-Final

        ice::f32 param = 0.0f;
        bool const is_param_condition = condition == Equal || condition == NotEqual
            || condition == Greater || condition == GreaterOrEqual
            || condition == Lower || condition == LowerOrEqual;
        if (is_param_condition)
        {
            ice::from_chars(cond.param.value, param);
        }

        action.add_condition(cond.source_name, condition, flags, param, from_action);
    }

    void InputActionDSLLayerBuilder::visit(
        ice::InputActionLayerBuilder::ActionBuilder& action,
        arctic::SyntaxNode<ice::syntax::LayerActionStep> node
    ) noexcept
    {
        ice::syntax::LayerActionStep const& info = node.data();
        ice::InputActionStep const step = detail::step_from_dsl(info.step);
        if (step == InputActionStep::Set || step == InputActionStep::Add || step == InputActionStep::Sub)
        {
            action.add_step(info.source, step, info.destination);
        }
        else
        {
            action.add_step(step);
        }
    }

    void InputActionDSLLayerBuilder::visit(
        ice::InputActionLayerBuilder::ActionBuilder& action,
        arctic::SyntaxNode<ice::syntax::LayerActionModifier> node
    ) noexcept
    {
        ice::syntax::LayerActionModifier const& info = node.data();
        ice::InputActionModifier const modifier = detail::modifier_from_dsl(info.operation);

        ice::f32 param_value = 0.0f;
        if (ice::from_chars(info.param, param_value) == false)
        {
            ICE_ASSERT_CORE(false);
        }

        action.add_modifier(modifier, param_value, info.component);
    }

    auto detail::key_from_dsl(arctic::String value) noexcept -> ice::input::KeyboardKey
    {
        using ice::input::KeyboardKey;

        KeyboardKey result = KeyboardKey::Unknown;
        if (value.size() == 1)
        {
            if (value[0] >= 'a')
            {
                ICE_ASSERT_CORE(value[0] <= 'z');
                char const key_diff = value[0] - 'a';

                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::KeyA) + key_diff);
            }
            else if (value[0] >= 'A')
            {
                ICE_ASSERT_CORE(value[0] <= 'Z');
                char const key_diff = value[0] - 'A';

                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::KeyA) + key_diff);
            }
            else if (value[0] >= '0')
            {
                ICE_ASSERT_CORE(value[0] <= '9');
                char const key_diff = value[0] - '0';

                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::Key0) + key_diff);
            }
        }
        else if (strnicmp(value.data(), "space", 5) == 0)
        {
            result = KeyboardKey::Space;
        }
        else if (value == "up")
        {
            result = KeyboardKey::Up;
        }
        else if (value == "down")
        {
            result = KeyboardKey::Down;
        }
        else if (value == "left")
        {
            result = KeyboardKey::Left;
        }
        else if (value == "right")
        {
            result = KeyboardKey::Right;
        }
        return result;
    }

    auto detail::mouse_from_dsl(arctic::String value) noexcept -> ice::input::MouseInput
    {
        using ice::input::MouseInput;
        MouseInput result = MouseInput::Unknown;
        if (value == "lbutton")
        {
            result = MouseInput::ButtonLeft;
        }
        else if (value == "rbutton")
        {
            result = MouseInput::ButtonRight;
        }
        else if (value == "mbutton")
        {
            result = MouseInput::ButtonMiddle;
        }
        else if (value == "pos" || value == "position")
        {
            result = MouseInput::PositionX;
        }
        return result;
    }

    auto detail::datatype_from_dsl(arctic::Token token) noexcept -> ice::InputActionDataType
    {
        switch (token.type)
        {
            // Action conditions
        case grammar::UCT_ActionTypeBool: return InputActionDataType::Bool;
        case grammar::UCT_ActionTypeFloat1: return InputActionDataType::Float1;
        case grammar::UCT_ActionTypeFloat2: return InputActionDataType::Float2;
        case grammar::UCT_ActionTypeObject: return InputActionDataType::ActionObject;
        default: ICE_ASSERT_CORE(false); return InputActionDataType::Invalid;
        }
    }

    auto detail::condition_from_dsl(arctic::Token token, bool action_condition) noexcept -> ice::InputActionCondition
    {
        switch(token.type)
        {
            // Action conditions
        case grammar::UCT_WhenPressed: return InputActionCondition::Pressed;
        case grammar::UCT_WhenReleased: return InputActionCondition::Released;
        case grammar::UCT_WhenActive: return action_condition ? InputActionCondition::ActionToggleActive : InputActionCondition::Active;
        case grammar::UCT_WhenInactive: return InputActionCondition::ActionToggleInactive;
            // Arithmetic conditions
        case arctic::TokenType::OP_Equal: return InputActionCondition::Equal;
        case arctic::TokenType::OP_NotEqual: return InputActionCondition::NotEqual; // TODO
        case arctic::TokenType::OP_Greater: return InputActionCondition::Greater;
        case arctic::TokenType::OP_GreaterOrEqual: return InputActionCondition::GreaterOrEqual;
        case arctic::TokenType::OP_Less: return InputActionCondition::Lower;
        case arctic::TokenType::OP_LessOrEqual: return InputActionCondition::LowerOrEqual;
            // Special conditions
        case arctic::TokenType::KW_True: return InputActionCondition::AlwaysTrue;
        default: ICE_ASSERT_CORE(false); return InputActionCondition::Invalid;
        }
    }

    auto detail::step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep
    {
        switch(token.type)
        {
        case grammar::UCT_StepActivate: return InputActionStep::Activate;
        case grammar::UCT_StepDeactivate: return InputActionStep::Deactivate;
        case grammar::UCT_StepReset: return InputActionStep::Reset;
        case grammar::UCT_StepTime: return InputActionStep::Time;
            // Arithmetic steps
        case arctic::TokenType::OP_Assign: return InputActionStep::Set;
        case arctic::TokenType::OP_Plus: return InputActionStep::Add;
        case arctic::TokenType::OP_Minus: return InputActionStep::Sub;
        default: ICE_ASSERT_CORE(false); break;
        }
        return InputActionStep::Invalid;
    }

    auto detail::modifier_from_dsl(arctic::Token token) noexcept -> ice::InputActionModifier
    {
        switch (token.type)
        {
        case grammar::UCT_ModifierOpMax: return InputActionModifier::Max;
        case arctic::TokenType::OP_Div: return InputActionModifier::Div;
            // Not implemented yet
        case grammar::UCT_ModifierOpMin:
        case arctic::TokenType::OP_Mul:
        case arctic::TokenType::OP_Plus:
        case arctic::TokenType::OP_Minus:
        default: ICE_ASSERT_CORE("Not Implemented!" && false); return InputActionModifier::Invalid;
        }
    }

} // namespace ice
