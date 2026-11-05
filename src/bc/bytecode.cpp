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

void Bytecode_Function::emit(Opcode op, Opt<sema::Type_Id> type_id, uint32 dst, uint32 a1, uint32 a2, int64 imm, Sv str)
{
    add_instruction({op, type_id, dst, a1, a2, imm, str});
}

uint32 Bytecode_Function::get_register()
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
