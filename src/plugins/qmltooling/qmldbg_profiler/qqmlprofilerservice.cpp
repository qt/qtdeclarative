// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlprofilerservice.h"
#include "qv4profileradapter.h"
#include "qqmlprofileradapter.h"
#include "qqmlprofilerservicefactory.h"

#include <private/qjsengine_p.h>
#include <private/qqmldebugpluginmanager_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_QML_DEBUG_PLUGIN_LOADER(QQmlAbstractProfilerAdapter)

QQmlProfilerServiceImpl::QQmlProfilerServiceImpl(QObject *parent) :
    QQmlConfigurableDebugService<QQmlProfilerService>(1, parent),
    m_waitingForStop(false), m_globalEnabled(false), m_globalFeatures(0)
{
    m_timer.start();
    QQmlAbstractProfilerAdapter *quickAdapter =
            loadQQmlAbstractProfilerAdapter(QLatin1String("QQuickProfilerAdapter"));
    if (quickAdapter) {
        addGlobalProfiler(quickAdapter);
        quickAdapter->setService(this);
    }

    // try to load QQuick3D profiler adapter if it exists
    QQmlAbstractProfilerAdapter *quick3DAdapter =
            loadQQmlAbstractProfilerAdapter(QLatin1String("QQuick3DProfilerAdapter"));
    if (quick3DAdapter) {
        addGlobalProfiler(quick3DAdapter);
        quick3DAdapter->setService(this);
    }


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
        QList<QJSEngine *> enginesToRelease;
        for (QJSEngine *engine : std::as_const(m_stoppingEngines)) {
            const auto range = std::as_const(m_engineProfilers).equal_range(engine);
            const auto startTimesEnd = m_startTimes.cend();
            for (auto it = range.first; it != range.second; ++it) {
                if (std::find(m_startTimes.cbegin(), startTimesEnd, *it) != startTimesEnd) {
                    enginesToRelease.append(engine);
                    break;
                }
            }
        }
        sendMessages();
        for (QJSEngine *engine : std::as_const(enginesToRelease)) {
            m_stoppingEngines.removeOne(engine);
            emit detachedFromEngine(engine);
        }
    }
}

void QQmlProfilerServiceImpl::engineAboutToBeAdded(QJSEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be added from the engine thread");

    QMutexLocker lock(&m_configMutex);
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine)) {
        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(qmlEngine);
        QQmlProfilerAdapter *qmlAdapter = new QQmlProfilerAdapter(this, enginePrivate);
        addEngineProfiler(qmlAdapter, engine);
        QQmlProfilerAdapter *compileAdapter
                = new QQmlProfilerAdapter(this, &(enginePrivate->typeLoader));
        addEngineProfiler(compileAdapter, engine);
    }
    QV4ProfilerAdapter *v4Adapter = new QV4ProfilerAdapter(this, engine->handle());
    addEngineProfiler(v4Adapter, engine);
    QQmlConfigurableDebugService<QQmlProfilerService>::engineAboutToBeAdded(engine);
}

void QQmlProfilerServiceImpl::engineAdded(QJSEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be added from the engine thread");

    QMutexLocker lock(&m_configMutex);

    if (m_globalEnabled)
        startProfiling(engine, m_globalFeatures);

    const auto range = std::as_const(m_engineProfilers).equal_range(engine);
    for (auto it = range.first; it != range.second; ++it)
        (*it)->stopWaiting();
}

