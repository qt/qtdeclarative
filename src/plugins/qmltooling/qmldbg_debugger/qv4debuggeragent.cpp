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

#include "qv4debuggeragent.h"
#include "qv4debugservice.h"
#include "qv4datacollector.h"

#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

QT_BEGIN_NAMESPACE

QV4DebuggerAgent::QV4DebuggerAgent(QV4DebugServiceImpl *debugService)
    : m_breakOnThrow(false), m_debugService(debugService)
{}

QV4::Debugging::V4Debugger *QV4DebuggerAgent::firstDebugger() const
{
    // Currently only 1 single engine is supported, so:
    if (m_debuggers.isEmpty())
        return 0;
    else
        return m_debuggers.first();
}

bool QV4DebuggerAgent::isRunning() const
{
    // Currently only 1 single engine is supported, so:
    if (QV4::Debugging::V4Debugger *debugger = firstDebugger())
        return debugger->state() == QV4::Debugging::V4Debugger::Running;
    else
        return false;
}

void QV4DebuggerAgent::debuggerPaused(QV4::Debugging::V4Debugger *debugger,
                                      QV4::Debugging::PauseReason reason)
{
    Q_UNUSED(reason);

    m_debugService->clearHandles(debugger->engine());

    QJsonObject event, body, script;
    event.insert(QStringLiteral("type"), QStringLiteral("event"));

    switch (reason) {
    case QV4::Debugging::Step:
    case QV4::Debugging::PauseRequest:
    case QV4::Debugging::BreakPoint: {
        event.insert(QStringLiteral("event"), QStringLiteral("break"));
        QVector<QV4::StackFrame> frames = debugger->stackTrace(1);
        if (frames.isEmpty())
            break;

        const QV4::StackFrame &topFrame = frames.first();
        body.insert(QStringLiteral("invocationText"), topFrame.function);
        body.insert(QStringLiteral("sourceLine"), topFrame.line - 1);
        if (topFrame.column > 0)
            body.insert(QStringLiteral("sourceColumn"), topFrame.column);
        QJsonArray breakPoints;
        foreach (int breakPointId, breakPointIds(topFrame.source, topFrame.line))
            breakPoints.push_back(breakPointId);
        body.insert(QStringLiteral("breakpoints"), breakPoints);
        script.insert(QStringLiteral("name"), topFrame.source);
    } break;
    case QV4::Debugging::Throwing:
        // TODO: complete this!
        event.insert(QStringLiteral("event"), QStringLiteral("exception"));
        break;
    }

    if (!script.isEmpty())
        body.insert(QStringLiteral("script"), script);
    if (!body.isEmpty())
        event.insert(QStringLiteral("body"), body);
    m_debugService->send(event);
}

void QV4DebuggerAgent::sourcesCollected(QV4::Debugging::V4Debugger *debugger,
                                        const QStringList &sources, int requestSequenceNr)
{
    QJsonArray body;
    foreach (const QString &source, sources) {
        QJsonObject src;
        src[QLatin1String("name")] = source;
        src[QLatin1String("scriptType")] = 4;
        body.append(src);
    }

    QJsonObject response;
    response[QLatin1String("success")] = true;
    response[QLatin1String("running")] = debugger->state() == QV4::Debugging::V4Debugger::Running;
    response[QLatin1String("body")] = body;
    response[QLatin1String("command")] = QStringLiteral("scripts");
    response[QLatin1String("request_seq")] = requestSequenceNr;
    response[QLatin1String("type")] = QStringLiteral("response");
    m_debugService->send(response);
}

void QV4DebuggerAgent::addDebugger(QV4::Debugging::V4Debugger *debugger)
{
    Q_ASSERT(!m_debuggers.contains(debugger));
    m_debuggers << debugger;

    debugger->setBreakOnThrow(m_breakOnThrow);

    foreach (const BreakPoint &breakPoint, m_breakPoints.values())
        if (breakPoint.enabled)
            debugger->addBreakPoint(breakPoint.fileName, breakPoint.lineNr, breakPoint.condition);

    connect(debugger, SIGNAL(destroyed(QObject*)),
            this, SLOT(handleDebuggerDeleted(QObject*)));
    connect(debugger, SIGNAL(sourcesCollected(QV4::Debugging::V4Debugger*,QStringList,int)),
            this, SLOT(sourcesCollected(QV4::Debugging::V4Debugger*,QStringList,int)),
            Qt::QueuedConnection);
    connect(debugger,
            SIGNAL(debuggerPaused(QV4::Debugging::V4Debugger*,QV4::Debugging::PauseReason)),
            this, SLOT(debuggerPaused(QV4::Debugging::V4Debugger*,QV4::Debugging::PauseReason)),
            Qt::QueuedConnection);
}

