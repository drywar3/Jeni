#pragma once

#include "misc/misc.hpp"
#include "misc/densemap.hpp"
#include "sema/entities/id.hpp"

namespace sema
{
    struct Function_Scope {
        sema::Type_Id return_type;
    };

    struct Scope {
        using Map = ::Map<Sv, Symbol_Id>;

        enum Kind {
            Global,
            Function,
            Parameter,
            Block,
        };

        Map  map;
        Kind kind;
        Opt<Scope_Id> parent;

        Function_Scope func_scope;

        Scope(Alloc alloc, Kind kind, Opt<Scope_Id> parent);

        void put(Sv name, Symbol_Id id);
        bool has(Sv name);
        Opt<Symbol_Id> get(Sv name);
    };
}
