#pragma once
#include <ice/string/heap_string.hxx>
#include <ice/render/render_shader.hxx>
#include <arctic/arctic_syntax_node.hxx>

#include "shader_tools_asl_importer.hxx"

namespace ice
{

    struct ShaderConfig
    {
        ice::render::ShaderStageFlags stage;
    };

    auto transpile_shader_asl_to_glsl(
        ice::Allocator& allocator,
        ice::ASLScriptLoader& imports,
        ice::Data asl_source,
        ice::ShaderConfig config
    ) noexcept -> ice::HeapString<>;

} // namespace ice
