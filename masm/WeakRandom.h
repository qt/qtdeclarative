#ifndef MASM_WEAKRANDOM_H
#define MASM_WEAKRANDOM_H

#include <stdint.h>

struct WeakRandom {
    WeakRandom(int) {}
    uint32_t getUint32() { return 0; }
};

#endif // MASM_WEAKRANDOM_H
