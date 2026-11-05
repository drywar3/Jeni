#pragma once

#include "misc/misc.hpp"

enum Diag_Code: int {
    _FIRST = 100,

    UNEXPECTED_CHAR,
    INVALID_CHAR_LITERAL,

    UNDEFINED_SYMBOL,
    SYMBOL_REDECL,
    SYMBOL_KIND_MISMATCH,
    TYPE_MISMATCH,
    CYCLIC_DEP,
    NON_CONST_GLOB_EXPR,

    INVALID_TOKEN,
    CONSTANT_DECL_NOT_INIT,
    VARARG_NOT_END_PARAMS,
    DUP_ANNOTATION,
};

static inline const char *get_code_message(Diag_Code code)
{
    switch (code) {
        case _FIRST:               return "...";

        case UNEXPECTED_CHAR:      return "Unexpected character";
        case INVALID_CHAR_LITERAL: return "Invalid character literal";

        case UNDEFINED_SYMBOL:     return "Undefined symbol";
        case SYMBOL_REDECL:        return "Redefinition of symbol";
        case SYMBOL_KIND_MISMATCH: return "Redefinition of symbol as another kind";
        case TYPE_MISMATCH:        return "Type mismatch";
        case CYCLIC_DEP:           return "Cyclic Dependency";
        case NON_CONST_GLOB_EXPR:  return "Non-constant global expression";

        case INVALID_TOKEN:            return "Invalid token";
        case CONSTANT_DECL_NOT_INIT:   return "Missing constant initializer";
        case VARARG_NOT_END_PARAMS:    return "Variadic indicator did not end parameter list";
        case DUP_ANNOTATION:           return "Duplicate annotation";
        default: todo();
    }
}
