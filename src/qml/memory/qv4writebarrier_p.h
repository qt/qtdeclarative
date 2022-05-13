// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4WRITEBARRIER_P_H
#define QV4WRITEBARRIER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4global_p.h>

QT_BEGIN_NAMESPACE

#define WRITEBARRIER_none 1

#define WRITEBARRIER(x) (1/WRITEBARRIER_##x == 1)

namespace QV4 {
struct EngineBase;

namespace WriteBarrier {

enum Type {
    NoBarrier,
    Barrier
};

enum NewValueType {
    Primitive,
    Object,
    Unknown
};

// ### this needs to be filled with a real memory fence once marking is concurrent
Q_ALWAYS_INLINE void fence() {}

#if WRITEBARRIER(none)

template <NewValueType type>
static constexpr inline bool isRequired() {
    return false;
}

inline void write(EngineBase *engine, Heap::Base *base, ReturnedValue *slot, ReturnedValue value)
{
    Q_UNUSED(engine);
    Q_UNUSED(base);
    *slot = value;
}

inline void write(EngineBase *engine, Heap::Base *base, Heap::Base **slot, Heap::Base *value)
{
    Q_UNUSED(engine);
    Q_UNUSED(base);
    *slot = value;
}

#endif

}

}

QT_END_NAMESPACE

#endif
