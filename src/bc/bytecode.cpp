#include "bc/bytecode.hpp"
#include "bc/bytecode_compiler.hpp"
#include "compiler.hpp"
#include "sema/type/id.hpp"

#include <meta>

#define STACK_SIZE (1024*4)

using namespace bc;

const char *bc::opnames[OP_COUNT] = {
    [LOAD_PARAM] = "LOAD_PARAM",
    [LOAD_GLOBAL] = "LOAD_GLOBAL",
    [LOAD_STACK] = "LOAD_STACK",
    [ALLOC_STACK] = "ALLOC_STACK",
    [LOAD_DATA] = "LOAD_DATA",

    [STORE_REG] = "STORE_REG",
    [STORE_REG_IMM] = "STORE_REG_IMM",
    [STORE_STACK] = "STORE_STACK",
    [RETURN] = "RETURN",
    [PREP_CALL] = "PREP_CALL",
    [LOAD_ARG] = "LOAD_ARG",
    [DO_CALL] = "DO_CALL",
    [DO_CALL_PTR] = "DO_CALL_PTR",
    [LOAD_VALUE] = "LOAD_VALUE",
    [ADD] = "ADD",
    [SUB] = "SUB",
    [MUL] = "MUL",
    [DIV] = "DIV",
    [SHR] = "SHR",
    [SHL] = "SHL",
};

Bytecode_Machine::Bytecode_Machine(Alloc a, Compiler_Context *compiler)
    : data_section(a), functions(a), values(a), allocator(a), compiler(compiler), globals(a),
      prep_call_frame(None), layout(&compiler->store.layout)
{
    stack         = MINI_ALLOC_MANY(a, uint8, STACK_SIZE);
    memory[DATA]  = data_section.data();
    memory[STACK] = stack;
    global_frame = {
    .bcfn = nullptr,
        .ip   = 0,
        .registers = nullptr,
        .stack_top  = memory[STACK],
    };
    global_frame.stack_bot = global_frame.stack_top;
}

void Bytecode_Machine::destroy()
{
    // no-op
}

void Bytecode_Machine::add_function(sema::Symbol_Id id, Bytecode_Function *fn)
{
    functions.insert(id, fn);
}

bool Bytecode_Machine::contains_function(sema::Symbol_Id id)
{
    return functions.contains(id);
}

Bytecode_Function::Bytecode_Function(Alloc alloc, String name, sema::Symbol_Id id, Prototype proto)
    : name(name), id(id), registers(alloc), instructions(alloc),
      prototype(proto),
      stack_slots(alloc), allocator(alloc)
{
}

void Bytecode_Function::destroy()
{
    //no-op
}

void Bytecode_Function::add_instruction(Bytecode_Instruction instruction)
{
    instructions.append(instruction);
}

void Bytecode_Function::emit(Opcode op, Opt<sema::Type_Id> type_id, int64 dst, Reg a1, Reg a2, Reg a3, int64 imm, Sv str, int options)
{
    add_instruction({op, type_id, dst, a1, a2, a3, imm, str, options});
}

int64 Bytecode_Function::get_register()
{
    for (uint32 n = 0; n < registers.count(); n++) {
        if (registers[n]) {
            registers[n] = false;
            return n;
        }
    }
    return registers.append(false);
}

Stack_Slot Bytecode_Function::alloc_stack_slot(sema::Type_Layout *layout, sema::Type_Id type_id)
{
    usize index = stack_slots.count();
    usize current_stack_offset = this->current_stack_offset;
    this->current_stack_offset += layout->get_size(type_id);
    stack_slots.append({type_id, index, current_stack_offset});
    return stack_slots.last();
}

void Bytecode_Function::free_register(int64 reg)
{
    registers[reg] = true;
}

usize Bytecode_Machine::add_global(Sv name, sema::Type_Id type_id, sema::Symbol_Id symbol_id, bool is_function)
{
    usize count = globals.count();
    globals.append({name, is_function, type_id, symbol_id});
    return count;
}

uint32 Bytecode_Machine::find_or_insert_bytes_to_data_section(const uint8 *bytes, usize count)
{
    uint8 *start = memory[DATA];
    uint8 *head  = start;
    uint8 *tail  = head + data_section.count();

    while (head != tail) {
        if (usize(tail - head) < count) break;
        if (memcmp(head, bytes, count) == 0) {
            uint32 offset = head - start;
            return offset;
        }
        head += 1;
    }

    usize offset = data_section.count();
    data_section.append_range(bytes, count);
    memory[DATA] = data_section.data();
    return offset;
}

