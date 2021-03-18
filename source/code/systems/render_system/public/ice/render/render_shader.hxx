#pragma once
#include <ice/data.hxx>
#include <ice/span.hxx>

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
    };

    enum class ShaderAttribType
    {
        Vec1f,
        Vec2f,
        Vec3f,
        Vec4f,
    };

    struct ShaderInfo
    {
        ice::Data shader_data;
        ice::render::ShaderStageFlags shader_stage;
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

    constexpr auto operator|(ShaderStageFlags left, ShaderStageFlags right) noexcept -> ShaderStageFlags
    {
        ice::u32 left_value = static_cast<ice::u32>(left);
        ice::u32 right_value = static_cast<ice::u32>(right);
        return static_cast<ShaderStageFlags>(left_value | right_value);
    }

} // namespace ice::render
