#include "ast/error.hpp"
#include "parser/parser.hpp"
#include "ast/statements.hpp"

using namespace parser;

Parser::Parser(Source_Id id, Sv content, Compiler_Context *compiler)
    : compiler(compiler), tokens(compiler->allocator, id, content, &compiler->diagnostics),
      diagnostics(&compiler->diagnostics),
      state_stack(compiler->allocator),
      impl(this)
{
    tokens.prepare(10);
    current_ = tokens.peek(0);
}

bool Parser::is_done() const
{
    return (current_.kind == End_Of_File or previous_.kind == End_Of_File) or
            tokens.is_truly_done();
}

bool Parser::token_can_begin_expr(Token token) const
{
    switch (token.kind) {
        case KW_Eval:
        case Identifier:

        case LIT_Integer:
        case LIT_Float:
        case LIT_Char:
        case LIT_True:
        case LIT_False:
        case LIT_Cstring:

        case SEP_Lbracket:
        case SEP_Lparen:

        case OP_Star:

        case TK_Int:
        case TK_Uint:
        case TK_Char:
        case TK_Uchar:
        case TK_Void:
        case TK_Bool:
        case TK_Int8:
        case TK_Int16:
        case TK_Int32:
        case TK_Int64:
        case TK_Isize:
        case TK_Uint8:
        case TK_Uint16:
        case TK_Uint32:
        case TK_Uint64:
        case TK_Usize:
        case TK_Float32:
        case TK_Float64:
            return true;
        default: return false;
    }
}


/***************************************************************************/
/*********************** GRAMMAR PARSER DECLARATIONS ***********************/
ast::Statement *parse_variable_declaration(Parser &p);
ast::Statement *parse_function_declaration(Parser &p);
ast::Statement *parse_block(Parser &p);
ast::Statement *parse_if_statement(Parser &p);
ast::Statement *parse_return_statement(Parser &p);
/***************************************************************************/

ast::Statement *Parser::parse_statement()
{
    auto &self = *this;

    if (self->equals_one_of({KW_Var, KW_Const})) return parse_variable_declaration(self);
    if (self->equals(KW_Func))                   return parse_function_declaration(self);
    if (self->equals(SEP_Lbrace))                return parse_block(self);
    if (self->equals(KW_If))                     return parse_if_statement(self);
    if (self->equals(KW_Return))                 return parse_return_statement(self);

    if (token_can_begin_expr(self->current())) {
        auto expression = parse_expression();
        if (expression->is_error()) return nullptr;
        auto stmt = static_cast<ast::Statement*>(expression);
        stmt->kind = ast::Statement_Kind::Expression;
        self->expect(SEP_Semi_Colon);
        return stmt;
    }

    compiler->report_error(self->currentl(), "Expected a statement", INVALID_TOKEN);
    self->skip_until_one_of({KW_Var, KW_Func, KW_If});
    return nullptr;
}

Opt<ast::Name> Parser::parse_name()
{
    auto &p = *this;
    if (!p->equals(Identifier)) {
       p.compiler->report_error(
           p->currentl(), "Expected an identifier", INVALID_TOKEN);
       return None;
    }

    Token token = p->next();

    ast::Name name{.locus = token.locus};
    name.value = p->slice_out(token.locus);
    return name;
}

ast::Expression *Parser::parse_typehint_expression()
{
    push_state(State::Typehint_Expression);
    auto *typehint = parse_expression();
    pop_state();
    return typehint;
}

ast::Typehint *Parser::parse_typehint(bool emit_error)
{
    using namespace ast;
    using namespace typehint;

    auto &p = *this;

    if (auto tok = p->try_expect(OP_Star)) {
        auto mutability = Mutability(bool(p->try_expect(KW_Const)));
        auto inner      = parse_typehint_expression();
        if (inner->is_error()) return static_cast<Typehint*>(inner);
        return allocate<Pointer>(p.allocator(), tok->locus.merge(inner->locus), mutability, inner);
    }

    if (auto tok = p->try_expect(TK_Int)) {
        return allocate<Integer>(p.allocator(), tok->locus, true, Integer_Width::Int32);
    }

    if (auto tok = p->try_expect(TK_Uint8)) {
        return allocate<Integer>(p.allocator(), tok->locus, false, Integer_Width::Int8);
    }

    if (auto tok = p->try_expect(TK_Float32)) {
        return allocate<Float>(p.allocator(), tok->locus, Float_Width::F32);
    }

    if (auto tok = p->try_expect(TK_Char)) {
        return allocate<Integer>(p.allocator(), tok->locus, false, Integer_Width::Int8);
    }

    if (auto tok = p->try_expect(TK_Bool)) {
        return allocate<Boolean>(p.allocator(), tok->locus);
    }

    todo("%d", p.current_.kind);

    if (emit_error)
        p.compiler->report_error(
            p->currentl(), "Expected a type specifier", INVALID_TOKEN);
    return allocate<ast::Error>(p.allocator(), p->currentl());
}

