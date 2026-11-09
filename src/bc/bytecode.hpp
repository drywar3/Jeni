#pragma once

#include "misc/misc.hpp"
#include "misc/densemap.hpp"
#include "sema/entities/id.hpp"
#include "sema/entities/symbols.hpp"
#include "bc/stack_slot.hpp"

struct Compiler_Context;
struct Bytecode_Compiler;
namespace sema { struct Type_Layout; }

namespace bc {

    using Prototype = sema::Function_Prototype;
    using Reg = int64_t;

    struct Bytecode_Machine;
    struct Bytecode_Function;
    struct Bytecode_Instruction;

    struct Register {
        bool  free = true;
        int64 int_value;
        float32 float_value;
        char *pointer_value;
    };

    enum Block {
        STACK = 0,
        DATA  = 1,
    };

    struct Pointer {
        Block block;
        uint32 offset;
    };

    struct Bytecode_Value {
        enum Kind {
            Integer,
            Float,
            Bytes,
        };

        Kind kind;
        int64 integer;
        double float_;
        struct {
            uint8 *ptr;
            usize size;
        } bytes;
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
        Sv name;
        bool is_function;
        sema::Type_Id type_id;
        sema::Symbol_Id symbol_id;
    };

    struct Call_Frame {
        Bytecode_Function *bcfn;
        uint64 ip;
        Register *registers;
        uint8 *stack_top;
        uint8 *stack_bot;
        uint8 *return_slot;
        Reg return_register;
        sema::Type_Id return_type;
    };

    struct Bytecode_Machine {
        using Data_Section  = Arr<uint8>;
        using Function_Pool = DenseMap<sema::Symbol_Id, Bytecode_Function*>;
        using Value_Pool = Arr<Bytecode_Value>;

        Data_Section  data_section;
        Function_Pool functions;
        Value_Pool    values;

        uint8 *memory[2];
        uint8 *stack;

        Alloc         allocator;
        Compiler_Context *compiler;
        Arr<Global>   globals;
        Opt<Call_Frame> prep_call_frame;
        sema::Type_Layout *layout;
        Call_Frame global_frame;

        Bytecode_Machine(Alloc a, Compiler_Context *compiler);
        void destroy();

        usize add_global(Sv name, sema::Type_Id type_id, sema::Symbol_Id symbol_id, bool is_function = false);
        usize alloc_value(Bytecode_Value value);

        void add_function(sema::Symbol_Id, Bytecode_Function*);
        bool contains_function(sema::Symbol_Id);

        bool contains_sequence_of_bytes(const uint8 *bytes, usize count);
        uint32 find_or_insert_bytes_to_data_section(const uint8 *bytes, usize count);
        void append_to_data_section(const uint8 *byte, usize count);

        String emit_to_c_code(Compiler_Context &compiler);

        Call_Frame create_call_frame(Call_Frame &parent, Bytecode_Function *bcfn, usize register_count, sema::Type_Id return_type);

        usize eval(ast::Expression *expression, Bytecode_Compiler &bc);
        //void eval(Bytecode_Instruction *instructions, usize instruction_count);
        int step(Call_Frame &frame, Bytecode_Instruction instruction);
        void do_function(Call_Frame &frame, Bytecode_Function &bcfn);

    private:
        void write_to_mem_from_register(Call_Frame &frame, uint8 *mem, Reg reg, sema::Type_Id type);
        void write_to_register_from_mem(Call_Frame &frame, uint8 *mem, Reg reg, sema::Type_Id type);
    };

    enum Opcode : int {
        LOAD_PARAM,      // load_param  T, <stack_slot>
        LOAD_GLOBAL,   // load_global global_id
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
        DO_CALL_PTR,

        LOAD_VALUE,

        ADD,
        SUB,
        MUL,
        DIV,
        SHR,
        SHL,

        OP_COUNT = 1000,
    };

    enum {
       LHS_IS_POINTER = 1 << 0, // used to signify that we are operating on a pointer value.
    };


    extern const char *opnames[OP_COUNT];

    struct Bytecode_Instruction {
        Opcode opcode;
        Opt<sema::Type_Id> type_id;
        int64  dst = -1;
        Reg arg1;
        Reg arg2;
        Reg arg3;
        int64  imm;
        Sv     str;
        int options;
    };

    enum {
        BCFN_IS_EXTERNALLY_DEFINED = 0x1,
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
        usize current_stack_offset = 0;

        Bytecode_Function(Alloc alloc, String name, sema::Symbol_Id id, Prototype proto);
        void destroy();

        void add_instruction(Bytecode_Instruction instruction);
        void emit(Opcode op, Opt<sema::Type_Id> type_id, Reg dst = 0, Reg a1 = 0, Reg a2 = 0, Reg a3 = 0, int64 imm = 0, Sv str = "", int options = 0);

        int64 get_register();
        void   free_register(int64 reg);

        Stack_Slot alloc_stack_slot(sema::Type_Layout *, sema::Type_Id type_id);
    };

    enum {
        FAULT,
        OK,
        HALT,
    };

    struct Bytecode_Evaluator {
        Bytecode_Machine *m;
        Alloc allocator;
        Bytecode_Evaluator(Bytecode_Machine *machine, Alloc allocator);
    };
}
