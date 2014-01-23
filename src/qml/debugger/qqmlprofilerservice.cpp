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
#include "qqmldebugserver_p.h"
#include "qv4profileradapter_p.h"
#include <private/qqmlengine_p.h>

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

    // If there is no debug server it doesn't matter as we'll never get enabled anyway.
    if (QQmlDebugServer::instance() != 0)
        moveToThread(QQmlDebugServer::instance()->thread());
}

QQmlProfilerService::~QQmlProfilerService()
{
    // No need to lock here. If any engine or global profiler is still trying to register at this
    // point we have a nasty bug anyway.
    enabled = false;
    m_instance = 0;
    qDeleteAll(m_engineProfilers.keys());
    qDeleteAll(m_globalProfilers);
}

void QQmlProfilerService::dataReady(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(configMutex());
    bool dataComplete = true;
    for (QMultiMap<qint64, QQmlAbstractProfilerAdapter *>::iterator i(m_startTimes.begin()); i != m_startTimes.end();) {
        if (i.value() == profiler) {
            m_startTimes.erase(i++);
        } else {
            if (i.key() == -1)
                dataComplete = false;
            ++i;
        }
    }
    m_startTimes.insert(0, profiler);
    if (dataComplete) {
        QList<QQmlEngine *> enginesToRelease;
        foreach (QQmlEngine *engine, m_stoppingEngines) {
            foreach (QQmlAbstractProfilerAdapter *engineProfiler, m_engineProfilers.values(engine)) {
                if (m_startTimes.values().contains(engineProfiler)) {
                    enginesToRelease.append(engine);
                    break;
                }
            }
        }
        sendMessages();
        foreach (QQmlEngine *engine, enginesToRelease) {
            m_stoppingEngines.removeOne(engine);
            emit detachedFromEngine(engine);
        }
    }
}

QQmlProfilerService *QQmlProfilerService::instance()
{
    // just make sure that the service is properly registered
    m_instance = profilerInstance();
    return m_instance;
}

void QQmlProfilerService::engineAboutToBeAdded(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() != thread(), Q_FUNC_INFO, "QML profilers have to be added from the engine thread");

    QMutexLocker lock(configMutex());
    QQmlProfiler *qmlAdapter = new QQmlProfiler(this);
    QV4ProfilerAdapter *v4Adapter = new QV4ProfilerAdapter(this, QV8Engine::getV4(engine->handle()));
    addEngineProfiler(qmlAdapter, engine);
    addEngineProfiler(v4Adapter, engine);
    QQmlConfigurableDebugService::engineAboutToBeAdded(engine);
}

void QQmlProfilerService::engineAdded(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() != thread(), Q_FUNC_INFO, "QML profilers have to be added from the engine thread");

    QMutexLocker lock(configMutex());
    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine))
        profiler->stopWaiting();
}

void QQmlProfilerService::engineAboutToBeRemoved(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() != thread(), Q_FUNC_INFO, "QML profilers have to be removed from the engine thread");

    QMutexLocker lock(configMutex());
    bool isRunning = false;
    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine)) {
        if (profiler->isRunning())
            isRunning = true;
        profiler->startWaiting();
    }
    if (isRunning) {
        m_stoppingEngines.append(engine);
        stopProfiling(engine);
    } else {
        emit detachedFromEngine(engine);
    }
}

void QQmlProfilerService::engineRemoved(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() != thread(), Q_FUNC_INFO, "QML profilers have to be removed from the engine thread");

    QMutexLocker lock(configMutex());
    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine))
        delete profiler;
    m_engineProfilers.remove(engine);
}

void QQmlProfilerService::addEngineProfiler(QQmlAbstractProfilerAdapter *profiler, QQmlEngine *engine)
{
    profiler->moveToThread(thread());
    profiler->synchronize(m_timer);
    m_engineProfilers.insert(engine, profiler);
}

void QQmlProfilerService::addGlobalProfiler(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(configMutex());
    profiler->synchronize(m_timer);
    m_globalProfilers.append(profiler);
    // Global profiler, not connected to a specific engine.
    // Global profilers are started whenever any engine profiler is started and stopped when
    // all engine profilers are stopped.
    foreach (QQmlAbstractProfilerAdapter *engineProfiler, m_engineProfilers) {
        if (engineProfiler->isRunning()) {
            profiler->startProfiling();
            break;
        }
    }
}

void QQmlProfilerService::removeGlobalProfiler(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(configMutex());
    for (QMultiMap<qint64, QQmlAbstractProfilerAdapter *>::iterator i(m_startTimes.begin()); i != m_startTimes.end();) {
        if (i.value() == profiler)
            m_startTimes.erase(i++);
        else
            ++i;
    }
    m_globalProfilers.removeOne(profiler);
    delete profiler;
}

