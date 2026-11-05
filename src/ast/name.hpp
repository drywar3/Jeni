#pragma once

#include "misc/misc.hpp"
#include "parser/locus.hpp"

namespace ast
{
    struct Name {
        Sv    value;
        Locus locus;
    };
} // namespace ast
