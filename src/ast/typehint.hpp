#pragma once

#include "ast/expression.hpp"

namespace ast
{
    enum struct Integer_Width {
        Int8,
        Int16,
        Int32,
        Int64,
        Size,
    };

    enum struct Float_Width {
        F32,
        F64,
    };

    enum struct Typehint_Kind {
        Error,
        Signed_Int,
        Unsigned_Int,
        Float,
        Char,
        Void,
        Bool,
        Array,
        Function,
        Pointer,
        Nullable,
    };

    enum struct Mutability : bool {
        Constant = true,
        Mutable  = false,
    };

    struct Typehint : Expression {
        Typehint_Kind kind;
        Typehint(Locus locus, Typehint_Kind kind) : Expression(locus, Expression_Kind::Typehint), kind(kind) {}
    };

    namespace typehint
    {
        struct Integer : Typehint {
            Integer_Width width;

            Integer(Locus locus, bool is_signed, Integer_Width width)
                : Typehint(locus, is_signed? Typehint_Kind::Signed_Int : Typehint_Kind::Unsigned_Int),
                  width(width)
            {}
        };

        struct Pointer : Typehint {
            Mutability  access;
            Expression *inner;

            Pointer(Locus locus, Mutability access, Expression *inner)
                : Typehint(locus, Typehint_Kind::Pointer),
                  access(access), inner(inner)
            {}
        };

        struct Float : Typehint {
            Float_Width width;

            Float(Locus locus, Float_Width width)
                : Typehint(locus,Typehint_Kind::Float),
                  width(width)
            {}
        };

        struct Boolean : Typehint {
            Boolean(Locus locus) : Typehint(locus, Typehint_Kind::Bool) {}
        };
    } // namespace typehint
} // namespace ast
