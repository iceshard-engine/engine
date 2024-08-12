/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/span.hxx>
#include <ice/asset_category.hxx>

namespace ice::render
{

    enum class Shader : ice::uptr
    {
        Invalid = 0x0
    };

    enum class ShaderStageFlags : ice::u32
    {
        None = 0x0,

        VertexStage = 0x0001,
        FragmentStage = 0x0002,
        GeometryStage = 0x0004,
        TesselationControlStage = 0x0008,
        TesselationEvaluationStage = 0x0010,

        All = VertexStage | FragmentStage
            | GeometryStage | TesselationControlStage | TesselationEvaluationStage
    };

    enum class ShaderAttribType
    {
        Vec1f,
        Vec2f,
        Vec3f,
        Vec4f,
        Vec4f_Unorm8,
        Vec1u,
        Vec1i,
    };

    struct ShaderInfo
    {
        ice::Data shader_data;
    };

    struct ShaderInputAttribute
    {
        ice::u32 location;
        ice::u32 offset;
        ice::render::ShaderAttribType type;
    };

    struct ShaderInputBinding
    {
        ice::u32 binding;
        ice::u32 stride;
        ice::u32 instanced;
        ice::Span<ice::render::ShaderInputAttribute const> attributes;
    };

    static constexpr ice::AssetCategory AssetCategory_Shader = ice::make_asset_category("ice/render_system/shader");

} // namespace ice::render
