#include "sema/sema.hpp"
#include "sema/type/id.hpp"
#include "ast/statements.hpp"
#include "sema/type/matcher.hpp"
#include "sema/entities/symbols.hpp"

using namespace sema;
using namespace ast::stmt;

// Must only be called after [expr] has been resolved successfully.
bool expression_is_global_compat(Semantic_Context &sema, const ast::Expression *expr)
{
    switch (expr->kind) {
    case ast::Expression_Kind::Integer:
    case ast::Expression_Kind::Float:
    case ast::Expression_Kind::Char:
        return true;
    case ast::Expression_Kind::Identifier:
        if (auto opt = sema.get_symbol_at_locus(expr->locus)) {
            auto symbol = *opt;
            if (symbol->kind() == Symbol_Kind::Variable) {
                auto variable = static_cast<Variable*>(*symbol);
                return variable->mutability == Mutability::Constant;
            }
        }
        return false;
    default: todo();
    }
}


Worker_Status Semantic_Context::check_statement (Worker_Payload payload, bool is_resumption)
{
    auto &stmt = payload.get_stmt();
    switch (stmt->kind) {
        case ast::Statement_Kind::Variable:      return check_variable_declaration(stmt, is_resumption);
        case ast::Statement_Kind::Function:      return check_function_declaration(stmt, is_resumption);
        case ast::Statement_Kind::Block:         return check_block_statement(stmt, is_resumption);
        case ast::Statement_Kind::If_Statement:  return check_if_statement(stmt, is_resumption);
        case ast::Statement_Kind::Return:        return check_return_statement(stmt, is_resumption);
        case ast::Statement_Kind::Expression:
            {
                auto *expr = (ast::Expression**)(&stmt);
                return check_expression(Worker_Payload(*expr), is_resumption);
            } break;
        default: MINI_TODO();
    }
}

Worker_Status Semantic_Context::check_variable_declaration(ast::Statement *stmt, bool is_resumption)
{
    auto *variable = static_cast<Variable_Declaration*>(stmt);
    auto var_name = variable->name.value;
    auto var_loc  = variable->name.locus;

    Scope_Id current_scope = this->current_scope;

    if (!is_resumption) {
        auto save = this->current_scope;
        this->current_scope = current_scope;

        defer { this->current_scope = save; };

        if (!discover_variable_assignment(stmt)) return Worker_Status::Failed;
    }

    auto opt = find_symbol(var_name);
    if (!opt) MINI_UNREACHABLE("Compiler Bug: cannot get symbol pointer");

    auto symbol       = *opt;
    auto var_sym      = static_cast<Variable*>(*symbol);
    auto &initializer = variable->initializer;
    auto &typehint    = variable->typehint;
    symbol->state = Symbol_State::Waiting;

    // @Todo : variable may be uninitialized. i do not check that it is here!
    if (variable->type_is_defined) {
        auto status = check_expression(Worker_Payload(typehint), false);
        if (!status) return status;
        status = check_expression(Worker_Payload(initializer), is_resumption);
        if (!status) return status;

        Type_Id expected_type = get_type_at_locus(typehint->locus);
        Type_Id received_type = get_type_at_locus(initializer->locus);

        if (auto result = Type_Matcher(this, expected_type, received_type).match();
            !result.is_ok(true))
        {
            result.report(*this, expected_type, received_type, initializer->locus);
            return Worker_Status::Failed;
        }

        var_sym->type_id = expected_type;
        symbol->type_id  = expected_type;
        set_type_at_locus(var_loc, expected_type);
    } else {
        auto status = check_expression(Worker_Payload(initializer), is_resumption);
        if (!status) return status;
        auto inferred_type = get_type_at_locus(initializer->locus);
        var_sym->type_id   = inferred_type;
        symbol->type_id    = inferred_type;
        set_type_at_locus(var_loc, inferred_type);
    }

    if (current_scope == global_scope) {
        if (!expression_is_global_compat(*this, initializer)) {
            compiler->report_error(initializer->locus, "Cannot use expression in global context", NON_CONST_GLOB_EXPR);
            return Worker_Status::Failed;
        }
    }

    symbol->state = Symbol_State::Resolved;
    return Worker_Status::Done;
}

Worker_Status check_function_prototype(
    Semantic_Context &sema,
    ast::stmt::Function_Prototype &prototype,
    Symbol_Proxy symbol,
    sema::Function_Prototype &out
) {
    auto &parameters = prototype.parameters;

    // check that all parameters can be resolved completely before building the function prototype.
    for (auto &parameter : parameters.iter()) {
        if (!parameter.typehint and parameter.default_value) todo("Evaluate and then infer type from default value");
        if (parameter.typehint and parameter.default_value)  todo("Ensure typehint and default value type matches");
        Worker_Status status = sema.check_expression(parameter.typehint, false);
        if (!status) return status;
    }

    if (prototype.return_type) {
        Worker_Status status = sema.check_expression(prototype.return_type, false);
        if (!status) return status;
    }

    for (auto &parameter : parameters.iter()) {
        Type_Id type_id = sema.get_type_at_locus(parameter.typehint->locus);
        out.parameter_types.append(type_id);
        sema.set_type_at_locus(parameter.name.locus, type_id);
        Symbol_Ptr symbol = allocate<Variable>(
            sema.compiler->allocator,
            parameter.name.value, parameter.name.locus,
            sema.current_scope,
            Mutability::Constant,
            type_id,
            true);
        symbol->state = Symbol_State::Resolved;
        sema.register_symbol(sema.current_scope, symbol);
    }

    Type_Id rtype_id = type_id::VOID;
    if (prototype.return_type) {
        rtype_id = sema.get_type_at_locus(prototype.return_type->locus);
    }

    out.return_type = rtype_id;
    return Worker_Status::Done;
}

