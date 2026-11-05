#include "sema/type/matcher.hpp"

using namespace sema;

Type_Matcher::Type_Matcher(
    Semantic_Context *sema,
    Type_Id           source_id,
    Type_Id           target_id)
    : sema(sema), target_id(target_id), source_id(source_id)
{
}

bool Match_Result::is_ok(bool is_strict) const
{
    if (is_strict) {
        return value == Identity || value == Ok;
    } else {
        return value != Invalid;
    }
}

void Match_Result::report(Semantic_Context &sema, Type_Id target_id, Type_Id source_id, Locus offending_locus)
{
    Alloc a = sema.allocator;
    Type *target = *sema.store->types.at_index_ptr(usize(target_id));
    Type *source = *sema.store->types.at_index_ptr(usize(source_id));
    String message;

    switch (value) {
    case Int_To_Uint_Conversion:
        message = String::build(a,
             "Cannot implicitly pass an uint (%s) to a sint (%s)",
             source->display(a, sema.store),
             target->display(a, sema.store));
        break;
    case Uint_To_Int_Conversion:
        message = String::build(a,
            "Cannot implicitly pass an sint (%s) to a uint (%s)",
            source->display(a, sema.store),
            target->display(a, sema.store));
        break;
    case Invalid:
        message = String::build(a,
            "Cannot pass a value of type (%s) to (%s)",
            source->display(a, sema.store),
            target->display(a, sema.store));
        break;
    default: todo();
    }

    sema.compiler->report_error(
        offending_locus,
        message.data(),
        TYPE_MISMATCH
    );
}

Match_Result Type_Matcher::match() const
{
    if (target_id == source_id) return Match_Result::Identity;

    Type *target = *sema->store->types.at_index_ptr(usize(target_id));
    Type *source = *sema->store->types.at_index_ptr(usize(source_id));


    if (target->is_integer() and source->is_integer()) {
        if (target->is_signed_integer() and source->is_unsigned_integer()) {
            return Match_Result::Uint_To_Int_Conversion;
        }

        if (target->is_unsigned_integer() and source->is_signed_integer()) {
            return Match_Result::Int_To_Uint_Conversion;
        }
    }

    if (target->is_void_pointer() and source->is_typed_pointer())
        return Match_Result::Pointer_Void_Typing;

    return Match_Result::Invalid;
}
