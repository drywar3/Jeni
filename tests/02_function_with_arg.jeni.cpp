#include <stdint.h>
#include <stdbool.h>
extern unsigned char jeni_data_section[];
union _Register {
    int64_t int_value;
    double  float_value;
    char   *pointer_value;
};

extern "C" void putchar(unsigned char p0);

extern "C" unsigned char foo(unsigned char p0)
{
    _Register r0;
    auto sp0_slot = (unsigned char){};
    auto sp0 = (unsigned char*)&sp0_slot;
    *sp0 = p0;
    r0.int_value = (unsigned char)*sp0;
    return (unsigned char)r0.int_value;
}

extern "C" void hello()
{
    _Register r0, r1;
    r0.pointer_value = (char*)putchar;
    r1.int_value = 34;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 72;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 101;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 108;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 108;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 111;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 44;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 32;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 87;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 111;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 114;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 108;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 100;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 33;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
    r0.pointer_value = (char*)putchar;
    r1.int_value = 34;
    ((void(*)(unsigned char))r0.pointer_value)((unsigned char)r1.int_value);
}

extern "C" int puts(const unsigned char* p0);

extern "C" int printf(const unsigned char* p0);

extern "C" int main()
{
    _Register r0, r1, r2;
    auto sp0_slot = (int){};
    r0.int_value = 10;
    auto sp0 = &sp0_slot;
    *sp0 = (int)r0.int_value;
    r0.pointer_value = (char*)puts;
    r1.pointer_value = (char*)&jeni_data_section[0];
    ((int(*)(const unsigned char*))r0.pointer_value)((const unsigned char*)r1.pointer_value);
    r0.pointer_value = (char*)printf;
    r1.pointer_value = (char*)&jeni_data_section[14];
    ((int(*)(const unsigned char*))r0.pointer_value)((const unsigned char*)r1.pointer_value);
    r0.pointer_value = (char*)printf;
    r1.pointer_value = (char*)&jeni_data_section[27];
    r2.int_value = (int)*sp0;
    ((int(*)(const unsigned char*, int))r0.pointer_value)((const unsigned char*)r1.pointer_value, (int)r2.int_value);
    r0.int_value = 0;
    return (int)r0.int_value;
}


unsigned char jeni_data_section[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0xa, 0x0, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x25, 0x63, 0x0};
