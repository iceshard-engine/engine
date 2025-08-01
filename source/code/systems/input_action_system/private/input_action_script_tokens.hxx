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

        // General ASL keywords
        static constexpr arctic::TokenType ASL_KW_Layer{ ASLT_Keywords + 0 };
        static constexpr arctic::TokenType ASL_KW_Source{ ASLT_Keywords + 1 };
        static constexpr arctic::TokenType ASL_KW_Action{ ASLT_Keywords + 2 };
        static constexpr arctic::TokenType ASL_KW_Modifier{ ASLT_Keywords + 3 };

        static constexpr arctic::TokenType ASL_KW_When{ ASLT_Keywords + 4 };
        static constexpr arctic::TokenType ASL_KW_WhenAnd{ ASLT_Keywords + 5 };
        static constexpr arctic::TokenType ASL_KW_WhenOr{ ASLT_Keywords + 6 };

        // Keywords for Device types
        static constexpr arctic::TokenType ASL_KW_Mouse{ ASLT_Keywords + 10 };
        static constexpr arctic::TokenType ASL_KW_Keyboard{ ASLT_Keywords + 11 };
        static constexpr arctic::TokenType ASL_KW_Controller{ ASLT_Keywords + 12 };

        // Keywords for Action and Condition flags
        static constexpr arctic::TokenType ASL_KWF_Once{ ASLT_Keywords + 20 };
        static constexpr arctic::TokenType ASL_KWF_Toggled{ ASLT_Keywords + 21 };
        static constexpr arctic::TokenType ASL_KWF_CheckSeries{ ASLT_Keywords + 22 };

        // Condition operators (checks)
        static constexpr arctic::TokenType ASL_OP_IsPressed{ ASLT_Operators + 0 };
        static constexpr arctic::TokenType ASL_OP_IsReleased{ ASLT_Operators + 1 };
        static constexpr arctic::TokenType ASL_OP_IsActive{ ASLT_Operators + 2 };
        static constexpr arctic::TokenType ASL_OP_IsInactive{ ASLT_Operators + 3 };

        // Step operators (actions)
        static constexpr arctic::TokenType ASL_OP_Activate{ ASLT_Operators + 10 };
        static constexpr arctic::TokenType ASL_OP_Deactivate{ ASLT_Operators + 11 };
        static constexpr arctic::TokenType ASL_OP_Toggle{ ASLT_Operators + 12 };
        static constexpr arctic::TokenType ASL_OP_Reset{ ASLT_Operators + 13 };
        static constexpr arctic::TokenType ASL_OP_Time{ ASLT_Operators + 14 };

        // Modifier operators
        static constexpr arctic::TokenType ASL_OP_Min{ ASLT_Operators + 20 };
        static constexpr arctic::TokenType ASL_OP_Max{ ASLT_Operators + 21 };

        // Native types representing input source
        static constexpr arctic::TokenType ASL_NT_Axis1D{ ASLT_NativeTypes + 0 };
        static constexpr arctic::TokenType ASL_NT_Axis2D{ ASLT_NativeTypes + 1 };
        static constexpr arctic::TokenType ASL_NT_Axis3D{ ASLT_NativeTypes + 2 };
        static constexpr arctic::TokenType ASL_NT_Button{ ASLT_NativeTypes + 3 };

        // Native types representing outputs
        static constexpr arctic::TokenType ASL_NT_Bool{ ASLT_NativeTypes + 10 };
        static constexpr arctic::TokenType ASL_NT_Float1{ ASLT_NativeTypes + 11 };
        static constexpr arctic::TokenType ASL_NT_Float2{ ASLT_NativeTypes + 12 };
        static constexpr arctic::TokenType ASL_NT_Float3{ ASLT_NativeTypes + 13 };
        static constexpr arctic::TokenType ASL_NT_Object{ ASLT_NativeTypes + 14 };
    };

} // namespace ice::asl
