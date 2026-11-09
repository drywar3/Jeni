#include <stdint.h>
#include <stdbool.h>
extern unsigned char jeni_data_section[];
union _Register {
    int64_t int_value;
    double  float_value;
    char   *pointer_value;
};

extern "C" int getint2(int p0, int p1)
{
    _Register r0, r1, r2;
    auto sp0_slot = (int){};
    auto sp1_slot = (int){};
    auto sp0 = (int*)&sp0_slot;    // LOAD_PARAM(1)
    *sp0 = p0;    // LOAD_PARAM(2)
    auto sp1 = (int*)&sp1_slot;    // LOAD_PARAM(1)
    *sp1 = p1;    // LOAD_PARAM(2)
    r1.int_value = (int)*sp0;    // LOAD_STACK
    r2.int_value = (int)*sp1;    // LOAD_STACK
    r0.int_value = (int)((int)r1.int_value+(int)r2.int_value);
    return (int)r0.int_value;
}

extern "C" int get_integer(int p0, int p1, int p2)
{
    _Register r0, r1, r2, r3, r4;
    auto sp0_slot = (int){};
    auto sp1_slot = (int){};
    auto sp2_slot = (int){};
    auto sp0 = (int*)&sp0_slot;    // LOAD_PARAM(1)
    *sp0 = p0;    // LOAD_PARAM(2)
    auto sp1 = (int*)&sp1_slot;    // LOAD_PARAM(1)
    *sp1 = p1;    // LOAD_PARAM(2)
    auto sp2 = (int*)&sp2_slot;    // LOAD_PARAM(1)
    *sp2 = p2;    // LOAD_PARAM(2)
    r2.pointer_value = (char*)getint2;
    r3.int_value = (int)*sp0;    // LOAD_STACK
    r4.int_value = (int)*sp1;    // LOAD_STACK
    r1.int_value = ((int(*)(int, int))r2.pointer_value)((int)r3.int_value, (int)r4.int_value);
    r2.int_value = (int)*sp2;    // LOAD_STACK
    r0.int_value = (int)((int)r1.int_value*(int)r2.int_value);
    return (int)r0.int_value;
}

extern "C" int printf(const unsigned char* p0);

extern "C" int main()
{
    _Register r0, r1, r2, r3, r4;
    auto sp0_slot = (int){};
    auto sp1_slot = (int){};
    r1.pointer_value = (char*)get_integer;
    r2.int_value = 1;
    r3.int_value = 2;
    r4.int_value = 2;
    r0.int_value = ((int(*)(int, int, int))r1.pointer_value)((int)r2.int_value, (int)r3.int_value, (int)r4.int_value);
    auto sp0 = &sp0_slot;    // ALLOC_STACK
    *sp0 = (int)r0.int_value;    // STORE_STACK
    r0.int_value = *(int*)(unsigned char[]){0x78, 0x0, 0x0, 0x0};
    auto sp1 = &sp1_slot;    // ALLOC_STACK
    *sp1 = (int)r0.int_value;    // STORE_STACK
    r0.pointer_value = (char*)printf;
    r1.pointer_value = (char*)&jeni_data_section[0];    // LOAD_DATA
    r2.int_value = (int)*sp1;    // LOAD_STACK
    ((int(*)(const unsigned char*, int))r0.pointer_value)((const unsigned char*)r1.pointer_value, (int)r2.int_value);
    r0.pointer_value = (char*)printf;
    r1.pointer_value = (char*)&jeni_data_section[8];    // LOAD_DATA
    r2.int_value = (int)*sp0;    // LOAD_STACK
    ((int(*)(const unsigned char*, int))r0.pointer_value)((const unsigned char*)r1.pointer_value, (int)r2.int_value);
    r0.int_value = 0;
    return (int)r0.int_value;
}


unsigned char jeni_data_section[] = {0x79, 0x20, 0x3d, 0x20, 0x25, 0x64, 0xa, 0x0, 0x78, 0x20, 0x3d, 0x20, 0x25, 0x64, 0xa, 0x0};
