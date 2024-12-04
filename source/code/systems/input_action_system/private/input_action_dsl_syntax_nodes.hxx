#pragma once
#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax_node_types.hxx>
#include <arctic/arctic_syntax_entity.hxx>
#include <ice/base.hxx>

namespace ice::syntax
{

    using arctic::SyntaxNodeData;
    using arctic::SyntaxEntity;

    static constexpr ice::u32 SyntaxEntityBase = static_cast<ice::u32>(SyntaxEntity::E_CallArg) + 1;
    static constexpr SyntaxEntity SyntaxEntity_Layer{ SyntaxEntityBase + 0 };
    static constexpr SyntaxEntity SyntaxEntity_LayerSource{ SyntaxEntityBase + 1 };
    static constexpr SyntaxEntity SyntaxEntity_LayerSourceBinding{ SyntaxEntityBase + 2 };
    static constexpr SyntaxEntity SyntaxEntity_LayerAction{ SyntaxEntityBase + 3 };
    static constexpr SyntaxEntity SyntaxEntity_LayerActionCondition{ SyntaxEntityBase + 3 };
    static constexpr SyntaxEntity SyntaxEntity_LayerActionStep{ SyntaxEntityBase + 3 };

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

    struct LayerActionWhen : SyntaxNodeData
    {
        static constexpr SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity_LayerAction;
        using SyntaxNodeData::SyntaxNodeData;

        arctic::Token type; // when/and(_when)/or(_when)
        arctic::Token source_type; // source/action
        arctic::String source_name; // alpha-num
        arctic::String source_component; // x/y/z
        arctic::Token condition; // .pressed/released/active/inactive/</>/==/>=/<=/!=
        arctic::Token param; // int/float
    };

    struct LayerActionStep : SyntaxNodeData
    {
        static constexpr SyntaxEntity RepresentedSyntaxEntity = SyntaxEntity_LayerAction;
        using SyntaxNodeData::SyntaxNodeData;

        arctic::String source;
        arctic::String destination;
        arctic::Token step;
    };

} // namespace ice::syntax
