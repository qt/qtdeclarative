/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qqmlprofilerservice_p.h"

#include <QtCore/qdatastream.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>

// this contains QUnifiedTimer
#include <private/qabstractanimation_p.h>

QT_BEGIN_NAMESPACE

// instance will be set, unset in constructor. Allows static methods to be inlined.
QQmlProfilerService *QQmlProfilerService::instance = 0;
Q_GLOBAL_STATIC(QQmlProfilerService, profilerInstance)
bool QQmlProfilerService::enabled = false;

// convert to a QByteArray that can be sent to the debug client
// use of QDataStream can skew results
//     (see tst_qqmldebugtrace::trace() benchmark)
QByteArray QQmlProfilerData::toByteArray() const
{
    QByteArray data;
    //### using QDataStream is relatively expensive
    QQmlDebugStream ds(&data, QIODevice::WriteOnly);
    ds << time << messageType << detailType;
    if (messageType == (int)QQmlProfilerService::RangeStart &&
            detailType == (int)QQmlProfilerService::Binding)
        ds << bindingType;
    if (messageType == (int)QQmlProfilerService::RangeData)
        ds << detailData;
    if (messageType == (int)QQmlProfilerService::RangeLocation)
        ds << detailData << line << column;
    if (messageType == (int)QQmlProfilerService::Event &&
            detailType == (int)QQmlProfilerService::AnimationFrame)
        ds << framerate << animationcount;
    if (messageType == (int)QQmlProfilerService::PixmapCacheEvent) {
        ds << detailData;
        switch (detailType) {
        case QQmlProfilerService::PixmapSizeKnown: ds << line << column; break;
        case QQmlProfilerService::PixmapReferenceCountChanged: ds << animationcount; break;
        case QQmlProfilerService::PixmapCacheCountChanged: ds << animationcount; break;
        default: break;
        }
    }
    if (messageType == (int)QQmlProfilerService::SceneGraphFrame) {
        switch (detailType) {
        // RendererFrame: preprocessTime, updateTime, bindingTime, renderTime
        case QQmlProfilerService::SceneGraphRendererFrame: ds << subtime_1 << subtime_2 << subtime_3 << subtime_4; break;
        // AdaptationLayerFrame: glyphCount (which is an integer), glyphRenderTime, glyphStoreTime
        case QQmlProfilerService::SceneGraphAdaptationLayerFrame: ds << (int)subtime_1 << subtime_2 << subtime_3; break;
        // ContextFrame: compiling material time
        case QQmlProfilerService::SceneGraphContextFrame: ds << subtime_1; break;
        // RenderLoop: syncTime, renderTime, swapTime
        case QQmlProfilerService::SceneGraphRenderLoopFrame: ds << subtime_1 << subtime_2 << subtime_3; break;
        // TexturePrepare: bind, convert, swizzle, upload, mipmap
        case QQmlProfilerService::SceneGraphTexturePrepare: ds << subtime_1 << subtime_2 << subtime_3 << subtime_4 << subtime_5; break;
        // TextureDeletion: deletionTime
        case QQmlProfilerService::SceneGraphTextureDeletion: ds << subtime_1; break;
        // PolishAndSync: polishTime, waitTime, syncTime, animationsTime,
        case QQmlProfilerService::SceneGraphPolishAndSync: ds << subtime_1 << subtime_2 << subtime_3 << subtime_4; break;
        // WindowsRenderLoop: GL time, make current time, SceneGraph time
        case QQmlProfilerService::SceneGraphWindowsRenderShow: ds << subtime_1 << subtime_2 << subtime_3; break;
        // WindowsAnimations: update time
        case QQmlProfilerService::SceneGraphWindowsAnimations: ds << subtime_1; break;
        // WindowsRenderWindow: polish time
        case QQmlProfilerService::SceneGraphWindowsPolishFrame: ds << subtime_1; break;
        default:break;
        }
    }

    return data;
}

QQmlProfilerService::QQmlProfilerService()
    : QQmlDebugService(QStringLiteral("CanvasFrameRate"), 1)
{
    m_timer.start();

    // don't execute stateAboutToBeChanged(), messageReceived() in parallel
    QMutexLocker lock(&m_initializeMutex);

    if (registerService() == Enabled) {
        QUnifiedTimer::instance()->registerProfilerCallback(&animationFrame);
        if (blockingMode())
            m_initializeCondition.wait(&m_initializeMutex);
    }
}

QQmlProfilerService::~QQmlProfilerService()
{
    instance = 0;
}

void QQmlProfilerService::initialize()
{
    // just make sure that the service is properly registered
    instance = profilerInstance();
}

bool QQmlProfilerService::startProfiling()
{
    return profilerInstance()->startProfilingImpl();
}

bool QQmlProfilerService::stopProfiling()
{
    return profilerInstance()->stopProfilingImpl();
}

void QQmlProfilerService::sendStartedProfilingMessage()
{
    profilerInstance()->sendStartedProfilingMessageImpl();
}

void QQmlProfilerService::addEvent(EventType t)
{
    profilerInstance()->addEventImpl(t);
}

void QQmlProfilerService::animationFrame(qint64 delta)
{
    profilerInstance()->animationFrameImpl(delta);
}

void QQmlProfilerService::sceneGraphFrame(SceneGraphFrameType frameType, qint64 value1, qint64 value2, qint64 value3, qint64 value4, qint64 value5)
{
    profilerInstance()->sceneGraphFrameImpl(frameType, value1, value2, value3, value4, value5);
}

void QQmlProfilerService::sendProfilingData()
{
    profilerInstance()->sendMessages();
}

bool QQmlProfilerService::startProfilingImpl()
{
    bool success = false;
    if (!profilingEnabled()) {
        setProfilingEnabled(true);
        sendStartedProfilingMessageImpl();
        success = true;
    }
    return success;
}

