#include "input_action_script.hxx"
#include "input_action_script_grammar.hxx"

#include <arctic/arctic_lexer.hxx>
#include <arctic/arctic_lexer_tokenizer.hxx>
#include <arctic/arctic_parser.hxx>
#include <arctic/arctic_syntax_node_allocator.hxx>
#include <arctic/arctic_word_matcher.hxx>
#include <ice/string_utils.hxx>
#include <ice/sort.hxx>
#include <ice/log.hxx>

namespace ice::asl
{

    struct ASLNodeAllocator final : public arctic::SyntaxNodeAllocator
    {
        ASLNodeAllocator(ice::Allocator& backing) noexcept
            : _backing{ backing }
        { }

        auto allocate(arctic::usize size, arctic::usize align) noexcept -> void* override
        {
            ice::AllocResult const result = _backing.allocate({ ice::usize{ size }, ice::ualign(align) });
            return result.memory;
        }

        void deallocate(void* ptr) noexcept override
        {
            return _backing.deallocate(ptr);
        }

    private:
        ice::Allocator& _backing;
    };

    struct ASLEvents : arctic::SyntaxVisitorGroup<arctic::syntax::Root>
    {
        using arctic::SyntaxVisitor::visit;

        void visit(arctic::SyntaxNode<arctic::syntax::Root> node) noexcept
        {
            root = node;
        }

        arctic::SyntaxNode<arctic::syntax::Root> root;
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

        ice::ucount idx;
        ice::asl::TokenDefinition const needle{ .value = word.value };
        if (ice::binary_search(ice::span::from_std_const(Constant_TokenDefinitions), needle, idx))
        {
            result.type = Constant_TokenDefinitions[idx].type;
            word = processor.next();
        }
        else
        {
            result = arctic::arctic_lexer_tokenizer(word, processor, location);
        }
        return result;
    }

    bool parse_action_input_definition(
        ice::Allocator& allocator,
        ice::String definition,
        ice::asl::ActionInputParserEvents& handler
    ) noexcept
    {
        ice::asl::ASLNodeAllocator node_alloc{ allocator };

        arctic::WordMatcher matcher{};
        arctic::initialize_default_matcher(&matcher);

        arctic::Lexer lexer = arctic::create_lexer(
            arctic::create_word_processor(definition, &matcher),
            {.tokenizer = asil_lexer_tokenizer}
        );

        std::unique_ptr<arctic::Parser> parser = arctic::create_default_parser(
            { .rules = ice::asl::grammar::Rule_GlobalRules }
        );

        arctic::SyntaxVisitor* visitors[]{ &handler };
        bool const result = parser->parse(lexer, node_alloc, visitors);
        arctic::shutdown_matcher(&matcher);
        return result;
    }

} // namespace ice
