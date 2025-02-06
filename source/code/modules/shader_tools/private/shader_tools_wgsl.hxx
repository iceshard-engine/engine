#pragma once
#include <ice/resource_compiler.hxx>
#include <ice/string/heap_string.hxx>

#if ISP_WEBAPP || ISP_WINDOWS
#include <arctic/arctic_syntax_node.hxx>
#include "shader_tools_asl.hxx"

namespace ice::wgsl
{

    static constexpr ice::ErrorCode E_FailedToTranspileASLShaderToWGSL{ "E.1101:ResourceCompiler:Shader compiler failed to create WGSL shader from ASL." };

    auto compiler_compile_shader_source(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle const>,
        ice::Span<ice::URI const>,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>;

    auto compiler_build_shader_meta(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceCompilerResult const>,
        ice::Span<ice::URI const>,
        ice::ConfigBuilder& out_meta
    ) noexcept -> ice::Task<bool>;

} // namespace ice::wgsl

#endif // #if ISP_WEBAPP || ISP_WINDOWS
