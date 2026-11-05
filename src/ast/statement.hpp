#pragma once

#include "ast/node.hpp"

namespace ast
{
    enum struct Statement_Kind {
        Error,
        Function,
        Variable,
        Expression,
        If_Statement,
        For_Loop,
        While_Loop,
        For_Each_Loop,
        Block,
        Return,
    };

    struct Statement : Node {
        Statement(Locus locus, Statement_Kind kind) : Node(locus), kind(kind) {}

        Statement_Kind kind;

        template<typename Derived_Type>
        static Statement *make(Alloc alloc, Statement_Kind kind, Locus locus, Derived_Type derived)
        {
            auto *stmt_mem = (Statement*)MINI_ALLOC(alloc, Derived_Type);
            if (stmt_mem == nullptr) return nullptr;
            new (stmt_mem) Derived_Type(std::move(derived));
            stmt_mem->locus = locus;
            stmt_mem->kind  = kind;
            return stmt_mem;
        }
    };

    namespace stmt
    {

    } // namespace stmt
} // namespace ast
