/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "shader_tools_asl.hxx"
#include "shader_tools_asl_allocator.hxx"
#include "shader_tools_asl_database.hxx"
#include "shader_tools_asl_utils.hxx"

#include <ice/multi_hashmap.hxx>

namespace ice
{

    class ASLScriptFile : public ice::ASLEntityTracker, public ice::ASLGlobalVisitors
    {
    public:
        arctic::String const alias;

    public:
        ASLScriptFile(ice::ASLAllocator& alloc, arctic::String alias) noexcept;

    public: // Implements: ice::ASLEntityTracker
        auto find(arctic::String identifier) noexcept -> arctic::SyntaxNode<> override;

    public: // Implements: ice::ASLGlobalVisitors
        void visit(arctic::SyntaxNode<> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Function> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::ContextBlock> node) noexcept override;

    private:
        ice::ASLAllocator& _allocator;
        ice::MultiHashMap<arctic::SyntaxNode<arctic::syntax::Struct>> _usertypes;
        ice::MultiHashMap<arctic::SyntaxNode<arctic::syntax::Function>> _functions;
        ice::MultiHashMap<arctic::SyntaxNode<arctic::syntax::Function>> _native_functions;
        ice::MultiHashMap<arctic::SyntaxNode<arctic::syntax::ContextVariable>> _variables;
    };

} // namespace ice
