#pragma once

#include "ast/name.hpp"
#include "ast/expression.hpp"

namespace ast
{
    enum struct Operator {
        Add,
        Minus,
        Star,
        Division,
        Inc,
        Dec,
        Not,
        Dereference,
        Assign
    };

    struct Call_Argument {
        Expression *value;
        Opt<Name>     name;

        Call_Argument(Expression *value, Opt<Name> name);
    };

    namespace expr
    {
        struct Integer : Expression {
            int64 value;

            Integer(Locus locus, int64 value)
                : Expression(locus, Expression_Kind::Integer), value(value)
            {}
        };

        struct Float : Expression {
            double value;

            Float(Locus locus, double value)
                : Expression(locus, Expression_Kind::Float), value(value)
            {}
        };

        struct Char : Expression {
            uint8 value;

            Char(Locus locus, uint8 value)
                : Expression(locus, Expression_Kind::Char), value(value)
            {}
        };

        struct CString : Expression {
            Sv value;
            CString(Locus locus, Sv value) : Expression(locus, Expression_Kind::CString), value(value) {}
        };

        struct Boolean : Expression {
            bool value;

            Boolean(Locus locus, bool value) : Expression(locus, Expression_Kind::Boolean), value(value) {}
        };

        struct Identifier : Expression {
            Sv value;

            Identifier(Locus locus, Sv value)
                : Expression(locus, Expression_Kind::Identifier), value(value)
            {}
        };

        struct Unary_Operation : Expression {
            Operator    op;
            Expression *expr;

            Unary_Operation(Locus locus, Operator op, Expression *expr)
                : Expression(locus, Expression_Kind::Unary_Operation),
                  op(op), expr(expr)
            {}
        };

        struct Binary_Operation : Expression {
            Operator    op;
            Expression *left;
            Expression *right;

            Binary_Operation(Locus locus, Operator op, Expression *left, Expression *right)
                 : Expression(locus, Expression_Kind::Binary_Operation),
                   op(op), left(left), right(right)
             {}
         };

         struct Function_Call : Expression {
             using Args = Arr<Call_Argument>;

             Expression *callee;
             Args        args;

             Function_Call(Locus locus, Expression *callee, Args args);
         };

         struct Eval : Expression {
             Expression *expr;
             Eval(Locus locus, Expression *expr);
         };
    } // namespace expr
} // namespace ast
