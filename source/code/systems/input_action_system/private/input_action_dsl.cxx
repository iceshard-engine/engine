#include "input_action_dsl.hxx"
#include "input_action_dsl_grammar.hxx"

#include <arctic/arctic_lexer.hxx>
#include <arctic/arctic_lexer_tokenizer.hxx>
#include <arctic/arctic_parser.hxx>
#include <arctic/arctic_syntax_node_allocator.hxx>
#include <arctic/arctic_word_matcher.hxx>
#include <ice/sort.hxx>

namespace ice
{

    struct AsilNodeAllocator final : public arctic::SyntaxNodeAllocator
    {
        auto allocate(arctic::usize size, arctic::usize align) noexcept -> void* override
        {
            ICE_ASSERT_CORE(align <= 8);
            return malloc(size);
        }

        void deallocate(void* ptr) noexcept override
        {
            return free(ptr);
        }
    };

    struct AsilEvents : arctic::SyntaxVisitorGroup<arctic::syntax::Root>
    {
        arctic::SyntaxNode<arctic::syntax::Root> root;

        void visit(arctic::SyntaxNode<> node) noexcept override
        {
            SyntaxVisitorGroup<arctic::syntax::Root>::visit(node);
        }

        void visit(arctic::SyntaxNode<arctic::syntax::Root> node) noexcept override
        {
            root = node;
        }
    };

    struct TokenInfo
    {
        arctic::String value;
        arctic::TokenType type;
    };

    static constexpr ice::TokenInfo Constant_Tokens[]{
        { "action", ice::grammar::UCT_Action },
        { "active", ice::grammar::UCT_WhenActive },
        { "activate", ice::grammar::UCT_StepActivate },
        { "accumulated", ice::grammar::UCT_ActionFlagAccumulated },
        { "deactivate", ice::grammar::UCT_StepDeactivate },
        { "and", ice::grammar::UCT_WhenAnd },
        { "axis1d", ice::grammar::UCT_InputTypeAxis1D },
        { "axis2d", ice::grammar::UCT_InputTypeAxis2D },
        { "axis3d", ice::grammar::UCT_InputTypeAxis3D },
        { "bool", ice::grammar::UCT_ActionTypeBool },
        { "button", ice::grammar::UCT_InputTypeButton },
        { "inactive", ice::grammar::UCT_WhenInactive },
        { "float1", ice::grammar::UCT_ActionTypeFloat1 },
        { "float2", ice::grammar::UCT_ActionTypeFloat2 },
        { "float3", ice::grammar::UCT_ActionTypeFloat3 },
        // { "gamepad", ice::grammar::UCT_InputBindingPad },
        // { "gctrl", ice::grammar::UCT_InputBindingPad },
        //{ "km", ice::grammar::UCT_InputBindingKeyboard },
        { "kb", ice::grammar::UCT_InputBindingKeyboard },
        //{ "keymod", ice::grammar::UCT_InputBindingKeyboard },
        { "key", ice::grammar::UCT_InputBindingKeyboard },
        // { "keyboard", ice::grammar::UCT_InputBindingKeyboard },,
        { "max", ice::grammar::UCT_ModifierOpMax },
        { "min", ice::grammar::UCT_ModifierOpMin },
        { "mouse", ice::grammar::UCT_InputBindingMouse },
        { "mp", ice::grammar::UCT_InputBindingMouse },
        { "gamepad", ice::grammar::UCT_InputBindingPad },
        { "gp", ice::grammar::UCT_InputBindingPad },
        { "layer", ice::grammar::UCT_Layer },
        { "object", ice::grammar::UCT_ActionTypeObject },
        { "or", ice::grammar::UCT_WhenOr },
        { "once", ice::grammar::UCT_ActionFlagOnce },
        { "pressed", ice::grammar::UCT_WhenPressed },
        { "released", ice::grammar::UCT_WhenReleased },
        { "source", ice::grammar::UCT_Source },
        { "toggled", ice::grammar::UCT_ActionFlagToggled },
        { "toggle", ice::grammar::UCT_StepToggle },
        { "when", ice::grammar::UCT_When },
        { "reset", ice::grammar::UCT_StepReset },
        { "series", ice::grammar::UCT_WhenFlagCheckSeries },
        { "time", ice::grammar::UCT_StepTime },
        { "mod", ice::grammar::UCT_Modifier },
    };

    //! \brief A default implementation of a tokenizer for Arctic language.
    //!
    //! \param word [inout] A word to be tokenized. Update the value with the next word that was not tokenized.
    //! \param processor [in] A statefull generator object used to access the next word value.
    //! \param location [in] A pre-calculated source location to be assigned to the resulting token.
    //!     Provides properly calculated line and column values according to options, newlines and whitespace characers.
    auto asil_lexer_tokenizer(
        arctic::Word& word,
        arctic::WordProcessor& processor,
        arctic::TokenLocation location
    ) noexcept -> arctic::Token
    {
        arctic::Token result{
            .value = word.value,
            .type = arctic::TokenType::Invalid,
            .location = location
        };

        static auto const pred = [](ice::TokenInfo const& hay, arctic::Word const& needle) noexcept
        {
            return strnicmp(hay.value.data(), needle.value.data(), hay.value.size()) == 0;
        };

        ice::ucount idx;
        if (ice::search(ice::Span{Constant_Tokens}, word, pred, idx))
        {
            result.type = Constant_Tokens[idx].type;
        }
        else
        {
            return arctic::arctic_lexer_tokenizer(word, processor, location);
        }
        word = processor.next();
        return result;
    }

    bool parse_action_input_definition(
        ice::String definition,
        ice::ActionInputParserEvents& handler
    ) noexcept
    {
        arctic::WordMatcher matcher{};
        arctic::initialize_default_matcher(&matcher);

        arctic::Lexer lexer = arctic::create_lexer(
            arctic::create_word_processor(definition, &matcher),
            {.tokenizer = asil_lexer_tokenizer}
        );

        AsilEvents internal_handler{};
        AsilNodeAllocator node_alloc{};

        arctic::SyntaxVisitor* visitors[]{
            &internal_handler,
            &handler
        };

        arctic::SyntaxNode<> root;
        std::unique_ptr<arctic::Parser> parser = arctic::create_default_parser({ .rules = ice::grammar::Rule_GlobalRules });
        if (parser->parse(lexer, node_alloc, visitors) == true)
        {
            root = internal_handler.root;
        }
        arctic::shutdown_matcher(&matcher);
        return root == true;
    }

} // namespace ice
