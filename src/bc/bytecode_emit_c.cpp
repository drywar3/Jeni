#include "bc/bytecode.hpp"
#include "sema/store.hpp"
#include "sema/type/id.hpp"
#include "sema/entities/symbols.hpp"
#include "sema/entities/type.hpp"
#include "compiler.hpp"

#define TAB "    "
#define emit(c, fmt, ...) (c).buffer->append_fmt(fmt,##__VA_ARGS__)

using namespace sema;
using namespace bc;

struct Call_Context {
    Type_Id result_type_id;
    int64 result_register;
    int64 callee_register;
    Arr<std::pair<uint32, Type_Id>> args;
};

struct Context {
    String headers;
    String definitions;
    String code;
    Bytecode_Function *function = nullptr;
    Compiler_Context &cc;
    Opt<Call_Context> call_context = None;
    Bytecode_Machine *m;

    Context(Alloc a, Compiler_Context &cc, Bytecode_Machine *m) : headers(a), definitions(a), code(a), cc(cc), m(m) {}

    String *buffer = &headers;

    void swap(String &buffer)
    {
        this->buffer = &buffer;
    }
};

void emit_function(Context &context, Bytecode_Machine *m, Bytecode_Function *function);

void emit_std_headers(Context &c)
{
    emit(c, "#include <stdint.h>\n");
    emit(c, "#include <stdbool.h>\n");
}

void emit_register_type(Context &c)
{
    emit(c, "union _Register {\n");
    emit(c, "    int64_t int_value;\n");
    emit(c, "    double  float_value;\n");
    emit(c, "    char   *pointer_value;\n");
    emit(c, "};\n");
}

String Bytecode_Machine::emit_to_c_code(Compiler_Context &compiler)
{
    Context c{allocator, compiler, this};
    emit_std_headers(c);
    c.swap(c.definitions);
    emit(c, "extern unsigned char jeni_data_section[];\n");
    emit_register_type(c);
    c.swap(c.code);
    for (auto function : functions) {
        emit_function(c, this, function);
    }
    c.headers.append_fmt("%s\n%s", c.definitions.data(), c.code.data());

    c.headers.append_fmt("\nunsigned char jeni_data_section[] = {");

    usize n = 0;
    for (uint8 byte : data_section.iter()) {
        if (n != 0) c.headers.append_fmt(", ");
        c.headers.append_fmt("0x%x", byte);
        n += 1;
    }

    c.headers.append_fmt("};\n");


    return c.headers;
}

void emit_type(Context &c, sema::Type_Id type_id)
{
    auto &types = c.cc.store.types;
    auto type   = types.at_index(usize(type_id));
    switch (type->kind) {
    case Type::Kind::Error:     MINI_UNREACHABLE();
    case Type::Kind::Int32:     emit(c, "int");   break;
    case Type::Kind::Uint8:     emit(c, "unsigned char");   break;
    case Type::Kind::Void:      emit(c, "void");  break;
    case Type::Kind::Pointer:
        {
            auto pointer = static_cast<Pointer_Type*>(type);
            if (bool(pointer->access)) {
                emit(c, "const ");
            }
            emit_type(c, pointer->inner);
            emit(c, "*");
            break;
        }
    case Type::Kind::Function:
        {
            auto prot = static_cast<Function_Prototype*>(type);
            emit_type(c, prot->return_type);
            emit(c, "(*)(");
            usize n = 0;
            for (auto type : prot->parameter_types.iter()) {
                if (n != 0) emit(c, ", ");
                emit_type(c, type);
                n += 1;
            }
            emit(c, ")");
        } break;
    default: todo("%d", type->kind);
    }
}

void emit_register(Context &c, Type_Id type_id, uint32 reg, bool emit_as_lvalue = false)
{
    if (!emit_as_lvalue) {
        emit(c, "(");
        emit_type(c, type_id);
        emit(c, ")");
    }

    const char *field;
    auto &type = c.cc.store.types.at_index(usize(type_id));
    if (type->is_integer()) field = "int_value";
    else field = "pointer_value";
    emit(c, "r%d.%s", reg, field);
}

