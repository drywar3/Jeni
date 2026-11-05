#include "ast/statements.hpp"
#include "bc/bytecode_compiler.hpp"
#include "sema/sema.hpp"
#include "sema/type/id.hpp"

using namespace bc;
using namespace ast;
using namespace sema;

Bytecode_Compiler::Bytecode_Compiler(Compiler_Context *compiler)
    : compiler(compiler), store(compiler->store), allocator(compiler->allocator)
{
}

void Bytecode_Compiler::compile_program(const ast::Program &program)
{
    for (const auto &stmt : program.ast.iter()) {
        compile_statement(stmt);
    }
}

void compile_variable_declaration(Bytecode_Compiler &bc, Statement *stmt);
void compile_function_declaration(Bytecode_Compiler &bc, Statement *stmt);
void compile_return_statement(Bytecode_Compiler &bc, Statement *stmt);

uint32 compile_function_call_to_bytecode(Bytecode_Compiler &bc, Expression *expr);
uint32 compile_identifier_to_bytecode(Bytecode_Compiler &bc, Expression *expr);

Global_Initializer compile_global_constant_expression(Bytecode_Compiler &bc, Expression *expr);

void Bytecode_Compiler::compile_statement(ast::Statement *stmt)
{
    switch (stmt->kind) {
    case Statement_Kind::Variable: compile_variable_declaration(*this, stmt); break;
    case Statement_Kind::Function: compile_function_declaration(*this, stmt); break;
    case Statement_Kind::Return:   compile_return_statement(*this, stmt); break;
    default: todo();
    }
}

uint32 Bytecode_Compiler::compile_expression(ast::Expression *expr)
{
    assert(current_function != nullptr, "cannot compile expression without a function context");
    auto type_id = sema::Type_Id(*store.types.get_id(expr->locus));
    switch (expr->kind) {
    case Expression_Kind::Integer:
        {
            auto integer = static_cast<expr::Integer*>(expr);
            uint32 reg   = current_function->get_register();
            current_function->emit(STORE_REG_IMM, type_id, reg, 0, 0, integer->value);
            return reg;
        }
    case Expression_Kind::Identifier:
        {
            // @Todo : might need to tag a symbol with some bytecode information. like local slot.
            return compile_identifier_to_bytecode(*this, expr);
        }
    case Expression_Kind::Function_Call: return compile_function_call_to_bytecode(*this, expr);
    default: todo("%d", expr->kind);
    }
}

void compile_variable_declaration(Bytecode_Compiler &bc, Statement *stmt)
{
    auto &sema = bc.store.semas.get(stmt->sema_id);
    auto *variable = static_cast<stmt::Variable_Declaration*>(stmt);
    auto &symbol    = *bc.store.symbols.find(variable->name.locus);

    assert(symbol->type_id, "Compiler Bug: symbol type not resolved at this point");
    if (symbol->scope_id == sema.global_scope) {
        // This symbol was defined in the global scope.
        compile_global_constant_expression(bc, variable->initializer);
    } else {
        todo();
    }

    todo();
}

void compile_function_declaration(Bytecode_Compiler &bc, Statement *stmt)
{
    auto &sema     = bc.store.semas.get(stmt->sema_id);
    auto *function = static_cast<stmt::Function_Declaration*>(stmt);
    auto symbol_id = Symbol_Id(*bc.store.symbols.get_id(function->name.locus));
    if (!bc.compiler->machine.contains_function(symbol_id)) return;
    bc.compile_function_decl_to_bytecode(Symbol_Proxy(&sema, symbol_id), function, bc.store.prots[stmt->locus]);
}

void Bytecode_Compiler::compile_function_decl_to_bytecode(
    sema::Symbol_Proxy symbol,
    ast::stmt::Function_Declaration *decl_fn,
    Prototype prototype)
{
    Bytecode_Function *fn = allocate<Bytecode_Function>(
        allocator,
        allocator,
        String(decl_fn->name.value.to_string(allocator)),
        symbol.get_id(),
        prototype);
    compiler->machine.add_function(symbol.get_id(), fn);

    fn->flags |= BCFN_IS_EXTERNALLY_DEFINED;

    auto save = current_function;
    current_function = fn;
    defer { current_function = save; };

    if (decl_fn->body) {
        usize n = 0;
        for (const auto &param : decl_fn->prototype.parameters.iter()) {
            auto type_id       = Type_Id(*store.types.get_id(param.name.locus));
            uint32 stack_index = current_function->alloc_stack_slot(type_id);
            current_function->emit(LOAD_PARAM, type_id, n, stack_index);
            n += 1;
        }
        compile_block_to_bytecode(static_cast<stmt::Block*>(decl_fn->body));
        fn->flags &= ~BCFN_IS_EXTERNALLY_DEFINED;
    }
}

void Bytecode_Compiler::compile_block_to_bytecode(stmt::Block *block)
{
    for (auto *stmt : block->body.iter()) {
        compile_statement(stmt);
    }
}

Global_Initializer compile_global_constant_expression(Bytecode_Compiler &bc, Expression *expr)
{
    todo();
}

void compile_return_statement(Bytecode_Compiler &bc, Statement *stmt)
{
    auto return_stmt = static_cast<stmt::Return*>(stmt);
    auto type_id = type_id::VOID;
    uint32 reg = 0;
    if (return_stmt->value) {
        reg = bc.compile_expression(return_stmt->value);
        type_id = Type_Id(*bc.store.types.get_id(return_stmt->value->locus));
    }
    bc.current_function->emit(RETURN, type_id, 0, reg);
}

uint32 compile_function_call_to_bytecode(Bytecode_Compiler &bc, Expression *expr)
{
    auto function_call = static_cast<expr::Function_Call*>(expr);
    auto &sema = bc.store.semas.get(expr->sema_id);

    uint32 result = bc.current_function->get_register();

    auto &fn = *bc.current_function;
    uint32 callee = bc.compile_expression(function_call->callee);
    Type_Id callee_type_id = sema.get_type_at_locus(function_call->callee->locus);
    Type_Id type_id = sema.get_type_at_locus(expr->locus);

    Arr<std::pair<uint32, Type_Id>> args{bc.allocator};

    for (auto arg : function_call->args.iter()) {
        uint32 reg = bc.compile_expression(arg.value);
        auto type_id = sema.get_type_at_locus(arg.value->locus);
        args.append({reg, type_id});
    }

    fn.emit(PREP_CALL, type_id, result, callee);
    for (auto [arg, type_id] : args.iter()) {
        fn.emit(LOAD_ARG, type_id, 0, arg);
    }
    fn.emit(DO_CALL, callee_type_id);
    return result;
}

uint32 compile_identifier_to_bytecode(Bytecode_Compiler &bc, Expression *expr)
{
    auto ident = static_cast<expr::Identifier*>(expr);
    auto &sema = bc.store.semas.get(expr->sema_id);

    uint32 reg = bc.current_function->get_register();

    if (auto opt = sema.get_symbol_at_locus(ident->locus)) {
        auto symbol = *opt;
        if (sema.global_scope == symbol->scope_id) {
            assert(symbol->type_id, "");
            bc.current_function->emit(LOAD_CONSTANT, symbol->type_id, reg, 0, 0, 0, symbol->name);
        } else {
            todo();
        }
    }

    return reg;
}
