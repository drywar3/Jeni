#include "sema/sema.hpp"
#include "sema/type/id.hpp"
#include "ast/expressions.hpp"
#include "sema/type/resolver.hpp"
#include "sema/entities/symbols.hpp"

using namespace sema;
using namespace ast;
using namespace ast::expr;

Worker_Status check_typehint(Semantic_Context &sema, Expression *&expr)
{
    assert(expr->kind == Expression_Kind::Typehint, "Compiler Bug: expected a typehint expression");
    auto typehint = static_cast<Typehint*>(expr);
    Worker_Status status = Type_Resolver(sema, typehint, expr->locus).resolve();
    return status;
}

Worker_Status check_call_against_function_set(
    Semantic_Context &sema,
    Function_Call *function_call,
    Symbol_Proxy  symbol
) {
    auto function_set = static_cast<Function_Set*>(*symbol);
    sema::Function_Prototype prototype{sema.allocator};
    for (auto argument : function_call->args.iter()) {
        Type_Id type_id = sema.get_type_at_locus(argument.value->locus);
        prototype.parameter_types.append(type_id);
    }

    if (!function_set->map.contains(prototype)) {
        sema.compiler->report_error(
            function_call->callee->locus,
            String::build(
                sema.allocator,
                "No matching function call to symbol `%.*s` for %s",
                SVARG(symbol->name.base()),
                prototype.to_string(sema.allocator, sema.store)
            ).data(),
            UNDEFINED_SYMBOL
        );
        return Worker_Status::Failed;
    }

    auto function = function_set->map[prototype];
    prototype.return_type = function.return_type;
    sema.set_type_at_locus(function_call->locus, function.return_type);
    sema.store->prots.insert(function_call->locus, prototype);

    auto prot_type = allocate<Function_Prototype>(sema.allocator, prototype);
    auto type_id   = sema.register_or_insert_type(prot_type);
    sema.set_type_at_locus(function_call->callee->locus, type_id);

    return Worker_Status::Done;
}

Worker_Status check_function_call(Semantic_Context &sema, Expression *&expr)
{
    assert(expr->kind == Expression_Kind::Function_Call, "Compiler Bug: expected a function call expression");

    auto function_call = static_cast<Function_Call*>(expr);
    auto &callee       = function_call->callee;
    auto &args         = function_call->args;

    Worker_Status status = sema.check_expression(Worker_Payload(callee), false);
    if (!status) return status;

    for (auto &argument : args.iter()) {
        if (argument.name) todo("Handle named arguments");
        Worker_Status arg_status = sema.check_expression(Worker_Payload(argument.value), false);
        if (!arg_status) return arg_status;
    }

    if (auto opt = sema.get_symbol_at_locus(callee->locus)) {
        auto symbol = *opt;
        switch (symbol->kind()) {
        case Symbol_Kind::Function_Set: return check_call_against_function_set(sema, function_call, symbol);
        default: todo();
        }
    }

    todo();
}

Worker_Status check_identifier(Semantic_Context &sema, Expression *&expr)
{
    auto ident = static_cast<Identifier*>(expr);
    if (auto opt = sema.eagerly_find_symbol(ident->value)) {
        auto symbol = *opt;
        switch (symbol->state) {
        case Symbol_State::Unresolved:
            {
                auto status = Worker_Status::pending(sema.allocator, symbol.get_id(), sema.current_scope);
                return status;
            } break;
        case Symbol_State::Waiting:
            {
                sema.compiler->report_error(ident->locus, "Identifier refers to symbol that is currently being resolved", CYCLIC_DEP);
                return Worker_Status::Failed;
            }
        case Symbol_State::Resolved:
            {
                sema.set_symbol_at_locus(ident->locus, symbol.get_id());
                // @TODO: assert that the symbol does indeed have a type.
                sema.set_type_at_locus(ident->locus, *symbol->type_id);
            } break;
        default: todo();
        }
        return Worker_Status::Done;
    }

    sema.compiler->report_error(
        ident->locus,
        String::build(sema.allocator, "Identifier, `%.*s` is not found in this scope", SVARG(ident->value.base())).data(),
        UNDEFINED_SYMBOL
    );
    return Worker_Status::Failed;
}

Worker_Status Semantic_Context::check_expression(Worker_Payload payload, bool is_resumption)
{
    auto &expr = payload.get_expr();
    switch (expr->kind) {
        case Expression_Kind::Integer:
            {
                // @TODO:
                //    when type context is introduced try to infer the specific type for the integer literal
                //    from the context instead of defaulting to int32.
                auto integer = static_cast<Integer*>(expr);
                (void)integer;
                set_type_at_locus(expr->locus, type_id::INT32);
                return Worker_Status::Done;
            } break;
        case Expression_Kind::Float:
            {
                // @TODO:
                //    when type context is introduced try to infer the specific type for the integer literal
                //    from the context instead of defaulting to float32.
                auto float_expr = static_cast<Float*>(expr);
                (void)float_expr;
                set_type_at_locus(expr->locus, type_id::FLOAT32);
                return Worker_Status::Done;
            } break;
        case Expression_Kind::Char:
            {
                auto char_expr = static_cast<Char*>(expr);
                (void)char_expr;
                set_type_at_locus(expr->locus, type_id::UINT8);
                return Worker_Status::Done;
            } break;
        case Expression_Kind::Boolean:
            {
                set_type_at_locus(expr->locus, type_id::BOOL);
                return Worker_Status::Done;
            } break;
        case Expression_Kind::Typehint:       return check_typehint(*this, expr);
        case Expression_Kind::Function_Call:  return check_function_call(*this, expr);
        case Expression_Kind::Identifier:     return check_identifier(*this, expr);
        default: MINI_TODO();
    }
}
