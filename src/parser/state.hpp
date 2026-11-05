#pragma once

#include "parser/token.hpp"

namespace parser
{
    struct State {
        enum Kind {
            Global,
            If_Condition,
            Typehint_Expression,
            Namespace_Definition,
            Generic_Typehint,
            If_Header_Condition,
        };

        Kind  kind;
        Token begin;
        bool  has_error;

        State(Kind kind) : kind(kind), has_error(false) {}
    };

    using State_Stack = Arr<State>;
} // namespace  parser
