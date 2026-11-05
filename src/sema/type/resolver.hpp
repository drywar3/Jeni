#pragma once

#include "sema/sema.hpp"
#include "ast/typehint.hpp"

namespace sema
{
    struct Type_Resolver
    {
        Semantic_Context &sema;
        ast::Typehint    *typehint;
        Opt<Locus>        target_locus;

        Type_Resolver(Semantic_Context &sema, ast::Typehint *typehint, Opt<Locus> tloc = None);

        Worker_Status resolve();
        Worker_Status resolve_pointer_type();
    };
} // namespace sema