Worker_Status Semantic_Context::check_function_declaration(ast::Statement *stmt, bool is_resumption)
{
    auto function = static_cast<Function_Declaration*>(stmt);

    if (!is_resumption) {
        auto save = this->current_scope;
        this->current_scope = current_scope;

        defer { this->current_scope = save; };

        if (!discover_statement(stmt)) return Worker_Status::Failed;
    }

    auto opt    = find_symbol(function->name.value);
    assert(opt.has_value(), "Compiler Bug: missing symbol");
    auto symbol = *opt;
    assert(symbol->kind() == Symbol_Kind::Function_Set, "Compiler Bug: symbol referring to function is not a function set");

    auto func_set = static_cast<Function_Set*>(*symbol);

    auto &ast_prototype = function->prototype;

    enter_scope(Scope::Function);
    defer { exit_scope(); };


    enter_scope(Scope::Parameter); // Enter a separate scope for the function parameters.
    defer { exit_scope(); };

    sema::Function_Prototype prototype{allocator};
    if (auto s = check_function_prototype(*this, ast_prototype, symbol, prototype); !s) return s;

    if (func_set->map.contains(prototype)) {
        auto prev = func_set->map[prototype];

        auto message = String::build(allocator, "Another function with the same signature already exists");

        if (prev.return_type != prototype.return_type) {
            message.append_fmt(". (a function cannot be overloaded with just differing return types)");
        }

        compiler->report_error(
            function->name.locus, message.data(), SYMBOL_REDECL);
        return Worker_Status::Failed;
    }

    find_last_scope_of_kind(Scope::Function, [prototype](auto *sema, auto *scope) {
        scope->func_scope.return_type = prototype.return_type;
        return 0;
    });

    func_set->map.insert(prototype, Function {
        .locus = function->name.locus,
        .return_type = prototype.return_type,
    });

    symbol->state = Symbol_State::Resolved;

    if (function->body) {
        Worker_Status body_status = check_statement(function->body, false);
        if (!body_status) {
            if (body_status == Worker_Status::Pending) todo();
            else return body_status;
        }
    }
    set_symbol_at_locus(function->name.locus, symbol.symbol_id);
    store->prots[stmt->locus] = prototype;

    // The function is fully resolved at this point.
    // It should be safe to generate the bytecode for it now.
    compiler->bc.compile_function_decl_to_bytecode(symbol, function, prototype);
    return Worker_Status::Done;
}

Worker_Status Semantic_Context::check_block_statement(ast::Statement *stmt, bool is_resumption)
{
    auto block = static_cast<Block*>(stmt);
    usize start = 0;

    if (store->bsp.contains(stmt->locus)) {
        start = store->bsp[stmt->locus];
    }

    if (!is_resumption) enter_scope();
    defer {
        if (!is_resumption) exit_scope();
    };

    for (usize n = start; n < block->body.count(); n++) {
        store->bsp[stmt->locus] = n;
        auto &stmt = block->body[n];
        bool stmt_is_resumption = is_resumption and (n == start);
        Worker_Status status = check_statement(stmt, stmt_is_resumption);
        if (!status) {
            if (status == Worker_Status::Pending) todo();
            else return status;
        }
    }

    return Worker_Status::Done;
}

Worker_Status check_if_header(Semantic_Context &sema, ast::If_Header &header)
{
    auto &condition = header.condition;
    auto &body      = header.body;
    auto status = sema.check_expression(condition, false);
    if (!status) return status;

    // the condition of an if-header must always be of a boolean type.
    Type_Id condition_type = sema.get_type_at_locus(condition->locus);
    if (auto result = Type_Matcher(&sema, type_id::BOOL, condition_type).match();
        !result.is_ok(true))
    {
        sema.compiler->report_error(
            condition->locus,
            "Condition of an if-header must be a boolean",
            TYPE_MISMATCH
        );
        return Worker_Status::Done;
    }


    status = sema.check_statement(body, false);
    if (!status) return status;
    return Worker_Status::Done;
}

Worker_Status Semantic_Context::check_if_statement(ast::Statement *stmt, bool is_resumption)
{
    auto if_stmt = static_cast<If_Statement*>(stmt);
    ast::If_Header then_branch = if_stmt->then_branch;

    auto status = check_if_header(*this, then_branch);
    if (!status) return status;

    // @Todo : I probably want to keep checking all branches even after one fails.
    // instead of bailing on the first failed one.
    for (ast::If_Header &branch : if_stmt->branches.iter()) {
        auto branch_status = check_if_header(*this, branch);
        if (!branch_status) return branch_status;
    }

    if (auto &else_branch = if_stmt->else_branch) {
        status = check_statement(else_branch, false);
        if (!status) return status;
    }
    return Worker_Status::Done;
}

Worker_Status Semantic_Context::check_return_statement(ast::Statement *stmt, bool is_resumption)
{
    auto return_stmt = static_cast<Return*>(stmt);
    auto result = find_last_scope_of_kind(
        Scope::Kind::Function,
        [](Semantic_Context *sema, Scope *scope) -> Scope* {
            return scope;
        }
    );

    Type_Id expected_type = type_id::VOID;
    Type_Id recieved_type = type_id::VOID;

    if (result) {
        auto scope = *result;
        expected_type = scope->func_scope.return_type;
    }

    if (auto &value = return_stmt->value) {
        auto status = check_expression(value, is_resumption);
        if (!status) return status;
        recieved_type = get_type_at_locus(value->locus);
    }

    if (auto result = Type_Matcher(this, expected_type, recieved_type).match();
        !result.is_ok(true))
    {
        result.report(*this, expected_type, recieved_type, stmt->locus);
        return Worker_Status::Failed;
    }

    return Worker_Status::Done;
}
