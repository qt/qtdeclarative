#ifndef MASM_JSGLOBALDATA_H
#define MASM_JSGLOBALDATA_H

#include "ExecutableAllocator.h"
#include "WeakRandom.h"

namespace JSC {

struct JSGlobalData {
    ExecutableAllocator executableAllocator;
};

}

#endif // MASM_JSGLOBALDATA_H
