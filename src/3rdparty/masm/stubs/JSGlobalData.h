// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef MASM_JSGLOBALDATA_H
#define MASM_JSGLOBALDATA_H

#include "ExecutableAllocator.h"
#include "WeakRandom.h"

namespace JSC {

class JSGlobalData {
public:
    JSGlobalData(QV4::ExecutableAllocator *realAllocator)
        : executableAllocator(realAllocator)
    {}
    ExecutableAllocator executableAllocator;
};

}

#endif // MASM_JSGLOBALDATA_H
