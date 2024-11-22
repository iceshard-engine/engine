#include "input_action_dsl.hxx"

#include <arctic/arctic_lexer.hxx>
#include <arctic/arctic_lexer_tokenizer.hxx>
#include <arctic/arctic_parser.hxx>
#include <arctic/arctic_syntax_node_allocator.hxx>
#include <arctic/arctic_word_matcher.hxx>

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
        if (word.value == "action")
        {
            result.type = ice::grammar::UCT_Action;
        }
        else if (word.value == "bool")
        {
            result.type = ice::grammar::UCT_ActionTypeBool;
        }
        else if (word.value == "button")
        {
            result.type = ice::grammar::UCT_InputTypeButton;
        }
        else if (word.value == "float1")
        {
            result.type = ice::grammar::UCT_ActionTypeFloat1;
        }
        else if (word.value == "float2")
        {
            result.type = ice::grammar::UCT_ActionTypeFloat2;
        }
        else if (word.value == "float3")
        {
            result.type = ice::grammar::UCT_ActionTypeFloat3;
        }
        else if (word.value == "kb" || word.value == "keyboard")
        {
            result.type = ice::grammar::UCT_InputBindingKeyboard;
        }
        else if (word.value == "pad")
        {
            result.type = ice::grammar::UCT_InputBindingPad;
        }
        else if (word.value == "source")
        {
            result.type = ice::grammar::UCT_Source;
        }
        else if (word.value == "layer")
        {
            result.type = ice::grammar::UCT_Layer;
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
