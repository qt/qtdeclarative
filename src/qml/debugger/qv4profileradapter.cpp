/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4profileradapter_p.h"
#include "qqmlprofilerservice_p.h"
#include "qqmldebugservice_p.h"

QT_BEGIN_NAMESPACE

QV4ProfilerAdapter::QV4ProfilerAdapter(QQmlProfilerService *service, QV4::ExecutionEngine *engine) :
    QQmlAbstractProfilerAdapter(service)
{
    engine->enableProfiler();
    connect(this, SIGNAL(profilingEnabled()), engine->profiler, SLOT(startProfiling()));
    connect(this, SIGNAL(profilingEnabledWhileWaiting()), engine->profiler, SLOT(startProfiling()),
            Qt::DirectConnection);
    connect(this, SIGNAL(profilingDisabled()), engine->profiler, SLOT(stopProfiling()));
    connect(this, SIGNAL(profilingDisabledWhileWaiting()), engine->profiler, SLOT(stopProfiling()),
            Qt::DirectConnection);
    connect(this, SIGNAL(dataRequested()), engine->profiler, SLOT(reportData()));
    connect(this, SIGNAL(referenceTimeKnown(QElapsedTimer)),
            engine->profiler, SLOT(setTimer(QElapsedTimer)));
    connect(engine->profiler, SIGNAL(dataReady(QList<QV4::Profiling::FunctionCallProperties>)),
            this, SLOT(receiveData(QList<QV4::Profiling::FunctionCallProperties>)));
}


qint64 QV4ProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    QByteArray message;
    while (true) {
        while (!stack.empty() && (data.empty() || stack.top() <= data.front().start)) {
            if (stack.top() > until)
                return stack.top();
            QQmlDebugStream d(&message, QIODevice::WriteOnly);
            d << stack.pop() << RangeEnd << Javascript;
            messages.append(message);
        }
        while (!data.empty() && (stack.empty() || data.front().start < stack.top())) {
            if (data.front().start > until)
                return data.front().start;
            const QV4::Profiling::FunctionCallProperties &props = data.front();
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
            data.pop_front();
        }
        if (stack.empty() && data.empty())
            return -1;
    }
}

void QV4ProfilerAdapter::receiveData(const QList<QV4::Profiling::FunctionCallProperties> &new_data)
{
    data = new_data;
    stack.clear();
    service->dataReady(this);
}

QT_END_NAMESPACE
