#pragma once

#include "sema/store.hpp"
#include "compiler.hpp"

namespace sema
{
    struct Type_Layout {
        struct Entry {
            usize size;
            usize alignment;
        };

        Semantic_Storage *store;
        Target            target;
        HashMap<Type_Id, Entry> sizes;

        Type_Layout(Alloc a, Semantic_Storage *store, Target target);

        usize get_size(Type_Id type_id);
    private:
        Entry calculate(Type_Id type_id);
    };
} // namespace sema