/*
 *   - variable declared and initialized with the type being defined.
 *   var   some_var_name :   some_type = some_value;
 *
 *   - constant declared and initialized with the type being defined.
 *   const another_var_name: some_type = some_value;
 *
 *   - constant declared and initialized without the type being defined.
 *   const yet_another_name = some_value;
 *
 *   - variable declared with the type defined.
 *  var my_variable_name: int;
 *
 *   - if [is_constant] then an initializer must be provided. we have enough information to
 *     ensure this at the parser. thus, the snippet below would be invalid.
 *  const MAX_NUMBER: int;
 */
ast::Statement *parse_variable_declaration(Parser &p)
{
    MINI_ASSERT(p->equals_one_of({KW_Var, KW_Const}), "invalid grammar");
    // consume var|const.
    auto begin       = p->next();
    auto is_constant = ast::Mutability(begin.kind == KW_Const);

    ast::Expression *typehint      = nullptr;
    ast::Expression *initializer = nullptr;

    // consume the variable name.
    auto name        = ({
        auto opt = p.parse_name();
        if (!opt.has_value()) return nullptr;
        *opt;
    });

    // consume an optional typehint.
    if (p->try_expect(SEP_Colon)) {
        typehint = p.parse_typehint_expression();
        if (typehint->is_error()) {
            p->skip_until_one_of({SEP_Semi_Colon}, true);
            return typehint;
        }
    }

    if (p->try_expect(OP_Assign)) {
        initializer = p.parse_expression();
        if (initializer->is_error()) {
            p->skip_until_one_of({SEP_Semi_Colon}, true);
            return initializer;
        }
    } else {
        if (bool(is_constant)) {
            p.compiler->report_error(name.locus, "All constant variables must be initialized at declaration", CONSTANT_DECL_NOT_INIT);
            p->skip_until_one_of({SEP_Semi_Colon}, true);
            return allocate<ast::Error>(p.allocator(), p->currentl());
        }
    }

    if (!p->try_expect(SEP_Semi_Colon)) {
        p.compiler->report_error(p->currentl(), "Expected `;` to end the variable declaration", INVALID_TOKEN);
        if (!p->skip_until_one_of({SEP_Semi_Colon}, true)) return nullptr;
    }

    return allocate<ast::stmt::Variable_Declaration>(
        p.allocator(),
        begin.locus.merge(p->previousl()),
        is_constant,
        name,
        typehint,
        initializer
    );
}

bool parse_function_prototype(Parser &p, ast::stmt::Function_Prototype &proto)
{
    if (!p->try_expect(SEP_Lparen)) {
        p.compiler->report_error(p->currentl(), "Expected `(` to begin function parameters", INVALID_TOKEN);
        return false;
    }

    while (!p.is_done() and !p->equals(SEP_Rparen)) {
        ast::stmt::Function_Parameter parameter{};

        if (p->equals_sequence({Identifier, SEP_Vararg})) {
            proto.vararg_name = *p.parse_name();
            auto tok = p->expect(SEP_Vararg);
            proto.flags |= PROTO_IS_VARIADIC;
            if (!p->equals(SEP_Rparen)) {
                p.compiler->report_error(tok->locus, "Expected `)` after variadic indicator", VARARG_NOT_END_PARAMS);
                return false;
            }
            break;
        }

        while (true) {
            if (auto tok = p->try_expect(KW_Move)) {
               if (parameter.flags & PARAM_IS_MOVE) {
                   p.compiler->report_error(tok->locus, "Duplicate annotation of `move`", DUP_ANNOTATION);
                   return false;
               }
               parameter.flags |= PARAM_IS_MOVE;
               continue;
            }

            if (auto tok = p->try_expect(KW_Using)) {
               if (parameter.flags & PARAM_IS_USING) {
                   p.compiler->report_error(tok->locus, "Duplicate annotation of `using`", DUP_ANNOTATION);
                   return false;
               }
               parameter.flags |= PARAM_IS_USING;
               continue;
            }

            if (auto tok = p->try_expect(KW_Var)) {
               if (parameter.flags & PARAM_IS_VAR) {
                   p.compiler->report_error(tok->locus, "Duplicate annotation of `var`", DUP_ANNOTATION);
                   return false;
               }
               parameter.flags |= PARAM_IS_VAR;
               continue;
            }

            break;
        }

        auto name_opt = p.parse_name();
        if (!name_opt) return false;

        parameter.name = *name_opt;

        if (p->try_expect_sequence({SEP_Colon, OP_Assign})) {
            // in the case of `:= ...`
            parameter.default_value = p.parse_expression();
            if (parameter.default_value->is_error()) return false;
        } else if (p->try_expect(SEP_Colon)) {
            parameter.typehint = p.parse_typehint_expression();
            if (parameter.typehint->is_error()) return p.parse_typehint_expression();
        } else {
            p.compiler->report_error(p.current_.locus, "Expected `:` after parameter name", INVALID_TOKEN);
            return false;
        }

        if (!parameter.default_value and
             p->try_expect(OP_Assign))
        {
            parameter.default_value = p.parse_expression();
            if (parameter.default_value->is_error()) return false;
        }

        proto.parameters.append(parameter);

        if (p->try_expect(SEP_Comma)) continue;
        break;
    }

    p->expect(SEP_Rparen);

    if (p->try_expect(SEP_Colon)) {
        proto.return_type = p.parse_typehint_expression();
        if (proto.return_type->is_error()) return false;
    }

    return true;
}

