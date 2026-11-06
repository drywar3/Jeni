#pragma once

#include "sema/store.hpp"
#include "ast/program.hpp"
#include "ast/statements.hpp"
#include "sema/symbol_proxy.hpp"
#include "bc/bytecode.hpp"

struct Compiler_Context;
struct Bytecode_Compiler {
    Compiler_Context *compiler;
    sema::Semantic_Storage &store;
    Alloc allocator;

    bc::Bytecode_Function *current_function;

    Bytecode_Compiler(Compiler_Context *compiler);

    void compile_program(const ast::Program &program);

    void compile_statement(ast::Statement *stmt);
    uint32 compile_expression(ast::Expression *expr, bool disregard_result = false);

    void compile_function_decl_to_bytecode(sema::Symbol_Proxy symbol, ast::stmt::Function_Declaration *decl_fn, bc::Prototype prototype);
    void compile_block_to_bytecode(ast::stmt::Block *block);
};
