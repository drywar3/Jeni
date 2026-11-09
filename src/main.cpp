#include <cstdio>

#include <mini.c/allocator.h>
#include <mini.c/bulk_allocator.h>
#include <mini.c/default_allocator.h>

#include <mini.cc/scope_guard.h>

#include "source.hpp"
#include "compiler.hpp"

int main (int argc, char **argv) {
    if (argc < 2) {
        printf("ERROR: no input was provided\n");
        return 0;
    }

    auto bka       = mini_bka_create(mini_default_allocator());
    auto allocator = mini_bka_allocator(&bka);

    auto input_path = mini_string_build(allocator, "%s", argv[1]);

    MINI_DEFER {
        mini_bka_destroy(&bka);
    };

    auto target = Target::x86_64_linux();
    auto compiler = Compiler_Context{allocator, target};
    if (!compiler.compile(String(input_path))) {
        if (!compiler.diagnostics.is_empty()) {
            for (auto diag : compiler.diagnostics.buffer.iter()) {
                diag.report(&compiler.get_sources(), stderr);
            }
        }
        return 1;
    }

    String out = compiler.machine.emit_to_c_code(compiler);
    auto out_path = String::build(allocator, "%s.cpp", input_path);
    FILE *file = fopen(out_path.data(), "wb");
    if (!file) {
        fprintf(stderr, "ERROR: failed to open `%s` for writing", out_path.data());
        return 1;
    }

    fprintf(file, "%s", out.data());
    fclose(file);

    printf("INFO: wrote generated c to `%s`\n", out_path.data());

    auto command = String::build(allocator, "cc -xc++ -w %s -o a.out", out_path.data());
    printf("RUN: %s\n", command.data());
    if (system(command.data())) {

        return 1;
    }

    return 0;
}
