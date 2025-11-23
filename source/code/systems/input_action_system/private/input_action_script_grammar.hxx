/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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

        static constexpr SyntaxRule NumberTypes[]{
            SyntaxRule{ TokenType::CT_Number },
            SyntaxRule{ TokenType::CT_NumberFloat },
        };

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

        static constexpr SyntaxRule ActionTypes[]{ // button, axis1d, axis2d or axis3d
            SyntaxRule{ TokenType::ASL_NT_Bool },
            SyntaxRule{ TokenType::ASL_NT_Float1 },
            SyntaxRule{ TokenType::ASL_NT_Float2 },
            SyntaxRule{ TokenType::ASL_NT_Float3 },
            SyntaxRule{ TokenType::ASL_NT_Object },
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

        static constexpr SyntaxRule NameExtended[]{
            SyntaxRule{ TokenType::CT_Symbol, NameField, SyntaxRule::store_value_extend<arctic::String> },
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

    struct Constant
    {
        using SymbolType = Symbol<&LayerConstant::name>;

        static constexpr SyntaxRule Rules[]{
            SyntaxRule{ TokenType::ASL_KW_Constant },
            SyntaxRule{ TokenType::CT_Symbol, &LayerConstant::name },
            SyntaxRule{ TokenType::CT_Dot },
            SyntaxRule{ SymbolType::NameExtended, MatchAll },
            SyntaxRule{ TokenType::OP_Assign },
            SyntaxRule{ TokenList::NumberTypes, MatchFirst, &LayerConstant::param },
            SyntaxRule{ TokenType::ST_EndOfLine },
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
            SyntaxRule{ SourceBindingNext, MatchSibling<LayerSourceBinding> }.optional().repeat(),
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

        static constexpr SyntaxRule OperationList[]{
            SyntaxRule{ CommandOperation, MatchAll },
            SyntaxRule{ ArithmeticOperation, MatchAll },
        };

        static constexpr SyntaxRule Rules[]{
            // If we don't end up using the destination string it's fine
            //   but we require the '.' character to be used when setting destinations via builder.
            SyntaxRule{ TokenType::CT_Dot, &LayerActionStep::destination },
            SyntaxRule{ OperationList, MatchFirst },
            SyntaxRule{ TokenType::ST_EndOfLine }, // We need to start and end with newlines
        };
    };

    struct Condition
    {
        using SymbolType = Symbol<&LayerActionCondition::source_name, &LayerActionCondition::source_type>;

        static constexpr SyntaxRule StateCheck[]{
            SyntaxRule{ TokenType::ASL_OP_IsPressed },
            SyntaxRule{ TokenType::ASL_OP_IsReleased },
            SyntaxRule{ TokenType::ASL_OP_IsActive },
            SyntaxRule{ TokenType::ASL_OP_IsInactive },
            SyntaxRule{ TokenType::KW_True },
        };

        static constexpr SyntaxRule ValueConditions[]{
            SyntaxRule{ TokenType::OP_Equal },
            SyntaxRule{ TokenType::OP_NotEqual },
            SyntaxRule{ TokenType::OP_Less },
            SyntaxRule{ TokenType::OP_LessOrEqual },
            SyntaxRule{ TokenType::OP_Greater },
            SyntaxRule{ TokenType::OP_GreaterOrEqual },
        };

        static constexpr SyntaxRule ValueCheck[]{
            SyntaxRule{ TokenList::XYZ, MatchFirst, &LayerActionCondition::source_name, arctic::SyntaxRule::store_value_extend<arctic::String> },
            SyntaxRule{ ValueConditions, MatchFirst, &LayerActionCondition::condition },
            SyntaxRule{ TokenList::NumberTypes, MatchFirst, &LayerActionCondition::param }
        };

        static constexpr SyntaxRule ConditionChecks[]{
            SyntaxRule{ StateCheck, MatchFirst, &LayerActionCondition::condition },
            SyntaxRule{ ValueCheck, MatchAll },
        };

        static constexpr SyntaxRule ConditionExpression[]{
            SyntaxRule{ TokenType::CT_Dot },
            SyntaxRule{ ConditionChecks, MatchFirst },
        };

        static constexpr SyntaxRule FlagList[]{
            SyntaxRule{ TokenType::ASL_KWF_CheckSeries, &LayerActionCondition::flag_series }
        };

        static constexpr SyntaxRule ConditionFlags[]{
            SyntaxRule{ TokenType::CT_Comma },
            SyntaxRule{ FlagList, MatchFirst },
            SyntaxRule{ TokenType::ST_EndOfLine }.noadvance(),
        };

        static constexpr SyntaxRule AndOrTokens[]{
            SyntaxRule{ TokenType::ASL_KW_WhenAnd },
            SyntaxRule{ TokenType::ASL_KW_WhenOr },
        };

        static constexpr SyntaxRule WhenCondition[]{
            SyntaxRule{ TokenType::ASL_KW_When, &LayerActionCondition::type },
            SyntaxRule{ SymbolType::NameWithCategory, MatchAll }.optional(),
            SyntaxRule{ ConditionExpression, MatchAll },
        };

        static constexpr SyntaxRule AndOrCondition[]{
            SyntaxRule{ AndOrTokens, MatchFirst, &LayerActionCondition::type },
            SyntaxRule{ SymbolType::NameWithCategory, MatchAll },
            SyntaxRule{ ConditionExpression, MatchAll },
        };

        static auto internal_condition_matcher(
            arctic::SyntaxRule const&,
            arctic::MatchContext& ctx
        ) noexcept -> arctic::ParseState;

        static constexpr SyntaxRule Rules[]{
            SyntaxRule{ internal_condition_matcher }.optional().repeat().noadvance()
        };
    };

    struct Modifier
    {
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

        static constexpr SyntaxRule Rules[]{
            SyntaxRule{ TokenType::ASL_KW_Modifier },
            SyntaxRule{ Rule_LayerActionModifierComponentRules, MatchAll },
            SyntaxRule{ Rule_LayerActionModifierOperationListRules, MatchFirst, &LayerActionModifier::operation },
            SyntaxRule{ TokenList::NumberTypes, MatchFirst, &LayerActionModifier::param },
            SyntaxRule{ TokenType::ST_EndOfLine },
        };
    };

    struct Action
    {
        static constexpr SyntaxRule ActionFlagList[]{
            SyntaxRule{ TokenType::ASL_KWF_Once, &LayerAction::flag_once },
            SyntaxRule{ TokenType::ASL_KWF_Toggled, &LayerAction::flag_toggled },
        };

        static constexpr SyntaxRule ActionFlags[]{
            SyntaxRule{ TokenType::CT_Comma },
            SyntaxRule{ ActionFlagList, MatchFirst },
        };

        static constexpr SyntaxRule Rules[]{
            SyntaxRule{ TokenType::ASL_KW_Action },
            SyntaxRule{ TokenType::CT_Symbol, &LayerAction::name },
            SyntaxRule{ TokenType::CT_Colon },
            SyntaxRule{ TokenList::ActionTypes, MatchFirst, &LayerAction::type },
            SyntaxRule{ ActionFlags, MatchAll }.optional(),
            SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
            SyntaxRule{ Condition::Rules, MatchAll }.optional(),
            SyntaxRule{ Modifier::Rules, MatchChild<LayerActionModifier> }.repeat().optional(),
        };
    };

    struct Definition
    {
        static constexpr SyntaxRule LayerSourceRules[]{
            SyntaxRule{ TokenType::ASL_KW_Source },
            SyntaxRule{ TokenList::InputNativeTypes, MatchFirst, &LayerSource::type },
            SyntaxRule{ TokenType::CT_Symbol, &LayerSource::name },
            SyntaxRule{ SourceBinding::Rules, MatchChild<LayerSourceBinding> }.optional(),
            SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        };

        static constexpr SyntaxRule LayerRules[]{
            SyntaxRule{ TokenType::ASL_KW_Layer },
            SyntaxRule{ TokenType::CT_Symbol, &Layer::name },
            SyntaxRule{ TokenType::CT_Colon },
            SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
            SyntaxRule{ Constant::Rules, MatchChild<LayerConstant> }.optional().repeat(),
            SyntaxRule{ LayerSourceRules, MatchChild<LayerSource> }.optional().repeat(),
            SyntaxRule{ Action::Rules, MatchChild<LayerAction> }.optional().repeat(),
        };
    };

    static constexpr SyntaxRule ScriptRules[]{
        SyntaxRule{ Definition::LayerRules, MatchSibling<Layer> },
        SyntaxRule{ TokenType::ST_EndOfLine }, // consume any outstanding newlines
    };

} // namespace ice::grammar
