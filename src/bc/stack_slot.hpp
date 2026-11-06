#pragma once

namespace bc
{
    // Identifies a variable on the stack.
    struct Stack_Slot {
        sema::Type_Id type_id;
        // used by the c-code generator.
        usize         index;
        // used by the bytecode_machine duing evaluation.
        usize         offset;
    };

} // namespace bc
