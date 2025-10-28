#include "input_action_script_grammar.hxx"
#include "input_action_script_parser.hxx"

#include <ice/string_utils.hxx>
#include <ice/input_action_info.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/log.hxx>

namespace ice
{

    using TokenType = ice::asl::TokenType;

    namespace detail
    {

        auto key_from_dsl(arctic::String value) noexcept -> ice::input::KeyboardKey;
        auto mouse_from_dsl(arctic::String value) noexcept -> ice::input::MouseInput;

        auto datatype_from_dsl(arctic::Token token) noexcept -> ice::InputActionDataType;
        auto condition_from_dsl(arctic::Token token, bool action_condition) noexcept -> ice::InputActionCondition;
        auto step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep;
        auto modifier_from_dsl(arctic::Token token) noexcept -> ice::InputActionModifier;

    } // namespace detail

    InputActionScriptParser::InputActionScriptParser(ice::Allocator& alloc) noexcept
        : ActionInputParserEvents{ }
        , _allocator{ alloc }
    { }

    void InputActionScriptParser::visit(arctic::SyntaxNode<ice::asl::Layer> node) noexcept
    {
        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Layer: {}", node.data().name);

        ice::UniquePtr<ice::InputActionBuilder::Layer> builder = ice::create_input_action_layer_builder(
            _allocator, node.data().name
        );

        arctic::SyntaxNode<> child = node.child();
        while (child != false)
        {
            if (child.type() == ice::asl::SyntaxEntity::ASL_D_LayerSource)
            {
                visit_source(*builder, child.to<ice::asl::LayerSource>());
            }
            else if (child.type() == ice::asl::SyntaxEntity::ASL_D_LayerAction)
            {
                visit_action(*builder, child.to<ice::asl::LayerAction>());
            }
            else if (child.type() == ice::asl::SyntaxEntity::ASL_D_LayerConstant)
            {
                visit_constant(*builder, child.to<ice::asl::LayerConstant>());
            }
            child = child.sibling();
        }

        ice::UniquePtr<ice::InputActionLayer> layer = builder->finalize(_allocator);
        if (layer != nullptr)
        {
            on_layer_parsed(ice::move(layer));
        }
    }

    void InputActionScriptParser::visit_constant(
        ice::InputActionBuilder::Layer& layer,
        arctic::SyntaxNode<ice::asl::LayerConstant> node
    ) noexcept
    {
        ice::asl::LayerConstant const& data = node.data();
        ICE_LOG(LogSeverity::Debug, LogTag::Core, "Constant '{}' = {}", data.name, data.param.value);

        ice::f32 param;
        if (ice::from_chars(data.param.value, param) == false)
        {
            return;
        }

        if (ice::compare(data.name, "axis.deadzone") == ice::CompareResult::Equal)
        {
            layer.set_constant(InputActionConstant::ControllerAxisDeadzone, param);
        }
    }

    void InputActionScriptParser::visit_source(
        ice::InputActionBuilder::Layer& layer,
        arctic::SyntaxNode<ice::asl::LayerSource> node
    ) noexcept
    {
        ice::InputActionSourceType type = InputActionSourceType::Key;
        switch(node.data().type.type)
        {
        case TokenType::ASL_NT_Button: type = InputActionSourceType::Button; break;
        case TokenType::ASL_NT_Axis1D: type = InputActionSourceType::Axis1d; break;
        case TokenType::ASL_NT_Axis2D: type = InputActionSourceType::Axis2d; break;
        default: ICE_ASSERT_CORE(false); type = InputActionSourceType::Axis2d; break;
        }

        ice::InputActionBuilder::Source source = layer.define_source(node.data().name, type);
        arctic::SyntaxNode binding = node.child<ice::asl::LayerSourceBinding>();
        while (binding)
        {
            using ice::input::DeviceType;
            using ice::input::KeyboardKey;
            using ice::input::KeyboardMod;
            using ice::input::MouseInput;

            if (binding.data().device.type == TokenType::ASL_KW_Keyboard)
            {
                ICE_ASSERT_CORE(type == InputActionSourceType::Key || type == InputActionSourceType::Button);

                KeyboardKey const key = detail::key_from_dsl(binding.data().source);
                ICE_ASSERT_CORE(key != KeyboardKey::Unknown);
                source.add_key(key);
            }
            else if (binding.data().device.type == TokenType::ASL_KW_Mouse)
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
            binding = binding.sibling<ice::asl::LayerSourceBinding>();
        }

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Source: {}", node.data().name);
    }

