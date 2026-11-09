#include "compiler.hpp"
#include "sema/sema.hpp"
#include "parser/parser.hpp"
#include "bc/bytecode_compiler.hpp"

#include <mini.c/fs.h>

Target Target::x86_64_linux()
{
    return {
        X86_64,
        LINUX,
        8,
        "x86_64-unknown-linux"
    };
}

Compiler_Context::Compiler_Context(Alloc a, Target target)
    : target(target), allocator(a), sources(a), programs(a),
      diagnostics(a, 10, 10), store(a, target),
      machine(a, this), bc(this)
{
}

const Source_Manager& Compiler_Context::get_sources() const
{
    return sources;
}

Source_Manager& Compiler_Context::get_sources()
{
    return sources;
}

const ast::Program *Compiler_Context::compile_source_into_program(String path)
{
    MINI_ASSERT(mini_fs_exists(path.data()), "path `%s` does not exist", path.data());
    Source_Id source_id = get_sources().open_source_file(path, allocator);

    if (programs.contains(source_id)) return programs.find(source_id);

    Sv content          = get_sources().get_content_of(source_id);

    using namespace parser;

    auto parser  = Parser(source_id, content, this);
    auto program = ast::Program(allocator, source_id);

    while (!parser.is_done()) {
        if (auto *stmt = parser.parse_statement())
            program.ast.append(stmt);
        else continue;
    }

    if (diagnostics.has_errors()) {
        return nullptr;
    }

    programs.insert(source_id, program);
    return programs.find(source_id);
}

bool Compiler_Context::analyze_program(const ast::Program &program)
{
    auto semantic_id = store.semas.current_id();
    auto &sema = store.semas.allocate_and_get(sema::Semantic_Context(semantic_id, program.source_id, this));
    return sema.analyze();
}

bool Compiler_Context::compile(MOVE String path)
{
    if (auto program = compile_source_into_program(path)) {
        if (!analyze_program(*program)) return false;
        bc.compile_program(*program);
        return true;
    }
    return false;
}

void Compiler_Context::report_error(Locus locus, const char *message, Diag_Code code)
{
    diagnostics.report(Diagnostic(allocator, Severity::Error, locus, message).with_code(code));
}
