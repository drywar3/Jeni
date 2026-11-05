#include "sema/sema.hpp"
#include "ast/statement.hpp"
#include "ast/statements.hpp"
#include "mini.c/mini_def.h"
#include "misc/misc.hpp"
#include "sema/entities/id.hpp"
#include "sema/symbol_proxy.hpp"
#include "sema/entities/scope.hpp"
#include "sema/entities/symbols.hpp"

using namespace sema;
using namespace ast;
using namespace ast::stmt;

Symbol *sema::symbol_proxy_fn(Semantic_Context *sema, Symbol_Id id)
{
   return *sema->store->symbols.at_index_ptr(usize(id));
}

Semantic_Context::Semantic_Context(Semantic_Id id, Source_Id source_id, Compiler_Context *compiler)
    : id(id), source_id(source_id),
      compiler(compiler), store(&compiler->store),
      allocator(compiler->allocator),
      workers(compiler->allocator)
{
    // assign the global scope of this analyzer.
    global_scope = Scope_Id(store->scopes.add_value(
        Scope(
            compiler->allocator,
            Scope::Global,
            None
        )
    ));

    // we always begin at the global_scope.
    current_scope = global_scope;
}

bool sema::Semantic_Context::analyze()
{
    auto &program = *compiler->programs.find(source_id);

    for (ast::Statement *stmt : program.ast.iter()) {
        if (!discover_statement(stmt)) return false;
    }

    for (ast::Statement *stmt : program.ast.iter()) {
        auto *node = static_cast<Node*>(stmt);
        node->sema_id = id;
        Worker_Status status = check_statement(Worker_Payload(stmt), false);
        if (!status) {
            if (status == Worker_Status::Failed) {
                if (compiler->diagnostics.has_reached_max_errors()) return false;
                else continue;
            } else todo();
        }
    }
    return !compiler->diagnostics.has_errors();
}

void Semantic_Context::add_worker(Symbol_Id symbol_id, Worker worker)
{
    if (auto worker_list = workers.find(symbol_id)) {
        worker_list->append(worker);
        return;
    }

    workers[symbol_id] = Arr<Worker>(compiler->allocator);
    workers[symbol_id].append(worker);
}

bool Semantic_Context::discover_statement(ast::Statement *stmt)
{
    switch (stmt->kind) {
        case ast::Statement_Kind::Variable: return discover_variable_assignment(stmt);
        case ast::Statement_Kind::Function: return discover_function_decl(stmt);
        default: MINI_TODO();
    }
}

bool Semantic_Context::discover_variable_assignment(ast::Statement *stmt)
{
    auto variable = static_cast<ast::stmt::Variable_Declaration*>(stmt);
    auto var_name = variable->name.value;
    auto var_loc  = variable->name.locus;

    if (auto opt = find_symbol_in(var_name, current_scope)) {
        auto symbol = *opt;
        if (symbol->locus != var_loc) {
            compiler->report_error(var_loc, "Redifinition of symbol", SYMBOL_REDECL);
            return false;
        }
        return true;
    }

    Symbol_Ptr symbol = allocate<Variable>(
        compiler->allocator,
        var_name, var_loc,
        current_scope,
        variable->mutability,
        None, // we cannot register the type of this symbol yet.
        false
    );

    register_symbol(current_scope, symbol);
    return true;
}

bool Semantic_Context::discover_function_decl(ast::Statement *stmt)
{
    auto function  = static_cast<Function_Declaration*>(stmt);
    auto func_name = function->name.value;
    auto func_loc  = function->name.locus;

    if (auto opt = find_symbol(func_name)) {
        auto symbol = *opt;
        if (symbol->locus == func_loc) return true;

        if (symbol->kind() != Symbol_Kind::Function_Set) {
            compiler->report_error(
                func_loc,
                String::build(compiler->allocator,
                              "Redefinition of `%.*s` as symbol of another kind",
                              SVARG(func_name.base())).data(),
                SYMBOL_KIND_MISMATCH
            );
            return false;
        }

        // the symbol already exists. at this point, we cannot fill any specific function in the set.
        // so just return true.
        return true;
    }

    Symbol_Ptr symbol = allocate<Function_Set>(
        compiler->allocator,
        func_name, func_loc,
        current_scope,
        compiler->allocator);
    auto symbol_id = register_symbol(current_scope, symbol);
    Type_Id type_id = register_or_insert_type(
        allocate<Function_Set_Type>(
            allocator,
            symbol_id));
    symbol->type_id = type_id;
    return true;
}
