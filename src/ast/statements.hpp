#pragma once

#include "ast/name.hpp"
#include "ast/typehint.hpp"
#include "ast/statement.hpp"
#include "ast/expressions.hpp"

constexpr int PARAM_IS_USING = 1 << 0;
constexpr int PARAM_IS_MOVE  = 1 << 1;
constexpr int PARAM_IS_VAR   = 1 << 2;

constexpr int PROTO_IS_VARIADIC = 1 << 0;

namespace ast
{
    struct If_Header {
        Expression *condition;
        Statement  *body;
    };
} // namespace ast

namespace ast::stmt
{
    struct Variable_Declaration : ast::Statement
    {
        Name        name;
        Mutability  mutability;
        Expression *typehint;
        Expression *initializer;
        bool        type_is_defined;
        bool        is_initialized;

        Variable_Declaration(Locus       locus,
                             Mutability  mutability,
                             Name        name,
                             Expression *typehint = nullptr,
                             Expression *initializer = nullptr)
            : ast::Statement(locus, Statement_Kind::Variable),
              name(name),
              mutability(mutability),
              typehint(typehint),
              initializer(initializer),
              type_is_defined(typehint != nullptr),
              is_initialized(initializer != nullptr)
        {
        }
    };

    struct Function_Parameter {
        Name name;
        Expression *typehint      = nullptr;
        Expression *default_value = nullptr;
        int flags                 = 0;
    };

    using Function_Parameters = Arr<Function_Parameter>;

    struct Function_Prototype {
        Expression *return_type = nullptr;
        Function_Parameters parameters;
        int flags = 0;
        Name vararg_name;
        Function_Prototype(Alloc a): parameters(a) {}
    };

    struct Function_Declaration : Statement
    {
        Name name;
        Function_Prototype prototype;
        Statement *body = nullptr;

        Function_Declaration(
            Locus locus,
            Name name,
            Function_Prototype proto,
            Statement *body = nullptr
        );
    };

    struct Block : Statement {
        using Body = Arr<Statement*>;
        Body body;
        Block(Locus locus, Body body);
    };

    struct If_Statement : Statement {
        using Branches = Arr<If_Header>;
        If_Header  then_branch;
        Branches   branches;
        Statement *else_branch;

        If_Statement(Locus locus, If_Header then_branch, Branches branches, Statement *else_branch);
    };

    struct Return : Statement {
        Expression *value;
        Return(Locus locus, Expression *value);
    };
} // namespace ast::stmt