char opcode_char(Opcode op)
{
    switch (op) {
    case ADD: return '+';
    case SUB: return '-';
    case MUL: return '*';
    case DIV: return '/';
    default: MINI_UNREACHABLE();
    }
}

void emit_instruction(Context &c, Bytecode_Machine *m, Bytecode_Instruction &inst)
{
    switch (inst.opcode) {
    case LOAD_PARAM:
        {
            auto param_index = inst.dst;
            auto stack_index = inst.arg2;
            auto param_type  = *inst.type_id;
            emit(c, TAB"auto sp%zu = (", stack_index);
            emit_type(c, param_type);
            emit(c, "*)&sp%zu_slot;" TAB "// LOAD_PARAM(1)\n", stack_index);
            emit(c, TAB"*sp%zu = p%zu;" TAB "// LOAD_PARAM(2)\n", stack_index, param_index);
        } break;
    case RETURN:
        {
            emit(c, TAB"return");
            auto ret_type = *inst.type_id;
            if (ret_type != type_id::VOID) {
                uint32 reg = inst.arg1;
                emit(c, " ");
                emit_register(c, ret_type, reg);
            }
            emit(c, ";\n");
        } break;
    case STORE_REG_IMM:
        {
            auto reg_type = *inst.type_id;
            auto value    = inst.imm;
            auto reg      = inst.dst;
            emit(c, TAB);
            emit_register(c, reg_type, reg, true);
            emit(c, " = %d;\n", value);
        } break;
    case STORE_STACK:
        {
            auto type  = *inst.type_id;
            auto reg   = inst.arg1;
            auto index = inst.arg3;
            emit(c, TAB"*sp%lld = ", index);
            emit_register(c, type, reg);
            emit(c, ";" TAB "// STORE_STACK\n");
        } break;
    case PREP_CALL:
        {
            auto result_type = *inst.type_id;
            auto result_reg  = inst.dst;
            auto callee_reg  = inst.arg1;
            assert(!c.call_context.has_value(), "");
            c.call_context = Call_Context{
                .result_type_id = result_type,
                .result_register = result_reg,
                .callee_register = callee_reg,
                .args = {c.cc.allocator},
            };
        } break;
    case LOAD_ARG:
        {
            assert(c.call_context.has_value(), "cannot load argument without call context");
            auto arg_type = *inst.type_id;
            auto arg_reg  = inst.arg1;
            c.call_context->args.append({arg_reg, arg_type});
        } break;
    case DO_CALL:
        {
            assert(c.call_context.has_value(), "cannot do call without call context");
            Call_Context call = *c.call_context;
            c.call_context = None;

            auto callee_type = *inst.type_id;

            emit(c, TAB);

            if (call.result_register >= 0) {
                emit_register(c, call.result_type_id, call.result_register, true);
                emit(c, " = ");
            }

            emit(c, "(");
            emit_register(c, callee_type, call.callee_register);
            emit(c, ")(");
            usize n = 0;
            for (auto [reg, type] : call.args.iter()) {
                if (n != 0) emit(c, ", ");
                emit_register(c, type, reg);
                n += 1;
            }
            emit(c, ");\n");
        } break;
    case LOAD_GLOBAL:
        {
            usize global_id    = inst.arg1;
            auto result_reg    = inst.dst;
            auto constant_type = *inst.type_id;

            Global &global = c.m->globals[global_id];
            emit(c, TAB);
            emit_register(c, constant_type, result_reg, true);
            emit(c, " = (char*)%.*s;\n", SVARG(global.name.base()));
        } break;
    case LOAD_STACK:
        {
            auto type  = *inst.type_id;
            auto dst   = inst.dst;
            auto index = inst.arg1;
            emit(c, TAB);
            emit_register(c, type, dst, true);
            emit(c, " = (");
            emit_type(c, type);
            emit(c, ")*sp%lld;" TAB "// LOAD_STACK\n", index);
        } break;

    case LOAD_DATA:
        {
            auto type = *inst.type_id;
            auto dst  = inst.dst;
            auto offset = inst.arg1;
            emit(c, TAB);
            emit_register(c, type, dst, true);
            emit(c, " = (char*)&jeni_data_section[%lld];" TAB "// LOAD_DATA\n", offset);
        } break;

    case ALLOC_STACK:
        {
            auto index = inst.arg2;
            emit(c, TAB);
            emit(c, "auto sp%lld = &sp%lld_slot;" TAB "// ALLOC_STACK\n", index, index);
        } break;

    case MUL:
    case ADD:
    case SUB:
    case DIV:
        {
            auto type = *inst.type_id;
            auto dst = inst.dst;
            auto left = inst.arg1;
            auto right = inst.arg2;
            emit(c, TAB);
            if (dst != -1) {
                emit_register(c, type, dst, true);
                emit(c, " = ");
            }
            emit(c, "("); emit_type(c, type); emit(c, ")");
            emit(c, "(");
            emit_register(c, type, left, false);
            emit(c, "%c", opcode_char(inst.opcode));
            emit_register(c, type, right, false);
            emit(c, ");\n");
        } break;

    case LOAD_VALUE:
        {
            auto type = *inst.type_id;
            auto dst  = inst.dst;
            auto id = inst.arg1;
            auto value = c.m->values[id];
            emit(c, TAB);
            emit_register(c, type, dst, true);
            emit(c, " = ");
            switch (value.kind) {
            case Bytecode_Value::Integer:
                emit(c, "%lld", value.integer);
                break;
            case Bytecode_Value::Bytes:
                {
                    auto bytes = value.bytes;
                    emit(c, "*("); emit_type(c, type); emit(c, "*)");
                    emit(c, "(unsigned char[]){");
                    for (usize n = 0; n < bytes.size; n++) {
                        if (n != 0) emit(c, ", ");
                        emit(c, "0x%x", bytes.ptr[n]);
                    }
                    emit(c, "};\n");
                } break;
            default: todo();
            }
        } break;
    default: todo();
    }
}

