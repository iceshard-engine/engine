#pragma once
#include <ice/resource_compiler.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/string/heap_string.hxx>

#if ISP_WINDOWS
#include <arctic/arctic_syntax_node.hxx>

#include "shader_tools_asl_importer.hxx"

namespace ice
{

    static constexpr ice::ErrorCode E_FailedToTranspileASLShaderToGLSL{ "E.1101:ResourceCompiler:Shader compiler failed to create GLSL shader from ASL." };

    struct ShaderConfig
    {
        ice::render::ShaderStageFlags stage;
    };

    auto transpile_shader_asl_to_glsl(
        ice::Allocator& allocator,
        ice::ASLScriptLoader& imports,
        ice::Data asl_source,
        ice::ShaderConfig config,
        ice::HeapString<>& out_entry_point
    ) noexcept -> ice::HeapString<>;

    auto compiler_supported_shader_resources() noexcept -> ice::Span<ice::String>
    {
        static ice::String supported_extensions[]{ ".asl", ".glsl" };
        return supported_extensions;
    }

} // namespace ice

#endif // #if ISP_WINDOWS
