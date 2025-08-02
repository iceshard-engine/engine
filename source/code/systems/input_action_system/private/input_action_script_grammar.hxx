#pragma once
#include "input_action_script_syntax_data.hxx"
#include "input_action_script_tokens.hxx"

#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax_rule_matchers.hxx>

namespace ice::asl::grammar
{
    using arctic::MatchAll;
    using arctic::MatchFirst;
    using arctic::MatchChild;
    using arctic::MatchSibling;
    using arctic::SyntaxRule;

    struct TokenList
    {
        static constexpr SyntaxRule DeviceKeywords[]{
            SyntaxRule{ TokenType::ASL_KW_Controller },
            SyntaxRule{ TokenType::ASL_KW_Keyboard },
            SyntaxRule{ TokenType::ASL_KW_Mouse },
        };

        static constexpr SyntaxRule InputNativeTypes[]{ // button, axis1d, axis2d or axis3d
            SyntaxRule{ TokenType::ASL_NT_Button },
            SyntaxRule{ TokenType::ASL_NT_Axis1D },
            SyntaxRule{ TokenType::ASL_NT_Axis2D },
            SyntaxRule{ TokenType::ASL_NT_Axis3D },
        };

        static constexpr SyntaxRule SymbolCategories[]{
            SyntaxRule{ TokenType::ASL_KW_Source },
            SyntaxRule{ TokenType::ASL_KW_Action },
        };

        static constexpr SyntaxRule StepCommands[]{
            SyntaxRule{ TokenType::ASL_OP_Activate },
            SyntaxRule{ TokenType::ASL_OP_Deactivate },
            SyntaxRule{ TokenType::ASL_OP_Toggle },
            SyntaxRule{ TokenType::ASL_OP_Reset },
            SyntaxRule{ TokenType::ASL_OP_Time },
        };

        static constexpr SyntaxRule StepValueOperations[]{
            SyntaxRule{ TokenType::OP_Plus },
            SyntaxRule{ TokenType::OP_Minus },
            SyntaxRule{ TokenType::OP_Assign },
        };

        template<char ComponentChar>
        static auto RuleFn_IsCharacter(arctic::SyntaxRule const&, arctic::MatchContext& ctx) noexcept -> arctic::ParseState
        {
            using enum arctic::ParseState;
            return ctx.token.value[0] == ComponentChar ? Success : Error_UnexpectedToken;
        }

        static constexpr SyntaxRule XYZ[]{
            SyntaxRule{ RuleFn_IsCharacter<'x'> },
            SyntaxRule{ RuleFn_IsCharacter<'y'> },
            SyntaxRule{ RuleFn_IsCharacter<'z'> },
        };
    };

    template<auto NameField, auto CategoryField = NameField>
    struct Symbol
    {
        template<typename T1, typename T2, T1 Field1, T2 Field2>
        struct StoreFnSelector
        {
            static constexpr auto StoreFn = SyntaxRule::store_value<arctic::String>;
        };

        template<typename T1, T1 Field1, T1 Field2>
        struct StoreFnSelector<T1, T1, Field1, Field2>
        {
            static constexpr auto StoreFn = NameField == CategoryField
                ? SyntaxRule::store_value_extend<arctic::String>
                : SyntaxRule::store_value<arctic::String>;
        };

        static constexpr auto StoreFn = StoreFnSelector<decltype(NameField), decltype(CategoryField), NameField, CategoryField>
            ::StoreFn;


        static constexpr SyntaxRule Category[]{
            SyntaxRule{ TokenList::SymbolCategories, MatchFirst, CategoryField },
            SyntaxRule{ TokenType::CT_Dot },
        };

        static constexpr SyntaxRule AxisComponent[]{
            SyntaxRule{ TokenType::CT_Dot },
            SyntaxRule{ TokenList::XYZ, MatchFirst, NameField, SyntaxRule::store_value_extend<arctic::String> }
        };