void Bytecode_Machine::append_to_data_section(const uint8 *bytes, usize count)
{
    data_section.append_range(bytes, count);
    memory[DATA] = data_section.data();
}

Call_Frame Bytecode_Machine::create_call_frame(Call_Frame &parent, Bytecode_Function *bcfn, usize register_count, sema::Type_Id return_type)
{
    Call_Frame frame{};
    frame.bcfn = bcfn;
    frame.registers   = MINI_ALLOC_MANY(allocator, Register, register_count);
    frame.return_type = return_type;
    usize size = compiler->store.layout.get_size(return_type);
    frame.return_slot = parent.stack_top;
    frame.stack_top   = parent.stack_top + size;
    frame.stack_bot   = frame.stack_top;
    return frame;
}

Bytecode_Evaluator::Bytecode_Evaluator(Bytecode_Machine *machine, Alloc allocator)
    : m(machine), allocator(allocator)
{
}

// void Bytecode_Evaluator::eval(Bytecode_Instruction *instructions, usize instruction_count)
// {
//     todo();
// }

usize Bytecode_Machine::eval(ast::Expression *expression, Bytecode_Compiler &bc)
{

    Bytecode_Function fn{allocator, String::build(allocator, "_temp"), sema::Symbol_Id(0), {}};

    auto save = bc.current_function;
    bc.current_function = &fn;
    defer { bc.current_function = save; };

    sema::Type_Id type = sema::Type_Id(*bc.store.types.get_id(expression->locus));
    uint32 result = bc.compile_expression(expression);


    Call_Frame frame = create_call_frame(global_frame, &fn, fn.registers.count(), type);
    do_function(frame, fn);

    if (expression->kind == ast::Expression_Kind::Function_Call) {
        write_to_mem_from_register(frame, frame.return_slot, result, type);
        usize value_id    = alloc_value({
            .kind = Bytecode_Value::Bytes,
            .bytes = {
                .ptr  = frame.return_slot,
                .size = compiler->store.layout.get_size(type),
            },
        });
        return value_id;
    }

    //Register reg = frame.registers[result];
    todo();
}

