// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MASM_WEAKRANDOM_H
#define MASM_WEAKRANDOM_H

#include <stdint.h>

struct WeakRandom {
    WeakRandom(int) {}
    uint32_t getUint32() { return 0; }
};

#endif // MASM_WEAKRANDOM_H
