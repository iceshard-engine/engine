#pragma once
#include "shader_tools_asl.hxx"
#include "shader_tools_asl_allocator.hxx"
#include "shader_tools_asl_database.hxx"
#include "shader_tools_asl_utils.hxx"

#include <ice/container/hashmap.hxx>

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
        void visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Function> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::ContextBlock> node) noexcept override;

    private:
        ice::ASLAllocator& _allocator;
        ice::HashMap<arctic::SyntaxNode<arctic::syntax::Struct>> _usertypes;
        ice::HashMap<arctic::SyntaxNode<arctic::syntax::Function>> _functions;
        ice::HashMap<arctic::SyntaxNode<arctic::syntax::Function>> _native_functions;
        ice::HashMap<arctic::SyntaxNode<arctic::syntax::ContextVariable>> _variables;
    };

} // namespace ice
