#pragma once

#include "sema/sema.hpp"

namespace sema
{
    struct Match_Result {
        enum Value {
            Identity,
            Ok,
            Int_To_Uint_Conversion,
            Uint_To_Int_Conversion,
            // trying to pass any integer type to a float type.
            Int_To_Float_Promotion,
            // trying to pass any float type to an integer type.
            Float_To_Int_Trucating,
            // trying to pass a typed pointer to a void pointer.
            Pointer_Void_Typing,
            // passing a const pointer (*const T) to a mutable pointer (*T)
            Const_To_Var_Pointer,
            // passing a pointer of one type to a pointer of a different type.
            Pointer_Reinterpretation,
            // outright invalid.
            // example:
            //     |
            //   1 |  var x: string = true;
            //     |
            Invalid,
        };

        Value value;

        Match_Result(Value value) : value(value) {}

        operator Value() const { return value; }

        bool is_ok(bool is_strict) const;
        void report(Semantic_Context &sema, Type_Id target_id, Type_Id source_id, Locus offending_locus);
    };

    struct Type_Matcher {
        Semantic_Context *sema;
        Type_Id           target_id;
        Type_Id           source_id;

        Type_Matcher(
            Semantic_Context *sema,
            Type_Id           source_id,
            Type_Id           target_id);
        Match_Result match() const;
    };
} // namespace sema
