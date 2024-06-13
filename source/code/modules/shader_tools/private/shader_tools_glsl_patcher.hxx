#pragma once
#include "shader_tools_asl_patcher.hxx"

namespace ice
{

    class GLSLPatcher : public ASLPatcher
    {
    public:
        using ASLPatcher::ASLPatcher;

        void visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Call> node) noexcept override;
    };

} // namespace ice
