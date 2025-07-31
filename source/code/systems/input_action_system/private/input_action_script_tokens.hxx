#pragma once
#include <ice/base.hxx>
#include <arctic/arctic_token.hxx>

namespace ice::asl
{

    static constexpr ice::u32 ASLT_Base = ice::u32(arctic::TokenType::ST_Any) + 1;
    static constexpr ice::u32 ASLT_Keywords = ASLT_Base + 1000;
    static constexpr ice::u32 ASLT_Operators = ASLT_Base + 2000;
    static constexpr ice::u32 ASLT_NativeTypes = ASLT_Base + 3000;

    struct TokenType
    {
        using enum arctic::TokenType;

        static constexpr arctic::TokenType ASL_KW_Layer{ ASLT_Keywords + 0 };
        static constexpr arctic::TokenType ASL_KW_Source{ ASLT_Keywords + 1 };
        static constexpr arctic::TokenType ASL_KW_Action{ ASLT_Keywords + 2 };

        static constexpr arctic::TokenType ASL_OP_Pressed{ ASLT_Operators + 0 };
        static constexpr arctic::TokenType ASL_OP_Released{ ASLT_Operators + 1 };
        static constexpr arctic::TokenType ASL_OP_Active{ ASLT_Operators + 2 };
        static constexpr arctic::TokenType ASL_OP_Inactive{ ASLT_Operators + 3 };

        static constexpr arctic::TokenType ASL_NT_Axis1D{ ASLT_NativeTypes + 0 };
        static constexpr arctic::TokenType ASL_NT_Axis2D{ ASLT_NativeTypes + 1 };
        static constexpr arctic::TokenType ASL_NT_Axis3D{ ASLT_NativeTypes + 2 };
    };


    static constexpr ice::u32 UCT_Base = ASLT_Base;
    static constexpr arctic::TokenType UCT_InputTypeButton{ UCT_Base + 2 };
    static constexpr arctic::TokenType UCT_InputBindingKeyboard{ UCT_Base + 6 };
    static constexpr arctic::TokenType UCT_InputBindingMouse{ UCT_Base + 7 };
    static constexpr arctic::TokenType UCT_InputBindingPad{ UCT_Base + 8 };
    static constexpr arctic::TokenType UCT_ActionTypeBool{ UCT_Base + 10 };
    static constexpr arctic::TokenType UCT_ActionTypeFloat1{ UCT_Base + 11 };
    static constexpr arctic::TokenType UCT_ActionTypeFloat2{ UCT_Base + 12 };
    static constexpr arctic::TokenType UCT_ActionTypeFloat3{ UCT_Base + 13 };
    static constexpr arctic::TokenType UCT_ActionTypeObject{ UCT_Base + 14 };
    static constexpr arctic::TokenType UCT_ActionFlagOnce{ UCT_Base + 15 };
    static constexpr arctic::TokenType UCT_ActionFlagToggled{ UCT_Base + 16 };
    //static constexparctic::r TokenType UCT_ActionFlagAccumulated{ UCT_Base + 17 };
    static constexpr arctic::TokenType UCT_When{ UCT_Base + 18 };
    static constexpr arctic::TokenType UCT_WhenAnd{ UCT_Base + 19 };
    static constexpr arctic::TokenType UCT_WhenOr{ UCT_Base + 20 };
    static constexpr arctic::TokenType UCT_WhenPressed{ UCT_Base + 21 };
    static constexpr arctic::TokenType UCT_WhenReleased{ UCT_Base + 22 };
    static constexpr arctic::TokenType UCT_WhenActive{ UCT_Base + 23 };
    static constexpr arctic::TokenType UCT_WhenInactive{ UCT_Base + 24 };
    static constexpr arctic::TokenType UCT_WhenFlagCheckSeries{ UCT_Base + 25 };
    static constexpr arctic::TokenType UCT_StepActivate{ UCT_Base + 26 };
    static constexpr arctic::TokenType UCT_StepDeactivate{ UCT_Base + 27 };
    static constexpr arctic::TokenType UCT_StepToggle{ UCT_Base + 28 };
    static constexpr arctic::TokenType UCT_StepReset{ UCT_Base + 29 };
    static constexpr arctic::TokenType UCT_StepTime{ UCT_Base + 30 };
    static constexpr arctic::TokenType UCT_Modifier{ UCT_Base + 31 };
    static constexpr arctic::TokenType UCT_ModifierOpMin{ UCT_Base + 32 };
    static constexpr arctic::TokenType UCT_ModifierOpMax{ UCT_Base + 33 };

} // namespace ice::asl
