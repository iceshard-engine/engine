/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_declarations.hxx>
#include <ice/render/render_shader.hxx>

namespace ice::render
{

    enum class PipelineLayout : ice::uptr
    {
        Invalid = 0x0
    };

    enum class Pipeline : ice::uptr
    {
        Invalid = 0x0
    };

    enum class PipelineStage : ice::u32
    {
        BottomOfPipe,
        TopOfPipe,
        Transfer,
        ColorAttachmentOutput,
        FramentShader,
    };

    enum class CullMode : ice::u32
    {
        Disabled,
        BackFace,
        FrontFace,
    };

    enum class FrontFace : ice::u32
    {
        ClockWise,
        CounterClockWise,
    };

    enum class PrimitiveTopology : ice::u32
    {
        LineStrip,
        LineStripWithAdjency,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        PatchList,
    };

    enum class DynamicState : ice::u32
    {
        None = 0x0,
        Viewport = 0x0000'0001,
        Scissor = 0x0000'0002,
    };

    struct PipelinePushConstant
    {
        ice::render::ShaderStageFlags shader_stage_flags;
        ice::u32 offset;
        ice::u32 size;
    };

    struct PipelineLayoutInfo
    {
        ice::Span<ice::render::PipelinePushConstant const> push_constants;
        ice::Span<ice::render::ResourceSetLayout const> resource_layouts;
    };

    struct PipelineProgramInfo
    {
        ice::render::Shader shader;
        ice::render::ShaderStageFlags stage;
        ice::String entry_point;
    };

    struct PipelineInfo
    {
        ice::render::PipelineLayout layout;
        ice::render::Renderpass renderpass;
        ice::Span<ice::render::PipelineProgramInfo const> shaders;
        ice::Span<ice::render::ShaderInputBinding const> vertex_bindings;

        ice::render::PrimitiveTopology primitive_topology = PrimitiveTopology::TriangleList;
        ice::render::CullMode cull_mode = CullMode::Disabled;
        ice::render::FrontFace front_face = FrontFace::ClockWise;
        ice::render::DynamicState dynamic_states = DynamicState::Viewport | DynamicState::Scissor;

        ice::u32 subpass_index;
        bool depth_test = true;
    };

} // namespace ice::render
