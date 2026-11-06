#include "sema/type/id.hpp"
#include "sema/type/resolver.hpp"

using namespace ast;
using namespace sema;

Type_Resolver::Type_Resolver(Semantic_Context &sema, ast::Typehint *typehint, Opt<Locus> tloc)
    : sema(sema), typehint(typehint), target_locus(tloc)
{
}

Worker_Status Type_Resolver::resolve_pointer_type()
{
    auto pointer = static_cast<typehint::Pointer*>(typehint);
    auto status  = Type_Resolver(sema, static_cast<Typehint*>(pointer->inner), pointer->inner->locus).resolve();
    if (!status) return status;

    usize n = 0;
    Type_Id type_id = sema.get_type_at_locus(pointer->inner->locus);
    for (const Type *type : sema.store->types) {
        if (type->is_pointer()) {
            auto pointer_type = static_cast<const Pointer_Type*>(type);
            if (pointer_type->inner == type_id) {
                Type_Id type_id = Type_Id(n);
                if (target_locus) sema.set_type_at_locus(*target_locus, type_id);
                return Worker_Status::Done;
            }
        }
        n += 1;
    }

    Type_Id pointer_type_id = sema.register_or_insert_type(
                                allocate<Pointer_Type>(
                                    sema.allocator,
                                    type_id,
                                    pointer->access));
    if (target_locus) sema.set_type_at_locus(*target_locus, pointer_type_id);
    return Worker_Status::Done;
}

Worker_Status Type_Resolver::resolve()
{
    if (auto expr = dynamic_cast<Expression*>(typehint);
        expr and expr->kind != Expression_Kind::Typehint) {
        switch (expr->kind) {
            case Expression_Kind::Identifier: break;
            /* case Expression_Kind::Member_Access: */ break;
            default: todo("");
        }
        todo();
    }

    switch (typehint->kind) {
        case Typehint_Kind::Pointer: return resolve_pointer_type();
        case Typehint_Kind::Bool:
            if (target_locus) sema.set_type_at_locus(*target_locus, type_id::BOOL);
            break;
        case Typehint_Kind::Float:
            {
                auto float_type = static_cast<typehint::Float*>(typehint);
                switch (float_type->width) {
                    case Float_Width::F32:
                        if (target_locus) sema.set_type_at_locus(*target_locus, type_id::FLOAT32);
                        break;
                    default: MINI_UNREACHABLE();
                }
            } break;
        case Typehint_Kind::Signed_Int:
        case Typehint_Kind::Unsigned_Int:
            {
                auto is_signed = typehint->kind == Typehint_Kind::Signed_Int;
                auto integer = static_cast<typehint::Integer*>(typehint);
                switch (integer->width) {
                    case Integer_Width::Int8:
                        // @Fixme : will crash if [target_locus] is None
                        if (is_signed) sema.set_type_at_locus(*target_locus, type_id::INT8);
                        else sema.set_type_at_locus(*target_locus, type_id::UINT8);
                        break;
                    case Integer_Width::Int16: todo();
                    case Integer_Width::Int64: todo();
                    case Integer_Width::Size:  todo();
                    case Integer_Width::Int32:
                        if (is_signed) {
                            if (target_locus) sema.set_type_at_locus(*target_locus, type_id::INT32);
                            break;
                        } else todo();
                    default: MINI_UNREACHABLE();
                }
            } break;
        default: todo();
    }
    return Worker_Status::Done;
}
