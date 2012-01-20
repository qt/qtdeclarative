/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativedebugtrace_p.h"

#include <QtCore/qdatastream.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>

// this contains QUnifiedTimer
#include <private/qabstractanimation_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QDeclarativeDebugTrace, traceInstance)

// convert to a QByteArray that can be sent to the debug client
// use of QDataStream can skew results
//     (see tst_qdeclarativedebugtrace::trace() benchmark)
QByteArray QDeclarativeDebugData::toByteArray() const
{
    QByteArray data;
    //### using QDataStream is relatively expensive
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << time << messageType << detailType;
    if (messageType == (int)QDeclarativeDebugTrace::RangeData)
        ds << detailData;
    if (messageType == (int)QDeclarativeDebugTrace::RangeLocation)
        ds << detailData << line << column;
    if (messageType == (int)QDeclarativeDebugTrace::Event &&
            detailType == (int)QDeclarativeDebugTrace::AnimationFrame)
        ds << framerate << animationcount;
    return data;
}

QDeclarativeDebugTrace::QDeclarativeDebugTrace()
    : QDeclarativeDebugService(QLatin1String("CanvasFrameRate"), 1),
      m_enabled(false), m_messageReceived(false)
{
    m_timer.start();

    if (registerService() == Enabled) {
        // wait for first message indicating whether to trace or not
        while (!m_messageReceived)
            waitForMessage();

        QUnifiedTimer::instance()->registerProfilerCallback( &animationFrame );
    }
}

QDeclarativeDebugTrace::~QDeclarativeDebugTrace()
{
}

void QDeclarativeDebugTrace::initialize()
{
    // just make sure that the service is properly registered
    traceInstance();
}

bool QDeclarativeDebugTrace::startProfiling()
{
    return traceInstance()->startProfilingImpl();
}

bool QDeclarativeDebugTrace::stopProfiling()
{
    return traceInstance()->stopProfilingImpl();
}

void QDeclarativeDebugTrace::addEvent(EventType t)
{
    traceInstance()->addEventImpl(t);
}

void QDeclarativeDebugTrace::startRange(RangeType t)
{
    traceInstance()->startRangeImpl(t);
}

void QDeclarativeDebugTrace::rangeData(RangeType t, const QString &data)
{
    traceInstance()->rangeDataImpl(t, data);
}

void QDeclarativeDebugTrace::rangeData(RangeType t, const QUrl &data)
{
    traceInstance()->rangeDataImpl(t, data);
}

void QDeclarativeDebugTrace::rangeLocation(RangeType t, const QString &fileName, int line, int column)
{
    traceInstance()->rangeLocationImpl(t, fileName, line, column);
}

void QDeclarativeDebugTrace::rangeLocation(RangeType t, const QUrl &fileName, int line, int column)
{
    traceInstance()->rangeLocationImpl(t, fileName, line, column);
}

void QDeclarativeDebugTrace::endRange(RangeType t)
{
    traceInstance()->endRangeImpl(t);
}

void QDeclarativeDebugTrace::animationFrame(qint64 delta)
{
    traceInstance()->animationFrameImpl(delta);
}

void QDeclarativeDebugTrace::sendProfilingData()
{
    traceInstance()->sendMessages();
}

bool QDeclarativeDebugTrace::startProfilingImpl()
{
    bool success = false;
    if (!profilingEnabled()) {
        setProfilingEnabled(true);
        addEventImpl(StartTrace);
        success = true;
    }
    return success;
}

bool QDeclarativeDebugTrace::stopProfilingImpl()
{
    bool success = false;
    if (profilingEnabled()) {
        addEventImpl(EndTrace);
        setProfilingEnabled(false);
        success = true;
    }
    return success;
}

void QDeclarativeDebugTrace::addEventImpl(EventType event)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData ed = {m_timer.nsecsElapsed(), (int)Event, (int)event, QString(), -1, -1, 0, 0};
    processMessage(ed);
}

void QDeclarativeDebugTrace::startRangeImpl(RangeType range)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData rd = {m_timer.nsecsElapsed(), (int)RangeStart, (int)range, QString(), -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeDebugTrace::rangeDataImpl(RangeType range, const QString &rData)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData rd = {m_timer.nsecsElapsed(), (int)RangeData, (int)range, rData, -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeDebugTrace::rangeDataImpl(RangeType range, const QUrl &rData)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData rd = {m_timer.nsecsElapsed(), (int)RangeData, (int)range, rData.toString(QUrl::FormattingOption(0x100)), -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeDebugTrace::rangeLocationImpl(RangeType range, const QString &fileName, int line, int column)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData rd = {m_timer.nsecsElapsed(), (int)RangeLocation, (int)range, fileName, line, column, 0, 0};
    processMessage(rd);
}

void QDeclarativeDebugTrace::rangeLocationImpl(RangeType range, const QUrl &fileName, int line, int column)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData rd = {m_timer.nsecsElapsed(), (int)RangeLocation, (int)range, fileName.toString(QUrl::FormattingOption(0x100)), line, column, 0, 0};
    processMessage(rd);
}

void QDeclarativeDebugTrace::endRangeImpl(RangeType range)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeDebugData rd = {m_timer.nsecsElapsed(), (int)RangeEnd, (int)range, QString(), -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeDebugTrace::animationFrameImpl(qint64 delta)
{
    Q_ASSERT(QDeclarativeDebugService::isDebuggingEnabled());
    if (!m_enabled)
        return;

    int animCount = QUnifiedTimer::instance()->runningAnimationCount();

    if (animCount > 0 && delta > 0) {
        // trim fps to integer
        int fps = 1000 / delta;
        QDeclarativeDebugData ed = {m_timer.nsecsElapsed(), (int)Event, (int)AnimationFrame, QString(), -1, -1, fps, animCount};
        processMessage(ed);
    }
}

/*
    Either send the message directly, or queue up
    a list of messages to send later (via sendMessages)
*/
void QDeclarativeDebugTrace::processMessage(const QDeclarativeDebugData &message)
{
    QMutexLocker locker(&m_mutex);
    m_data.append(message);
}

bool QDeclarativeDebugTrace::profilingEnabled()
{
    return m_enabled;
}

void QDeclarativeDebugTrace::setProfilingEnabled(bool enable)
{
    m_enabled = enable;
}

/*
    Send the messages queued up by processMessage
*/
void QDeclarativeDebugTrace::sendMessages()
{
    QMutexLocker locker(&m_mutex);
    QList<QByteArray> messages;
    for (int i = 0; i < m_data.count(); ++i)
        messages << m_data.at(i).toByteArray();
    QDeclarativeDebugService::sendMessages(messages);
    m_data.clear();

    //indicate completion
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << (qint64)-1 << (int)Complete;
    sendMessage(data);
}

void QDeclarativeDebugTrace::messageReceived(const QByteArray &message)
{
    QByteArray rwData = message;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    bool enabled;
    stream >> enabled;

    m_messageReceived = true;

    if (enabled) {
        startProfilingImpl();
    } else {
        if (stopProfilingImpl())
            sendMessages();
    }
}

QT_END_NAMESPACE
