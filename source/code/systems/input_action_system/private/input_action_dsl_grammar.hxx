#pragma once
#include "input_action_dsl_syntax_nodes.hxx"

#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax_rule_matchers.hxx>

namespace ice::grammar
{

    using arctic::MatchAll;
    using arctic::MatchFirst;
    using arctic::MatchChild;
    using arctic::MatchSibling;
    using arctic::SyntaxRule;
    using arctic::TokenType;

    static constexpr ice::u32 UCT_Base = static_cast<ice::u32>(TokenType::ST_Any) + 1;
    static constexpr TokenType UCT_Layer{ UCT_Base + 0 };
    static constexpr TokenType UCT_Source{ UCT_Base + 1 };
    static constexpr TokenType UCT_InputTypeButton{ UCT_Base + 2 };
    static constexpr TokenType UCT_InputTypeAxis1D{ UCT_Base + 3 };
    static constexpr TokenType UCT_InputTypeAxis2D{ UCT_Base + 4 };
    static constexpr TokenType UCT_InputTypeAxis3D{ UCT_Base + 5 };
    static constexpr TokenType UCT_InputBindingKeyboard{ UCT_Base + 6 };
    static constexpr TokenType UCT_InputBindingMouse{ UCT_Base + 7 };
    static constexpr TokenType UCT_InputBindingPad{ UCT_Base + 8 };
    static constexpr TokenType UCT_Action{ UCT_Base + 9 };
    static constexpr TokenType UCT_ActionTypeBool{ UCT_Base + 10 };
    static constexpr TokenType UCT_ActionTypeFloat1{ UCT_Base + 11 };
    static constexpr TokenType UCT_ActionTypeFloat2{ UCT_Base + 12 };
    static constexpr TokenType UCT_ActionTypeFloat3{ UCT_Base + 13 };
    static constexpr TokenType UCT_When{ UCT_Base + 14 };
    static constexpr TokenType UCT_WhenAnd{ UCT_Base + 15 };
    static constexpr TokenType UCT_WhenOr{ UCT_Base + 16 };
    static constexpr TokenType UCT_WhenPressed{ UCT_Base + 17 };
    static constexpr TokenType UCT_WhenReleased{ UCT_Base + 18 };
    static constexpr TokenType UCT_WhenActive{ UCT_Base + 19 };
    static constexpr TokenType UCT_WhenInactive{ UCT_Base + 20 };
    static constexpr TokenType UCT_StepActivate{ UCT_Base + 21 };
    static constexpr TokenType UCT_StepDeactivate{ UCT_Base + 22 };

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
        SyntaxRule{ UCT_InputTypeAxis1D },
        SyntaxRule{ UCT_InputTypeAxis2D },
        SyntaxRule{ UCT_InputTypeAxis3D },
    };

    static constexpr SyntaxRule Rule_LayerSourceRules[]{ // source <type> <name>: <binding>...
        SyntaxRule{ UCT_Source },
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

    ////////////////////////////////////////////////////////////////
    // Action Step rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionStepTargetTypeRules[]{
        SyntaxRule{ UCT_Source }, // TODO: Set action target type
        SyntaxRule{ UCT_Action }, // TODO: Set action target type
    };

    static constexpr SyntaxRule Rule_LayerActionStepTargetExplicitRules[]{
        SyntaxRule{ Rule_LayerActionStepTargetTypeRules, MatchFirst },
        SyntaxRule{ TokenType::CT_Dot },
    };

    static constexpr SyntaxRule Rule_LayerActionStepTargetRules[]{
        SyntaxRule{ Rule_LayerActionStepTargetExplicitRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::CT_Symbol }, // Capture name
    };

    static constexpr SyntaxRule Rule_LayerActionStepBuiltInListRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ UCT_StepActivate },
        SyntaxRule{ UCT_StepDeactivate },
    };

    static constexpr SyntaxRule Rule_LayerActionStepArithmeticRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ Rule_LayerActionComponentListRules, MatchFirst },
        SyntaxRule{ TokenType::OP_Assign },
        SyntaxRule{ Rule_LayerActionStepTargetRules, MatchAll }
    };

    static constexpr SyntaxRule Rule_LayerActionStepOperationRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ Rule_LayerActionStepBuiltInListRules, MatchFirst },
        SyntaxRule{ Rule_LayerActionStepArithmeticRules, MatchAll },

    };

    static constexpr SyntaxRule Rule_LayerActionStepRules[]{
        SyntaxRule{ TokenType::CT_Dot },
        SyntaxRule{ Rule_LayerActionStepOperationRules, MatchFirst }, // TODO: Capture,
    };

    ////////////////////////////////////////////////////////////////
    // Action Condition rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionWhenTargetTypeRules[]{
        SyntaxRule{ UCT_Source }, // TODO: Set action target type
        SyntaxRule{ UCT_Action }, // TODO: Set action target type
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetExplicitRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetTypeRules, MatchFirst, &syntax::LayerActionWhen::source_type },
        SyntaxRule{ TokenType::CT_Dot },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetExplicitRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerActionWhen::source_name }, // Capture name
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionConditionListRules[]{
        SyntaxRule{ UCT_WhenPressed },
        SyntaxRule{ UCT_WhenReleased },
        SyntaxRule{ UCT_WhenActive },
        SyntaxRule{ UCT_WhenInactive },
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
        SyntaxRule{ Rule_LayerActionComponentListRules, MatchFirst, &syntax::LayerActionWhen::source_component },
        SyntaxRule{ Rule_LayerActionWhenTargetComparisonListRules, MatchFirst, &syntax::LayerActionWhen::condition },
        SyntaxRule{ Rule_LayerActionWhenParamNumberTokenListRules, MatchFirst, &syntax::LayerActionWhen::param }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionConditionRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetActionConditionListRules, MatchFirst, &syntax::LayerActionWhen::condition }, // TODO: Capture,
        SyntaxRule{ Rule_LayerActionWhenTargetComponentRules, MatchAll }, // TODO: Capture,
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

    static constexpr SyntaxRule Rule_LayerActionWhenRules[]{
        // Just to create the child node we need to succeed once.
        SyntaxRule{ [](auto const&, auto& ctx) noexcept{ return arctic::ParseState::Success; } }.noadvance(),
        SyntaxRule{ Rule_LayerActionWhenBlockRules, MatchFirst, &syntax::LayerActionWhen::type },
        SyntaxRule{ Rule_LayerActionWhenTargetRules, MatchAll, &syntax::LayerActionWhen::source_name },
        SyntaxRule{ Rule_LayerActionWhenTargetActionRules, MatchAll },
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Rule_LayerActionStepRules, MatchChild<syntax::LayerActionStep> }.optional().repeat()
    };

    ////////////////////////////////////////////////////////////////
    // Action rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionTypeRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ UCT_ActionTypeBool },
        SyntaxRule{ UCT_ActionTypeFloat1 },
        SyntaxRule{ UCT_ActionTypeFloat2 },
        SyntaxRule{ UCT_ActionTypeFloat3 },
    };

    static constexpr SyntaxRule Rule_LayerActionRules[]{ // action <name>: <native_type>
        SyntaxRule{ UCT_Action },
        SyntaxRule{ TokenType::CT_Symbol, &syntax::LayerAction::name },
        SyntaxRule{ TokenType::CT_Colon },
        SyntaxRule{ Rule_LayerActionTypeRules, MatchFirst, &syntax::LayerAction::type },
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Rule_LayerActionWhenRules, MatchChild<syntax::LayerActionWhen> }.repeat().optional(),
        // SyntaxRule{ Rule_LayerActionRules, MatchChild<syntax::LayerAction> }.repeat().optional(),
    };

    static constexpr SyntaxRule Rule_LayerRules[]{ // layer <name>: <sources...> <actions...>
        SyntaxRule{ UCT_Layer },
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
