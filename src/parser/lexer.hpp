#pragma once

#include "source.hpp"
#include "diagnostics.hpp"
#include "parser/token.hpp"

namespace parser
{
    struct Lexer {
        Source_Id source_id;
        Sv        content;
        usize     line;
        usize     col, prev_col;
        // offset from the beginning of [content] in bytes.
        usize     off, prev_off;

        Alloc allocator;

        Lexer(Source_Id id, Sv content, Alloc allocator);
        bool is_done() const;

        auto next_token() -> Expect<Token, Diagnostic>;
    };
} // namespace parser
