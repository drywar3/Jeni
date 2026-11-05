#pragma once

#include <mini.cc/string.h>

namespace sema
{
    struct Profile {
        struct {
            // represents the name of the symbol as it will be used in the final binary.
            String canon;
            // represents the name of the symbol that will be used in compiler diagnostics.
            String repr;
            // represents the actual name of the symbol.
            String norm;
        } name;
    };
} // namespace sema