ast::Statement *parse_function_declaration(Parser &p)
{
    using namespace ast;
    using namespace ast::stmt;

    MINI_ASSERT(p->equals(KW_Func), "invalid grammar");
    auto begin = p->next();

    Function_Prototype prototype{p.compiler->allocator};
    Statement *body = nullptr;

    // consume the variable name.
    auto name = ({
        auto opt = p.parse_name();
        if (!opt.has_value()) return nullptr;
        *opt;
    });

    if (!parse_function_prototype(p, prototype)) {
        if (!p->skip_until_one_of({SEP_Semi_Colon, SEP_Lbrace})) return nullptr;
    }

    if (p->equals(SEP_Lbrace)) {
        body = p.parse_statement();
        if (!body or body->is_error()) return nullptr;
        MINI_ASSERT(body->kind == Statement_Kind::Block, "Invalid grammar");
    } else {
        if (!p->try_expect(SEP_Semi_Colon)) {
            p.compiler->report_error(p.current_.locus, "Expected `;` or `{` after function prototype", INVALID_TOKEN);
            return nullptr;
        }
    }

    return allocate<Function_Declaration>(
        p.compiler->allocator,
        begin.locus.merge(p.previous_.locus),
        name,
        prototype,
        body
    );
}

ast::Statement *parse_block(Parser &p)
{
    using namespace ast;
    using namespace ast::stmt;

    MINI_ASSERT(p->try_expect(SEP_Lbrace), "Invalid grammar");

    Token begin = p.previous_;
    Block::Body body{p.compiler->allocator};

    while (!p.is_done() and !p->equals(SEP_Rbrace)) {
        auto stmt = p.parse_statement();
        if (!stmt or stmt->is_error()) {
            if (!p->skip_until_one_of({SEP_Rbrace, SEP_Semi_Colon})) return stmt;
            if (p->equals({SEP_Semi_Colon})) p->next();
            continue;
        }
        body.append(stmt);
    }

    p->expect(SEP_Rbrace);
    return allocate<Block>(
        p.compiler->allocator,
        begin.locus.merge(p.previous_.locus),
        body
    );
}

ast::Expression *parse_if_header(Parser &p, ast::If_Header &header)
{
    p.push_state(State::If_Header_Condition);
    header.condition = p.parse_expression();
    if (header.condition->is_error()) return header.condition;
    header.body = p.parse_statement();
    if (header.body->is_error()) return static_cast<ast::Error*>(header.body);
    defer { p.pop_state(); };
    return header.condition;
}

ast::Statement *parse_if_statement(Parser &p)
{
    using namespace ast;
    using namespace ast::stmt;

    assert(p->equals(KW_If), "Invalid grammar");
    Token begin = p->next();

    If_Header then_branch;
    If_Statement::Branches branches{p.allocator()};
    Statement *else_branch = nullptr;

    if (auto c = parse_if_header(p, then_branch); c->is_error()) return c;

    while (p->try_expect_sequence({KW_Else, KW_If})) {
        If_Header elif_branch;
        if (auto c = parse_if_header(p, elif_branch); c->is_error()) return c;
        branches.append(elif_branch);
    }

    if (p->try_expect(KW_Else)) {
        else_branch = p.parse_statement();
        if (else_branch->is_error()) return else_branch;
    }

    return allocate<If_Statement>(p.allocator(), begin.locus, then_branch, branches, else_branch);
}

ast::Statement *parse_return_statement(Parser &p)
{
    using namespace ast;
    using namespace ast::stmt;
    assert(p->equals(KW_Return), "Invalid grammar");

    Token begin = p->next();
    Expression *value = nullptr;

    if (!p->equals(SEP_Semi_Colon)) {
        value = p.parse_expression();
        if (value->is_error()) return value;
    }

    if (!p->expect(SEP_Semi_Colon)) {
        p->skip_until_one_of({SEP_Semi_Colon}, true);
        return nullptr;
    }

    Locus locus = value ? begin.locus.merge(value->locus) : begin.locus;

    return allocate<Return>(p.allocator(), locus, value);
}
