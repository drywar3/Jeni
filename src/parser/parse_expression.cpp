#include "ast/error.hpp"
#include "parser/parser.hpp"
#include "ast/typehint.hpp"
#include "ast/expressions.hpp"

#include <mini.cc/scope_guard.h>

using namespace parser;

static ast::Operator get_operator_from_token(Token_Kind kind)
{
    switch (kind) {
    case OP_Add:    return ast::Operator::Add;
    case OP_Sub:    return ast::Operator::Minus;
    case OP_Star:   return ast::Operator::Star;
    case OP_Slash:  return ast::Operator::Division;
    case OP_Assign: return ast::Operator::Assign;
    default: MINI_UNREACHABLE("%d", kind);
    }
}

ast::Expression *parse_primary(Parser &p,     bool is_within_typehint_context = false);
ast::Expression *parse_dereference(Parser &p, bool is_within_typehint_context = false);
ast::Expression *parse_postfix(Parser &p,     bool is_within_typehint_context = false);
ast::Expression *parse_unary(Parser &p,       bool is_within_typehint_context = false);
ast::Expression *parse_factor(Parser &p,      bool is_within_typehint_context = false);
ast::Expression *parse_assignment(Parser &p,  bool is_within_typehint_context = false);
ast::Expression *parse_comparison(Parser &p,  bool is_within_typehint_context = false);
ast::Expression *parse_relational(Parser &p,  bool is_within_typehint_context = false);

ast::Expression *Parser::parse_expression()
{
    auto &p = *this;
    if (!p.token_can_begin_expr(p.current_)) {
        p.compiler->report_error(p.current_.locus, "Expected an expression", INVALID_TOKEN);
        return allocate<ast::Error>(allocator(), p->currentl());
    }
    return parse_relational(p, last_state_is(State::Typehint_Expression));
}

ast::Expression *parse_primary(Parser &p,     bool is_within_typehint_context)
{
    if (auto tok = p->try_expect(LIT_Integer); tok and !is_within_typehint_context) {
        auto view = p->slice_out(tok->locus);
        if (tok->base == 8) MINI_TODO("handle base 8 integers");

        char *str = view.to_string(p.allocator());
        MINI_DEFER { MINI_FREE(p.allocator(), str); };

        int64 value = std::strtoll(str, nullptr, tok->base);
        return allocate<ast::expr::Integer>(
            p.allocator(),
            tok->locus,
            value
        );
    }

    if (auto tok = p->try_expect(LIT_Float); tok and !is_within_typehint_context) {
        char *buffer = MINI_ALLOC_MANY(p.allocator(), char, tok->locus.length() + 1);
        defer { MINI_FREE(p.allocator(), buffer); };
        Sv sv = p->slice_out(tok->locus);
        memcpy(buffer, sv.data(), sv.length());
        buffer[sv.length()] = '\0';
        double value        = strtof(buffer, NULL);
        return allocate<ast::expr::Float>(
            p.allocator(),
            tok->locus,
            value);
    }

    if (auto tok = p->try_expect(LIT_Char); tok and !is_within_typehint_context) {
        auto view = p->slice_out(tok->locus);
        assert(view.length() == 2, "character token is more than 2 bytes long");
        const char *data = view.data();
        assert(data[0] == '\'', "character token does not begin with \'");
        uint8 value = data[1];
        return allocate<ast::expr::Char>(
            p.allocator(),
            tok->locus,
            value);
    }

    if (auto tok = p->try_expect(Identifier)) {
        auto view = p->slice_out(tok->locus);
        return allocate<ast::expr::Identifier>(
            p.allocator(),
            tok->locus,
            view
        );
    }

    // handle `true` and `false`.
    {
        if (auto tok = p->try_expect_one_of({LIT_True, LIT_False})) {
            bool value = (tok->kind == LIT_True);
            return allocate<ast::expr::Boolean>(p.allocator(), tok->locus, value);
        }
    }

    auto typehint = p.parse_typehint(false);
    if (typehint->is_error()) {
        const char *message = "Expected a primary expression";

        if (is_within_typehint_context) message = "Expected a type specifier";

        p.compiler->report_error(p->currentl(), message, INVALID_TOKEN);
    }

    return typehint;
}

ast::Expression *parse_dereference(Parser &p, bool is_within_typehint_context)
{
    auto base = parse_primary(p, is_within_typehint_context);
    if (is_within_typehint_context or base->is_error()) return base;

    if (p->try_expect(OP_Caret)) {
        MINI_TODO("parse dereference");
    }

    return base;
}

