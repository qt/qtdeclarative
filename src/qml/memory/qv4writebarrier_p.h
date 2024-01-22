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
#include <private/qv4enginebase_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
struct EngineBase;
typedef quint64 ReturnedValue;

struct WriteBarrier {

    static constexpr bool isInsertionBarrier = true;

    Q_ALWAYS_INLINE static void write(EngineBase *engine, Heap::Base *base, ReturnedValue *slot, ReturnedValue value)
    {
        if (engine->isGCOngoing)
            write_slowpath(engine, base, slot, value);
        *slot = value;
    }
    Q_QML_EXPORT Q_NEVER_INLINE static void write_slowpath(
            EngineBase *engine, Heap::Base *base,
            ReturnedValue *slot, ReturnedValue value);

    Q_ALWAYS_INLINE static void write(EngineBase *engine, Heap::Base *base, Heap::Base **slot, Heap::Base *value)
    {
        if (engine->isGCOngoing)
            write_slowpath(engine, base, slot, value);
        *slot = value;
    }
    Q_QML_EXPORT Q_NEVER_INLINE static void write_slowpath(
            EngineBase *engine, Heap::Base *base,
            Heap::Base **slot, Heap::Base *value);

    // MemoryManager isn't a complete type here, so make Engine a template argument
    // so that we can still call engine->memoryManager->markStack()
    template<typename F, typename Engine = EngineBase>
    static void markCustom(Engine *engine, F &&markFunction) {
        if (engine->isGCOngoing)
            (std::forward<F>(markFunction))(engine->memoryManager->markStack());
    }
};

       // ### this needs to be filled with a real memory fence once marking is concurrent
Q_ALWAYS_INLINE void fence() {}

}

QT_END_NAMESPACE

#endif
