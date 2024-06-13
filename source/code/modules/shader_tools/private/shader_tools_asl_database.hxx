#pragma once
#include <arctic/arctic_syntax_node.hxx>
#include <arctic/arctic_syntax_node_types.hxx>

namespace ice
{

    struct ASLEntityTracker
    {
        virtual ~ASLEntityTracker() noexcept = default;

        virtual auto find(arctic::String entity) noexcept -> arctic::SyntaxNode<>
        {
            return {};
        }

        virtual auto find_struct(
            arctic::syntax::Type const& type
        ) noexcept -> arctic::SyntaxNode<arctic::syntax::Struct>
        {
            return {};
        }
    };

} // namespace ice
