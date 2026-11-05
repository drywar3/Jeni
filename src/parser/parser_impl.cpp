#include "parser/parser.hpp"
#include "parser/parser_impl.hpp"

using namespace parser;

Token Parser_Impl::current() const
{
    return parser->current_;
}

Token Parser_Impl::previous() const
{
    return parser->previous_;
}

Locus Parser_Impl::currentl() const { return current().locus; }
Locus Parser_Impl::previousl() const { return previous().locus; }

Sv Parser_Impl::slice_out(Locus locus) const
{
    return parser->tokens.lexer.content.substr(locus.first_byte,
                                               locus.length());
}

Token Parser_Impl::next()
{
    parser->previous_ = current();
    parser->current_  = parser->tokens.advance();
    return parser->previous_;
}

bool Parser_Impl::equals(Token_Kind kind) const
{
    return parser->current_.kind == kind;
}

bool Parser_Impl::equals_one_of(Token_Pack kinds)
{
    for (auto tok : kinds) {
        if (equals(tok)) return true;
    }
    return false;
}

Opt<Token> Parser_Impl::expect(Token_Kind kind)
{
    if (equals(kind)) {
        next();
        return previous();
    }

    parser->compiler->report_error(currentl(), mini_string_build(
        parser->compiler->allocator, "Expected `%s` but found `%s` instead.",
        token_to_string(kind), token_to_string(current().kind)), INVALID_TOKEN);
    return None;
}

bool Parser_Impl::equals_sequence(Token_Pack kinds)
{
    usize count = kinds.end() - kinds.begin();
    for (usize n = 0; n < count; n++) {
        Token current = parser->tokens.peek(n);
        Token_Kind expected = kinds.begin()[n];
        if (current.kind != expected) return false;
    }
    return true;
}

bool Parser_Impl::expect_sequence(Token_Pack kinds) { MINI_UNREACHABLE(); }

Opt<Token> Parser_Impl::try_expect(Token_Kind kind)
{
    if (equals(kind)) {
        next();
        return previous();
    }
    return None;
}

bool Parser_Impl::try_expect_sequence(Token_Pack kinds)
{
    usize count = kinds.end() - kinds.begin();
    for (usize n = 0; n < count; n++) {
        Token current = parser->tokens.peek(n);
        Token_Kind expected = kinds.begin()[n];

        if (current.kind != expected) return false;
    }

    for (usize n = 0; n < count; n++) {
        next();
    }

    return true;
}

bool Parser_Impl::skip_until_one_of(Token_Pack kinds, bool skip_past)
{
    while (!parser->is_done()) {
        for (auto tok : kinds) {
            if (equals(tok)) {
                if (skip_past) next();
                return true;
            }
        }
        next();
    }
    return false;
}

Opt<Token> Parser_Impl::try_expect_one_of(Token_Pack kinds)
{
    for (const auto kind : kinds) {
        if (auto tok = try_expect(kind)) return tok;
    }
    return None;
}
