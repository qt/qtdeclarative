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

QT_BEGIN_NAMESPACE

QV4ProfilerAdapter::QV4ProfilerAdapter(QQmlProfilerService *service, QV4::ExecutionEngine *engine) :
    QQmlAbstractProfilerAdapter(service), dataPos(0), memoryPos(0)
{
    engine->enableProfiler();
    connect(this, SIGNAL(profilingEnabled(quint64)),
            engine->profiler, SLOT(startProfiling(quint64)));
    connect(this, SIGNAL(profilingEnabledWhileWaiting(quint64)),
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
    QByteArray message;
    while (memory_data.length() > memoryPos && memory_data[memoryPos].timestamp <= until) {
        QQmlDebugStream d(&message, QIODevice::WriteOnly);
        QV4::Profiling::MemoryAllocationProperties &props = memory_data[memoryPos];
        d << props.timestamp << MemoryAllocation << props.type << props.size;
        ++memoryPos;
        messages.append(message);
    }
    return memory_data.length() == memoryPos ? -1 : memory_data[memoryPos].timestamp;
}

qint64 QV4ProfilerAdapter::finalizeMessages(qint64 until, QList<QByteArray> &messages,
                                            qint64 callNext)
{
    if (callNext == -1) {
        data.clear();
        dataPos = 0;
    }

    qint64 memoryNext = appendMemoryEvents(until, messages);

    if (memoryNext == -1) {
        memory_data.clear();
        memoryPos = 0;
        return callNext;
    }

    return callNext == -1 ? memoryNext : qMin(callNext, memoryNext);
}

qint64 QV4ProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    QByteArray message;
    while (true) {
        while (!stack.isEmpty() && (dataPos == data.length() ||
                                    stack.top() <= data[dataPos].start)) {
            if (stack.top() > until)
                return finalizeMessages(until, messages, stack.top());

            appendMemoryEvents(stack.top(), messages);
            QQmlDebugStream d(&message, QIODevice::WriteOnly);
            d << stack.pop() << RangeEnd << Javascript;
            messages.append(message);
        }
        while (dataPos != data.length() && (stack.empty() || data[dataPos].start < stack.top())) {
            const QV4::Profiling::FunctionCallProperties &props = data[dataPos];
            if (props.start > until)
                return finalizeMessages(until, messages, props.start);

            appendMemoryEvents(props.start, messages);

            QQmlDebugStream d_start(&message, QIODevice::WriteOnly);
            d_start << props.start << RangeStart << Javascript;
            messages.push_back(message);
            message.clear();
            QQmlDebugStream d_location(&message, QIODevice::WriteOnly);
            d_location << props.start << RangeLocation << Javascript << props.file << props.line
                       << props.column;
            messages.push_back(message);
            message.clear();
            QQmlDebugStream d_data(&message, QIODevice::WriteOnly);
            d_data << props.start << RangeData << Javascript << props.name;
            messages.push_back(message);
            message.clear();
            stack.push(props.end);
            ++dataPos;
        }
        if (stack.empty() && dataPos == data.length())
            return finalizeMessages(until, messages, -1);
    }
}

void QV4ProfilerAdapter::receiveData(
        const QVector<QV4::Profiling::FunctionCallProperties> &new_data,
        const QVector<QV4::Profiling::MemoryAllocationProperties> &new_memory_data)
{
    // In rare cases it could be that another flush or stop event is processed while data from
    // the previous one is still pending. In that case we just append the data.

    if (data.isEmpty())
        data = new_data;
    else
        data.append(new_data);

    if (memory_data.isEmpty())
        memory_data = new_memory_data;
    else
        memory_data.append(new_memory_data);

    service->dataReady(this);
}

QT_END_NAMESPACE
