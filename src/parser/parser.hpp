#pragma once

#include "compiler.hpp"
#include "ast/name.hpp"
#include "misc/misc.hpp"
#include "parser/state.hpp"
#include "parser/token.hpp"
#include "ast/typehint.hpp"
#include "ast/statement.hpp"
#include "ast/expression.hpp"
#include "parser/parser_impl.hpp"
#include "parser/token_buffer.hpp"

#include <initializer_list>

namespace parser
{
    struct Parser {
        Compiler_Context *compiler;
        Token_Buffer   tokens;
        Token current_, previous_;
        Diagnostic_Pool *diagnostics;
        State_Stack state_stack;
        Parser_Impl impl;

        Parser(Source_Id id, Sv content, Compiler_Context *compiler);

        bool is_done() const;

        Opt<ast::Name> parse_name();
        ast::Statement  *parse_statement();
        ast::Expression *parse_expression();
        ast::Expression *parse_typehint_expression();
        ast::Typehint   *parse_typehint(bool emit_error = true);

        Parser_Impl *operator->() { return &impl; }

        auto allocator() const { return compiler->allocator; }

        void push_state(State state) { state_stack.append(state); }
        Opt<State> pop_state()
        {
            if (state_stack.empty()) return None;
            return state_stack.pop();
        }

        State &peek_state()
        {
            MINI_ASSERT(!state_stack.empty(), "nothing in state stack");
            return state_stack.last();
        }

        const State &peek_state() const
        {
            MINI_ASSERT(!state_stack.empty(), "nothing in state stack");
            return state_stack.last();
        }

        bool last_state_is(State::Kind kind) const
        {
            if (state_stack.empty()) return false;
            return peek_state().kind == kind;
        }

        bool token_can_begin_expr(Token token) const;
    };
} // namespace parser
