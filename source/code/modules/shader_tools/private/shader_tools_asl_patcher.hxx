#pragma once
#include "shader_tools_asl.hxx"
#include "shader_tools_asl_importer.hxx"
#include "shader_tools_asl_allocator.hxx"
#include <arctic/arctic_syntax_node.hxx>

namespace ice
{

    class ASLPatcher : public ice::ASLPatcherVisitors
    {
    public:
        ASLPatcher(ASLAllocator& alloc, ASLImportTracker& imports) noexcept;

    public: // implements: ice::ASLPatcherVisitors
        void visit(arctic::SyntaxNode<> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Type> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Call> node) noexcept override;

    protected:
        ASLAllocator& _allocator;
        ASLImportTracker& _imports;
    };

} // namespace ice
