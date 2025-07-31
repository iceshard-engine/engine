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
        auto mod_from_dsl(arctic::String value) noexcept -> ice::input::KeyboardMod;
        auto mouse_from_dsl(arctic::String value) noexcept -> ice::input::MouseInput;

        auto datatype_from_dsl(arctic::Token token) noexcept -> ice::InputActionDataType;
        auto condition_from_dsl(arctic::Token token, bool action_condition) noexcept -> ice::InputActionCondition;
        auto step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep;
        auto modifier_from_dsl(arctic::Token token) noexcept -> ice::InputActionModifier;

    } // namespace detail

    InputActionDSLLayerBuilder::InputActionDSLLayerBuilder(
        ice::UniquePtr<ice::InputActionBuilder::Layer> builder
    ) noexcept
        : ActionInputParserEvents{ }
        , _builder{ ice::move(builder) }
    {
    }

    void InputActionDSLLayerBuilder::visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept
    {
        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Layer: {}", node.data().name);
        _builder->set_name(node.data().name);

        arctic::SyntaxNode<> child = node.child();
        while (child != false)
        {
            if (child.type() == ice::syntax::SyntaxEntity_LayerSource)
            {
                visit_source(child.to<ice::syntax::LayerSource>());
            }
            else if (child.type() == ice::syntax::SyntaxEntity_LayerAction)
            {
                visit_layer(child.to<ice::syntax::LayerAction>());
            }
            child = child.sibling();
        }
    }

    void InputActionDSLLayerBuilder::visit_source(arctic::SyntaxNode<ice::syntax::LayerSource> node) noexcept
    {
        ice::InputActionSourceType type = InputActionSourceType::Key;
        switch(node.data().type.type)
        {
        case ice::grammar::UCT_InputTypeButton: type = InputActionSourceType::Button; break;
        case ice::grammar::UCT_InputTypeAxis1D: type = InputActionSourceType::Axis1d; break;
        case ice::grammar::UCT_InputTypeAxis2D: type = InputActionSourceType::Axis2d; break;
        }

        ice::InputActionBuilder::Source source = _builder->define_source(node.data().name, type);
        arctic::SyntaxNode binding = node.child<ice::syntax::LayerInputBinding>();
        while (binding)
        {
            using ice::input::DeviceType;
            using ice::input::KeyboardKey;
            using ice::input::KeyboardMod;
            using ice::input::MouseInput;

            if (binding.data().device.type == ice::grammar::UCT_InputBindingKeyboard)
            {
                ICE_ASSERT_CORE(type == InputActionSourceType::Key || type == InputActionSourceType::Button);

                //ice::u64 const valsize = binding.data().device.value.size();
                //if (binding.data().device.value[valsize / 2] == 'm')
                //{
                //    KeyboardMod const mod = detail::mod_from_dsl(binding.data().source);
                //    ICE_ASSERT_CORE(mod != KeyboardMod::None);
                //    source.add_keymod(mod);
                //}
                //else
                {
                    KeyboardKey const key = detail::key_from_dsl(binding.data().source);
                    ICE_ASSERT_CORE(key != KeyboardKey::Unknown);
                    source.add_key(key);
                }
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

            // TODO: We need to fix building layers with sources having multiple inputs, before enabling this.
            binding = binding.sibling<ice::syntax::LayerInputBinding>();
        }

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Source: {}", node.data().name);
    }

    void InputActionDSLLayerBuilder::visit_layer(arctic::SyntaxNode<ice::syntax::LayerAction> node) noexcept
    {
        ice::syntax::LayerAction const& action_info = node.data();
        ice::InputActionDataType const action_datatype = detail::datatype_from_dsl(action_info.type);

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Action: {}", action_info.name);
        ice::InputActionBuilder::Action action = _builder->define_action(action_info.name, action_datatype);
        ice::InputActionBehavior behavior = InputActionBehavior::Default;
        if (action_info.flag_once)
        {
            behavior = InputActionBehavior::ActiveOnce;
        }
        else if (action_info.flag_toggled)
        {
            behavior = InputActionBehavior::Toggled;
        }
        //else if (action_info.flag_accumulated)
        //{
        //    behavior = InputActionBehavior::Accumulated;
        //}
        action.set_behavior(behavior);

        arctic::SyntaxNode<> child = node.child();
        while(child != false)
        {
            if (child.type() == ice::syntax::SyntaxEntity_LayerActionCondition)
            {
                // Returns the final node of the series. The next sibling is either a: modifier, condition_series or action
                child = visit_cond(action.add_condition_series(), child.to<ice::syntax::LayerActionWhen>());
            }
            else if (child.type() == ice::syntax::SyntaxEntity_LayerActionModifier)
            {
                visit_mod(action, child.to<ice::syntax::LayerActionModifier>());
            }
            child = child.sibling();
        }
    }

    auto InputActionDSLLayerBuilder::visit_cond(
        ice::InputActionBuilder::ConditionSeries series,
        arctic::SyntaxNode<ice::syntax::LayerActionWhen> node
    ) noexcept -> arctic::SyntaxNode<>
    {
        using enum ice::InputActionCondition;

        arctic::SyntaxNode<ice::syntax::LayerActionWhen> cond_node = node;
        ICE_ASSERT_CORE(cond_node.data().type.type == grammar::UCT_When);

        ICE_LOG(LogSeverity::Debug, LogTag::Tool, "New condition series");
        do
        {
            node = cond_node; // Set the node to the current cond_node, necessary to return a valid node
            ice::syntax::LayerActionWhen const& cond = cond_node.data();
            ICE_LOG(LogSeverity::Debug, LogTag::Tool, "- {} {}.{} {} {}", cond.type.value, cond.source_type.value, cond.source_name, cond.condition.value, cond.param.value);

            bool const from_action = cond.source_type.type == grammar::UCT_Action
                || (cond.source_type.type == arctic::TokenType::Invalid && cond.source_name.empty()); // "self"
            ice::InputActionCondition const condition = detail::condition_from_dsl(cond.condition, from_action);

            ice::InputActionConditionFlags flags = InputActionConditionFlags::None;
            if (cond_node.child<syntax::LayerActionStep>())
            {
                flags |= InputActionConditionFlags::RunSteps;
            }
            if (cond.flag_series)
            {
                flags |= InputActionConditionFlags::SeriesCheck;
            }

            if (cond.type.type == grammar::UCT_WhenAnd)
            {
                flags |= InputActionConditionFlags::SeriesAnd;
            }
            // If we are When or WhenOr, we default to 'SeriesOr'
            else
            {
                flags |= InputActionConditionFlags::SeriesOr;
            }

            ice::f32 param = 0.0f;
            bool const is_param_condition = condition == Equal || condition == NotEqual
                || condition == Greater || condition == GreaterOrEqual
                || condition == Lower || condition == LowerOrEqual;
            if (is_param_condition)
            {
                ice::from_chars(cond.param.value, param);
            }

            // Adds the new condition
            series.add_condition(
                cond.source_name, condition, flags, param, from_action
            );

            // Move over all steps defined for this condition
            arctic::SyntaxNode<> steps = cond_node.child();
            while (steps != false)
            {
                if (steps.type() == ice::syntax::SyntaxEntity_LayerActionStep)
                {
                    visit_step(series, steps.to<ice::syntax::LayerActionStep>());
                }
                steps = steps.sibling();
            }

            cond_node = cond_node.sibling<ice::syntax::LayerActionWhen>();
        } while (cond_node && cond_node.data().type.type != grammar::UCT_When);

        // TODO: Check for '.continue' flag to skip setting 'Final' flag here
        //if (/* DOES NOT HAVE '.continue' flag */)
        {
            series.set_finished();
        }

        return node;
    }

    void InputActionDSLLayerBuilder::visit_step(
        ice::InputActionBuilder::ConditionSeries& condition_series,
        arctic::SyntaxNode<ice::syntax::LayerActionStep> node
    ) noexcept
    {
        ice::syntax::LayerActionStep const& info = node.data();
        ice::InputActionStep const step = detail::step_from_dsl(info.step);
        if (step == InputActionStep::Set || step == InputActionStep::Add || step == InputActionStep::Sub)
        {
            condition_series.add_step(info.source, step, info.destination);
        }
        else
        {
            condition_series.add_step(step);
        }
    }

    void InputActionDSLLayerBuilder::visit_mod(
        ice::InputActionBuilder::Action& action,
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
        using ice::input::KeyboardMod;

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
        else if (ice::compare(value, "space") == CompareResult::Equal)
        {
            result = KeyboardKey::Space;
        }
        else if (ice::compare(value, "up") == CompareResult::Equal)
        {
            result = KeyboardKey::Up;
        }
        else if (ice::compare(value, "down") == CompareResult::Equal)
        {
            result = KeyboardKey::Down;
        }
        else if (ice::compare(value, "left") == CompareResult::Equal)
        {
            result = KeyboardKey::Left;
        }
        else if (ice::compare(value, "right") == CompareResult::Equal)
        {
            result = KeyboardKey::Right;
        }
        else if (ice::compare(value, "mode") == CompareResult::Equal)
        {
            return KeyboardKey::KeyMode;
        }
        else if (ice::compare(value, "numlock") == CompareResult::Equal)
        {
            return KeyboardKey::NumPadNumlockClear;
        }
        else if (ice::compare(value, "capslock") == CompareResult::Equal)
        {
            return KeyboardKey::KeyCapsLock;
        }
        else if (value.size() >= 4 && (value[0] == 'l' || value[0] == 'r'))
        {
            ice::u8 const mod_left = value[0] == 'l';
            value = value.substr(1);

            if (ice::compare(value, "shift") == CompareResult::Equal)
            {
                result = mod_left ? KeyboardKey::KeyLeftShift : KeyboardKey::KeyRightShift;
            }
            else if (ice::compare(value, "ctrl") == CompareResult::Equal)
            {
                result = mod_left ? KeyboardKey::KeyLeftCtrl : KeyboardKey::KeyRightCtrl;
            }
            else if (ice::compare(value, "alt") == CompareResult::Equal)
            {
                result = mod_left ? KeyboardKey::KeyLeftAlt : KeyboardKey::KeyRightAlt;
            }
            else if (ice::compare(value, "gui") == CompareResult::Equal)
            {
                result = mod_left ? KeyboardKey::KeyLeftGui : KeyboardKey::KeyRightGui;
            }
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
        case grammar::UCT_StepToggle: return InputActionStep::Toggle;
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
