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

    struct ActionInputParserEvents;

    bool parse_action_input_definition(
        ice::String definition,
        ice::ActionInputParserEvents& handler
    ) noexcept;

    using ActionInputParserEventsBase = arctic::SyntaxVisitorGroup<
        ice::syntax::Layer
    >;

    struct ActionInputParserEvents : ice::ActionInputParserEventsBase
    {
        void visit(arctic::SyntaxNode<> node) noexcept override final
        {
            ActionInputParserEventsBase::visit(node);
        }

        void visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept = 0;
    };

} // namespace ice
