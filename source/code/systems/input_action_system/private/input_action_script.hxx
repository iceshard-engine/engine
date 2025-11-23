/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <arctic/arctic.hxx>
#include <arctic/arctic_token.hxx>
#include <arctic/arctic_syntax.hxx>
#include <arctic/arctic_syntax_visitor.hxx>
#include <arctic/arctic_syntax_node_types.hxx>
#include "input_action_script_syntax_data.hxx"

#include <ice/string_types.hxx>

namespace ice::asl
{

    struct ActionInputParserEvents : arctic::SyntaxVisitorGroup<ice::asl::Layer>
    {
        void visit(arctic::SyntaxNode<> node) noexcept override final
        {
            SyntaxVisitorGroup::visit(node);
        }

        void visit(arctic::SyntaxNode<ice::asl::Layer> node) noexcept override = 0;
    };

    bool parse_action_input_definition(
        ice::Allocator& allocator,
        ice::String definition,
        ice::asl::ActionInputParserEvents& handler
    ) noexcept;

} // namespace ice
