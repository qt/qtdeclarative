// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPROFILERSERVICE_P_H
#define QQMLPROFILERSERVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmlconfigurabledebugservice_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmlprofilerdefinitions_p.h>
#include <private/qqmlabstractprofileradapter_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qtimer.h>

#include <limits>

QT_BEGIN_NAMESPACE

class QUrl;
using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

class QQmlProfilerServiceImpl :
        public QQmlConfigurableDebugService<QQmlProfilerService>,
        public QQmlProfilerDefinitions
{
    Q_OBJECT
public:

    void engineAboutToBeAdded(QJSEngine *engine) override;
    void engineAboutToBeRemoved(QJSEngine *engine) override;
    void engineAdded(QJSEngine *engine) override;
    void engineRemoved(QJSEngine *engine) override;

    void addGlobalProfiler(QQmlAbstractProfilerAdapter *profiler) override;
    void removeGlobalProfiler(QQmlAbstractProfilerAdapter *profiler) override;

    void startProfiling(QJSEngine *engine,
                        quint64 features = std::numeric_limits<quint64>::max()) override;
    void stopProfiling(QJSEngine *engine) override;

    QQmlProfilerServiceImpl(QObject *parent = nullptr);
    ~QQmlProfilerServiceImpl() override;

    void dataReady(QQmlAbstractProfilerAdapter *profiler) override;

Q_SIGNALS:
    void startFlushTimer();
    void stopFlushTimer();

protected:
    void stateAboutToBeChanged(State state) override;
    void messageReceived(const QByteArray &) override;

private:
    friend class QQmlProfilerServiceFactory;

    void sendMessages();
    void addEngineProfiler(QQmlAbstractProfilerAdapter *profiler, QJSEngine *engine);
    void removeProfilerFromStartTimes(const QQmlAbstractProfilerAdapter *profiler);
    void flush();

    QElapsedTimer m_timer;
    QTimer m_flushTimer;
    bool m_waitingForStop;

    bool m_globalEnabled;
    quint64 m_globalFeatures;

    QList<QQmlAbstractProfilerAdapter *> m_globalProfilers;
    QMultiHash<QJSEngine *, QQmlAbstractProfilerAdapter *> m_engineProfilers;
    QList<QJSEngine *> m_stoppingEngines;
    QMultiMap<qint64, QQmlAbstractProfilerAdapter *> m_startTimes;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERSERVICE_P_H

