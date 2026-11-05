#pragma once

#include "source.hpp"
#include "ast/statement.hpp"
#include "ast/expression.hpp"

namespace ast {
    struct Program {
        using Ast = mini::Array<ast::Statement *>;

        Ast ast;
        Source_Id source_id;

        Program(Alloc a, Source_Id source_id) : ast(a), source_id(source_id) {}
    };
} // namespace ast;
