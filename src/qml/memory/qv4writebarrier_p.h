/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
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
static Q_CONSTEXPR inline bool isRequired() {
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
