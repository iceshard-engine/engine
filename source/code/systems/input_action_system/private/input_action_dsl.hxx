#pragma once
#include <arctic/arctic.hxx>
#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax.hxx>
#include <arctic/arctic_syntax_visitor.hxx>
#include <arctic/arctic_syntax_node_types.hxx>
#include "input_action_dsl_syntax_nodes.hxx"

#include <ice/string_types.hxx>

namespace ice
{

    struct ActionInputParserEvents : arctic::SyntaxVisitorGroup<ice::syntax::Layer>
    {
        void visit(arctic::SyntaxNode<> node) noexcept override final
        {
            SyntaxVisitorGroup::visit(node);
        }

        void visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept override = 0;
    };

    bool parse_action_input_definition(
        ice::String definition,
        ice::ActionInputParserEvents& handler
    ) noexcept;

} // namespace ice
