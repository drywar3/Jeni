#pragma once

//#include "sema/store.hpp"
//#include "compiler.hpp"
#include "compiler/target.hpp"
#include "sema/entities/id.hpp"
#include "misc/map.hpp"

namespace sema
{
    struct Semantic_Storage;
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
