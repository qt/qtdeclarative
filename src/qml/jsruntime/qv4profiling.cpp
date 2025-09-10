// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4profiling_p.h"
#include <private/qv4mm_p.h>
#include <private/qv4string_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Profiling {

FunctionLocation FunctionCall::resolveLocation() const
{
    return FunctionLocation(m_function->name()->toQString(),
                            m_function->executableCompilationUnit()->fileName(),
                            m_function->compiledFunction->location.line(),
                            m_function->compiledFunction->location.column());
}

FunctionCallProperties FunctionCall::properties() const
{
    FunctionCallProperties props = {
        m_start,
        m_end,
        reinterpret_cast<quintptr>(m_function)
    };
    return props;
}

Profiler::Profiler(QV4::ExecutionEngine *engine) : featuresEnabled(0), m_engine(engine)
{
    static const int metatypes[] = {
        qRegisterMetaType<QVector<QV4::Profiling::FunctionCallProperties> >(),
        qRegisterMetaType<QVector<QV4::Profiling::MemoryAllocationProperties> >(),
        qRegisterMetaType<FunctionLocationHash>()
    };
    Q_UNUSED(metatypes);
    m_timer.start();
}

void Profiler::stopProfiling()
{
    featuresEnabled = 0;
    reportData();
    m_sentLocations.clear();
}

bool operator<(const FunctionCall &call1, const FunctionCall &call2)
{
    return call1.m_start < call2.m_start ||
            (call1.m_start == call2.m_start && (call1.m_end < call2.m_end ||
            (call1.m_end == call2.m_end && call1.m_function < call2.m_function)));
}

void Profiler::reportData()
{
    std::sort(m_data.begin(), m_data.end());
    QVector<FunctionCallProperties> properties;
    FunctionLocationHash locations;
    properties.reserve(m_data.size());

    for (const FunctionCall &call : std::as_const(m_data)) {
        properties.append(call.properties());
        Function *function = call.function();
        Q_ASSERT(function);
        SentMarker &marker = m_sentLocations[reinterpret_cast<quintptr>(function)];
        if (!marker.isValid()) {
            FunctionLocation &location = locations[properties.constLast().id];
            if (!location.isValid())
                location = call.resolveLocation();
            marker.setFunction(function);
        }
    }

    emit dataReady(locations, properties, m_memory_data);
    m_data.clear();
    m_memory_data.clear();
}

void Profiler::startProfiling(quint64 features)
{
    if (featuresEnabled == 0) {
        if (features & (1 << FeatureMemoryAllocation)) {
            qint64 timestamp = m_timer.nsecsElapsed();
            MemoryAllocationProperties heap = {timestamp,
                                               (qint64)m_engine->memoryManager->getAllocatedMem() -
                                               (qint64)m_engine->memoryManager->getLargeItemsMem(),
                                               HeapPage};
            m_memory_data.append(heap);
            MemoryAllocationProperties smallP = {timestamp,
                                                (qint64)m_engine->memoryManager->getUsedMem(),
                                                SmallItem};
            m_memory_data.append(smallP);
            MemoryAllocationProperties large = {timestamp,
                                                (qint64)m_engine->memoryManager->getLargeItemsMem(),
                                                LargeItem};
            m_memory_data.append(large);
        }

        featuresEnabled = features;
    }
}

} // namespace Profiling
} // namespace QV4

QT_END_NAMESPACE

#include "moc_qv4profiling_p.cpp"