ast::Expression *parse_postfix(Parser &p,     bool is_within_typehint_context)
{
    auto base = parse_dereference(p, is_within_typehint_context);
    if (base->is_error()) return base;
    while (true) {
        /*
         *  function call:
         *     <expr> ( <arguments> )
         */

        if (p->try_expect(SEP_Lparen) and !is_within_typehint_context) {
            ast::expr::Function_Call::Args args{p.allocator()};

            while (!p.is_done() and !p->equals(SEP_Rparen)) {
                Opt<ast::Name> name = None;
                ast::Expression *value   = nullptr;
                if (p->equals_sequence({Identifier, OP_Assign})) {
                    name = p.parse_name();
                    assert(name.has_value(), "Compiler Bug: ...");
                    p->expect(OP_Assign);
                }
                value = p.parse_expression();
                if (value->is_error()) return value;

                args.append(ast::Call_Argument(value, name));

                if (p->try_expect(SEP_Comma)) continue;
                break;
            }

            p->expect(SEP_Rparen);
            base = allocate<ast::expr::Function_Call>(
                p.allocator(),
                base->locus.merge(p.previous_.locus),
                base,
                args);
            continue;
        }

        if (p->try_expect(SEP_Dot)) {
            MINI_TODO("parse member access");
        }

        break;
    }

    return base;
}

ast::Expression *parse_unary(Parser &p,       bool is_within_typehint_context)
{
    if (p->equals(OP_Star)) {
        auto begin = p->next();

        if (is_within_typehint_context) {
            auto access = ast::Mutability::Mutable;
            if (p->try_expect(KW_Const)) access = ast::Mutability::Constant;
            auto typehint = p.parse_typehint_expression();
            if (typehint->is_error()) return typehint;
            return allocate<ast::typehint::Pointer>(
                p.allocator(), begin.locus.merge(typehint->locus), access, typehint);
        }

        auto expr = p.parse_expression();
        if (expr->is_error()) return expr;
        return allocate<ast::expr::Unary_Operation>(
            p.allocator(),
            begin.locus.merge(expr->locus),
            ast::Operator::Star,
            expr
        );
    }
    return parse_postfix(p, is_within_typehint_context);
}

ast::Expression *parse_factor(Parser &p,      bool is_within_typehint_context)
{
    auto *left = parse_unary(p, is_within_typehint_context);
    if (is_within_typehint_context) return left;
    while (p->equals_one_of({OP_Star, OP_Slash})) {
        auto op    = get_operator_from_token(p->next().kind);
        auto right = parse_unary(p, false);
        left       = allocate<ast::expr::Binary_Operation>(
            p.allocator(), left->locus.merge(right->locus), op, left, right);
    }
    return left;
}

ast::Expression *parse_term(Parser &p,        bool is_within_typehint_context)
{
    auto *left = parse_factor(p, is_within_typehint_context);
    if (is_within_typehint_context) return left;
    while (p->equals_one_of({OP_Add, OP_Sub})) {
        auto op    = get_operator_from_token(p->next().kind);
        auto right = parse_factor(p, false);
        left       = allocate<ast::expr::Binary_Operation>(
            p.allocator(), left->locus.merge(right->locus), op, left, right);
    }
    return left;
}


ast::Expression *parse_assignment(Parser &p,  bool is_within_typehint_context)
{
    auto *left = parse_term(p, is_within_typehint_context);
    if (is_within_typehint_context) return left;

    if (auto tok = p->try_expect(OP_Assign)) {
        auto op     = get_operator_from_token(tok->kind);
        auto *right = parse_relational(p, false);
        left = allocate<ast::expr::Binary_Operation>(
            p.allocator(), left->locus.merge(right->locus),
            op, left, right);
    }

    return left;
}

ast::Expression *parse_comparison(Parser &p,  bool is_within_typehint_context)
{
    auto *left = parse_assignment(p, is_within_typehint_context);
    if (is_within_typehint_context) return left;

    while (p->equals_one_of({OP_Greater, OP_Less,
                             OP_Greater_Equals, OP_Less_Equals})) {
        auto op     = get_operator_from_token(p->next().kind);
        auto *right = parse_assignment(p, false);
        left = allocate<ast::expr::Binary_Operation>(
            p.allocator(), left->locus.merge(right->locus),
            op, left, right);
    }

    return left;
}

ast::Expression *parse_relational(Parser &p,  bool is_within_typehint_context)
{
    auto *left = parse_comparison(p, is_within_typehint_context);
    if (is_within_typehint_context) return left;

    while (p->equals_one_of({OP_Equals, OP_Not_Equals})) {
        auto op     = get_operator_from_token(p->next().kind);
        auto *right = parse_comparison(p, false);
        left = allocate<ast::expr::Binary_Operation>(
            p.allocator(), left->locus.merge(right->locus),
            op, left, right);
    }

    return left;
}
