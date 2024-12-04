#include "input_action_dsl_grammar.hxx"
#include "input_action_dsl_layer_builder.hxx"

#include <ice/input_action_definitions.hxx>
#include <ice/log.hxx>

namespace ice
{

    namespace detail
    {

        auto key_from_dsl(arctic::String value) noexcept -> ice::input::KeyboardKey;
        auto mouse_from_dsl(arctic::String value) noexcept -> ice::input::MouseInput;

        auto condition_from_dsl(arctic::Token token) noexcept -> ice::InputActionCondition;
        auto step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep;

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
        ice::InputActionData action_datatype = InputActionData::Bool;
        if (node.data().type.type == ice::grammar::UCT_ActionTypeFloat2)
        {
            action_datatype = InputActionData::Float2;
        }

        ICE_LOG(LogSeverity::Info, LogTag::Engine, "Action: {}", node.data().name);
        ice::InputActionLayerBuilder::ActionBuilder action = _builder->define_action(node.data().name, action_datatype);

        arctic::SyntaxNode<> child = node.child();
        while(child != false)
        {
            if (child.type() == ice::syntax::SyntaxEntity_LayerActionCondition)
            {
                visit(action, child.to<ice::syntax::LayerActionWhen>());
            }
            else if (child.type() == ice::syntax::SyntaxEntity_LayerActionStep)
            {
                visit(action, child.to<ice::syntax::LayerActionStep>());
            }
            child = child.sibling();
        }
    }

    void InputActionDSLLayerBuilder::visit(
        ice::InputActionLayerBuilder::ActionBuilder& action,
        arctic::SyntaxNode<ice::syntax::LayerActionWhen> node
    ) noexcept
    {
        ice::syntax::LayerActionWhen const& cond = node.to<ice::syntax::LayerActionWhen>().data();
        ICE_LOG(LogSeverity::Debug, LogTag::Tool, "- {} {}.{}.{} {} {}", cond.type.value, cond.source_type.value, cond.source_name, cond.source_component, cond.condition.value, cond.param.value);

        ice::InputActionCondition const condition = detail::condition_from_dsl(cond.condition);

        ice::InputActionConditionFlags flags = InputActionConditionFlags::None;

        // No siblings or next subling beeing "when" == "final" condition.
        if (auto sib = node.sibling<ice::syntax::LayerActionWhen>(); !sib || sib.data().type.type == grammar::UCT_When)
        {
            flags |= InputActionConditionFlags::Final;
        }
        if (node.child()) // Only steps can introduce children.
        {
            flags |= InputActionConditionFlags::RunSteps;
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

        action.add_condition(cond.source_name, condition, flags);
    }

    void InputActionDSLLayerBuilder::visit(
        ice::InputActionLayerBuilder::ActionBuilder& action,
        arctic::SyntaxNode<ice::syntax::LayerActionStep> node
    ) noexcept
    {
        action.add_step(detail::step_from_dsl(node.data().step));
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

    auto detail::condition_from_dsl(arctic::Token token) noexcept -> ice::InputActionCondition
    {
        switch(token.type)
        {
        case grammar::UCT_WhenPressed: return InputActionCondition::Pressed;
        case grammar::UCT_WhenReleased: return InputActionCondition::Released;
        case grammar::UCT_WhenActive: return InputActionCondition::Active;
        // case grammar::UCT_WhenInactive: return InputActionCondition::; TODO
        case arctic::TokenType::OP_Equal: return InputActionCondition::Equal;
        // case arctic::TokenType::OP_NotEqual: return InputActionCondition::; // TODO
        case arctic::TokenType::OP_Greater: return InputActionCondition::Greater;
        case arctic::TokenType::OP_GreaterOrEqual: return InputActionCondition::GreaterOrEqual;
        case arctic::TokenType::OP_Less: return InputActionCondition::Lower;
        case arctic::TokenType::OP_LessOrEqual: return InputActionCondition::LowerOrEqual;
        default: return InputActionCondition::Invalid;
        }
    }

    auto detail::step_from_dsl(arctic::Token token) noexcept -> ice::InputActionStep
    {
        switch(token.type)
        {
        case grammar::UCT_StepActivate: return InputActionStep::Activate;
        case grammar::UCT_StepDeactivate: return InputActionStep::Deactivate;
        default: break;
        }
        return InputActionStep::Invalid;
    }

} // namespace ice
