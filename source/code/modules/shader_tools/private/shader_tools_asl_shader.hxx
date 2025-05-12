/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "shader_tools_asl.hxx"
#include "shader_tools_asl_importer.hxx"
#include "shader_tools_asl_allocator.hxx"

namespace ice
{

    class ASLShader : public ice::ASLShaderVisitors
    {
    public:
        ASLShader(ice::Allocator& alloc, ASLEntityTracker& tracker, arctic::String shader_stage) noexcept;

        auto find_struct(
            arctic::SyntaxNode<arctic::syntax::Type> const& type
        ) noexcept -> arctic::SyntaxNode<arctic::syntax::Struct>;

        void push_struct(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept;

    public: // Implements: ice::ASLShaderVisitors
        void visit(arctic::SyntaxNode<> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Function> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Variable> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::ContextBlock> node) noexcept override;

    public: // Public because we use it in the generation step directly
        arctic::String const _shader_stage;
        ASLEntityTracker& _tracker;

        arctic::SyntaxNode<arctic::syntax::ContextVariable> _pushcontants;
        arctic::SyntaxNode<arctic::syntax::Struct> _inputs;
        arctic::SyntaxNode<arctic::syntax::Struct> _outputs;
        arctic::SyntaxNode<arctic::syntax::Function> _mainfunc;

        ice::Array<arctic::SyntaxNode<arctic::syntax::Struct>> _structs;
        ice::Array<arctic::SyntaxNode<arctic::syntax::Function>> _functions;
        ice::Array<arctic::SyntaxNode<arctic::syntax::ContextVariable>> _uniforms;
    };

} // namespace ice
