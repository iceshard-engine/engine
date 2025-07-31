#pragma once
#include "input_action_dsl_syntax_nodes.hxx"
#include "input_action_script_tokens.hxx"

#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax_rule_matchers.hxx>

namespace ice::grammar
{
    using namespace ice::asl;

    using arctic::MatchAll;
    using arctic::MatchFirst;
    using arctic::MatchChild;
    using arctic::MatchSibling;
    using arctic::SyntaxRule;
    //using TokenType = ::ice::asl::TokenType;

    static constexpr SyntaxRule Rule_ColonOrCommaRules[]{ // , or :
        SyntaxRule{ TokenType::CT_Colon },
        SyntaxRule{ TokenType::CT_Comma },
    };

    static constexpr SyntaxRule Rule_ParenOrCommaRules[]{ // , or :
        SyntaxRule{ TokenType::CT_ParenOpen },
        SyntaxRule{ TokenType::CT_Comma },
    };

    static constexpr SyntaxRule Rule_LayerSourceBindingDeviceRules[]{
        SyntaxRule{ UCT_InputBindingKeyboard },
        SyntaxRule{ UCT_InputBindingMouse },
        SyntaxRule{ UCT_InputBindingPad },
    };

    static constexpr SyntaxRule Rule_LayerSourceBindingRules[]{ // ': <dev>.<src>' or ', <dev>.<src>'
        SyntaxRule{ Rule_ColonOrCommaRules, MatchFirst },
        SyntaxRule{ Rule_LayerSourceBindingDeviceRules, MatchFirst, &syntax::LayerInputBinding::device },
        SyntaxRule{ TokenType::CT_Dot },
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerInputBinding::source },
    };

    static constexpr SyntaxRule Rule_LayerInputTypeRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ UCT_InputTypeButton },
        SyntaxRule{ TokenType::ASL_NT_Axis1D },
        SyntaxRule{ TokenType::ASL_NT_Axis2D },
        SyntaxRule{ TokenType::ASL_NT_Axis3D },
    };

    static constexpr SyntaxRule Rule_LayerSourceRules[]{ // source <type> <name>: <binding>...
        SyntaxRule{ TokenType::ASL_KW_Source },
        SyntaxRule{ Rule_LayerInputTypeRules, MatchFirst, &syntax::LayerSource::type }, // TODO: Additional types
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerSource::name },
        SyntaxRule{ TokenType::CT_Colon }.noadvance().optional(),
        SyntaxRule{ Rule_LayerSourceBindingRules, MatchChild<syntax::LayerInputBinding> }.repeat().optional(),
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
    };

    ////////////////////////////////////////////////////////////////
    // Action utility rules
    ////////////////////////////////////////////////////////////////

    template<char ComponentChar>
    auto Check_IsValidTargetComponent(arctic::SyntaxRule const&, arctic::MatchContext& ctx) noexcept -> arctic::ParseState
    {
        using enum arctic::ParseState;
        return ctx.token.value[0] == ComponentChar ? Success : Error_UnexpectedToken;
    }

    static constexpr SyntaxRule Rule_LayerActionComponentListRules[]{
        SyntaxRule{ Check_IsValidTargetComponent<'x'> },
        SyntaxRule{ Check_IsValidTargetComponent<'y'> },
        SyntaxRule{ Check_IsValidTargetComponent<'z'> },
    };

    static constexpr SyntaxRule Rule_LayerActionComponentPostfixRules[]{
        SyntaxRule{ TokenType::CT_Dot },
        SyntaxRule{ Rule_LayerActionComponentListRules, MatchFirst, &syntax::LayerActionStep::source, SyntaxRule::store_value_extend<arctic::String> }
    };

    ////////////////////////////////////////////////////////////////
    // Action Step rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionStepTargetTypeRules[]{
        SyntaxRule{ TokenType::ASL_KW_Source }, // TODO: Set action target type
        SyntaxRule{ TokenType::ASL_KW_Action }, // TODO: Set action target type
    };

    static constexpr SyntaxRule Rule_LayerActionStepTargetExplicitRules[]{
        SyntaxRule{ Rule_LayerActionStepTargetTypeRules, MatchFirst, &syntax::LayerActionStep::source_type },
        SyntaxRule{ TokenType::CT_Dot },
    };

    static constexpr SyntaxRule Rule_LayerActionStepTargetRules[]{
        SyntaxRule{ Rule_LayerActionStepTargetExplicitRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerActionStep::source }, // Capture name
        SyntaxRule{ Rule_LayerActionComponentPostfixRules, MatchAll }.optional()
    };

    static constexpr SyntaxRule Rule_LayerActionStepBuiltInListRules[]{
        SyntaxRule{ UCT_StepActivate },
        SyntaxRule{ UCT_StepDeactivate },
        SyntaxRule{ UCT_StepToggle },
        SyntaxRule{ UCT_StepReset },
        SyntaxRule{ UCT_StepTime },
    };

    static constexpr SyntaxRule Rule_LayerActionStepArithmeticOperationRules[]{
        SyntaxRule{ TokenType::OP_Plus },
        SyntaxRule{ TokenType::OP_Minus },
        SyntaxRule{ TokenType::OP_Assign },
    };

    static constexpr SyntaxRule Rule_LayerActionStepArithmeticRules[]{ // button, axis1d, axis2d or axis3d
        // Explicitly extend the string to cover the previous token and the new matched one. (ex: '.' + 'x' => '.x')
        SyntaxRule{ Rule_LayerActionComponentListRules, MatchFirst, &syntax::LayerActionStep::destination, SyntaxRule::store_value_extend<arctic::String> },
        SyntaxRule{ Rule_LayerActionStepArithmeticOperationRules, MatchFirst, &syntax::LayerActionStep::step },
        SyntaxRule{ Rule_LayerActionStepTargetRules, MatchAll }
    };

    static constexpr SyntaxRule Rule_LayerActionStepOperationRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ Rule_LayerActionStepBuiltInListRules, MatchFirst, &syntax::LayerActionStep::step },
        SyntaxRule{ Rule_LayerActionStepArithmeticRules, MatchAll },

    };

    static constexpr SyntaxRule Rule_LayerActionStepRules[]{
        // If we don't end up using the destination string it's fine
        //   but we require the '.' character to be used when setting destinations via builder.
        SyntaxRule{ TokenType::CT_Dot, &syntax::LayerActionStep::destination },
        SyntaxRule{ Rule_LayerActionStepOperationRules, MatchFirst }, // TODO: Capture,
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
    };

    ////////////////////////////////////////////////////////////////
    // Action Condition rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionWhenTargetTypeRules[]{
        SyntaxRule{ TokenType::ASL_KW_Source },
        SyntaxRule{ TokenType::ASL_KW_Action },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetExplicitRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetTypeRules, MatchFirst, &syntax::LayerActionWhen::source_type },
        SyntaxRule{ TokenType::CT_Dot },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetExplicitRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerActionWhen::source_name }.optional(), // Capture name
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionConditionListRules[]{
        SyntaxRule{ UCT_WhenPressed },
        SyntaxRule{ UCT_WhenReleased },
        SyntaxRule{ UCT_WhenActive },
        SyntaxRule{ UCT_WhenInactive },
        SyntaxRule{ TokenType::KW_True },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetComparisonListRules[]{
        SyntaxRule{ TokenType::OP_Equal },
        SyntaxRule{ TokenType::OP_NotEqual },
        SyntaxRule{ TokenType::OP_Less },
        SyntaxRule{ TokenType::OP_LessOrEqual },
        SyntaxRule{ TokenType::OP_Greater },
        SyntaxRule{ TokenType::OP_GreaterOrEqual },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenParamNumberTokenListRules[]{
        SyntaxRule{ TokenType::CT_Number },
        SyntaxRule{ TokenType::CT_NumberFloat },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetComponentRules[]{
        SyntaxRule{ Rule_LayerActionComponentListRules, MatchFirst, &syntax::LayerActionWhen::source_name, arctic::SyntaxRule::store_value_extend<arctic::String> },
        SyntaxRule{ Rule_LayerActionWhenTargetComparisonListRules, MatchFirst, &syntax::LayerActionWhen::condition },
        SyntaxRule{ Rule_LayerActionWhenParamNumberTokenListRules, MatchFirst, &syntax::LayerActionWhen::param }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionConditionRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetActionConditionListRules, MatchFirst, &syntax::LayerActionWhen::condition },
        SyntaxRule{ Rule_LayerActionWhenTargetComponentRules, MatchAll },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionRules[]{
        SyntaxRule{ TokenType::CT_Dot },
        SyntaxRule{ Rule_LayerActionWhenTargetActionConditionRules, MatchFirst }, // TODO: Capture,
    };

    static constexpr SyntaxRule Rule_LayerActionWhenBlockRules[]{
        SyntaxRule{ UCT_When },
        SyntaxRule{ UCT_WhenAnd },
        SyntaxRule{ UCT_WhenOr },
    };

    static constexpr SyntaxRule Rule_LayerActionFlagsListRules[]{
        SyntaxRule{ UCT_ActionFlagOnce, &syntax::LayerAction::flag_once },
        SyntaxRule{ UCT_ActionFlagToggled, &syntax::LayerAction::flag_toggled },
        //SyntaxRule{ UCT_ActionFlagAccumulated, &syntax::LayerAction::flag_accumulated },
    };

    static constexpr SyntaxRule Rule_LayerActionFlagsRules[]{
        SyntaxRule{ grammar::TokenType::CT_Comma },
        SyntaxRule{ Rule_LayerActionFlagsListRules, MatchFirst }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenFlagsListRules[]{
        SyntaxRule{ UCT_WhenFlagCheckSeries, &syntax::LayerActionWhen::flag_series }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenFlagsRules[]{
        SyntaxRule{ grammar::TokenType::CT_Comma },
        SyntaxRule{ Rule_LayerActionWhenFlagsListRules, MatchFirst }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenRules[]{
        // Just to create the child node we need to succeed once.
        SyntaxRule{ [](auto const&, auto& ctx) noexcept{ return arctic::ParseState::Success; } }.noadvance(),
        SyntaxRule{ Rule_LayerActionWhenBlockRules, MatchFirst, &syntax::LayerActionWhen::type },
        SyntaxRule{ Rule_LayerActionWhenTargetRules, MatchAll },
        SyntaxRule{ Rule_LayerActionWhenTargetActionRules, MatchAll },
        SyntaxRule{ Rule_LayerActionWhenFlagsRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Rule_LayerActionStepRules, MatchChild<syntax::LayerActionStep> }.optional().repeat(),
    };

    ////////////////////////////////////////////////////////////////
    // Action modifier rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionModifierComponentRules[]{
        SyntaxRule{ TokenType::CT_Dot, &syntax::LayerActionModifier::component },
        SyntaxRule{ Rule_LayerActionComponentListRules, MatchFirst, &syntax::LayerActionModifier::component, arctic::SyntaxRule::store_value_extend<arctic::String> }
    };

    static constexpr SyntaxRule Rule_LayerActionModifierOperationListRules[]{
        SyntaxRule{ TokenType::OP_Div },
        SyntaxRule{ TokenType::OP_Mul },
        SyntaxRule{ TokenType::OP_Plus },
        SyntaxRule{ TokenType::OP_Minus },
        SyntaxRule{ UCT_ModifierOpMin },
        SyntaxRule{ UCT_ModifierOpMax },
    };

    static constexpr SyntaxRule Rule_LayerActionModifierRules[]{
        SyntaxRule{ UCT_Modifier },
        SyntaxRule{ Rule_LayerActionModifierComponentRules, MatchAll },
        SyntaxRule{ Rule_LayerActionModifierOperationListRules, MatchFirst, &syntax::LayerActionModifier::operation },
        SyntaxRule{ Rule_LayerActionWhenParamNumberTokenListRules, MatchFirst, &syntax::LayerActionModifier::param },
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
    };

    ////////////////////////////////////////////////////////////////
    // Action rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionTypeRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ UCT_ActionTypeBool },
        SyntaxRule{ UCT_ActionTypeFloat1 },
        SyntaxRule{ UCT_ActionTypeFloat2 },
        SyntaxRule{ UCT_ActionTypeFloat3 },
        SyntaxRule{ UCT_ActionTypeObject },
    };

    static constexpr SyntaxRule Rule_LayerActionRules[]{ // action <name>: <native_type>
        SyntaxRule{ TokenType::ASL_KW_Action },
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerAction::name },
        SyntaxRule{ TokenType::CT_Colon },
        SyntaxRule{ Rule_LayerActionTypeRules, MatchFirst, &syntax::LayerAction::type },
        SyntaxRule{ Rule_LayerActionFlagsRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Rule_LayerActionWhenRules, MatchChild<syntax::LayerActionWhen> }.repeat().optional(),
        SyntaxRule{ Rule_LayerActionModifierRules, MatchChild<syntax::LayerActionModifier> }.repeat().optional(),
        // SyntaxRule{ Rule_LayerActionRules, MatchChild<syntax::LayerAction> }.repeat().optional(),
    };

    static constexpr SyntaxRule Rule_LayerRules[]{ // layer <name>: <sources...> <actions...>
        SyntaxRule{ TokenType::ASL_KW_Layer },
        SyntaxRule{ TokenType::CT_Symbol, &syntax::Layer::name },
        SyntaxRule{ TokenType::CT_Colon },
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Rule_LayerSourceRules, MatchChild<syntax::LayerSource> }.repeat().optional(),
        SyntaxRule{ Rule_LayerActionRules, MatchChild<syntax::LayerAction> }.repeat().optional(),
    };

    static constexpr SyntaxRule Rule_GlobalRules[]{ // <layers>...
        SyntaxRule{ Rule_LayerRules, MatchSibling<syntax::Layer> },
        SyntaxRule{ TokenType::ST_EndOfLine },
    };

} // namespace ice::grammar
