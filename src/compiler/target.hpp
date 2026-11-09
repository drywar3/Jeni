#pragma once

#include "misc/misc.hpp"

enum Architecture {
    X86_64,
    AARCH64,
};

enum Os {
    MACOS,
    LINUX,
    WINDOWS,
};

struct Target {
    Architecture arch;
    Os os;
    usize pointer_size;
    Sv triple;

    static Target x86_64_linux();
};
