#pragma once

#include "parser/locus.hpp"
#include "sema/sema_id.hpp"

namespace ast
{
    struct Node
    {
        Locus locus;

        sema::Semantic_Id sema_id;

        Node(Locus locus) : locus(locus) {}
        virtual bool is_error() const { return false; }
    };
} // namespace ast