        static constexpr SyntaxRule NameWithCategory[]{
            SyntaxRule{ Category, MatchAll }.optional(),
            SyntaxRule{ TokenType::CT_Symbol, NameField, StoreFn },
        };

        static constexpr SyntaxRule NameWithComponents[]{
            SyntaxRule{ TokenType::CT_Symbol, NameField }, // Capture name
            SyntaxRule{ AxisComponent, MatchAll }.optional() // ... and components
        };

        static constexpr SyntaxRule FullName[]{
            SyntaxRule{ Category, MatchAll }.optional(),
            SyntaxRule{ TokenType::CT_Symbol, NameField, StoreFn },
            SyntaxRule{ AxisComponent, MatchAll }.optional()
        };
    };

    struct SourceBinding
    {
        static constexpr SyntaxRule SourceBindingInfo[]{
            SyntaxRule{ TokenList::DeviceKeywords, MatchFirst, &LayerSourceBinding::device },
            SyntaxRule{ TokenType::CT_Dot },
            SyntaxRule{ TokenType::CT_Symbol, &LayerSourceBinding::source },
        };

        static constexpr SyntaxRule SourceBindingNext[]{
            SyntaxRule{ TokenType::CT_Comma },
            SyntaxRule{ SourceBindingInfo, MatchAll }
        };

        static constexpr SyntaxRule Rules[]{
            SyntaxRule{ TokenType::CT_Colon },
            SyntaxRule{ SourceBindingInfo, MatchAll },
            SyntaxRule{ SourceBindingNext, MatchSibling<LayerSourceBinding> }.optional().repeat()
        };
    };

    struct ActionStep
    {
        using SymbolType = Symbol<&LayerActionStep::source, &LayerActionStep::source_type>;

        static constexpr SyntaxRule CommandOperation[]{
            SyntaxRule{ TokenList::StepCommands, MatchFirst, &LayerActionStep::step }
        };

        static constexpr SyntaxRule ArithmeticOperation[]{
            SyntaxRule{ TokenList::XYZ, MatchFirst, &LayerActionStep::destination, SyntaxRule::store_value_extend<arctic::String> },
            SyntaxRule{ TokenList::StepValueOperations, MatchFirst, &LayerActionStep::step },
            SyntaxRule{ SymbolType::FullName, MatchAll }
        };

        static constexpr SyntaxRule OperationList[]{ // button, axis1d, axis2d or axis3d
            SyntaxRule{ CommandOperation, MatchAll },
            SyntaxRule{ ArithmeticOperation, MatchAll },
        };

        static constexpr SyntaxRule Operation[]{
            // If we don't end up using the destination string it's fine
            //   but we require the '.' character to be used when setting destinations via builder.
            SyntaxRule{ TokenType::CT_Dot, &LayerActionStep::destination },
            SyntaxRule{ OperationList, MatchFirst }, // TODO: Capture,
            SyntaxRule{ TokenType::ST_EndOfLine },
        };
    };

    struct Condition
    {
        using SymbolType = Symbol<&LayerActionCondition::source_name, &LayerActionCondition::source_type>;

        static constexpr SyntaxRule ConditionAndOr[]{
            SyntaxRule{ TokenType::ASL_KW_WhenAnd },
            SyntaxRule{ TokenType::ASL_KW_WhenOr },
        };

        static constexpr SyntaxRule ConditionStart[]{
            SyntaxRule{ TokenType::ASL_KW_When, &LayerActionCondition::type },
            SyntaxRule{ SymbolType::NameWithCategory, MatchAll },
        };
    };

    struct Definition
    {
        static constexpr SyntaxRule LayerSource[]{
            SyntaxRule{ TokenType::ASL_KW_Source },
            SyntaxRule{ TokenList::InputNativeTypes, MatchFirst, &LayerSource::type },
            SyntaxRule{ TokenType::CT_Symbol, &LayerSource::name },
            SyntaxRule{ SourceBinding::Rules, MatchChild<LayerSourceBinding> }.optional(),
            SyntaxRule{ TokenType::ST_EndOfLine },
        };
    };

