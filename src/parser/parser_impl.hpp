#pragma once

#include "parser/token.hpp"

namespace parser
{
    struct Parser;
    using Token_Pack = std::initializer_list<Token_Kind>;

    struct Parser_Impl {
        Parser *parser;

        Token current() const;
        Token previous() const;

        Locus currentl() const;
        Locus previousl() const;

        Sv slice_out(Locus locus) const;

        Token next();
        bool equals(Token_Kind kind) const;
        bool equals_sequence(Token_Pack kinds);
        bool equals_one_of(Token_Pack kinds);

        Opt<Token> expect(Token_Kind kind);
        bool expect_sequence(Token_Pack kinds);

        Opt<Token> try_expect(Token_Kind kind);
        Opt<Token> try_expect_one_of(Token_Pack kinds);
        bool try_expect_sequence(Token_Pack kinds);

        bool skip_until_one_of(Token_Pack kinds, bool skip_past = false);

    };
} // namespace parser