int Bytecode_Machine::step(Call_Frame &frame, Bytecode_Instruction inst)
{
    switch (inst.opcode) {
    case STORE_REG_IMM:
        {
            auto reg      = inst.dst;
            auto value    = inst.imm;
            frame.registers[reg].int_value = value;
            return OK;
        } break;
    case LOAD_GLOBAL:
        {
            auto global_id = inst.arg1;
            auto reg = inst.dst;
            frame.registers[reg].int_value = global_id;
            return OK;
        } break;

    case PREP_CALL:
        {
            auto result_type = *inst.type_id;
            auto result_reg = inst.dst;
            auto callee_reg = inst.arg1;
            auto global_id  = frame.registers[callee_reg].int_value;
            auto &global = globals[global_id];
            assert(functions.contains(global.symbol_id), "cannot find function");
            assert(!prep_call_frame.has_value(), "unfinished PREP_CALL (in %s): %s", frame.bcfn->name.data(), prep_call_frame->bcfn->name.data());
            auto function = *functions.find(global.symbol_id);
            prep_call_frame = create_call_frame(frame, function, function->registers.count(), result_type);
            prep_call_frame->return_register = result_reg;
            return OK;
        } break;

    case DO_CALL:
        {
            assert(prep_call_frame.has_value(), "cannot DO_CALL without call frame");
            Call_Frame new_frame = *prep_call_frame;
            prep_call_frame = None;
            do_function(new_frame, *new_frame.bcfn);
            auto dst = new_frame.return_register;
            write_to_register_from_mem(frame, new_frame.return_slot, dst, new_frame.return_type);
            return OK;
        } break;

    case RETURN:
        {
           auto type  = *inst.type_id;
           uint32 reg = inst.arg1;
           if (type != sema::type_id::VOID) {
               //usize size_of_type = compiler->store.layout.get_size(type);
               printf("RETURNING: %d from REG(%d)\n", frame.registers[reg].int_value, reg);
               write_to_mem_from_register(frame, frame.return_slot, reg, type);
           }
           return OK;
        } break;

    case ALLOC_STACK:
        {
            // auto type   = *inst.type_id;
            // uint8 *stack_top = frame.stack_top;
            // usize size_of_type = layout->get_size(type);
            // assert(stack_top - stack < STACK_SIZE, "stack overflow");
            // frame.stack_top += size_of_type; // @Check
            return OK;
        } break;

    case STORE_STACK:
        {
            auto type   = *inst.type_id;
            auto reg    = inst.dst;
            auto offset = inst.arg2;
            uint8 *stack_bot = frame.stack_bot;
            uint8 *memory = stack_bot + offset;
            write_to_mem_from_register(frame, memory, reg, type);
            return OK;
        } break;

    case LOAD_STACK:
        {
            auto type   = *inst.type_id;
            auto reg    = inst.dst;
            auto offset = inst.arg2;
            uint8 *stack_bot = frame.stack_bot;
            uint8 *memory = stack_bot + offset;
            write_to_register_from_mem(frame, memory, reg, type);
            return OK;
        } break;

    case LOAD_PARAM:
        {
            return OK;
        } break;

    case LOAD_ARG:
        {
            auto type = *inst.type_id;
            auto arg  = inst.arg1;
            assert(prep_call_frame.has_value(), "cannot LOAD_ARG without call frame");
            uint8 *stack_top = prep_call_frame->stack_top;
            write_to_mem_from_register(frame, stack_top, arg, type);
            prep_call_frame->stack_top += layout->get_size(type);
            return OK;
        } break;

    case MUL:
        {
            //auto type = *inst.type_id;
            auto dst  = inst.dst;
            auto left = inst.arg1;
            auto right = inst.arg2;
            frame.registers[dst].int_value =
                frame.registers[left].int_value * frame.registers[right].int_value;
            return OK;
        }

    case ADD:
        {
            //auto type = *inst.type_id;
            auto dst  = inst.dst;
            auto left = inst.arg1;
            auto right = inst.arg2;
            frame.registers[dst].int_value =
                frame.registers[left].int_value + frame.registers[right].int_value;
            return OK;
        }

    case SUB:
        {
            //auto type = *inst.type_id;
            auto dst  = inst.dst;
            auto left = inst.arg1;
            auto right = inst.arg2;
            frame.registers[dst].int_value =
                frame.registers[left].int_value - frame.registers[right].int_value;
            return OK;
        }

    case DIV:
        {
            //auto type = *inst.type_id;
            auto dst  = inst.dst;
            auto left = inst.arg1;
            auto right = inst.arg2;
            assert(frame.registers[right].int_value != 0, "division by zero");
            frame.registers[dst].int_value =
                frame.registers[left].int_value / frame.registers[right].int_value;
            return OK;
        }

    default:
        {
            auto opcode = inst.opcode;
            todo("unhandled instruction: %s\n", opnames[opcode]);
        }
    }
}

void Bytecode_Machine::do_function(Call_Frame &frame, Bytecode_Function &bcfn)
{
    for (usize n = 0; n < bcfn.instructions.count(); n++) {
        Bytecode_Instruction instruction = bcfn.instructions[n];
        if (step(frame, instruction) != OK) todo();
    }
}

usize Bytecode_Machine::alloc_value(Bytecode_Value value) {
    usize count = values.count();
    values.append(value);
    return count;
}

void Bytecode_Machine::write_to_mem_from_register(Call_Frame &frame, uint8 *dst, Reg reg, sema::Type_Id type)
{
    usize size = layout->get_size(type);
    uint8 *source;
    Register r = frame.registers[reg];
    switch (type) {
    case sema::type_id::INT32:
        source = (uint8*)&r.int_value;
        break;
    default: todo();
    }
    memcpy(dst, source, size);
}

void Bytecode_Machine::write_to_register_from_mem(Call_Frame &frame, uint8 *src, Reg reg, sema::Type_Id type)
{
    usize size = layout->get_size(type);
    uint8 *dst;
    Register *r = &frame.registers[reg];
    switch (type) {
    case sema::type_id::INT32:
        dst = (uint8*)&r->int_value;
        break;
    default: todo();
    }
    memcpy(dst, src, size);
}
