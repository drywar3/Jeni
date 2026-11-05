#pragma once

#include "sema/entities/id.hpp"

namespace sema
{
    struct Semantic_Context;

    template<typename T, auto Fn> struct Proxy {
        Semantic_Context *sema;
        Symbol_Id symbol_id;

        Proxy(Semantic_Context *sema, Symbol_Id id)
            : sema(sema), symbol_id(id) {}

        auto get_id() const { return symbol_id; }

        T *operator->() {
            return Fn(sema, symbol_id);
        }

        T *operator*() {
            return Fn(sema, symbol_id);
        }
    };

    Symbol *symbol_proxy_fn(Semantic_Context *sema, Symbol_Id id);

    using Symbol_Proxy = Proxy<Symbol, symbol_proxy_fn>;
}
