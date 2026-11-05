#pragma once

#include "diagnostics.hpp"
#include "parser/lexer.hpp"

namespace parser
{
    struct Token_Buffer {
        Lexer            lexer;
        Arr<Token>       tokens;
        Diagnostic_Pool *diagnostics;
        usize            cursor;

        Token_Buffer(Alloc allocator,
                     Source_Id id,
                     Sv content,
                     Diagnostic_Pool *diagnostics);

        void prepare(usize window);
        Token peek(int ahead);
        Token advance();

        bool is_truly_done() const;
    };
} // namespace parser
