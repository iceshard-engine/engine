#pragma once
#include <ice/string_types.hxx>

#include <arctic/arctic.hxx>
#include <arctic/arctic_syntax.hxx>
#include <arctic/arctic_syntax_visitor.hxx>
#include <arctic/arctic_syntax_node_types.hxx>

namespace ice
{

    struct ActionInputParserEvents;

    bool parse_action_input_definition(
        ice::String definition,
        ice::ActionInputParserEvents& handler
    ) noexcept;

    namespace syntax
    {

        using arctic::SyntaxNodeData;
        using arctic::SyntaxEntity;

        static constexpr ice::u32 SyntaxEntityBase = static_cast<ice::u32>(SyntaxEntity::E_CallArg) + 1;
        static constexpr SyntaxEntity SyntaxEntity_Layer{ SyntaxEntityBase + 0 };
        static constexpr SyntaxEntity SyntaxEntity_LayerSource{ SyntaxEntityBase + 1 };
        static constexpr SyntaxEntity SyntaxEntity_LayerSourceBinding{ SyntaxEntityBase + 2 };
        static constexpr SyntaxEntity SyntaxEntity_LayerAction{ SyntaxEntityBase + 3 };

        struct Layer : SyntaxNodeData
        {
            static constexpr SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity_Layer;
            using SyntaxNodeData::SyntaxNodeData;

            arctic::String name;
        };

        struct LayerSource : SyntaxNodeData
        {
            static constexpr SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity_LayerSource;
            using SyntaxNodeData::SyntaxNodeData;

            arctic::Token type;
            arctic::String name;
        };

        struct LayerInputBinding : SyntaxNodeData
        {
            static constexpr SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity_LayerSourceBinding;
            using SyntaxNodeData::SyntaxNodeData;

            arctic::Token device;
            arctic::String source;
        };

        struct LayerAction : SyntaxNodeData
        {
            static constexpr SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity_LayerAction;
            using SyntaxNodeData::SyntaxNodeData;

            arctic::String name;
            arctic::Token type;
        };

    } // namespace syntax

    using ActionInputParserEventsBase = arctic::SyntaxVisitorGroup<
        arctic::syntax::Root,
        ice::syntax::Layer
    >;

    struct ActionInputParserEvents : ice::ActionInputParserEventsBase
    {
        arctic::SyntaxNode<arctic::syntax::Root> root;

        void visit(arctic::SyntaxNode<> node) noexcept override final
        {
            ActionInputParserEventsBase::visit(node);
        }

        void visit(arctic::SyntaxNode<arctic::syntax::Root> node) noexcept = 0;

        void visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept = 0;
    };
    namespace grammar
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
        static constexpr TokenType UCT_InputBindingPad{ UCT_Base + 7 };
        static constexpr TokenType UCT_Action{ UCT_Base + 8 };
        static constexpr TokenType UCT_ActionTypeBool{ UCT_Base + 9 };
        static constexpr TokenType UCT_ActionTypeFloat1{ UCT_Base + 10 };
        static constexpr TokenType UCT_ActionTypeFloat2{ UCT_Base + 11 };
        static constexpr TokenType UCT_ActionTypeFloat3{ UCT_Base + 12 };
        static constexpr TokenType UCT_When{ UCT_Base + 13 };

        static constexpr SyntaxRule Rule_ColonOrCommaRules[]{ // , or :
            SyntaxRule{ TokenType::CT_Colon },
            SyntaxRule{ TokenType::CT_Comma },
        };

        static constexpr SyntaxRule Rule_ParenOrCommaRules[]{ // , or :
            SyntaxRule{ TokenType::CT_ParenOpen },
            SyntaxRule{ TokenType::CT_Comma },
        };

        static constexpr SyntaxRule Rule_LayerInputBindingDeviceRules[]{ // kb/keyboard or pad
            SyntaxRule{ UCT_InputBindingKeyboard },
            SyntaxRule{ UCT_InputBindingPad },
        };

        static constexpr SyntaxRule Rule_LayerInputBindingRules[]{ // ': <dev>.<src>' or ', <dev>.<src>'
            SyntaxRule{ Rule_ColonOrCommaRules, MatchFirst },
            SyntaxRule{ Rule_LayerInputBindingDeviceRules, MatchFirst, &syntax::LayerInputBinding::device },
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
            SyntaxRule{ Rule_LayerInputBindingRules, MatchChild<syntax::LayerInputBinding> }.repeat().optional(),
            SyntaxRule{ TokenType::ST_EndOfLine }.repeat(),
        };

        static constexpr SyntaxRule Rule_LayerActionWhenTargetTypeRules[]{
            SyntaxRule{ UCT_Source },
            SyntaxRule{ UCT_Action },
        };

        static constexpr SyntaxRule Rule_LayerActionWhenTargetNameRules[]{
            SyntaxRule{ Rule_ParenOrCommaRules, MatchFirst },
            SyntaxRule{ TokenType::CT_Symbol }, // Capture name
        };

        static constexpr SyntaxRule Rule_LayerActionWhenTargetRules[]{
            SyntaxRule{ Rule_LayerActionWhenTargetTypeRules, MatchFirst },
            SyntaxRule{ TokenType::CT_ParenOpen }.noadvance(),
            SyntaxRule{ TokenType::CT_ParenClose },
        };

        static constexpr SyntaxRule Rule_LayerActionWhenRules[]{
            SyntaxRule{ UCT_When },
            SyntaxRule{ Rule_LayerActionWhenTargetRules, MatchFirst }, // TODO: Capture
            SyntaxRule{ TokenType::CT_Colon },
            SyntaxRule{ TokenType::CT_Dot }, // TODO: Make part of special keyword
            SyntaxRule{ TokenType::CT_Symbol }, // TODO: Mape part of special symbol and capture
            SyntaxRule{ TokenType::ST_EndOfLine },
        };

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
            // SyntaxRule{ Rule_LayerActionRules, MatchChild<syntax::LayerAction> }.repeat().optional(),
        };

        static constexpr SyntaxRule Rule_LayerRules[]{ // layer <name>: <sources...> <actions...>
            SyntaxRule{ UCT_Layer },
            SyntaxRule{ TokenType::CT_Symbol, &syntax::Layer::name },
            SyntaxRule{ TokenType::CT_Colon },
            SyntaxRule{ TokenType::ST_EndOfLine },
            SyntaxRule{ Rule_LayerSourceRules, MatchChild<syntax::LayerSource> }.repeat().optional(),
            SyntaxRule{ Rule_LayerActionRules, MatchChild<syntax::LayerAction> }.repeat().optional(),
        };

        static constexpr SyntaxRule Rule_GlobalRules[]{ // <layers>...
            SyntaxRule{ Rule_LayerRules, MatchSibling<syntax::Layer> },
            SyntaxRule{ TokenType::ST_EndOfLine },
        };

    } // namespace grammar

} // namespace ice
