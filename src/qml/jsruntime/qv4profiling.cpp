/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4profiling_p.h"
#include <private/qv4mm_p.h>
#include <private/qv4string_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Profiling {

FunctionCallProperties FunctionCall::resolve() const
{
    FunctionCallProperties props = {
        m_start,
        m_end,
        m_function->name()->toQString(),
        m_function->compilationUnit->fileName(),
        m_function->compiledFunction->location.line,
        m_function->compiledFunction->location.column
    };
    return props;
}


Profiler::Profiler(QV4::ExecutionEngine *engine) : featuresEnabled(0), m_engine(engine)
{
    static int meta = qRegisterMetaType<QVector<QV4::Profiling::FunctionCallProperties> >();
    static int meta2 = qRegisterMetaType<QVector<QV4::Profiling::MemoryAllocationProperties> >();
    Q_UNUSED(meta);
    Q_UNUSED(meta2);
    m_timer.start();
}

void Profiler::stopProfiling()
{
    featuresEnabled = 0;
    reportData();
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
    QVector<FunctionCallProperties> resolved;
    resolved.reserve(m_data.size());

    foreach (const FunctionCall &call, m_data)
        resolved.append(call.resolve());

    emit dataReady(resolved, m_memory_data);
    m_data.clear();
    m_memory_data.clear();
}

void Profiler::startProfiling(quint64 features)
{
    if (featuresEnabled == 0) {
        if (features & (1 << FeatureMemoryAllocation)) {
            qint64 timestamp = m_timer.nsecsElapsed();
            MemoryAllocationProperties heap = {timestamp,
                                               (qint64)m_engine->memoryManager->getAllocatedMem(),
                                               HeapPage};
            m_memory_data.append(heap);
            MemoryAllocationProperties small = {timestamp,
                                                (qint64)m_engine->memoryManager->getUsedMem(),
                                                SmallItem};
            m_memory_data.append(small);
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