void QQmlProfilerServiceImpl::engineAboutToBeRemoved(QJSEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be removed from the engine thread");

    QMutexLocker lock(&m_configMutex);
    bool isRunning = false;
    const auto range = std::as_const(m_engineProfilers).equal_range(engine);
    for (auto it = range.first; it != range.second; ++it) {
        QQmlAbstractProfilerAdapter *profiler = *it;
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

void QQmlProfilerServiceImpl::engineRemoved(QJSEngine *engine)
{
    Q_ASSERT_X(QThread::currentThread() == engine->thread(), Q_FUNC_INFO,
               "QML profilers have to be removed from the engine thread");

    QMutexLocker lock(&m_configMutex);
    const auto range = std::as_const(m_engineProfilers).equal_range(engine);
    for (auto it = range.first; it != range.second; ++it) {
        QQmlAbstractProfilerAdapter *profiler = *it;
        removeProfilerFromStartTimes(profiler);
        delete profiler;
    }
    m_engineProfilers.remove(engine);
}

void QQmlProfilerServiceImpl::addEngineProfiler(QQmlAbstractProfilerAdapter *profiler, QJSEngine *engine)
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
    for (QQmlAbstractProfilerAdapter *engineProfiler : std::as_const(m_engineProfilers))
        features |= engineProfiler->features();

    if (features != 0)
        profiler->startProfiling(features);
}

void QQmlProfilerServiceImpl::removeGlobalProfiler(QQmlAbstractProfilerAdapter *profiler)
{
    QMutexLocker lock(&m_configMutex);
    removeProfilerFromStartTimes(profiler);
    m_globalProfilers.removeOne(profiler);
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
void QQmlProfilerServiceImpl::startProfiling(QJSEngine *engine, quint64 features)
{
    QMutexLocker lock(&m_configMutex);

    if (features & static_cast<quint64>(1) << ProfileDebugMessages) {
        if (QDebugMessageService *messageService =
                QQmlDebugConnector::instance()->service<QDebugMessageService>())
            messageService->synchronizeTime(m_timer);
    }

    QQmlDebugPacket d;

    d << m_timer.nsecsElapsed() << static_cast<qint32>(Event) << static_cast<qint32>(StartTrace);
    bool startedAny = false;
    if (engine != nullptr) {
        const auto range = std::as_const(m_engineProfilers).equal_range(engine);
        for (auto it = range.first; it != range.second; ++it) {
            QQmlAbstractProfilerAdapter *profiler = *it;
            if (!profiler->isRunning()) {
                profiler->startProfiling(features);
                startedAny = true;
            }
        }
        if (startedAny)
            d << idForObject(engine);
    } else {
        m_globalEnabled = true;
        m_globalFeatures = features;

        QSet<QJSEngine *> engines;
        for (QMultiHash<QJSEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
                i != m_engineProfilers.end(); ++i) {
            if (!i.value()->isRunning()) {
                engines << i.key();
                i.value()->startProfiling(features);
                startedAny = true;
            }
        }
        for (QJSEngine *profiledEngine : std::as_const(engines))
            d << idForObject(profiledEngine);
    }

    if (startedAny) {
        for (QQmlAbstractProfilerAdapter *profiler : std::as_const(m_globalProfilers)) {
            if (!profiler->isRunning())
                profiler->startProfiling(features);
        }

        emit startFlushTimer();
        emit messageToClient(name(), d.data());
    }
}

/*!
 * Stop profiling the given \a engine. If \a engine is 0, stop all currently running engine
 * profilers.
 *
 * If afterwards no more engine profilers are running, also stop all global profilers. Otherwise
 * only make them report their data.
 */
void QQmlProfilerServiceImpl::stopProfiling(QJSEngine *engine)
{
    QMutexLocker lock(&m_configMutex);
    QList<QQmlAbstractProfilerAdapter *> stopping;
    QList<QQmlAbstractProfilerAdapter *> reporting;

    if (engine == nullptr)
        m_globalEnabled = false;

    bool stillRunning = false;
    for (QMultiHash<QJSEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
            i != m_engineProfilers.end(); ++i) {
        if (i.value()->isRunning()) {
            m_startTimes.insert(-1, i.value());
            if (engine == nullptr || i.key() == engine) {
                stopping << i.value();
            } else {
                reporting << i.value();
                stillRunning = true;
            }
        }
    }

    if (stopping.isEmpty())
        return;

    for (QQmlAbstractProfilerAdapter *profiler : std::as_const(m_globalProfilers)) {
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

    for (QQmlAbstractProfilerAdapter *profiler : std::as_const(reporting))
        profiler->reportData();

    for (QQmlAbstractProfilerAdapter *profiler : std::as_const(stopping))
        profiler->stopProfiling();
}

/*
    Send the queued up messages.
*/
void QQmlProfilerServiceImpl::sendMessages()
{
    QList<QByteArray> messages;

    QQmlDebugPacket traceEnd;
    if (m_waitingForStop) {
        traceEnd << m_timer.nsecsElapsed() << static_cast<qint32>(Event)
                 << static_cast<qint32>(EndTrace);

        QSet<QJSEngine *> seen;
        for (QQmlAbstractProfilerAdapter *profiler : std::as_const(m_startTimes)) {
            for (QMultiHash<QJSEngine *, QQmlAbstractProfilerAdapter *>::iterator i(m_engineProfilers.begin());
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
        qint64 next = first->sendMessages(m_startTimes.isEmpty() ?
                                              std::numeric_limits<qint64>::max() :
                                              m_startTimes.begin().key(), messages);
        if (next != -1)
            m_startTimes.insert(next, first);

        if (messages.size() >= QQmlAbstractProfilerAdapter::s_numMessagesPerBatch) {
            emit messagesToClient(name(), messages);
            messages.clear();
        }
    }

    bool stillRunning = false;
    for (const QQmlAbstractProfilerAdapter *profiler : std::as_const(m_engineProfilers)) {
        if (profiler->isRunning()) {
            stillRunning = true;
            break;
        }
    }

    if (m_waitingForStop) {
        // EndTrace can be sent multiple times, as it's engine specific.
        messages << traceEnd.data();

        if (!stillRunning) {
            // Complete is only sent once, when no engines are running anymore.
            QQmlDebugPacket ds;
            ds << static_cast<qint64>(-1) << static_cast<qint32>(Complete);
            messages << ds.data();
            m_waitingForStop = false;
        }
    }

    emit messagesToClient(name(), messages);

    // Restart flushing if any profilers are still running
    if (stillRunning)
        emit startFlushTimer();
}

void QQmlProfilerServiceImpl::stateAboutToBeChanged(QQmlDebugService::State newState)
{
    QMutexLocker lock(&m_configMutex);

    if (state() == newState)
        return;

    // Stop all profiling and send the data before we get disabled.
    if (newState != Enabled) {
        for (auto it = m_engineProfilers.keyBegin(), end = m_engineProfilers.keyEnd();
             it != end; ++it) {
            stopProfiling(*it);
        }
    }
}

void QQmlProfilerServiceImpl::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(&m_configMutex);

    QQmlDebugPacket stream(message);

    int engineId = -1;
    quint64 features = std::numeric_limits<quint64>::max();
    bool enabled;
    quint32 flushInterval = 0;
    stream >> enabled;
    if (!stream.atEnd())
        stream >> engineId;
    if (!stream.atEnd())
        stream >> features;
    if (!stream.atEnd()) {
        stream >> flushInterval;
        m_flushTimer.setInterval(
                    static_cast<int>(qMin(flushInterval,
                                          static_cast<quint32>(std::numeric_limits<int>::max()))));
        auto timerStart = static_cast<void(QTimer::*)()>(&QTimer::start);
        if (flushInterval > 0) {
            connect(&m_flushTimer, &QTimer::timeout, this, &QQmlProfilerServiceImpl::flush);
            connect(this, &QQmlProfilerServiceImpl::startFlushTimer, &m_flushTimer, timerStart);
            connect(this, &QQmlProfilerServiceImpl::stopFlushTimer, &m_flushTimer, &QTimer::stop);
        } else {
            disconnect(&m_flushTimer, &QTimer::timeout, this, &QQmlProfilerServiceImpl::flush);
            disconnect(this, &QQmlProfilerServiceImpl::startFlushTimer, &m_flushTimer, timerStart);
            disconnect(this, &QQmlProfilerServiceImpl::stopFlushTimer,
                       &m_flushTimer, &QTimer::stop);
        }
    }

    bool useMessageTypes = false;
    if (!stream.atEnd())
        stream >> useMessageTypes;

    // If engineId == -1 objectForId() and then the cast will return 0.
    if (enabled && useMessageTypes) // If the client doesn't support message types don't profile.
        startProfiling(qobject_cast<QJSEngine *>(objectForId(engineId)), features);
    else if (!enabled)              // On stopProfiling the client doesn't repeat useMessageTypes.
        stopProfiling(qobject_cast<QJSEngine *>(objectForId(engineId)));

    stopWaiting();
}

void QQmlProfilerServiceImpl::flush()
{
    QMutexLocker lock(&m_configMutex);
    QList<QQmlAbstractProfilerAdapter *> reporting;

    for (QQmlAbstractProfilerAdapter *profiler : std::as_const(m_engineProfilers)) {
        if (profiler->isRunning()) {
            m_startTimes.insert(-1, profiler);
            reporting.append(profiler);
        }
    }

    for (QQmlAbstractProfilerAdapter *profiler : std::as_const(m_globalProfilers)) {
        if (profiler->isRunning()) {
            m_startTimes.insert(-1, profiler);
            reporting.append(profiler);
        }
    }

    for (QQmlAbstractProfilerAdapter *profiler : std::as_const(reporting))
        profiler->reportData();
}

QT_END_NAMESPACE

#include "moc_qqmlprofilerservice.cpp"
