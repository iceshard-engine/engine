/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "input_action_script_grammar.hxx"

namespace ice::asl::grammar
{

    auto Condition::internal_condition_matcher(
        arctic::SyntaxRule const&,
        arctic::MatchContext& ctx
    ) noexcept -> arctic::ParseState
    {
        using arctic::MatchContext;
        using arctic::ParseState;
        using arctic::SyntaxNode;
        using arctic::Token;

        static constexpr SyntaxRule cond_when{ WhenCondition, MatchAll };
        static constexpr SyntaxRule cond_orand{ AndOrCondition, MatchAll };
        static constexpr SyntaxRule cond_flags{ ConditionFlags, MatchAll };
        static constexpr SyntaxRule steps{ ActionStep::Rules, MatchAll };

        ParseState result = ParseState::Success;
        SyntaxNode parent = ctx.node;
        SyntaxNode condition{ };
        MatchContext local_ctx = ctx;

        if (ctx.token.type != TokenType::ASL_KW_When)
        {
            return ParseState::Error_UnexpectedToken;
        }
        // We require 'when' conditions to be follwing a newline token.
        if (ctx.prev_token.type != TokenType::ST_EndOfLine)
        {
            return ParseState::Error_UnexpectedToken;
        }

        // Add a new child node and store the condition type.
        local_ctx.node = condition = parent.append_child(SyntaxNode<LayerActionCondition>{ local_ctx.alloc });
        result = cond_when.execute(local_ctx);

        do
        {
            Token tok = local_ctx.token;
            if (tok.type == TokenType::ASL_KW_WhenOr
                || tok.type == TokenType::ASL_KW_WhenAnd)
            {
                // Add a new sibling node and store the condition type.
                local_ctx.node = condition = condition.append_sibling(SyntaxNode<LayerActionCondition>{ local_ctx.alloc });
                result = cond_orand.execute(local_ctx);
            }
            else if (tok.type == TokenType::CT_Dot)
            {
                MatchContext stepsctx = local_ctx;
                while (result == ParseState::Success && tok.type == TokenType::CT_Dot)
                {
                    stepsctx.node = condition.append_child(SyntaxNode<LayerActionStep>{ local_ctx.alloc });
                    result = steps.execute(stepsctx);
                    tok = stepsctx.token;
                }

                local_ctx.token = stepsctx.token;
                local_ctx.prev_token = stepsctx.prev_token;
            }
            else if (tok.type == TokenType::CT_Comma)
            {
                result = cond_flags.execute(local_ctx);
            }
            else if (tok.type == TokenType::ST_EndOfLine)
            {
                local_ctx.prev_token = ice::exchange(local_ctx.token, local_ctx.lexer.next());
                switch (local_ctx.token.type)
                {
                case TokenType::ASL_KW_WhenOr: // For additional conditions
                case TokenType::ASL_KW_WhenAnd:
                case TokenType::CT_Comma:
                case TokenType::CT_Dot: break; // For steps
                case TokenType::ASL_KW_When:
                case TokenType::ASL_KW_Modifier:
                default: result = ParseState::Error_UnexpectedToken; break;
                }
            }
            else
            {
                break;
            }
        } while (result == ParseState::Success);

        ctx.token = local_ctx.token;
        ctx.prev_token = local_ctx.prev_token;
        return result;
    }

} // namespace ice::asl::grammar
