#pragma once
#include <arctic/arctic.hxx>
#include <arctic/arctic_word_matcher.hxx>
#include <arctic/arctic_word_processor.hxx>
#include <arctic/arctic_lexer.hxx>
#include <arctic/arctic_parser.hxx>
#include <arctic/arctic_syntax.hxx>
#undef assert
#include <ice/shader_tools.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/resource_compiler_api.hxx>

namespace ice
{

    using ASLImportVisitors = arctic::SyntaxVisitorGroup<
        arctic::syntax::Import
    >;

    using ASLGlobalVisitors = arctic::SyntaxVisitorGroup<
        arctic::syntax::ContextBlock,
        arctic::syntax::Define,
        arctic::syntax::Struct,
        arctic::syntax::Function
    >;

    using ASLPatcherVisitors = arctic::SyntaxVisitorGroup<
        arctic::syntax::Define,
        arctic::syntax::Call,
        arctic::syntax::Type
    >;

    using ASLShaderVisitors = arctic::SyntaxVisitorGroup<
        arctic::syntax::Struct,
        arctic::syntax::Function,
        arctic::syntax::Variable,
        arctic::syntax::ContextBlock
    >;

    using ASLTranspilerVisitors = arctic::SyntaxVisitorGroup<
        arctic::syntax::ContextBlock
    >;

    struct ShaderConfig
    {
        ice::render::ShaderStageFlags stage;
    };

    struct ShaderCompilerContext
    {
        ShaderCompilerContext(
            ice::Allocator& alloc,
            ice::ShaderTargetPlatform target,
            ice::ShaderStage stage
        ) noexcept
            : target{ target }
            , stage{ stage }
            , shader_main{ alloc }
        { }

        ice::ShaderTargetPlatform const target;
        ice::ShaderStage const stage;

        ice::i32 shader_type;
        ice::HeapString<> shader_main;
    };

    inline auto shader_context(ice::ResourceCompilerCtx& ctx) noexcept -> ice::ShaderCompilerContext*
    {
        return reinterpret_cast<ice::ShaderCompilerContext*>(ctx.userdata);
    }

} // namespace ice
