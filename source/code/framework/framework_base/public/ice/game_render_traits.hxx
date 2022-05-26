#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_trait.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_SetDefaultCamera = "action/camera/set-default"_shard;
    static constexpr ice::Shard Shard_DebugDrawCommand = "action/debug-render/draw_command"_shard;
    static constexpr ice::Shard Shard_DrawTextCommand = "action/render/draw-text"_shard;

    static constexpr ice::StringID Constant_TraitName_RenderBase
        = "ice.base-framework.trait-render-base"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderClear
        = "ice.base-framework.trait-render-clear"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderPostprocess
        = "ice.base-framework.trait-render-postprocess"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderFinish
        = "ice.base-framework.trait-render-finish"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderCamera
        = "ice.base-framework.trait-render-camera"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderTextureLoader
        = "ice.base-framework.trait-render-texture-loader"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderDebug
        = "ice.base-framework.trait-render-debug"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderTilemap
        = "ice.base-framework.trait-render-tilemap"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderSprites
        = "ice.base-framework.trait-render-sprites"_sid;

    static constexpr ice::StringID Constant_TraitName_RenderGlyphs
        = "ice.base-framework.trait-render-glyphs"_sid;

    static constexpr ice::StringID Constant_GfxStage_Clear
        = "ice.base-framework.gfx-stage-clear"_sid;

    static constexpr ice::StringID Constant_GfxStage_Postprocess
        = "ice.base-framework.gfx-stage-postprocess"_sid;

    static constexpr ice::StringID Constant_GfxStage_Finish
        = "ice.base-framework.gfx-stage-finish"_sid;

    static constexpr ice::StringID Constant_GfxStage_DrawTilemap
        = "ice.base-framework.gfx-stage-draw-tilemap"_sid;

    static constexpr ice::StringID Constant_GfxStage_DrawSprites
        = "ice.base-framework.gfx-stage-draw-sprites"_sid;

    static constexpr ice::StringID Constant_GfxStage_DrawGlyphs
        = "ice.base-framework.gfx-stage-draw-glyphs"_sid;

    static constexpr ice::StringID Constant_GfxStage_DrawDebug
        = "ice.base-framework.gfx-stage-draw-debug"_sid;

    struct DrawTextCommand
    {
        ice::vec2u position;
        ice::Utf8String text;
        ice::Utf8String font;
        ice::u32 font_size = 16;
    };

    struct DebugDrawCommand
    {
        ice::u32 vertex_count;
        ice::vec3f const* vertex_list;
        ice::vec1u const* vertex_color_list;
    };

    struct DebugDrawCommandList
    {
        ice::u32 list_size;
        ice::DebugDrawCommand const* list;
    };

} // namespace ice

template<>
constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::DebugDrawCommandList const*> = ice::payload_id("ice::DebugDrawCommandList const*");

template<>
constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::DrawTextCommand const*> = ice::payload_id("ice::DrawTextCommand const*");
