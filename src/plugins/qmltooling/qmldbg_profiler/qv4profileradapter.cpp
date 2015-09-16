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

#include "qv4profileradapter.h"
#include "qqmlprofilerservice.h"

#include <private/qpacket_p.h>

QT_BEGIN_NAMESPACE

QV4ProfilerAdapter::QV4ProfilerAdapter(QQmlProfilerService *service, QV4::ExecutionEngine *engine) :
    QQmlAbstractProfilerAdapter(service), m_functionCallPos(0), m_memoryPos(0)
{
    engine->enableProfiler();
    connect(this, SIGNAL(profilingEnabled(quint64)),
            this, SLOT(forwardEnabled(quint64)));
    connect(this, SIGNAL(profilingEnabledWhileWaiting(quint64)),
            this, SLOT(forwardEnabledWhileWaiting(quint64)), Qt::DirectConnection);
    connect(this, SIGNAL(v4ProfilingEnabled(quint64)),
            engine->profiler, SLOT(startProfiling(quint64)));
    connect(this, SIGNAL(v4ProfilingEnabledWhileWaiting(quint64)),
            engine->profiler, SLOT(startProfiling(quint64)), Qt::DirectConnection);
    connect(this, SIGNAL(profilingDisabled()), engine->profiler, SLOT(stopProfiling()));
    connect(this, SIGNAL(profilingDisabledWhileWaiting()), engine->profiler, SLOT(stopProfiling()),
            Qt::DirectConnection);
    connect(this, SIGNAL(dataRequested()), engine->profiler, SLOT(reportData()));
    connect(this, SIGNAL(referenceTimeKnown(QElapsedTimer)),
            engine->profiler, SLOT(setTimer(QElapsedTimer)));
    connect(engine->profiler, SIGNAL(dataReady(QVector<QV4::Profiling::FunctionCallProperties>,
                                               QVector<QV4::Profiling::MemoryAllocationProperties>)),
            this, SLOT(receiveData(QVector<QV4::Profiling::FunctionCallProperties>,
                                   QVector<QV4::Profiling::MemoryAllocationProperties>)));
}

qint64 QV4ProfilerAdapter::appendMemoryEvents(qint64 until, QList<QByteArray> &messages)
{
    while (m_memoryData.length() > m_memoryPos && m_memoryData[m_memoryPos].timestamp <= until) {
        QPacket d;
        QV4::Profiling::MemoryAllocationProperties &props = m_memoryData[m_memoryPos];
        d << props.timestamp << MemoryAllocation << props.type << props.size;
        ++m_memoryPos;
        messages.append(d.data());
    }
    return m_memoryData.length() == m_memoryPos ? -1 : m_memoryData[m_memoryPos].timestamp;
}

qint64 QV4ProfilerAdapter::finalizeMessages(qint64 until, QList<QByteArray> &messages,
                                            qint64 callNext)
{
    if (callNext == -1) {
        m_functionCallData.clear();
        m_functionCallPos = 0;
    }

    qint64 memoryNext = appendMemoryEvents(until, messages);

    if (memoryNext == -1) {
        m_memoryData.clear();
        m_memoryPos = 0;
        return callNext;
    }

    return callNext == -1 ? memoryNext : qMin(callNext, memoryNext);
}

qint64 QV4ProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (true) {
        while (!m_stack.isEmpty() &&
               (m_functionCallPos == m_functionCallData.length() ||
                m_stack.top() <= m_functionCallData[m_functionCallPos].start)) {
            if (m_stack.top() > until)
                return finalizeMessages(until, messages, m_stack.top());

            appendMemoryEvents(m_stack.top(), messages);
            QPacket d;
            d << m_stack.pop() << RangeEnd << Javascript;
            messages.append(d.data());
        }
        while (m_functionCallPos != m_functionCallData.length() &&
               (m_stack.empty() || m_functionCallData[m_functionCallPos].start < m_stack.top())) {
            const QV4::Profiling::FunctionCallProperties &props =
                    m_functionCallData[m_functionCallPos];
            if (props.start > until)
                return finalizeMessages(until, messages, props.start);

            appendMemoryEvents(props.start, messages);

            QPacket d_start;
            d_start << props.start << RangeStart << Javascript;
            messages.push_back(d_start.data());
            QPacket d_location;
            d_location << props.start << RangeLocation << Javascript << props.file << props.line
                       << props.column;
            messages.push_back(d_location.data());
            QPacket d_data;
            d_data << props.start << RangeData << Javascript << props.name;
            messages.push_back(d_data.data());
            m_stack.push(props.end);
            ++m_functionCallPos;
        }
        if (m_stack.empty() && m_functionCallPos == m_functionCallData.length())
            return finalizeMessages(until, messages, -1);
    }
}

void QV4ProfilerAdapter::receiveData(
        const QVector<QV4::Profiling::FunctionCallProperties> &functionCallData,
        const QVector<QV4::Profiling::MemoryAllocationProperties> &memoryData)
{
    // In rare cases it could be that another flush or stop event is processed while data from
    // the previous one is still pending. In that case we just append the data.

    if (m_functionCallData.isEmpty())
        m_functionCallData = functionCallData;
    else
        m_functionCallData.append(functionCallData);

    if (m_memoryData.isEmpty())
        m_memoryData = memoryData;
    else
        m_memoryData.append(memoryData);

    service->dataReady(this);
}

quint64 QV4ProfilerAdapter::translateFeatures(quint64 qmlFeatures)
{
    quint64 v4Features = 0;
    const quint64 one = 1;
    if (qmlFeatures & (one << ProfileJavaScript))
        v4Features |= (one << QV4::Profiling::FeatureFunctionCall);
    if (qmlFeatures & (one << ProfileMemory))
        v4Features |= (one << QV4::Profiling::FeatureMemoryAllocation);
    return v4Features;
}

void QV4ProfilerAdapter::forwardEnabled(quint64 features)
{
    emit v4ProfilingEnabled(translateFeatures(features));
}

void QV4ProfilerAdapter::forwardEnabledWhileWaiting(quint64 features)
{
    emit v4ProfilingEnabledWhileWaiting(translateFeatures(features));
}

QT_END_NAMESPACE
