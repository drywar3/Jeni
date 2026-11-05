#include "sema/sema.hpp"

Opt<sema::Symbol_Proxy> sema::Semantic_Context::find_symbol(Sv name)
{
    return find_symbol_in(name, current_scope);
}

Opt<sema::Symbol_Proxy> sema::Semantic_Context::find_symbol_in(Sv name, Scope_Id scope_id)
{
    auto &scope = store->scopes.at_index(usize(scope_id));
    if (scope.has(name)) {
        return Symbol_Proxy(this, *scope.get(name));
    }
    return None;
}

Opt<sema::Symbol_Proxy> sema::Semantic_Context::eagerly_find_symbol(Sv name)
{
    return eagerly_find_symbol_in(name, current_scope);
}

Opt<sema::Symbol_Proxy> sema::Semantic_Context::eagerly_find_symbol_in(Sv name, Scope_Id scope_id)
{
    Opt<Scope_Id> current = scope_id;
    while (current.has_value()) {
        auto &scope = store->scopes.at_index(usize(*current));
        if (scope.has(name)) {
            return Symbol_Proxy(this, *scope.get(name));
        }
        current = scope.parent;
    }
    return None;
}

sema::Symbol_Id sema::Semantic_Context::register_symbol(Scope_Id scope_id, Symbol_Ptr symbol_ptr)
{
    auto &scope = store->scopes.at_index(usize(scope_id));
    auto symbol_id = Symbol_Id(store->symbols.add_value(symbol_ptr));
    scope.put(symbol_ptr->name, symbol_id);
    set_symbol_at_locus(symbol_ptr->locus, symbol_id);
    return symbol_id;
}

sema::Type_Id sema::Semantic_Context::get_type_at_locus(Locus locus)
{
    MINI_ASSERT(
        store->types.contains(locus), "Compiler Bug: cannot retrieve type id");
    return Type_Id(store->types[locus]);
}

void sema::Semantic_Context::set_type_at_locus(Locus locus, Type_Id type_id)
{
    store->types.link(locus, usize(type_id));
}

Opt<sema::Symbol_Proxy> sema::Semantic_Context::get_symbol_at_locus(Locus locus)
{
    if (!store->symbols.contains(locus)) return None;
    return Symbol_Proxy(this, Symbol_Id(*store->symbols.get_id(locus)));
}

sema::Symbol_Proxy sema::Semantic_Context::get_symbol_at_locus_unchecked(Locus locus)
{
    MINI_ASSERT(store->symbols.contains(locus), "Compiler Bug: cannot retrieve symbol");
    return Symbol_Proxy(this, Symbol_Id(*store->symbols.get_id(locus)));
}

void sema::Semantic_Context::set_symbol_at_locus(Locus locus, Symbol_Id symbol_id)
{
    store->symbols.link(locus, usize(symbol_id));
}

sema::Type_Id sema::Semantic_Context::register_or_insert_type(Type *type)
{
    auto &types = store->types;
    usize id = 0;
    for (const auto &type_ : types) {
        assert(type_ != nullptr, "Compiler Bug: invalid type pointer");
        if (*type == *type_) {
            return Type_Id(id);
        }
        id += 1;
    }
    return sema::Type_Id(types.add_value(type));;
}

sema::Scope_Id sema::Semantic_Context::enter_scope(Scope::Kind kind)
{
    auto new_scope = Scope_Id(store->scopes.add_value(Scope(allocator, kind, current_scope)));
    current_scope  = new_scope;
    return new_scope;
}

void sema::Semantic_Context::exit_scope()
{
    const Scope &scope_object = store->scopes.at_index(usize(current_scope));
    if (!scope_object.parent) current_scope = global_scope;
    else                      current_scope = *scope_object.parent;
}