void QV4DebuggerAgent::removeDebugger(QV4::Debugging::V4Debugger *debugger)
{
    m_debuggers.removeAll(debugger);
    disconnect(debugger, SIGNAL(destroyed(QObject*)),
               this, SLOT(handleDebuggerDeleted(QObject*)));
    disconnect(debugger, SIGNAL(sourcesCollected(QV4::Debugging::V4Debugger*,QStringList,int)),
               this, SLOT(sourcesCollected(QV4::Debugging::V4Debugger*,QStringList,int)));
    disconnect(debugger,
               SIGNAL(debuggerPaused(QV4::Debugging::V4Debugger*,QV4::Debugging::PauseReason)),
               this,
               SLOT(debuggerPaused(QV4::Debugging::V4Debugger*,QV4::Debugging::PauseReason)));
}

void QV4DebuggerAgent::handleDebuggerDeleted(QObject *debugger)
{
    m_debuggers.removeAll(static_cast<QV4::Debugging::V4Debugger *>(debugger));
}

void QV4DebuggerAgent::pause(QV4::Debugging::V4Debugger *debugger) const
{
    debugger->pause();
}

void QV4DebuggerAgent::pauseAll() const
{
    foreach (QV4::Debugging::V4Debugger *debugger, m_debuggers)
        pause(debugger);
}

void QV4DebuggerAgent::resumeAll() const
{
    foreach (QV4::Debugging::V4Debugger *debugger, m_debuggers)
        if (debugger->state() == QV4::Debugging::V4Debugger::Paused)
            debugger->resume(QV4::Debugging::V4Debugger::FullThrottle);
}

int QV4DebuggerAgent::addBreakPoint(const QString &fileName, int lineNumber, bool enabled, const QString &condition)
{
    if (enabled)
        foreach (QV4::Debugging::V4Debugger *debugger, m_debuggers)
            debugger->addBreakPoint(fileName, lineNumber, condition);

    int id = m_breakPoints.size();
    m_breakPoints.insert(id, BreakPoint(fileName, lineNumber, enabled, condition));
    return id;
}

void QV4DebuggerAgent::removeBreakPoint(int id)
{
    BreakPoint breakPoint = m_breakPoints.value(id);
    if (!breakPoint.isValid())
        return;

    m_breakPoints.remove(id);

    if (breakPoint.enabled)
        foreach (QV4::Debugging::V4Debugger *debugger, m_debuggers)
            debugger->removeBreakPoint(breakPoint.fileName, breakPoint.lineNr);
}

void QV4DebuggerAgent::removeAllBreakPoints()
{
    QList<int> ids = m_breakPoints.keys();
    foreach (int id, ids)
        removeBreakPoint(id);
}

void QV4DebuggerAgent::enableBreakPoint(int id, bool onoff)
{
    BreakPoint &breakPoint = m_breakPoints[id];
    if (!breakPoint.isValid() || breakPoint.enabled == onoff)
        return;
    breakPoint.enabled = onoff;

    foreach (QV4::Debugging::V4Debugger *debugger, m_debuggers) {
        if (onoff)
            debugger->addBreakPoint(breakPoint.fileName, breakPoint.lineNr, breakPoint.condition);
        else
            debugger->removeBreakPoint(breakPoint.fileName, breakPoint.lineNr);
    }
}

QList<int> QV4DebuggerAgent::breakPointIds(const QString &fileName, int lineNumber) const
{
    QList<int> ids;

    for (QHash<int, BreakPoint>::const_iterator i = m_breakPoints.begin(), ei = m_breakPoints.end(); i != ei; ++i)
        if (i->lineNr == lineNumber && fileName.endsWith(i->fileName))
            ids.push_back(i.key());

    return ids;
}

void QV4DebuggerAgent::setBreakOnThrow(bool onoff)
{
    if (onoff != m_breakOnThrow) {
        m_breakOnThrow = onoff;
        foreach (QV4::Debugging::V4Debugger *debugger, m_debuggers)
            debugger->setBreakOnThrow(onoff);
    }
}

QT_END_NAMESPACE
