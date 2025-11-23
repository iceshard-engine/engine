/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax_node_types.hxx>
#include <arctic/arctic_syntax_entity.hxx>

namespace ice::asl
{

    static constexpr ice::u32 ASLE_Base = ice::u32(arctic::SyntaxEntity::E_CallArg) + 1;
    static constexpr ice::u32 ASLE_Definitions = ASLE_Base + 0;
    static constexpr ice::u32 ASLE_Expressions = ASLE_Base + 100;

    using arctic::SyntaxNodeData;

    struct SyntaxEntity
    {
        static constexpr arctic::SyntaxEntity ASL_D_Layer{ ASLE_Definitions + 0 };
        static constexpr arctic::SyntaxEntity ASL_D_LayerConstant{ ASLE_Definitions + 1 };
        static constexpr arctic::SyntaxEntity ASL_D_LayerSource{ ASLE_Definitions + 2 };
        static constexpr arctic::SyntaxEntity ASL_D_LayerAction{ ASLE_Definitions + 3 };

        static constexpr arctic::SyntaxEntity ASL_E_LayerSourceBinding{ ASLE_Expressions + 0 };
        static constexpr arctic::SyntaxEntity ASL_E_LayerActionStep{ ASLE_Expressions + 1 };
        static constexpr arctic::SyntaxEntity ASL_E_LayerActionCondition{ ASLE_Definitions + 2 };
        static constexpr arctic::SyntaxEntity ASL_E_LayerActionModifier{ ASLE_Definitions + 3 };
    };

#define NodeData(type) \
    static constexpr arctic::SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity::type; \
    using arctic::SyntaxNodeData::SyntaxNodeData

    struct Layer : arctic::SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_D_Layer);

        arctic::String name;
    };

    struct LayerConstant : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_D_LayerConstant);

        arctic::String name;
        arctic::Token param;
    };

    struct LayerSource : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_D_LayerSource);

        arctic::Token type;
        arctic::String name;
    };

    struct LayerAction : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_D_LayerAction);

        arctic::String name;
        arctic::Token type;
        bool flag_toggled = false;
        bool flag_once = false;
    };

    struct LayerSourceBinding : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_E_LayerSourceBinding);

        arctic::Token device;
        arctic::String source;
    };

    struct LayerActionCondition : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_E_LayerActionCondition);

        arctic::Token type; // when/and(_when)/or(_when)
        arctic::Token source_type = {}; // source/action
        arctic::String source_name; // alpha-num[.x/y/z]
        //arctic::String source_component; // x/y/z
        arctic::Token condition; // .pressed/released/active/inactive/</>/==/>=/<=/!=
        arctic::Token param; // int/float
        bool flag_series = false;
    };

    struct LayerActionStep : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_E_LayerActionStep);

        arctic::Token source_type;
        arctic::String source;
        arctic::String destination;
        arctic::Token step;
    };

    struct LayerActionModifier : SyntaxNodeData
    {
        NodeData(SyntaxEntity::ASL_E_LayerActionModifier);

        arctic::String component;
        arctic::Token operation;
        arctic::String param;
    };

#undef NodeData

} // namespace ice::syntax
