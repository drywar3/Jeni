#pragma once

#include "parser/locus.hpp"
#include "sema/entities/id.hpp"
#include "sema/entities/profile.hpp"

namespace sema
{
    enum struct Symbol_Kind {
        Variable,
        Type,
        Namespace,
        Paramaeter,
        Function_Set,
    };

    enum struct Symbol_State {
        // the symbol has fully being resolved and all information is complete.
        Resolved,
        // the symbol exists but has not yet been resolved.
        Unresolved,
        // the symbol is not resolved as it is waiting on another (or more) symbol[s] to be resolved.
        Waiting,
        // the symbol could not be resolved due to an error.
        // proper diagnostics should have been reported.
        Failed,
    };

    struct Symbol {
        Sv             name;
        Locus          locus;
        Scope_Id       scope_id; // the scope it was defined in.
        Opt<Symbol_Id> definition_namespace;
        Opt<sema::Type_Id>  type_id;
        Symbol_State   state;
        Profile        profile;

        Symbol(Sv name, Locus locus, Scope_Id scope_id, Opt<Symbol_Id> def_ns, Opt<Type_Id> type_id, Symbol_State state);

        virtual Symbol_Kind kind() const = 0;
    };
} // namespace sema
