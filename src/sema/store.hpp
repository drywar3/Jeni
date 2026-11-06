#pragma once

#include "misc/id_allocator.hpp"
#include "parser/locus.hpp"
#include "misc/densemap.hpp"
#include "sema/entities/type.hpp"
#include "sema/entities/scope.hpp"
#include "sema/entities/symbols.hpp"

namespace sema
{
    struct Semantic_Context;
    enum struct Semantic_Id;

    using Symbol_Ptr = Symbol*;
    using Type_Ptr   = Type*;

    using Symbol_Storage = DenseMap<Locus, Symbol_Ptr>;
    using Scope_Storage  = DenseMap<Locus, Scope>;
    using Type_Storage   = DenseMap<Locus, Type_Ptr>;
    using Semantic_Pool  = Id_Allocator<Semantic_Context, Semantic_Id>;
    using Block_Save_Point = HashMap<Locus, usize>;
    using Prototype_Storage= HashMap<Locus, Function_Prototype>;

    struct Semantic_Storage {
        Symbol_Storage symbols;
        Type_Storage   types;
        Scope_Storage  scopes;
        Semantic_Pool  semas;
        Block_Save_Point bsp;
        Prototype_Storage prots;

        Type *char_pointer_type;

        Semantic_Storage(Alloc alloc);
    };
} // namespace sema
