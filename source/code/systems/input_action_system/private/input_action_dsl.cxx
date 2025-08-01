#include "input_action_dsl.hxx"
#include "input_action_dsl_grammar.hxx"

#include <arctic/arctic_lexer.hxx>
#include <arctic/arctic_lexer_tokenizer.hxx>
#include <arctic/arctic_parser.hxx>
#include <arctic/arctic_syntax_node_allocator.hxx>
#include <arctic/arctic_word_matcher.hxx>
#include <ice/string_utils.hxx>
#include <ice/sort.hxx>
#include <ice/log.hxx>

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

    using TokenType = ice::asl::TokenType;

    static constexpr ice::TokenInfo Constant_Tokens[]{
        { "action", TokenType::ASL_KW_Action },
        { "active", TokenType::ASL_OP_IsActive },
        { "activate", TokenType::ASL_OP_Activate },
        //{ "accumulated", ice::grammar::UCT_ActionFlagAccumulated },
        { "deactivate", TokenType::ASL_OP_Deactivate },
        { "and", TokenType::ASL_KW_WhenAnd },
        { "axis1d", TokenType::ASL_NT_Axis1D },
        { "axis2d", TokenType::ASL_NT_Axis2D },
        { "axis3d", TokenType::ASL_NT_Axis3D },
        { "button", TokenType::ASL_NT_Button },
        { "inactive", TokenType::ASL_OP_IsInactive },
        { "bool", TokenType::ASL_NT_Bool },
        { "float1", TokenType::ASL_NT_Float1 },
        { "float2", TokenType::ASL_NT_Float2 },
        { "float3", TokenType::ASL_NT_Float3 },
        // { "gamepad", ice::grammar::UCT_InputBindingPad },
        // { "gctrl", ice::grammar::UCT_InputBindingPad },
        //{ "km", ice::grammar::UCT_InputBindingKeyboard },
        { "kb", TokenType::ASL_KW_Keyboard },
        //{ "keymod", ice::grammar::UCT_InputBindingKeyboard },
        { "key", TokenType::ASL_KW_Keyboard },
        // { "keyboard", ice::grammar::UCT_InputBindingKeyboard },,
        { "max", TokenType::ASL_OP_Max },
        { "min", TokenType::ASL_OP_Min },
        { "mouse", TokenType::ASL_KW_Mouse },
        { "mp", TokenType::ASL_KW_Mouse },
        { "gamepad", TokenType::ASL_KW_Controller },
        { "gp", TokenType::ASL_KW_Controller },
        { "layer", TokenType::ASL_KW_Layer },
        { "object", TokenType::ASL_NT_Object },
        { "or", TokenType::ASL_KW_WhenOr },
        { "once", TokenType::ASL_KWF_Once },
        { "pressed", TokenType::ASL_OP_IsPressed },
        { "released", TokenType::ASL_OP_IsReleased },
        { "source", TokenType::ASL_KW_Source },
        { "toggled", TokenType::ASL_KWF_Toggled },
        { "toggle", TokenType::ASL_OP_Toggle },
        { "when", TokenType::ASL_KW_When },
        { "reset", TokenType::ASL_OP_Reset },
        { "series", TokenType::ASL_KWF_CheckSeries },
        { "time", TokenType::ASL_OP_Time },
        { "mod", TokenType::ASL_KW_Modifier },
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
            return ice::compare(hay.value, needle.value, hay.value.size()) == CompareResult::Equal;
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
