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

#include "qqmlprofilerservice.h"
#include "qv4profileradapter.h"
#include "qqmlprofileradapter.h"
#include "qqmlprofilerservicefactory.h"
#include <private/qqmlengine_p.h>

#include <QtCore/qdatastream.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

QQmlProfilerServiceImpl::QQmlProfilerServiceImpl(QObject *parent) :
    QQmlConfigurableDebugService<QQmlProfilerService>(1, parent),
    m_waitingForStop(false)
{
    m_timer.start();
}

QQmlProfilerServiceImpl::~QQmlProfilerServiceImpl()
{
    // No need to lock here. If any engine or global profiler is still trying to register at this
    // point we have a nasty bug anyway.
    qDeleteAll(m_engineProfilers);
    qDeleteAll(m_globalProfilers);
}

void QQmlProfilerServiceImpl::dataReady(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(&m_configMutex);
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

void QQmlProfilerServiceImpl::engineAboutToBeAdded(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be added from the engine thread");

    QMutexLocker lock(&m_configMutex);
    QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
    QQmlProfilerAdapter *qmlAdapter = new QQmlProfilerAdapter(this, enginePrivate);
    QQmlProfilerAdapter *compileAdapter
            = new QQmlProfilerAdapter(this, &(enginePrivate->typeLoader));
    QV4ProfilerAdapter *v4Adapter
            = new QV4ProfilerAdapter(this, QV8Engine::getV4(engine->handle()));
    addEngineProfiler(qmlAdapter, engine);
    addEngineProfiler(compileAdapter, engine);
    addEngineProfiler(v4Adapter, engine);
    QQmlConfigurableDebugService<QQmlProfilerService>::engineAboutToBeAdded(engine);
}

void QQmlProfilerServiceImpl::engineAdded(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be added from the engine thread");

    QMutexLocker lock(&m_configMutex);
    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine))
        profiler->stopWaiting();
}

void QQmlProfilerServiceImpl::engineAboutToBeRemoved(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be removed from the engine thread");

    QMutexLocker lock(&m_configMutex);
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

void QQmlProfilerServiceImpl::engineRemoved(QQmlEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be removed from the engine thread");

    QMutexLocker lock(&m_configMutex);
    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine)) {
        removeProfilerFromStartTimes(profiler);
        delete profiler;
    }
    m_engineProfilers.remove(engine);
}

void QQmlProfilerServiceImpl::addEngineProfiler(QQmlAbstractProfilerAdapter *profiler, QQmlEngine *engine)
{
    profiler->moveToThread(thread());
    profiler->synchronize(m_timer);
    m_engineProfilers.insert(engine, profiler);
}

void QQmlProfilerServiceImpl::addGlobalProfiler(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(&m_configMutex);
    profiler->synchronize(m_timer);
    m_globalProfilers.append(profiler);
    // Global profiler, not connected to a specific engine.
    // Global profilers are started whenever any engine profiler is started and stopped when
    // all engine profilers are stopped.
    quint64 features = 0;
    foreach (QQmlAbstractProfilerAdapter *engineProfiler, m_engineProfilers)
        features |= engineProfiler->features();

    if (features != 0)
        profiler->startProfiling(features);
}

void QQmlProfilerServiceImpl::removeGlobalProfiler(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(&m_configMutex);
    removeProfilerFromStartTimes(profiler);
    m_globalProfilers.removeOne(profiler);
    delete profiler;
}

void QQmlProfilerServiceImpl::removeProfilerFromStartTimes(const QQmlAbstractProfilerAdapter *profiler)
{
    for (QMultiMap<qint64, QQmlAbstractProfilerAdapter *>::iterator i(m_startTimes.begin());
            i != m_startTimes.end();) {
        if (i.value() == profiler) {
            m_startTimes.erase(i++);
            break;
        } else {
            ++i;
        }
    }
}

/*!
 * Start profiling the given \a engine. If \a engine is 0, start all engine profilers that aren't
 * currently running.
 *
 * If any engine profiler is started like that also start all global profilers.
 */
void QQmlProfilerServiceImpl::startProfiling(QQmlEngine *engine, quint64 features)
{
    QMutexLocker lock(&m_configMutex);

    QByteArray message;
    QQmlDebugStream d(&message, QIODevice::WriteOnly);

    d << m_timer.nsecsElapsed() << (int)Event << (int)StartTrace;
    bool startedAny = false;
    if (engine != 0) {
        foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers.values(engine)) {
            if (!profiler->isRunning()) {
                profiler->startProfiling(features);
                startedAny = true;
            }
        }
        if (startedAny)
            d << idForObject(engine);
    } else {
        QSet<QQmlEngine *> engines;
        for (QMultiHash<QQmlEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
                i != m_engineProfilers.end(); ++i) {
            if (!i.value()->isRunning()) {
                engines << i.key();
                i.value()->startProfiling(features);
                startedAny = true;
            }
        }
        foreach (QQmlEngine *profiledEngine, engines)
            d << idForObject(profiledEngine);
    }

    if (startedAny) {
        foreach (QQmlAbstractProfilerAdapter *profiler, m_globalProfilers) {
            if (!profiler->isRunning())
                profiler->startProfiling(features);
        }

        emit startFlushTimer();
    }

    emit messageToClient(name(), message);
}

