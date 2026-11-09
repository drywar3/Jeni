#pragma once

#include "misc/map.hpp"
#include "sema/def.hpp"
#include "ast/typehint.hpp"
#include "misc/densemap.hpp"
#include "sema/entities/id.hpp"
#include "sema/entities/symbol.hpp"
#include "sema/entities/type.hpp"

namespace sema
{
    struct Semantic_Storage;
    struct Semantic_Context;

    struct Function_Arity {
        usize min;
        usize max;
    };

    struct Function_Prototype : Type {
        sema::Type_Id return_type;
        Arr<sema::Type_Id> parameter_types;
        Function_Arity arity;

        int flags = 0;

        Function_Prototype() : Type(Kind::Function) {}
        Function_Prototype(Alloc a, int flags = 0) : Type(Kind::Function), parameter_types(a), flags(flags) {}

        Function_Prototype with_arity(Function_Arity arity) { this->arity = arity; return *this; }

        String to_string(Alloc a, Semantic_Storage *store);

        bool operator==(const Function_Prototype &other) const {
            if (parameter_types.count() != other.parameter_types.count()) return false;
            for (usize n = 0; n < parameter_types.count(); n++) {
                if (parameter_types[n] != other.parameter_types[n]) return false;
            }
            return true;
        }

        bool match(Semantic_Context &sema, const Function_Prototype &other);
    };

}

template <> struct Hash<sema::Function_Prototype> {
    std::size_t operator()(const sema::Function_Prototype &prot) const
    {
        uint64 seed = 0;
        for (auto param : prot.parameter_types.iter()) {
            hash_combine(seed, usize(param));
        }
        return seed;
    }
};

namespace sema
{
    struct Variable : Symbol {
        Mutability   mutability;
        Opt<Type_Id> type_id;
        bool         is_initialized;

        Variable(
            Sv name,
            Locus locus,
            Scope_Id scope_id,
            Mutability mutability,
            Opt<Type_Id> type_id,
            bool is_initialized);

        Symbol_Kind kind() const override {
            return Symbol_Kind::Variable;
        }
    };

    struct Function {
        Locus locus;
        sema::Type_Id return_type;
    };

    struct Function_Set : Symbol {
        using Map = ::Map<Function_Prototype, Function>;
        Map map;

        Function_Set(Sv name, Locus locus, Scope_Id scope_id, Alloc alloc);

        Symbol_Kind kind() const override {
            return Symbol_Kind::Function_Set;
        }

        Opt<Function*> get_compatible_function(Semantic_Context &sema, const Function_Prototype &prototype);
    };
} // namespace sema
