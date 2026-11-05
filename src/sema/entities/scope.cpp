#include "sema/entities/scope.hpp"
#include "mini.c/mini_def.h"

using namespace sema;

Scope::Scope(Alloc alloc, Kind kind, Opt<Scope_Id> parent)
    : map(alloc), kind(kind), parent(parent)
{
}

void Scope::put(Sv name, Symbol_Id id)
{
    map.insert(name, id);
}

bool Scope::has(Sv name)
{
    return map.contains(name);
}

Opt<Symbol_Id> Scope::get(Sv name)
{
    if (auto symbol_id = map.find(name)) return *symbol_id;
    return None;
}