void emit_function_prelude(Context &c, Bytecode_Function *fn)
{
    if (!fn->registers.empty()) {
        emit(c, TAB"_Register ");
        usize n = 0;
        for (auto r : fn->registers.iter()) {
            (void)r;
            if (n != 0) emit(c, ", ");
            emit(c, "r%zu", n);
            n += 1;
        }
        emit(c, ";\n");
    }

    if (!fn->stack_slots.empty()) {
        usize n = 0;
        for (auto s : fn->stack_slots.iter()) {
            emit(c, TAB"auto sp%zu_slot = (", n);
            emit_type(c, s.type_id);
            emit(c, "){};\n");
            n += 1;
        }
    }
}

void emit_function(Context &c, Bytecode_Machine *m, Bytecode_Function *fn)
{
    //Symbol *symbol = c.cc.store.symbols.at_index(usize(fn->id));
    Prototype prot = fn->prototype;

    bool is_extern_defn = fn->flags & BCFN_IS_EXTERNALLY_DEFINED;

    emit(c, "extern \"C\" ");
    emit_type(c, prot.return_type);
    emit(c, " %s(", fn->name.data());

    for (usize n = 0; n < prot.parameter_types.count(); n++) {
        if (n != 0) emit(c, ", ");
        emit_type(c, prot.parameter_types[n]);
        emit(c, " p%zu", n);
    }
    emit(c, ")");
    if (is_extern_defn) emit(c, ";\n");
    else {
        emit(c, "\n{\n");
        emit_function_prelude(c, fn);
        for (auto inst : fn->instructions.iter()) {
            emit_instruction(c, m, inst);
        }
        emit(c, "}\n");
    }
    emit(c, "\n");
}
