/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeprofilerservice_p.h"

#include <QtCore/qdatastream.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>

// this contains QUnifiedTimer
#include <private/qabstractanimation_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QDeclarativeProfilerService, profilerInstance)

QDeclarativeBindingProfiler::QDeclarativeBindingProfiler(const QString &url, int line, int column)
{
    QDeclarativeProfilerService::startRange(QDeclarativeProfilerService::Binding);
    QDeclarativeProfilerService::rangeLocation(QDeclarativeProfilerService::Binding, url, line, column);
}

QDeclarativeBindingProfiler::~QDeclarativeBindingProfiler()
{
    QDeclarativeProfilerService::endRange(QDeclarativeProfilerService::Binding);
}

void QDeclarativeBindingProfiler::addDetail(const QString &details)
{
    QDeclarativeProfilerService::rangeData(QDeclarativeProfilerService::Binding, details);
}

// convert to a QByteArray that can be sent to the debug client
// use of QDataStream can skew results
//     (see tst_qdeclarativedebugtrace::trace() benchmark)
QByteArray QDeclarativeProfilerData::toByteArray() const
{
    QByteArray data;
    //### using QDataStream is relatively expensive
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << time << messageType << detailType;
    if (messageType == (int)QDeclarativeProfilerService::RangeData)
        ds << detailData;
    if (messageType == (int)QDeclarativeProfilerService::RangeLocation)
        ds << detailData << line << column;
    if (messageType == (int)QDeclarativeProfilerService::Event &&
            detailType == (int)QDeclarativeProfilerService::AnimationFrame)
        ds << framerate << animationcount;
    return data;
}

QDeclarativeProfilerService::QDeclarativeProfilerService()
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

QDeclarativeProfilerService::~QDeclarativeProfilerService()
{
}

void QDeclarativeProfilerService::initialize()
{
    // just make sure that the service is properly registered
    profilerInstance();
}

bool QDeclarativeProfilerService::startProfiling()
{
    return profilerInstance()->startProfilingImpl();
}

bool QDeclarativeProfilerService::stopProfiling()
{
    return profilerInstance()->stopProfilingImpl();
}

void QDeclarativeProfilerService::sendStartedProfilingMessage()
{
    profilerInstance()->sendStartedProfilingMessageImpl();
}

void QDeclarativeProfilerService::addEvent(EventType t)
{
    profilerInstance()->addEventImpl(t);
}

void QDeclarativeProfilerService::startRange(RangeType t)
{
    profilerInstance()->startRangeImpl(t);
}

void QDeclarativeProfilerService::rangeData(RangeType t, const QString &data)
{
    profilerInstance()->rangeDataImpl(t, data);
}

void QDeclarativeProfilerService::rangeLocation(RangeType t, const QString &fileName, int line, int column)
{
    profilerInstance()->rangeLocationImpl(t, fileName, line, column);
}

void QDeclarativeProfilerService::rangeLocation(RangeType t, const QUrl &fileName, int line, int column)
{
    profilerInstance()->rangeLocationImpl(t, fileName, line, column);
}

void QDeclarativeProfilerService::endRange(RangeType t)
{
    profilerInstance()->endRangeImpl(t);
}

void QDeclarativeProfilerService::animationFrame(qint64 delta)
{
    profilerInstance()->animationFrameImpl(delta);
}

void QDeclarativeProfilerService::sendProfilingData()
{
    profilerInstance()->sendMessages();
}

bool QDeclarativeProfilerService::startProfilingImpl()
{
    bool success = false;
    if (!profilingEnabled()) {
        setProfilingEnabled(true);
        sendStartedProfilingMessageImpl();
        success = true;
    }
    return success;
}

bool QDeclarativeProfilerService::stopProfilingImpl()
{
    bool success = false;
    if (profilingEnabled()) {
        addEventImpl(EndTrace);
        setProfilingEnabled(false);
        success = true;
    }
    return success;
}

void QDeclarativeProfilerService::sendStartedProfilingMessageImpl()
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData ed = {m_timer.nsecsElapsed(), (int)Event, (int)StartTrace, QString(), -1, -1, 0, 0};
    QDeclarativeDebugService::sendMessage(ed.toByteArray());
}

void QDeclarativeProfilerService::addEventImpl(EventType event)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData ed = {m_timer.nsecsElapsed(), (int)Event, (int)event, QString(), -1, -1, 0, 0};
    processMessage(ed);
}

void QDeclarativeProfilerService::startRangeImpl(RangeType range)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeStart, (int)range, QString(), -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeProfilerService::rangeDataImpl(RangeType range, const QString &rData)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeData, (int)range, rData, -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeProfilerService::rangeDataImpl(RangeType range, const QUrl &rData)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeData, (int)range, rData.toString(QUrl::FormattingOption(0x100)), -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeProfilerService::rangeLocationImpl(RangeType range, const QString &fileName, int line, int column)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeLocation, (int)range, fileName, line, column, 0, 0};
    processMessage(rd);
}

void QDeclarativeProfilerService::rangeLocationImpl(RangeType range, const QUrl &fileName, int line, int column)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeLocation, (int)range, fileName.toString(QUrl::FormattingOption(0x100)), line, column, 0, 0};
    processMessage(rd);
}

void QDeclarativeProfilerService::endRangeImpl(RangeType range)
{
    if (!QDeclarativeDebugService::isDebuggingEnabled() || !m_enabled)
        return;

    QDeclarativeProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeEnd, (int)range, QString(), -1, -1, 0, 0};
    processMessage(rd);
}

void QDeclarativeProfilerService::animationFrameImpl(qint64 delta)
{
    Q_ASSERT(QDeclarativeDebugService::isDebuggingEnabled());
    if (!m_enabled)
        return;

    int animCount = QUnifiedTimer::instance()->runningAnimationCount();

    if (animCount > 0 && delta > 0) {
        // trim fps to integer
        int fps = 1000 / delta;
        QDeclarativeProfilerData ed = {m_timer.nsecsElapsed(), (int)Event, (int)AnimationFrame, QString(), -1, -1, fps, animCount};
        processMessage(ed);
    }
}

/*
    Either send the message directly, or queue up
    a list of messages to send later (via sendMessages)
*/
void QDeclarativeProfilerService::processMessage(const QDeclarativeProfilerData &message)
{
    QMutexLocker locker(&m_mutex);
    m_data.append(message);
}

bool QDeclarativeProfilerService::profilingEnabled()
{
    return m_enabled;
}

void QDeclarativeProfilerService::setProfilingEnabled(bool enable)
{
    m_enabled = enable;
}

/*
    Send the messages queued up by processMessage
*/
void QDeclarativeProfilerService::sendMessages()
{
    QMutexLocker locker(&m_mutex);
    QList<QByteArray> messages;
    for (int i = 0; i < m_data.count(); ++i)
        messages << m_data.at(i).toByteArray();
    m_data.clear();

    //indicate completion
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << (qint64)-1 << (int)Complete;
    messages << data;

    QDeclarativeDebugService::sendMessages(messages);
}

void QDeclarativeProfilerService::stateAboutToBeChanged(QDeclarativeDebugService::State newState)
{
    if (state() == newState)
        return;

    if (state() == Enabled
            && m_enabled) {
        stopProfilingImpl();
        sendMessages();
    }
}

void QDeclarativeProfilerService::messageReceived(const QByteArray &message)
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