/*!
 * Stop profiling the given \a engine. If \a engine is 0, stop all currently running engine
 * profilers.
 *
 * If afterwards no more engine profilers are running, also stop all global profilers. Otherwise
 * only make them report their data.
 */
void QQmlProfilerServiceImpl::stopProfiling(QQmlEngine *engine)
{
    QMutexLocker lock(&m_configMutex);
    QList<QQmlAbstractProfilerAdapter *> stopping;
    QList<QQmlAbstractProfilerAdapter *> reporting;

    bool stillRunning = false;
    for (QMultiHash<QQmlEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
            i != m_engineProfilers.end(); ++i) {
        if (i.value()->isRunning()) {
            if (engine == 0 || i.key() == engine) {
                m_startTimes.insert(-1, i.value());
                stopping << i.value();
            } else {
                stillRunning = true;
            }
        }
    }

    if (stopping.isEmpty())
        return;

    foreach (QQmlAbstractProfilerAdapter *profiler, m_globalProfilers) {
        if (!profiler->isRunning())
            continue;
        m_startTimes.insert(-1, profiler);
        if (stillRunning) {
            reporting << profiler;
        } else {
            stopping << profiler;
        }
    }

    emit stopFlushTimer();
    m_waitingForStop = true;

    foreach (QQmlAbstractProfilerAdapter *profiler, reporting)
        profiler->reportData();

    foreach (QQmlAbstractProfilerAdapter *profiler, stopping)
        profiler->stopProfiling();
}

/*
    Send the queued up messages.
*/
void QQmlProfilerServiceImpl::sendMessages()
{
    QList<QByteArray> messages;

    QByteArray data;

    if (m_waitingForStop) {
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

    if (m_waitingForStop) {
        //indicate completion
        messages << data;
        data.clear();

        QQmlDebugStream ds(&data, QIODevice::WriteOnly);
        ds << (qint64)-1 << (int)Complete;
        messages << data;
        m_waitingForStop = false;
    }

    emit messagesToClient(name(), messages);

    // Restart flushing if any profilers are still running
    foreach (const QQmlAbstractProfilerAdapter *profiler, m_engineProfilers) {
        if (profiler->isRunning()) {
            emit startFlushTimer();
            break;
        }
    }
}

void QQmlProfilerServiceImpl::stateAboutToBeChanged(QQmlDebugService::State newState)
{
    QMutexLocker lock(&m_configMutex);

    if (state() == newState)
        return;

    // Stop all profiling and send the data before we get disabled.
    if (newState != Enabled) {
        foreach (QQmlEngine *engine, m_engineProfilers.keys())
            stopProfiling(engine);
    }
}

void QQmlProfilerServiceImpl::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(&m_configMutex);

    QByteArray rwData = message;
    QQmlDebugStream stream(&rwData, QIODevice::ReadOnly);

    int engineId = -1;
    quint64 features = std::numeric_limits<quint64>::max();
    bool enabled;
    uint flushInterval = 0;
    stream >> enabled;
    if (!stream.atEnd())
        stream >> engineId;
    if (!stream.atEnd())
        stream >> features;
    if (!stream.atEnd()) {
        stream >> flushInterval;
        m_flushTimer.setInterval(flushInterval);
        if (flushInterval > 0) {
            connect(&m_flushTimer, SIGNAL(timeout()), this, SLOT(flush()));
            connect(this, SIGNAL(startFlushTimer()), &m_flushTimer, SLOT(start()));
            connect(this, SIGNAL(stopFlushTimer()), &m_flushTimer, SLOT(stop()));
        } else {
            disconnect(&m_flushTimer, SIGNAL(timeout()), this, SLOT(flush()));
            disconnect(this, SIGNAL(startFlushTimer()), &m_flushTimer, SLOT(start()));
            disconnect(this, SIGNAL(stopFlushTimer()), &m_flushTimer, SLOT(stop()));
        }
    }

    // If engineId == -1 objectForId() and then the cast will return 0.
    if (enabled)
        startProfiling(qobject_cast<QQmlEngine *>(objectForId(engineId)), features);
    else
        stopProfiling(qobject_cast<QQmlEngine *>(objectForId(engineId)));

    stopWaiting();
}

void QQmlProfilerServiceImpl::flush()
{
    QMutexLocker lock(&m_configMutex);
    QList<QQmlAbstractProfilerAdapter *> reporting;

    foreach (QQmlAbstractProfilerAdapter *profiler, m_engineProfilers) {
        if (profiler->isRunning()) {
            m_startTimes.insert(-1, profiler);
            reporting.append(profiler);
        }
    }

    foreach (QQmlAbstractProfilerAdapter *profiler, m_globalProfilers) {
        if (profiler->isRunning()) {
            m_startTimes.insert(-1, profiler);
            reporting.append(profiler);
        }
    }

    foreach (QQmlAbstractProfilerAdapter *profiler, reporting)
        profiler->reportData();
}

QT_END_NAMESPACE
