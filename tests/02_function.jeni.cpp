#include <stdint.h>
#include <stdbool.h>
union _Register {
    int64_t int_value;
    double  float_value;
    char   *pointer_value;
};

extern "C" void putchar(int p0);

extern "C" void hello()
{
    _Register r0, r1;
    r0.pointer_value = (char*)putchar;
    r1.int_value = 72;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 101;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 108;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 108;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 111;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 44;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 32;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 87;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 111;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 114;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 108;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 100;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 33;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 10;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
}

extern "C" int main()
{
    _Register r0;
    r0.pointer_value = (char*)hello;
    ((void(*)())r0.pointer_value)();
    r0.pointer_value = (char*)hello;
    ((void(*)())r0.pointer_value)();
    r0.pointer_value = (char*)hello;
    ((void(*)())r0.pointer_value)();
    r0.pointer_value = (char*)hello;
    ((void(*)())r0.pointer_value)();
    r0.pointer_value = (char*)hello;
    ((void(*)())r0.pointer_value)();
}

extern "C" void world(int p0)
{
    _Register r0, r1;
    auto sp0_slot = (int){};
    auto sp0 = (int*)&sp0_slot;
    *sp0 = p0;
    r0.pointer_value = (char*)putchar;
    r1.int_value = (int)*sp0;
    ((void(*)(int))r0.pointer_value)((int)r1.int_value);
}

