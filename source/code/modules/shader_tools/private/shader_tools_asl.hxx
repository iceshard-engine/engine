#pragma once
#include <arctic/arctic.hxx>
#include <arctic/arctic_word_matcher.hxx>
#include <arctic/arctic_word_processor.hxx>
#include <arctic/arctic_lexer.hxx>
#include <arctic/arctic_parser.hxx>
#include <arctic/arctic_syntax.hxx>
#undef assert

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
        arctic::syntax::ContextBlock
    >;

    using ASLTranspilerVisitors = arctic::SyntaxVisitorGroup<
        arctic::syntax::ContextBlock
    >;

} // namespace ice
