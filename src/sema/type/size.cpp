#include "sema/type/size.hpp"
#include "sema/store.hpp"

using namespace sema;

Type_Layout::Type_Layout(Alloc a, Semantic_Storage *store, Target target)
    : store(store), target(target), sizes(a)
{}

usize Type_Layout::get_size(Type_Id type_id)
{
    if (sizes.contains(type_id)) {
        return sizes[type_id].size;
    }

    Entry entry = calculate(type_id);
    sizes[type_id] = entry;
    return entry.size;
}

Type_Layout::Entry Type_Layout::calculate(Type_Id type_id)
{
    auto &type = *store->types.at_index(usize(type_id));
    usize size = 0, alignment = 0;
    switch (type.kind) {
    case Type::Kind::Int8:
    case Type::Kind::Uint8:
        size = 1;
        alignment = 1;
        break;
    case Type::Kind::Int16:
    case Type::Kind::Uint16:
        size = 2;
        alignment = 2;
        break;
    case Type::Kind::Int32:
    case Type::Kind::Uint32:
        size = 4;
        alignment = 4;
        break;
    case Type::Kind::Int64:
    case Type::Kind::Uint64:
        size = 8;
        alignment = 8;
        break;
    case Type::Kind::Pointer:
        size = target.pointer_size;
        alignment = target.pointer_size;
        break;
    default: todo();
    }
    return {size, alignment};
}