bool QQmlProfilerService::stopProfilingImpl()
{
    bool success = false;
    if (profilingEnabled()) {
        addEventImpl(EndTrace);
        setProfilingEnabled(false);
        success = true;
    }
    return success;
}

void QQmlProfilerService::sendStartedProfilingMessageImpl()
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData ed = {m_timer.nsecsElapsed(), (int)Event, (int)StartTrace,
                           QString(), -1, -1, 0, 0, 0,
                           0, 0, 0, 0, 0};
    QQmlDebugService::sendMessage(ed.toByteArray());
}

void QQmlProfilerService::addEventImpl(EventType event)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData ed = {m_timer.nsecsElapsed(), (int)Event, (int)event,
                           QString(), -1, -1, 0, 0, 0,
                           0, 0, 0, 0, 0};
    processMessage(ed);
}

void QQmlProfilerService::startRange(RangeType range, BindingType bindingType)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeStart, (int)range,
                           QString(), -1, -1, 0, 0, (int)bindingType,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::rangeData(RangeType range, const QString &rData)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeData, (int)range,
                           rData, -1, -1, 0, 0, 0,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::rangeData(RangeType range, const QUrl &rData)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeData, (int)range,
                           rData.toString(), -1, -1, 0, 0, 0,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::rangeLocation(RangeType range, const QString &fileName, int line, int column)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeLocation, (int)range,
                           fileName, line, column, 0, 0, 0,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::rangeLocation(RangeType range, const QUrl &fileName, int line, int column)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeLocation, (int)range,
                           fileName.toString(), line, column, 0, 0, 0,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::endRange(RangeType range)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)RangeEnd, (int)range,
                           QString(), -1, -1, 0, 0, 0,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::pixmapEventImpl(PixmapEventType eventType, const QUrl &url)
{
    // assuming enabled checked by caller
    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)PixmapCacheEvent, (int)eventType,
                           url.toString(), -1, -1, -1, -1, -1,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::pixmapEventImpl(PixmapEventType eventType, const QUrl &url, int width, int height)
{
    // assuming enabled checked by caller
    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)PixmapCacheEvent, (int)eventType,
                           url.toString(), width, height, -1, -1, -1,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::pixmapEventImpl(PixmapEventType eventType, const QUrl &url, int count)
{
    // assuming enabled checked by caller
    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)PixmapCacheEvent, (int)eventType,
                           url.toString(), -1, -1, -1, count, -1,
                           0, 0, 0, 0, 0};
    processMessage(rd);
}

void QQmlProfilerService::sceneGraphFrameImpl(SceneGraphFrameType frameType, qint64 value1, qint64 value2, qint64 value3, qint64 value4, qint64 value5)
{
    if (!QQmlDebugService::isDebuggingEnabled() || !enabled)
        return;

    // because I already have some space to store ints in the struct, I'll use it to store the frame data
    // even though the field names do not match
    QQmlProfilerData rd = {m_timer.nsecsElapsed(), (int)SceneGraphFrame, (int)frameType, QString(),
                           -1, -1, -1, -1, -1,
                           value1, value2, value3, value4, value5};
    processMessage(rd);
}

void QQmlProfilerService::animationFrameImpl(qint64 delta)
{
    Q_ASSERT(QQmlDebugService::isDebuggingEnabled());
    if (!enabled)
        return;

    int animCount = QUnifiedTimer::instance()->runningAnimationCount();

    if (animCount > 0 && delta > 0) {
        // trim fps to integer
        int fps = 1000 / delta;
        QQmlProfilerData ed = {m_timer.nsecsElapsed(), (int)Event, (int)AnimationFrame,
                               QString(), -1, -1, fps, animCount, 0,
                               0, 0, 0, 0, 0};
        processMessage(ed);
    }
}

/*
    Either send the message directly, or queue up
    a list of messages to send later (via sendMessages)
*/
void QQmlProfilerService::processMessage(const QQmlProfilerData &message)
{
    QMutexLocker locker(&m_dataMutex);
    m_data.append(message);
}

bool QQmlProfilerService::profilingEnabled()
{
    return enabled;
}

void QQmlProfilerService::setProfilingEnabled(bool enable)
{
    enabled = enable;
}

/*
    Send the messages queued up by processMessage
*/
void QQmlProfilerService::sendMessages()
{
    QMutexLocker locker(&m_dataMutex);

    QList<QByteArray> messages;
    for (int i = 0; i < m_data.count(); ++i)
        messages << m_data.at(i).toByteArray();
    m_data.clear();

    //indicate completion
    QByteArray data;
    QQmlDebugStream ds(&data, QIODevice::WriteOnly);
    ds << (qint64)-1 << (int)Complete;
    messages << data;

    QQmlDebugService::sendMessages(messages);
}

void QQmlProfilerService::stateAboutToBeChanged(QQmlDebugService::State newState)
{
    QMutexLocker lock(&m_initializeMutex);

    if (state() == newState)
        return;

    if (state() == Enabled
            && enabled) {
        stopProfilingImpl();
        sendMessages();
    }

    if (state() != Enabled) {
        // wake up constructor in blocking mode
        // (we might got disabled before first message arrived)
        m_initializeCondition.wakeAll();
    }
}

void QQmlProfilerService::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(&m_initializeMutex);

    QByteArray rwData = message;
    QQmlDebugStream stream(&rwData, QIODevice::ReadOnly);

    bool enabled;
    stream >> enabled;

    if (enabled) {
        startProfilingImpl();
    } else {
        if (stopProfilingImpl())
            sendMessages();
    }

    // wake up constructor in blocking mode
    m_initializeCondition.wakeAll();
}

QT_END_NAMESPACE
