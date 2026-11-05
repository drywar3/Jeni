#pragma once

#include "misc/misc.hpp"
#include "misc/densemap.hpp"
#include "sema/entities/id.hpp"
#include "sema/entities/symbols.hpp"

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

        String emit_to_c_code(Compiler_Context &compiler);
    };

    enum Opcode {
        LOAD_PARAM,
        LOAD_CONSTANT,
        STORE_REG,
        STORE_REG_IMM,
        RETURN,
        PREP_CALL,
        LOAD_ARG,
        DO_CALL,
    };

    struct Bytecode_Instruction {
        Opcode opcode;
        Opt<sema::Type_Id> type_id;
        uint32 dst;
        uint32 arg1;
        uint32 arg2;
        int64  imm;
        Sv     str;
    };

    enum {
        BCFN_IS_EXTERNALLY_DEFINED = 0x1,
    };

    struct Stack_Slot {
        sema::Type_Id type_id;
        usize         index;
        usize         offset;
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
        void emit(Opcode op, Opt<sema::Type_Id> type_id, uint32 dst = 0, uint32 a1 = 0, uint32 a2 = 0, int64 imm = 0, Sv str = "");

        uint32 get_register();
        uint32 alloc_stack_slot(sema::Type_Id type_id);
    };


}
