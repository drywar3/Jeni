#pragma once

#include "misc/misc.hpp"
#include "misc/densemap.hpp"
#include "sema/entities/id.hpp"
#include "sema/entities/symbols.hpp"
#include "bc/stack_slot.hpp"

struct Compiler_Context;
namespace bc {

    using Prototype = sema::Function_Prototype;


    struct Bytecode_Machine;
    struct Bytecode_Function;
    struct Bytecode_Instruction;

    struct Register {
        bool  free = true;
        int64 int_value;
        float32 float_value;
        char *pointer_value;
    };

    struct Global_Initializer {
        enum Kind {
            Integer,
            Float,
            Bool,
            Pointer,
        };
        Kind kind;
        int64 int_value;
        double float_value;
        bool bool_value;
        char *pointer_value;
    };

    struct Global {
        String name;
        sema::Type_Id type_id;
        Global_Initializer initializer;
    };

    enum Block {
        STACK = 0,
        DATA  = 1,
    };

    struct Pointer {
        Block block;
        uint32 value;
    };

    struct Bytecode_Machine {
        using Data_Section  = Arr<uint8>;
        using Function_Pool = DenseMap<sema::Symbol_Id, Bytecode_Function*>;

        Data_Section  data_section;
        Function_Pool functions;

        uint8 *memory[2];

        Alloc         allocator;

        Bytecode_Machine(Alloc a);
        void destroy();

        void add_function(sema::Symbol_Id, Bytecode_Function*);
        bool contains_function(sema::Symbol_Id);

        bool contains_sequence_of_bytes(const uint8 *bytes, usize count);
        uint32 find_or_insert_bytes_to_data_section(const uint8 *bytes, usize count);
        void append_to_data_section(const uint8 *byte, usize count);

        String emit_to_c_code(Compiler_Context &compiler);
    };

    enum Opcode {
        LOAD_PARAM,      // load_param T, <stack_slot>
        LOAD_CONSTANT,   // load_constant T, <name>
        LOAD_STACK,      // load_stack T, <stack_slot>, <stack_offset>

        // allocate memory on the stack
        // - [dst]  unused
        // - [arg1] stack_offset | 0 - if stack_slot is used
        // - [arg2] stack_slot   | 0 - if stack_offset is used
        // - [arg3] unused
        ALLOC_STACK,

        LOAD_DATA,

        STORE_REG,
        STORE_REG_IMM,
        // store a value into a stack slot/offset
        // - [dst]  unused
        // - [arg1] value to store
        // - [arg2] stack_offset | 0 - if stack_slot is used
        // - [arg3] stack_slot   | 0 - if stack_offset is used
        STORE_STACK,
        RETURN,
        PREP_CALL,
        LOAD_ARG,
        DO_CALL,
    };


    using Reg = int64_t;

    struct Bytecode_Instruction {
        Opcode opcode;
        Opt<sema::Type_Id> type_id;
        int64  dst = -1;
        Reg arg1;
        Reg arg2;
        Reg arg3;
        int64  imm;
        Sv     str;
    };

    enum {
        BCFN_IS_EXTERNALLY_DEFINED = 0x1,
    };

    struct Call_Frame {
        sema::Symbol_Id fn_id;
        Prototype       prototype;
    };

    struct Bytecode_Function {
        using Registers    = Arr<bool>;
        using Instructions = Arr<Bytecode_Instruction>;
        using Stack_Slots  = Arr<Stack_Slot>;

        String    name;
        sema::Symbol_Id id;
        Registers registers;
        Instructions instructions;
        Prototype prototype;
        Stack_Slots stack_slots;
        Alloc allocator;
        int flags = 0;

        Bytecode_Function(Alloc alloc, String name, sema::Symbol_Id id, Prototype proto);
        void destroy();

        void add_instruction(Bytecode_Instruction instruction);
        void emit(Opcode op, Opt<sema::Type_Id> type_id, Reg dst = 0, Reg a1 = 0, Reg a2 = 0, Reg a3 = 0, int64 imm = 0, Sv str = "");

        int64 get_register();
        void   free_register(int64 reg);

        uint32 alloc_stack_slot(sema::Type_Id type_id);
    };
}
