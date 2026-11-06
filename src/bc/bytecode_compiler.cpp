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

int64 compile_function_call_to_bytecode(Bytecode_Compiler &bc, Expression *expr, bool disregard_result);
uint32 compile_identifier_to_bytecode(Bytecode_Compiler &bc, Expression *expr);

Global_Initializer compile_global_constant_expression(Bytecode_Compiler &bc, Expression *expr);

void Bytecode_Compiler::compile_statement(ast::Statement *stmt)
{
    switch (stmt->kind) {
    case Statement_Kind::Variable: compile_variable_declaration(*this, stmt); break;
    case Statement_Kind::Function: compile_function_declaration(*this, stmt); break;
    case Statement_Kind::Return:   compile_return_statement(*this, stmt); break;
    case Statement_Kind::Expression:   compile_expression(static_cast<Expression*>(stmt), true); break;
    default: todo();
    }
}

uint32 Bytecode_Compiler::compile_expression(ast::Expression *expr, bool disregard_result)
{
    assert(current_function != nullptr, "cannot compile expression without a function context");
    auto type_id = sema::Type_Id(*store.types.get_id(expr->locus));

    switch (expr->kind) {
    case Expression_Kind::Integer:
        {
            uint32 reg   = -1;
            if (!disregard_result) reg = current_function->get_register();
            auto integer = static_cast<expr::Integer*>(expr);
            current_function->emit(STORE_REG_IMM, type_id, reg, 0, 0, 0, integer->value);
            return reg;
        }
    case Expression_Kind::CString:
        {
            uint32 reg   = -1;
            if (!disregard_result) reg = current_function->get_register();
            auto cstring = static_cast<expr::CString*>(expr);
            const uint8 *bytes = reinterpret_cast<const uint8*>(cstring->value.data());
            auto count = cstring->value.length();
            uint32 offset = compiler->machine.find_or_insert_bytes_to_data_section(bytes, count);
            // @Todo : this will append many unnecessary bytes to the data section.
            compiler->machine.append_to_data_section((uint8[]){0}, 1);
            current_function->emit(LOAD_DATA, type_id, reg, offset);
            return reg;
        }
    case Expression_Kind::Identifier:
        {
            // @Todo : might need to tag a symbol with some bytecode information. like local slot.
            return compile_identifier_to_bytecode(*this, expr);
        }
    case Expression_Kind::Char:
        {
            uint32 reg = -1;
            if (!disregard_result) reg = current_function->get_register();
            auto char_expr = static_cast<expr::Char*>(expr);
            current_function->emit(STORE_REG_IMM, type_id, reg, 0, 0, 0, (uint8)char_expr->value);
            return reg;
        }
    case Expression_Kind::Function_Call: return compile_function_call_to_bytecode(*this, expr, disregard_result);
    default: todo("%d", expr->kind);
    }
}

void compile_variable_declaration(Bytecode_Compiler &bc, Statement *stmt)
{
    auto &sema = bc.store.semas.get(stmt->sema_id);
    auto *variable = static_cast<stmt::Variable_Declaration*>(stmt);
    auto &symbol    = *bc.store.symbols.find(variable->name.locus);

    assert(symbol->type_id, "Compiler Bug: symbol type not resolved at this point");
    Type_Id type_id = *symbol->type_id;

    if (symbol->scope_id == sema.global_scope) {
        // This symbol was defined in the global scope.
        //compile_global_constant_expression(bc, variable->initializer);
        todo();
    } else {
        uint32 stack_index = bc.current_function->alloc_stack_slot(type_id);
        symbol->stack_slot.index = stack_index;

        assert(variable->initializer,);
        auto result = bc.compile_expression(variable->initializer);
        bc.current_function->emit(ALLOC_STACK, type_id, 0, 0, stack_index);
        bc.current_function->emit(STORE_STACK, type_id, 0, result, 0, stack_index);
        bc.current_function->free_register(result);
        return;
    }

    todo();
}

void compile_function_declaration(Bytecode_Compiler &bc, Statement *stmt)
{
    auto &sema     = bc.store.semas.get(stmt->sema_id);
    auto *function = static_cast<stmt::Function_Declaration*>(stmt);
    auto symbol_id = Symbol_Id(*bc.store.symbols.get_id(function->name.locus));
    if (bc.compiler->machine.contains_function(symbol_id)) return;
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
            auto symbol        = *store.symbols.find(param.name.locus);
            symbol->stack_slot.index = stack_index;
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
    bc.current_function->free_register(reg);
}

int64 compile_function_call_to_bytecode(Bytecode_Compiler &bc, Expression *expr, bool disregard_result)
{
    auto function_call = static_cast<expr::Function_Call*>(expr);
    auto &sema = bc.store.semas.get(expr->sema_id);

    int64 result = -1;

    if (!disregard_result) result = bc.current_function->get_register();

    auto &fn = *bc.current_function;
    int64 callee = bc.compile_expression(function_call->callee);
    Type_Id callee_type_id = sema.get_type_at_locus(function_call->callee->locus);
    Type_Id type_id = sema.get_type_at_locus(expr->locus);

    Arr<std::pair<uint32, Type_Id>> args{bc.allocator};

    for (auto arg : function_call->args.iter()) {
        int64 reg = bc.compile_expression(arg.value);
        auto type_id = sema.get_type_at_locus(arg.value->locus);
        args.append({reg, type_id});
    }

    fn.emit(PREP_CALL, type_id, result, callee);
    for (auto [arg, type_id] : args.iter()) {
        fn.emit(LOAD_ARG, type_id, 0, arg);
    }

    fn.emit(DO_CALL, callee_type_id);
    fn.free_register(callee);

    for (auto [arg, _] : args.iter()) {
        fn.free_register(arg);
    }

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
            bc.current_function->emit(LOAD_CONSTANT, symbol->type_id, reg, 0, 0, 0, 0, symbol->name);
        } else {
            auto &stack_slot = symbol->stack_slot;
            bc.current_function->emit(LOAD_STACK, symbol->type_id, reg, stack_slot.index, stack_slot.offset, 0);
        }
    } else MINI_UNREACHABLE();
    return reg;
}
