#include "misc/misc.hpp"
#include "sema/sema.hpp"
#include "sema/store.hpp"
#include "sema/type/id.hpp"
#include "mini.c/mini_def.h"
#include "sema/entities/id.hpp"
#include "sema/entities/type.hpp"

using namespace sema;

static void initialize_builtin_types(Semantic_Storage &store, Alloc alloc);

Semantic_Storage::Semantic_Storage(Alloc alloc, Target target)
    : symbols(alloc),
      types(alloc),
      scopes(alloc),
      semas(alloc),
      bsp(alloc),
      prots(alloc),
      layout(alloc, this, target)
{
    initialize_builtin_types(*this, alloc);
    char_pointer_type = allocate<Pointer_Type>(alloc, type_id::UINT8, Mutability::Constant);
    types.add_value(char_pointer_type);
}

void initialize_builtin_types(Semantic_Storage &store, Alloc alloc)
{
    using namespace sema::type_id;
    static Type_Ptr IDS[usize(LAST_)]{};

    IDS[usize(VOID)] = allocate<Type>(alloc, Type::Void);
    IDS[usize(ERROR)] = allocate<Type>(alloc, Type::Error);

    IDS[usize(INT8)] = allocate<Type>(alloc, Type::Int8);
    IDS[usize(INT16)] = allocate<Type>(alloc, Type::Int16);
    IDS[usize(INT32)] = allocate<Type>(alloc, Type::Int32);
    IDS[usize(INT64)] = allocate<Type>(alloc, Type::Int64);

    IDS[usize(UINT8)] = allocate<Type>(alloc, Type::Uint8);
    IDS[usize(UINT16)] = allocate<Type>(alloc, Type::Uint16);
    IDS[usize(UINT32)] = allocate<Type>(alloc, Type::Uint32);
    IDS[usize(UINT64)] = allocate<Type>(alloc, Type::Uint64);

    IDS[usize(FLOAT32)] = allocate<Type>(alloc, Type::Float32);
    IDS[usize(FLOAT64)] = allocate<Type>(alloc, Type::Float64);

    IDS[usize(CHAR)] = allocate<Type>(alloc, Type::Char);
    IDS[usize(BOOL)] = allocate<Type>(alloc, Type::Bool);

    static_assert(int(LAST_) == 14, "LAST_ type id has changed");
    for (usize n = 0; n < usize(LAST_); n++) {
        store.types.add_value(IDS[n]);
    }
}