    ////////////////////////////////////////////////////////////////
    // Action utility rules
    ////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////
    // Action Step rules
    ////////////////////////////////////////////////////////////////


    static constexpr SyntaxRule Rule_LayerActionStepRules[]{
        // If we don't end up using the destination string it's fine
        //   but we require the '.' character to be used when setting destinations via builder.
        SyntaxRule{ TokenType::CT_Dot, &LayerActionStep::destination },
        SyntaxRule{ ActionStep::Operation, MatchFirst }, // TODO: Capture,
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
        SyntaxRule{ Rule_LayerActionWhenTargetTypeRules, MatchFirst, &LayerActionCondition::source_type },
        SyntaxRule{ TokenType::CT_Dot },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetExplicitRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::CT_Symbol, &LayerActionCondition::source_name }.optional(), // Capture name
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionConditionListRules[]{
        SyntaxRule{ TokenType::ASL_OP_IsPressed },
        SyntaxRule{ TokenType::ASL_OP_IsReleased },
        SyntaxRule{ TokenType::ASL_OP_IsActive },
        SyntaxRule{ TokenType::ASL_OP_IsInactive },
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
        SyntaxRule{ TokenList::XYZ, MatchFirst, &LayerActionCondition::source_name, arctic::SyntaxRule::store_value_extend<arctic::String> },
        SyntaxRule{ Rule_LayerActionWhenTargetComparisonListRules, MatchFirst, &LayerActionCondition::condition },
        SyntaxRule{ Rule_LayerActionWhenParamNumberTokenListRules, MatchFirst, &LayerActionCondition::param }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionConditionRules[]{
        SyntaxRule{ Rule_LayerActionWhenTargetActionConditionListRules, MatchFirst, &LayerActionCondition::condition },
        SyntaxRule{ Rule_LayerActionWhenTargetComponentRules, MatchAll },
    };

    static constexpr SyntaxRule Rule_LayerActionWhenTargetActionRules[]{
        SyntaxRule{ TokenType::CT_Dot },
        SyntaxRule{ Rule_LayerActionWhenTargetActionConditionRules, MatchFirst }, // TODO: Capture,
    };

    static constexpr SyntaxRule Rule_LayerActionWhenBlockRules[]{
        SyntaxRule{ TokenType::ASL_KW_When },
        SyntaxRule{ TokenType::ASL_KW_WhenAnd },
        SyntaxRule{ TokenType::ASL_KW_WhenOr },
    };

    static constexpr SyntaxRule Rule_LayerActionFlagsListRules[]{
        SyntaxRule{ TokenType::ASL_KWF_Once, &LayerAction::flag_once },
        SyntaxRule{ TokenType::ASL_KWF_Toggled, &LayerAction::flag_toggled },
    };

    static constexpr SyntaxRule Rule_LayerActionFlagsRules[]{
        SyntaxRule{ TokenType::CT_Comma },
        SyntaxRule{ Rule_LayerActionFlagsListRules, MatchFirst }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenFlagsListRules[]{
        SyntaxRule{ TokenType::ASL_KWF_CheckSeries, &LayerActionCondition::flag_series }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenFlagsRules[]{
        SyntaxRule{ TokenType::CT_Comma },
        SyntaxRule{ Rule_LayerActionWhenFlagsListRules, MatchFirst }
    };

    static constexpr SyntaxRule Rule_LayerActionWhenRules[]{
        // Just to create the child node we need to succeed once.
        SyntaxRule{ [](auto const&, auto& ctx) noexcept{ return arctic::ParseState::Success; } }.noadvance(),
        SyntaxRule{ Rule_LayerActionWhenBlockRules, MatchFirst, &LayerActionCondition::type },
        SyntaxRule{ Rule_LayerActionWhenTargetRules, MatchAll },
        SyntaxRule{ Rule_LayerActionWhenTargetActionRules, MatchAll },
        SyntaxRule{ Rule_LayerActionWhenFlagsRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ ActionStep::Operation, MatchChild<LayerActionStep> }.optional().repeat(),
    };

    ////////////////////////////////////////////////////////////////
    // Action modifier rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionModifierComponentRules[]{
        SyntaxRule{ TokenType::CT_Dot, &LayerActionModifier::component },
        SyntaxRule{ TokenList::XYZ, MatchFirst, &LayerActionModifier::component, arctic::SyntaxRule::store_value_extend<arctic::String> }
    };

    static constexpr SyntaxRule Rule_LayerActionModifierOperationListRules[]{
        SyntaxRule{ TokenType::OP_Div },
        SyntaxRule{ TokenType::OP_Mul },
        SyntaxRule{ TokenType::OP_Plus },
        SyntaxRule{ TokenType::OP_Minus },
        SyntaxRule{ TokenType::ASL_OP_Min },
        SyntaxRule{ TokenType::ASL_OP_Max },
    };

    static constexpr SyntaxRule Rule_LayerActionModifierRules[]{
        SyntaxRule{ TokenType::ASL_KW_Modifier },
        SyntaxRule{ Rule_LayerActionModifierComponentRules, MatchAll },
        SyntaxRule{ Rule_LayerActionModifierOperationListRules, MatchFirst, &LayerActionModifier::operation },
        SyntaxRule{ Rule_LayerActionWhenParamNumberTokenListRules, MatchFirst, &LayerActionModifier::param },
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
    };

    ////////////////////////////////////////////////////////////////
    // Action rules
    ////////////////////////////////////////////////////////////////

    static constexpr SyntaxRule Rule_LayerActionTypeRules[]{ // button, axis1d, axis2d or axis3d
        SyntaxRule{ TokenType::ASL_NT_Bool },
        SyntaxRule{ TokenType::ASL_NT_Float1 },
        SyntaxRule{ TokenType::ASL_NT_Float2 },
        SyntaxRule{ TokenType::ASL_NT_Float3 },
        SyntaxRule{ TokenType::ASL_NT_Object },
    };

    static constexpr SyntaxRule Rule_LayerActionRules[]{ // action <name>: <native_type>
        SyntaxRule{ TokenType::ASL_KW_Action },
        SyntaxRule{ TokenType::CT_Symbol, &LayerAction::name },
        SyntaxRule{ TokenType::CT_Colon },
        SyntaxRule{ Rule_LayerActionTypeRules, MatchFirst, &LayerAction::type },
        SyntaxRule{ Rule_LayerActionFlagsRules, MatchAll }.optional(),
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Rule_LayerActionWhenRules, MatchChild<LayerActionCondition> }.repeat().optional(),
        SyntaxRule{ Rule_LayerActionModifierRules, MatchChild<LayerActionModifier> }.repeat().optional(),
        // SyntaxRule{ Rule_LayerActionRules, MatchChild<syntax::LayerAction> }.repeat().optional(),
    };

    static constexpr SyntaxRule Rule_LayerRules[]{ // layer <name>: <sources...> <actions...>
        SyntaxRule{ TokenType::ASL_KW_Layer },
        SyntaxRule{ TokenType::CT_Symbol, &Layer::name },
        SyntaxRule{ TokenType::CT_Colon },
        SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        SyntaxRule{ Definition::LayerSource, MatchChild<LayerSource> }.repeat().optional(),
        SyntaxRule{ Rule_LayerActionRules, MatchChild<LayerAction> }.repeat().optional(),
    };

    static constexpr SyntaxRule Rule_GlobalRules[]{ // <layers>...
        SyntaxRule{ Rule_LayerRules, MatchSibling<Layer> },
        SyntaxRule{ TokenType::ST_EndOfLine },
    };

} // namespace ice::grammar
