#include "bc/bytecode.hpp"

#define STACK_SIZE (1024*4)

using namespace bc;

Bytecode_Machine::Bytecode_Machine(Alloc a)
    : data_section(a), functions(a), allocator(a)
{
    memory[STACK] = MINI_ALLOC_MANY(a, uint8, STACK_SIZE);
    memory[DATA]  = data_section.data();
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

void Bytecode_Function::emit(Opcode op, Opt<sema::Type_Id> type_id, int64 dst, Reg a1, Reg a2, Reg a3, int64 imm, Sv str)
{
    add_instruction({op, type_id, dst, a1, a2, a3, imm, str});
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

uint32 Bytecode_Function::alloc_stack_slot(sema::Type_Id type_id)
{
    usize index = stack_slots.count();
    stack_slots.append({type_id, index});
    return index;
}

void Bytecode_Function::free_register(int64 reg)
{
    registers[reg] = true;
}

uint32 Bytecode_Machine::find_or_insert_bytes_to_data_section(const uint8 *bytes, usize count)
{
    uint8 *start = memory[DATA];
    uint8 *head  = start;
    uint8 *tail  = head + data_section.count();

    while (head != tail) {
        if (tail - head < count) break;
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
