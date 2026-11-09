#include "sema/entities/symbol.hpp"
#include "sema/entities/symbols.hpp"
#include "sema/entities/id.hpp"
#include "sema/store.hpp"
#include "sema/sema.hpp"
#include "sema/type/matcher.hpp"

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

bool Function_Prototype::match(Semantic_Context &sema, const Function_Prototype &other)
{
    usize given_pcount    = other.parameter_types.count();

    bool we_are_variadic = (flags & PROTO_IS_VARIADIC);
    Function_Arity arity = this->arity;

    // (int, int, ...)  -- min = 2, max = 2, variadic = true
    // (int, int, int)  -- count = 3

    bool insufficient_args_provided = given_pcount < arity.min;
    bool too_many_arg_provided      = (given_pcount > arity.max) && !we_are_variadic;

    if (insufficient_args_provided or too_many_arg_provided) return false;

    for (usize n = 0; n < arity.max; n++) {
        Type_Id expected  = parameter_types[n];
        Type_Id given     = other.parameter_types[n];
        auto match_result = Type_Matcher(&sema, expected, given).match();
        if (!match_result.is_ok(true)) {
            return false;
        }
    }

    return true;
}

Opt<Function*> Function_Set::get_compatible_function(Semantic_Context &sema, const Function_Prototype &given_prototype)
{
    for (auto &[expected_prototype, function, _] : map) {
        if (expected_prototype.match(sema, given_prototype)) return &function;
    }
    return None;
}
