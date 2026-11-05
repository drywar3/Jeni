#include "sema/entities/symbol.hpp"
#include "sema/entities/symbols.hpp"
#include "sema/entities/id.hpp"
#include "sema/store.hpp"

using namespace sema;

Symbol::Symbol(Sv name, Locus locus, Scope_Id scope_id, Opt<Symbol_Id> def_ns, Opt<Type_Id> type_id, Symbol_State state)
    : name(name), locus(locus), scope_id(scope_id), definition_namespace(def_ns), type_id(type_id), state(state)
{
}

Variable::Variable(
    Sv name,
    Locus locus,
    Scope_Id scope_id,
    Mutability mutability,
    Opt<Type_Id> type_id,
    bool is_initialized)
    : Symbol(name, locus, scope_id, None, type_id, Symbol_State::Unresolved),
      mutability(mutability),
      type_id(type_id),
      is_initialized(is_initialized)
{
}

Function_Set::Function_Set(Sv name, Locus locus, Scope_Id scope_id, Alloc alloc)
    : Symbol(name, locus, scope_id, None, None, Symbol_State::Unresolved),
      map(alloc)
{
}

String Function_Prototype::to_string(Alloc a, Semantic_Storage *store)
{
    String s(a);
    usize n = 0;
    s.append_fmt("(");
    for (auto type_id : parameter_types.iter()) {
        if (n != 0) s.append_fmt(", ");
        s.append_fmt("%s", (*store->types.at_index_ptr(usize(type_id)))->display(a, store).data());
        n += 1;
    }
    s.append_fmt(")");
    return s;
}
