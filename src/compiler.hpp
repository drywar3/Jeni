#pragma once

#include "sema/store.hpp"
#include "source.hpp"
#include "ast/program.hpp"
#include "diagnostics.hpp"
#include "compiler/target.hpp"
#include "bc/bytecode.hpp"
#include "bc/bytecode_compiler.hpp"

using Program_Map = HashMap<Source_Id, ast::Program>;

enum Architecture {
    X86_64,
    AARCH64,
};

enum Os {
    MACOS,
    LINUX,
    WINDOWS,
};

struct Target {
    Architecture arch;
    Os os;
    usize pointer_size;
    Sv triple;

    static Target x86_64_linux();
};

struct Compiler_Context {
    Target          target;
    Alloc           allocator;
    Source_Manager  sources;
    Program_Map     programs;
    Diagnostic_Pool diagnostics;
    sema::Semantic_Storage store;
    bc::Bytecode_Machine machine;
    Bytecode_Compiler bc;

    Compiler_Context(Alloc a, Target target);

    auto get_allocator() const { return allocator; }
    const Target &get_target() const { return target; }

    const Source_Manager& get_sources() const;
    Source_Manager& get_sources();

    const ast::Program *compile_source_into_program(MOVE String path);
    bool analyze_program(const ast::Program &program);

    bool compile(MOVE String path);

    void report_error(Locus locus, const char *message, Diag_Code code);
};
