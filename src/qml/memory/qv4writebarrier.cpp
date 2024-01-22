// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <private/qv4value_p.h>
#include <private/qv4mm_p.h>

QT_BEGIN_NAMESPACE

namespace {
    void markHeapBase(QV4::MarkStack* markStack, QV4::Heap::Base *base){
        if (!base)
            return;
        base->mark(markStack);
    }
}
namespace QV4 {

void WriteBarrier::write_slowpath(EngineBase *engine, Heap::Base *base, ReturnedValue *slot, ReturnedValue value)
{
    Q_UNUSED(base);
    Q_UNUSED(slot);
    MarkStack * markStack = engine->memoryManager->markStack();
    if constexpr (isInsertionBarrier)
        markHeapBase(markStack, Value::fromReturnedValue(value).heapObject());
}

void WriteBarrier::write_slowpath(EngineBase *engine, Heap::Base *base, Heap::Base **slot, Heap::Base *value)
{
    Q_UNUSED(base);
    Q_UNUSED(slot);
    MarkStack * markStack = engine->memoryManager->markStack();
    if constexpr (isInsertionBarrier)
        markHeapBase(markStack, value);
}

}
QT_END_NAMESPACE