    void InputActionScriptParser::visit_action(
        ice::InputActionBuilder::Layer& layer,
        arctic::SyntaxNode<ice::asl::LayerAction> node
    ) noexcept
    {
        ice::asl::LayerAction const& action_info = node.data();
        ice::InputActionDataType const action_datatype = detail::datatype_from_dsl(action_info.type);

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Action: {}", action_info.name);
        ice::InputActionBuilder::Action action = layer.define_action(action_info.name, action_datatype);
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
            if (child.type() == ice::asl::SyntaxEntity::ASL_E_LayerActionCondition)
            {
                // Returns the final node of the series. The next sibling is either a: modifier, condition_series or action
                child = visit_cond(action.add_condition_series(), child.to<ice::asl::LayerActionCondition>());
            }
            else if (child.type() == ice::asl::SyntaxEntity::ASL_E_LayerActionModifier)
            {
                visit_mod(action, child.to<ice::asl::LayerActionModifier>());
            }
            child = child.sibling();
        }
    }

    auto InputActionScriptParser::visit_cond(
        ice::InputActionBuilder::ConditionSeries series,
        arctic::SyntaxNode<ice::asl::LayerActionCondition> node
    ) noexcept -> arctic::SyntaxNode<>
    {
        using enum ice::InputActionCondition;

        arctic::SyntaxNode<ice::asl::LayerActionCondition> cond_node = node;
        ICE_ASSERT_CORE(cond_node.data().type.type == TokenType::ASL_KW_When);

        ICE_LOG(LogSeverity::Debug, LogTag::Tool, "New condition series");
        do
        {
            node = cond_node; // Set the node to the current cond_node, necessary to return a valid node
            ice::asl::LayerActionCondition const& cond = cond_node.data();
            ICE_LOG(LogSeverity::Debug, LogTag::Tool, "- {} {}.{} {} {}", cond.type.value, cond.source_type.value, cond.source_name, cond.condition.value, cond.param.value);

            bool const from_action = cond.source_type.type == TokenType::ASL_KW_Action
                || (cond.source_type.type == arctic::TokenType::Invalid && cond.source_name.empty()); // "self"
            ice::InputActionCondition const condition = detail::condition_from_dsl(cond.condition, from_action);

            ice::InputActionConditionFlags flags = InputActionConditionFlags::None;
            if (cond_node.child<asl::LayerActionStep>())
            {
                flags |= InputActionConditionFlags::RunSteps;
            }
            if (cond.flag_series)
            {
                flags |= InputActionConditionFlags::SeriesCheck;
            }

            if (cond.type.type == TokenType::ASL_KW_WhenAnd)
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
                if (steps.type() == ice::asl::SyntaxEntity::ASL_E_LayerActionStep)
                {
                    visit_step(series, steps.to<ice::asl::LayerActionStep>());
                }
                steps = steps.sibling();
            }

            cond_node = cond_node.sibling<ice::asl::LayerActionCondition>();
        } while (cond_node && cond_node.data().type.type != TokenType::ASL_KW_When);

        // TODO: Check for '.continue' flag to skip setting 'Final' flag here
        //if (/* DOES NOT HAVE '.continue' flag */)
        {
            series.set_finished();
        }

        return node;
    }

    void InputActionScriptParser::visit_step(
        ice::InputActionBuilder::ConditionSeries& condition_series,
        arctic::SyntaxNode<ice::asl::LayerActionStep> node
    ) noexcept
    {
        ice::asl::LayerActionStep const& info = node.data();
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

    void InputActionScriptParser::visit_mod(
        ice::InputActionBuilder::Action& action,
        arctic::SyntaxNode<ice::asl::LayerActionModifier> node
    ) noexcept
    {
        ice::asl::LayerActionModifier const& info = node.data();
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
            if (value[0] >= 'a' && value[0] <= 'z')
            {
                char const key_diff = value[0] - 'a';
                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::KeyA) + key_diff);
            }
            else if (value[0] >= 'A' && value[0] <= 'Z')
            {
                ICE_ASSERT_CORE(value[0] <= 'Z');
                char const key_diff = value[0] - 'A';

                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::KeyA) + key_diff);
            }
            else if (value[0] >= '0' && value[0] <= '9')
            {
                char const key_diff = value[0] - '0';
                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::Key0) + key_diff);
            }
        }
        else if (value.size() == 2 && (value[0] == 'f' || value[0] == 'F'))
        {
            if (value[1] >= '1' && value[1] <= '9')
            {
                char const key_diff = value[1] - '1';
                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::KeyF1) + key_diff);
            }
        }
        else if (value.size() == 3)
        {
            if ((value[0] == 'f' || value[0] == 'F') && value[1] == '1' && value[2] >= '0' && value[2] <= '2')
            {
                char const key_diff = value[2] - '0';
                result = static_cast<KeyboardKey>(static_cast<ice::u16>(KeyboardKey::KeyF10) + key_diff);
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
        case TokenType::ASL_NT_Bool: return InputActionDataType::Bool;
        case TokenType::ASL_NT_Float1: return InputActionDataType::Float1;
        case TokenType::ASL_NT_Float2: return InputActionDataType::Float2;
        case TokenType::ASL_NT_Object: return InputActionDataType::ActionObject;
        default: ICE_ASSERT_CORE(false); return InputActionDataType::Invalid;
        }
    }

    auto detail::condition_from_dsl(arctic::Token token, bool action_condition) noexcept -> ice::InputActionCondition
    {
        switch(token.type)
        {
            // Action conditions
        case TokenType::ASL_OP_IsPressed: return InputActionCondition::Pressed;
        case TokenType::ASL_OP_IsReleased: return InputActionCondition::Released;
        case TokenType::ASL_OP_IsActive: return action_condition ? InputActionCondition::ActionToggleActive : InputActionCondition::Active;
        case TokenType::ASL_OP_IsInactive: return InputActionCondition::ActionToggleInactive;
            // Arithmetic conditions
        case TokenType::OP_Equal: return InputActionCondition::Equal;
        case TokenType::OP_NotEqual: return InputActionCondition::NotEqual; // TODO
        case TokenType::OP_Greater: return InputActionCondition::Greater;
        case TokenType::OP_GreaterOrEqual: return InputActionCondition::GreaterOrEqual;
        case TokenType::OP_Less: return InputActionCondition::Lower;
        case TokenType::OP_LessOrEqual: return InputActionCondition::LowerOrEqual;
            // Special conditions
        case TokenType::KW_True: return InputActionCondition::AlwaysTrue;
        default: ICE_ASSERT_CORE(false); return InputActionCondition::Invalid;
        }
    }

    auto detail::step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep
    {
        switch(token.type)
        {
        case TokenType::ASL_OP_Activate: return InputActionStep::Activate;
        case TokenType::ASL_OP_Deactivate: return InputActionStep::Deactivate;
        case TokenType::ASL_OP_Toggle: return InputActionStep::Toggle;
        case TokenType::ASL_OP_Reset: return InputActionStep::Reset;
        case TokenType::ASL_OP_Time: return InputActionStep::Time;
            // Arithmetic steps
        case TokenType::OP_Assign: return InputActionStep::Set;
        case TokenType::OP_Plus: return InputActionStep::Add;
        case TokenType::OP_Minus: return InputActionStep::Sub;
        default: ICE_ASSERT_CORE(false); break;
        }
        return InputActionStep::Invalid;
    }

    auto detail::modifier_from_dsl(arctic::Token token) noexcept -> ice::InputActionModifier
    {
        switch (token.type)
        {
        case TokenType::OP_Plus: return InputActionModifier::Add;
        case TokenType::OP_Minus: return InputActionModifier::Sub;
        case TokenType::OP_Mul: return InputActionModifier::Mul;
        case TokenType::OP_Div: return InputActionModifier::Div;
        case TokenType::ASL_OP_Max: return InputActionModifier::MaxOf;
        case TokenType::ASL_OP_Min: return InputActionModifier::MinOf;
        default: ICE_ASSERT_CORE("Not Implemented!" && false); return InputActionModifier::Invalid;
        }
    }

} // namespace ice
