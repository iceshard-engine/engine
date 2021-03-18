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
        ColorAttachmentOutput,
        FramentShader,
    };

    struct PipelinePushConstant
    {
        ice::render::ShaderStageFlags shader_stage_flags;
        ice::u32 offset;
        ice::u32 size;
    };

    struct PipelineLayoutInfo
    {
        ice::Span<ice::render::PipelinePushConstant> push_constants;
        ice::Span<ice::render::ResourceSetLayout> resource_layouts;
    };

    struct PipelineInfo
    {
        ice::render::PipelineLayout layout;
        ice::render::Renderpass renderpass;
        ice::Span<ice::render::Shader> shaders;
        ice::Span<ice::render::ShaderStageFlags> shaders_stages;
        ice::Span<ice::render::ShaderInputBinding> shader_bindings;
    };

} // namespace ice::render
