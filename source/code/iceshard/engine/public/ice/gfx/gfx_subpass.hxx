/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>

namespace ice::gfx
{

    struct GfxSubpass_Primitives
    {
        static constexpr ice::StringID ResName_PipelineLayout =
            "ice.gfx_subpass.primitives_pipeline_layout"_sid;

        static constexpr ice::StringID ResName_ResourceLayout =
            "ice.gfx_subpass.primitives_resource_layout"_sid;

        static constexpr ice::u32 ResConst_CameraUniformSet = 0;

        static constexpr ice::u32 ResConst_CameraUniformBinding = 0;
    };

    struct GfxSubpass_Terrain : GfxSubpass_Primitives
    {
        static constexpr ice::StringID ResName_PipelineLayout =
            "ice.gfx_subpass.terrain_pipeline_layout"_sid;

        static constexpr ice::StringID ResName_ResourceLayout =
            "ice.gfx_subpass.terrain_resource_layout"_sid;

        static constexpr ice::u32 ResConst_TerrainHeightmapSet = 1;

        static constexpr ice::u32 ResConst_TerrainHeightmapBinding = 0;
        static constexpr ice::u32 ResConst_TerrainSamplerBinding = 1;
        static constexpr ice::u32 ResConst_TerrainUniformBinding = 2;
    };

    struct GfxSubpass_ImGui
    {
        static constexpr ice::StringID ResName_PipelineLayout =
            "ice.gfx_subpass.imgui_pipeline_layout"_sid;

        static constexpr ice::StringID ResName_ResourceLayout =
            "ice.gfx_subpass.imgui_resource_layout"_sid;

        static constexpr ice::u32 ResConst_FontTextureBinding = 0;
        static constexpr ice::u32 ResConst_FontSamplerBinding = 1;
    };

} // namespace ice::gfx
