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

QT_BEGIN_NAMESPACE

// instance will be set, unset in constructor. Allows static methods to be inlined.
QQmlProfilerService *QQmlProfilerService::m_instance = 0;
Q_GLOBAL_STATIC(QQmlProfilerService, profilerInstance)
bool QQmlProfilerService::enabled = false;

// convert to QByteArrays that can be sent to the debug client
// use of QDataStream can skew results
//     (see tst_qqmldebugtrace::trace() benchmark)
void QQmlProfilerData::toByteArrays(QList<QByteArray> &messages) const
{
    QByteArray data;
    Q_ASSERT_X(((messageType | detailType) & (1 << 31)) == 0, Q_FUNC_INFO, "You can use at most 31 message types and 31 detail types.");
    for (uint decodedMessageType = 0; (messageType >> decodedMessageType) != 0; ++decodedMessageType) {
        if ((messageType & (1 << decodedMessageType)) == 0)
            continue;

        for (uint decodedDetailType = 0; (detailType >> decodedDetailType) != 0; ++decodedDetailType) {
            if ((detailType & (1 << decodedDetailType)) == 0)
                continue;

            //### using QDataStream is relatively expensive
            QQmlDebugStream ds(&data, QIODevice::WriteOnly);
            ds << time << decodedMessageType << decodedDetailType;

            switch (decodedMessageType) {
            case QQmlProfilerService::Event:
                if (decodedDetailType == (int)QQmlProfilerService::AnimationFrame)
                    ds << framerate << count;
                break;
            case QQmlProfilerService::RangeStart:
                if (decodedDetailType == (int)QQmlProfilerService::Binding)
                    ds << bindingType;
                break;
            case QQmlProfilerService::RangeData:
                ds << detailString;
                break;
            case QQmlProfilerService::RangeLocation:
                ds << (detailUrl.isEmpty() ? detailString : detailUrl.toString()) << x << y;
                break;
            case QQmlProfilerService::RangeEnd: break;
            case QQmlProfilerService::PixmapCacheEvent:
                ds << detailUrl.toString();
                switch (decodedDetailType) {
                    case QQmlProfilerService::PixmapSizeKnown: ds << x << y; break;
                    case QQmlProfilerService::PixmapReferenceCountChanged: ds << count; break;
                    case QQmlProfilerService::PixmapCacheCountChanged: ds << count; break;
                    default: break;
                }
                break;
            case QQmlProfilerService::SceneGraphFrame:
                switch (decodedDetailType) {
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
                    // WindowsRenderWindow: polish time; always comes packed after a RenderLoop
                    case QQmlProfilerService::SceneGraphWindowsPolishFrame: ds << subtime_4; break;
                    default:break;
                }
                break;
            case QQmlProfilerService::Complete: break;
            }
            messages << data;
            data.clear();
        }
    }
}

void QQmlProfilerService::animationTimerCallback(qint64 delta)
{
    Q_QML_PROFILE(animationFrame(delta));
}

QQmlProfilerService::QQmlProfilerService()
    : QQmlConfigurableDebugService(QStringLiteral("CanvasFrameRate"), 1)
{
    m_timer.start();

    QMutexLocker lock(configMutex());
    // TODO: This is problematic as the service could be enabled at a later point in time. In that
    //       case we might miss the callback registration.
    if (state() == Enabled)
        QUnifiedTimer::instance()->registerProfilerCallback(&animationTimerCallback);
}

QQmlProfilerService::~QQmlProfilerService()
{
    enabled = false;
    m_instance = 0;
}

QQmlProfilerService *QQmlProfilerService::instance()
{
    // just make sure that the service is properly registered
    m_instance = profilerInstance();
    return m_instance;
}

bool QQmlProfilerService::startProfiling()
{
    return profilerInstance()->startProfilingImpl();
}

bool QQmlProfilerService::stopProfiling()
{
    return profilerInstance()->stopProfilingImpl();
}

void QQmlProfilerService::sendProfilingData()
{
    profilerInstance()->sendMessages();
}

bool QQmlProfilerService::startProfilingImpl()
{
    if (QQmlDebugService::isDebuggingEnabled() && !enabled) {
        enabled = true;
        QList<QByteArray> messages;
        QQmlProfilerData(m_timer.nsecsElapsed(), 1 << Event, 1 << StartTrace).toByteArrays(messages);
        QQmlDebugService::sendMessages(messages);
        return true;
    } else {
        return false;
    }
}

bool QQmlProfilerService::stopProfilingImpl()
{
    if (enabled) {
        enabled = false;
        // We cannot use instance here as this is called from the debugger thread.
        // It may be called before the QML engine (and the profiler) is ready.
        processMessage(QQmlProfilerData(m_timer.nsecsElapsed(), 1 << Event, 1 << EndTrace));
        return true;
    } else {
        return false;
    }
}

/*
    Send the messages queued up by processMessage
*/
void QQmlProfilerService::sendMessages()
{
    QMutexLocker locker(&m_dataMutex);

    QList<QByteArray> messages;
    for (int i = 0; i < m_data.count(); ++i)
        m_data.at(i).toByteArrays(messages);
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
    QMutexLocker lock(configMutex());

    if (state() == newState)
        return;

    if (newState != Enabled && enabled) {
        stopProfilingImpl();
        sendMessages();
    }
}

void QQmlProfilerService::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(configMutex());

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

    stopWaiting();
}

/*!
 * \fn void QQmlVmeProfiler::Data::clear()
 * Resets the profiling data to defaults.
 */

/*!
 * \fn bool QQmlVmeProfiler::startBackground(const QString &typeName)
 * If profiling is enabled clears the current range data, then stops the
 * profiler previously running in the foreground if any, then starts a new one
 * in the background, setting the given typeName. \a typeName is the type of
 * object being created.
 */

/*!
 * \fn bool QQmlVmeProfiler::start(const QString &typeName, const QUrl &url, int line, int column)
 * If profiling is enabled clears the current range data, then stops the
 * profiler previously running in the foreground if any, then starts a new one
 * in the foreground, setting the given location. \a url is the URL of
 * file being executed, \line line is the current line in in that file, and
 * \a column is the current column in that file.
 */

/*!
 * \fn bool QQmlVmeProfiler::pop()
 * Stops the currently running profiler, if any, then retrieves an old one from the stack
 * of paused profilers and starts that if possible.
 */

/*!
 * \fn void QQmlVmeProfiler::push()
 * Pushes the currently running profiler on the stack of paused profilers. Note: The profiler
 * isn't paused here. That's a separate step. If it's never paused, but pop()'ed later that
 * won't do any harm, though.
 */

/*!
 * \fn void QQmlVmeProfiler::clear()
 * Stops the currently running (foreground and background) profilers and removes all saved
 * data about paused profilers.
 */

/*!
 * \fn void QQmlVmeProfiler::stop()
 * Stop profiler running in the foreground, if any.
 */

/*!
 * \fn bool QQmlVmeProfiler::foreground(const QUrl &url, int line, int column)
 * Stops the profiler currently running in the foreground, if any and puts the
 * next profiler from the background in its place if there are any profilers in
 * the background. Additionally the rangeLocation is set. \a url is the URL of
 * file being executed, \line line is the current line in in that file, and
 * \a column is the current column in that file.
 */

QT_END_NAMESPACE
