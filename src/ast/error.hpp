#pragma once

#include "ast/typehint.hpp"
#include "ast/statement.hpp"
#include "ast/expression.hpp"

namespace ast
{
    struct Error : Typehint {
        Error(Locus locus) :
            Typehint(locus, Typehint_Kind::Error)
        {}
        bool is_error() const override { return true; }
    };
} // namespace ast
