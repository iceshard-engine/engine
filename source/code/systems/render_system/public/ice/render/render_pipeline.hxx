#pragma once
#include <ice/render/render_pass.hxx>
#include <ice/render/render_resource.hxx>

namespace ice::render
{

    enum class ShaderStageFlags : ice::u32
    {
        None = 0x0,
        VertexStage = 0x0001,
        FragmentStage = 0x0002,
    };

    constexpr auto operator|(ShaderStageFlags left, ShaderStageFlags right) noexcept -> ShaderStageFlags
    {
        ice::u32 left_value = static_cast<ice::u32>(left);
        ice::u32 right_value = static_cast<ice::u32>(right);
        return static_cast<ShaderStageFlags>(left_value | right_value);
    }

    struct ShaderInfo
    {
        ice::Data shader_data;
        ice::render::ShaderStageFlags shader_stage;
    };

    enum class Shader : ice::uptr
    {
        Invalid = 0x0
    };

    enum class ShaderAttribType
    {
        Vec4f,
        Vec3f,
        Vec2f,
        Vec1f,
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

    enum class PipelineLayout : ice::uptr
    {
        Invalid = 0x0
    };

    struct PipelineInfo
    {
        ice::render::PipelineLayout layout;
        ice::render::RenderPass renderpass;
        ice::Span<ice::render::Shader> shaders;
        ice::Span<ice::render::ShaderStageFlags> shaders_stages;
        ice::Span<ice::render::ShaderInputBinding> shader_bindings;
    };

    enum class Pipeline : ice::uptr
    {
        Invalid = 0x0
    };

} // namespace ice::render
