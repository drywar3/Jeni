#include "sema/type/id.hpp"
#include "sema/entities/type.hpp"

using namespace sema;

Pointer_Type::Pointer_Type(Type_Id inner, Mutability access)
    : Type(Kind::Pointer), inner(inner), access(access)
{
}

bool Type::operator==(const Type &other) const
{
    if (kind != other.kind) return false;
    switch (kind) {
    case Kind::Function_Set:
        {
            auto self      = static_cast<const Function_Set_Type*>(this);
            auto other_ptr = static_cast<const Function_Set_Type*>(&other);
            return self->symbol_id == other_ptr->symbol_id;
        }
    // @TODO: don't default to return true.
    default: return true;
    }
}

String Type::display(Alloc a, Semantic_Storage *store)
{
    switch (kind) {
    case Kind::Int32:   return String::build(a, "int32");
    case Kind::Uint8:   return String::build(a, "uint8");
    case Kind::Void:    return String::build(a, "void");
    case Kind::Float32: return String::build(a, "float32");
    case Kind::Float64: return String::build(a, "float64");
    case Kind::Bool:    return String::build(a, "bool");
    case Kind::Function_Set: MINI_UNREACHABLE();
    default: todo();
    }
}


bool Type::is_signed_integer() const
{
    switch (kind) {
        case Kind::Int8:
        case Kind::Int16:
        case Kind::Int32:
        case Kind::Int64:
        case Kind::Isize:
            return true;
        default: return false;
    }
}

bool Type::is_unsigned_integer() const
{
    switch (kind) {
        case Kind::Uint8:
        case Kind::Uint16:
        case Kind::Uint32:
        case Kind::Uint64:
        case Kind::Usize:
            return true;
        default: return false;
    }
}

bool Type::is_integer() const
{
    return is_signed_integer() or is_unsigned_integer();
}

bool Type::is_typed_pointer() const
{
    if (kind != Kind::Pointer) return false;
    auto pointer_type = static_cast<const Pointer_Type*>(this);
    return pointer_type->inner != type_id::VOID;
}

bool Type::is_void_pointer()  const
{
    if (kind != Kind::Pointer) return false;
    auto pointer_type = static_cast<const Pointer_Type*>(this);
    return pointer_type->inner == type_id::VOID;
}

bool Type::is_pointer() const
{
    return kind == Kind::Pointer;
}
