#include "parser/token_buffer.hpp"

parser::Token_Buffer::Token_Buffer(Alloc allocator,
                                   Source_Id id,
                                   Sv content,
                                   Diagnostic_Pool *diagnostics)
   : lexer(id, content, allocator), tokens(allocator),
     diagnostics(diagnostics), cursor(0)
{
}

void parser::Token_Buffer::prepare(usize window)
{
    if (lexer.is_done()) return;

    if (cursor + window >= tokens.count()) {
        auto needed = (cursor + window) * 2;
        for (usize n = 0; n < needed; n++) {
            if (auto result = lexer.next_token(); result.has_value()) {
                tokens.append(*result);
            } else {
                diagnostics->report(result.error());
            }
        }
    }
}

parser::Token parser::Token_Buffer::peek(int ahead)
{
    if (is_truly_done()) return tokens.last();

    if (cursor + ahead >= tokens.count()) prepare(ahead);

    return tokens[cursor + ahead];
}

parser::Token parser::Token_Buffer::advance()
{
    cursor += 1;
    auto current = peek(0);
    prepare(2);
    return current;
}

bool parser::Token_Buffer::is_truly_done() const
{
    /* when the lexer has reached the end of the content given and the
     * token buffer is also exhausted then we are truly done */
    return lexer.is_done() && cursor >= tokens.count();
}
