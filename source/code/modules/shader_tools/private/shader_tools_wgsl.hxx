#pragma once
#include <ice/resource_compiler.hxx>
#include <ice/string/heap_string.hxx>

#if ISP_WEBAPP
#include <arctic/arctic_syntax_node.hxx>

namespace ice
{

    auto compiler_supported_shader_resources() noexcept -> ice::Span<ice::String>;

    bool compiler_context_prepare(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept;

    bool compiler_context_cleanup(ice::Allocator& alloc, ice::ResourceCompilerCtx& ctx) noexcept;

    auto compiler_compile_shader_source(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceHandle* const>,
        ice::Span<ice::URI const>,
        ice::Allocator& alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>;

    auto compiler_build_shader_meta(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle* source,
        ice::ResourceTracker& tracker,
        ice::Span<ice::ResourceCompilerResult const>,
        ice::Span<ice::URI const>,
        ice::MutableMetadata& out_meta
    ) noexcept -> ice::Task<bool>;

    auto transpile_shader_asl_to_wgsl(
        ice::Allocator& allocator,
        ice::Data asl_source
    ) noexcept -> ice::HeapString<>;

} // namespace ice

#endif // #if ISP_WEBAPP
