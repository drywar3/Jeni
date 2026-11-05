#pragma once

#include "sema/def.hpp"
#include "misc/misc.hpp"
#include "sema/entities/id.hpp"

namespace sema
{
    struct Semantic_Storage;
    struct Type {
        enum Kind {
            Error,
            Int8,
            Int16,
            Int32,
            Int64,
            Isize,
            Uint8,
            Uint16,
            Uint32,
            Uint64,
            Float32,
            Float64,
            Usize,
            Char,
            Void,
            Bool,
            Function,
            Function_Set,
            Pointer,
            Struct,
        };

        Kind kind;

        Type(Kind kind) : kind(kind) {}

        String display(Alloc a, Semantic_Storage *store);

        bool is_integer() const;
        bool is_signed_integer() const;
        bool is_unsigned_integer() const;

        bool is_typed_pointer() const;
        bool is_void_pointer()  const;

        bool is_pointer() const;

        bool operator==(const Type &other) const;
    };

    struct Function_Set_Type : Type {
        Symbol_Id symbol_id;
        Function_Set_Type(Symbol_Id symbol_id) : Type(Type::Function_Set), symbol_id(symbol_id) {}
    };

    struct Pointer_Type : Type {
        Type_Id inner;
        Mutability access;
        Pointer_Type(Type_Id inner, Mutability access);
    };
}
