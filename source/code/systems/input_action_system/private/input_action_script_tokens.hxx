/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/sort.hxx>
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
        static constexpr arctic::TokenType ASL_KW_Constant{ ASLT_Keywords + 1 };
        static constexpr arctic::TokenType ASL_KW_Source{ ASLT_Keywords + 2 };
        static constexpr arctic::TokenType ASL_KW_Action{ ASLT_Keywords + 3 };
        static constexpr arctic::TokenType ASL_KW_Modifier{ ASLT_Keywords + 4 };

        static constexpr arctic::TokenType ASL_KW_When{ ASLT_Keywords + 10 };
        static constexpr arctic::TokenType ASL_KW_WhenAnd{ ASLT_Keywords + 11 };
        static constexpr arctic::TokenType ASL_KW_WhenOr{ ASLT_Keywords + 12 };

        // Keywords for Device types
        static constexpr arctic::TokenType ASL_KW_Mouse{ ASLT_Keywords + 20 };
        static constexpr arctic::TokenType ASL_KW_Keyboard{ ASLT_Keywords + 21 };
        static constexpr arctic::TokenType ASL_KW_Controller{ ASLT_Keywords + 22 };

        // Keywords for Action and Condition flags
        static constexpr arctic::TokenType ASL_KWF_Once{ ASLT_Keywords + 30 };
        static constexpr arctic::TokenType ASL_KWF_Toggled{ ASLT_Keywords + 31 };
        static constexpr arctic::TokenType ASL_KWF_CheckSeries{ ASLT_Keywords + 32 };

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

        // Get axis component operators
        static constexpr arctic::TokenType ASL_OP_CmpX{ ASLT_Operators + 30 };
        static constexpr arctic::TokenType ASL_OP_CmpY{ ASLT_Operators + 31 };
        static constexpr arctic::TokenType ASL_OP_CmpZ{ ASLT_Operators + 32 };

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

    struct TokenDefinition
    {
        arctic::TokenType type;
        arctic::String value;
    };

    static constexpr std::array<ice::asl::TokenDefinition, 37> Constant_TokenDefinitionsUnsorted{
        TokenDefinition{ TokenType::ASL_KW_Action, "action" },
        TokenDefinition{ TokenType::ASL_KW_Constant, "constant" },
        TokenDefinition{ TokenType::ASL_KW_Controller, "gamepad" },
        TokenDefinition{ TokenType::ASL_KW_Controller, "gp" },
        TokenDefinition{ TokenType::ASL_KW_Keyboard, "kb" },
        TokenDefinition{ TokenType::ASL_KW_Keyboard, "key" },
        TokenDefinition{ TokenType::ASL_KW_Layer, "layer" },
        TokenDefinition{ TokenType::ASL_KW_Modifier, "mod" },
        TokenDefinition{ TokenType::ASL_KW_Mouse, "mouse" },
        TokenDefinition{ TokenType::ASL_KW_Mouse, "mp" },
        TokenDefinition{ TokenType::ASL_KW_Source, "source" },
        TokenDefinition{ TokenType::ASL_KW_When, "when" },
        TokenDefinition{ TokenType::ASL_KW_WhenAnd, "and" },
        TokenDefinition{ TokenType::ASL_KW_WhenOr, "or" },
        TokenDefinition{ TokenType::ASL_KWF_CheckSeries, "series" },
        TokenDefinition{ TokenType::ASL_KWF_Once, "once" },
        TokenDefinition{ TokenType::ASL_KWF_Toggled, "toggled" },
        TokenDefinition{ TokenType::ASL_NT_Axis1D, "axis1d" },
        TokenDefinition{ TokenType::ASL_NT_Axis2D, "axis2d" },
        TokenDefinition{ TokenType::ASL_NT_Axis3D, "axis3d" },
        TokenDefinition{ TokenType::ASL_NT_Bool, "bool" },
        TokenDefinition{ TokenType::ASL_NT_Button, "button" },
        TokenDefinition{ TokenType::ASL_NT_Float1, "float1" },
        TokenDefinition{ TokenType::ASL_NT_Float2, "float2" },
        TokenDefinition{ TokenType::ASL_NT_Float3, "float3" },
        TokenDefinition{ TokenType::ASL_NT_Object, "object" },
        TokenDefinition{ TokenType::ASL_OP_Activate, "activate" },
        TokenDefinition{ TokenType::ASL_OP_Deactivate, "deactivate" },
        TokenDefinition{ TokenType::ASL_OP_IsActive, "active" },
        TokenDefinition{ TokenType::ASL_OP_IsInactive, "inactive" },
        TokenDefinition{ TokenType::ASL_OP_IsPressed, "pressed" },
        TokenDefinition{ TokenType::ASL_OP_IsReleased, "released" },
        TokenDefinition{ TokenType::ASL_OP_Max, "max" },
        TokenDefinition{ TokenType::ASL_OP_Min, "min" },
        TokenDefinition{ TokenType::ASL_OP_Reset, "reset" },
        TokenDefinition{ TokenType::ASL_OP_Time, "time" },
        TokenDefinition{ TokenType::ASL_OP_Toggle, "toggle" },
    };

    constexpr auto operator<=>(ice::asl::TokenDefinition left, ice::asl::TokenDefinition right) noexcept
    {
        return left.value <=> right.value;
    }

    constexpr bool operator==(ice::asl::TokenDefinition left, ice::asl::TokenDefinition right) noexcept
    {
        return left.value == right.value;
    }

    static constexpr std::array<ice::asl::TokenDefinition, 37> Constant_TokenDefinitions
        = ice::constexpr_sort_stdarray(Constant_TokenDefinitionsUnsorted, 0);

} // namespace ice::asl
