#pragma once

#include "misc/misc.hpp"
#include "ast/node.hpp"
#include "ast/statement.hpp"

namespace ast
{
    enum struct Expression_Kind {
        Error,
        Integer,
        Char,
        Float,
        Identifier,
        String,
        CString,
        Boolean,
        Data_Initializer,
        Typehint,
        Eval,
        Address_Of,
        Dereference,
        Binary_Operation,
        Unary_Operation,
        Function_Call,
    };

    struct Expression : Statement {
        Expression(Locus locus, Expression_Kind kind) :
            Statement(locus, Statement_Kind::Expression),
            kind(kind)
        {
        }

        Expression_Kind kind;

        template<typename Derived_Type>
        static Expression *make(Alloc alloc, Locus locus, Expression_Kind kind, Derived_Type derived)
        {
            auto *expr_mem = (Expression*)MINI_ALLOC(alloc, Derived_Type);
            if (expr_mem == nullptr) return nullptr;
            new (expr_mem) Derived_Type(std::move(derived));
            expr_mem->locus = locus;
            expr_mem->kind  = kind;
            return expr_mem;
        }
    };

    namespace expr
    {

    } // namespace expr
} // namespace ast