void QQmlProfilerService::startProfiling(QQmlEngine *engine)
{
    QMutexLocker lock(configMutex());

    QByteArray message;
    QQmlDebugStream d(&message, QIODevice::WriteOnly);
    d << m_timer.nsecsElapsed() << (int)Event << (int)StartTrace << idForObject(engine);
    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine)) {
        profiler->startProfiling();
    }
    if (!m_engineProfilers.values(engine).empty()) {
        foreach (QQmlAbstractProfilerAdapter *profiler, m_globalProfilers) {
            if (!profiler->isRunning())
                profiler->startProfiling();
        }
    }

    QQmlDebugService::sendMessage(message);
}

void QQmlProfilerService::stopProfiling(QQmlEngine *engine)
{
    QMutexLocker lock(configMutex());

    bool stillRunning = false;
    m_startTimes.clear();
    for (QMultiHash<QQmlEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
            i != m_engineProfilers.end(); ++i) {
        if (i.value()->isRunning()) {
            if (i.key() == engine) {
                m_startTimes.insert(-1, i.value());
                i.value()->stopProfiling();
            } else {
                stillRunning = true;
            }
        }
    }
    foreach (QQmlAbstractProfilerAdapter *profiler, m_globalProfilers) {
        if (!profiler->isRunning())
            continue;
        m_startTimes.insert(-1, profiler);
        if (stillRunning) {
            profiler->reportData();
        } else {
            profiler->stopProfiling();
        }
    }
}

QQmlProfiler::QQmlProfiler(QQmlProfilerService *service) :
    QQmlAbstractProfilerAdapter(service), next(0)
{
    connect(this, SIGNAL(profilingEnabled()), this, SLOT(startProfiling()));
    connect(this, SIGNAL(profilingEnabledWhileWaiting()), this, SLOT(startProfiling()),
            Qt::DirectConnection);
    connect(this, SIGNAL(profilingDisabled()), this, SLOT(stopProfiling()));
    connect(this, SIGNAL(profilingDisabledWhileWaiting()), this, SLOT(stopProfiling()),
            Qt::DirectConnection);
}

qint64 QQmlProfiler::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    QMutexLocker lock(&QQmlProfilerService::instance()->m_dataMutex);
    QVector<QQmlProfilerData> *data = &(QQmlProfilerService::instance()->m_data);
    while (next < data->size() && data->at(next).time <= until) {
        data->at(next++).toByteArrays(messages);
    }
    return next < data->size() ? data->at(next).time : -1;
}

void QQmlProfiler::startProfiling()
{
    if (!QQmlProfilerService::enabled) {
        next = 0;
        service->m_data.clear();
        QQmlProfilerService::enabled = true;
    }
}

void QQmlProfiler::stopProfiling()
{
    next = 0;
    QQmlProfilerService::enabled = false;
    service->dataReady(this);
}

/*
    Send the queued up messages.
*/
void QQmlProfilerService::sendMessages()
{
    QList<QByteArray> messages;

    QByteArray data;
    QQmlDebugStream traceEnd(&data, QIODevice::WriteOnly);
    traceEnd << m_timer.nsecsElapsed() << (int)Event << (int)EndTrace;

    QSet<QQmlEngine *> seen;
    foreach (QQmlAbstractProfilerAdapter *profiler, m_startTimes) {
        for (QMultiHash<QQmlEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
                i != m_engineProfilers.end(); ++i) {
            if (i.value() == profiler && !seen.contains(i.key())) {
                seen << i.key();
                traceEnd << idForObject(i.key());
            }
        }
    }

    while (!m_startTimes.empty()) {
        QQmlAbstractProfilerAdapter *first = m_startTimes.begin().value();
        m_startTimes.erase(m_startTimes.begin());
        if (!m_startTimes.empty()) {
            qint64 next = first->sendMessages(m_startTimes.begin().key(), messages);
            if (next != -1)
                m_startTimes.insert(next, first);
        } else {
            first->sendMessages(std::numeric_limits<qint64>::max(), messages);
        }
    }

    //indicate completion
    messages << data;
    data.clear();

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

    // Stop all profiling and send the data before we get disabled.
    if (newState != Enabled) {
        foreach (QQmlEngine *engine, m_engineProfilers.keys())
            stopProfiling(engine);
    }
}

void QQmlProfilerService::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(configMutex());

    QByteArray rwData = message;
    QQmlDebugStream stream(&rwData, QIODevice::ReadOnly);

    int engineId = -1;
    bool enabled;
    stream >> enabled;
    if (!stream.atEnd())
        stream >> engineId;

    // The second time around there will be specific engineIds.
    // We only have to wait after the first, empty start message.
    if (engineId == -1) {
        // Wait until no engine registers within RegisterTimeout anymore.
        foreach (QQmlEngine *engine, m_engineProfilers.keys().toSet()) {
            if (enabled)
                startProfiling(engine);
            else
                stopProfiling(engine);
        }
    } else {
        if (enabled)
            startProfiling(qobject_cast<QQmlEngine *>(objectForId(engineId)));
        else
            stopProfiling(qobject_cast<QQmlEngine *>(objectForId(engineId)));
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
