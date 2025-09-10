// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlenginecontrolservice.h"
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>
#include <QJSEngine>

QT_BEGIN_NAMESPACE

using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

QQmlEngineControlServiceImpl::QQmlEngineControlServiceImpl(QObject *parent) :
    QQmlEngineControlService(1, parent)
{
    blockingMode = QQmlDebugConnector::instance()->blockingMode();
}

void QQmlEngineControlServiceImpl::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(&dataMutex);
    QQmlDebugPacket d(message);
    qint32 command;
    qint32 engineId;
    d >> command >> engineId;
    QJSEngine *engine = qobject_cast<QJSEngine *>(objectForId(engineId));
    if (command == StartWaitingEngine && startingEngines.contains(engine)) {
        startingEngines.removeOne(engine);
        emit attachedToEngine(engine);
    } else if (command == StopWaitingEngine && stoppingEngines.contains(engine)) {
        stoppingEngines.removeOne(engine);
        emit detachedFromEngine(engine);
    }
}

void QQmlEngineControlServiceImpl::engineAboutToBeAdded(QJSEngine *engine)
{
    QMutexLocker lock(&dataMutex);
    if (blockingMode && state() == Enabled) {
        Q_ASSERT(!stoppingEngines.contains(engine));
        Q_ASSERT(!startingEngines.contains(engine));
        startingEngines.append(engine);
        sendMessage(EngineAboutToBeAdded, engine);
    } else {
        emit attachedToEngine(engine);
    }
}

void QQmlEngineControlServiceImpl::engineAboutToBeRemoved(QJSEngine *engine)
{
    QMutexLocker lock(&dataMutex);
    if (blockingMode && state() == Enabled) {
        Q_ASSERT(!stoppingEngines.contains(engine));
        Q_ASSERT(!startingEngines.contains(engine));
        stoppingEngines.append(engine);
        sendMessage(EngineAboutToBeRemoved, engine);
    } else {
        emit detachedFromEngine(engine);
    }
}

void QQmlEngineControlServiceImpl::engineAdded(QJSEngine *engine)
{
    if (state() == Enabled) {
        QMutexLocker lock(&dataMutex);
        Q_ASSERT(!startingEngines.contains(engine));
        Q_ASSERT(!stoppingEngines.contains(engine));
        sendMessage(EngineAdded, engine);
    }
}

void QQmlEngineControlServiceImpl::engineRemoved(QJSEngine *engine)
{
    if (state() == Enabled) {
        QMutexLocker lock(&dataMutex);
        Q_ASSERT(!startingEngines.contains(engine));
        Q_ASSERT(!stoppingEngines.contains(engine));
        sendMessage(EngineRemoved, engine);
    }
}

void QQmlEngineControlServiceImpl::sendMessage(QQmlEngineControlServiceImpl::MessageType type,
                                               QJSEngine *engine)
{
    QQmlDebugPacket d;
    d << static_cast<qint32>(type) << idForObject(engine);
    emit messageToClient(name(), d.data());
}

void QQmlEngineControlServiceImpl::stateChanged(State)
{
    // We flush everything for any kind of state change, to avoid complicated timing issues.
    QMutexLocker lock(&dataMutex);
    for (QJSEngine *engine : std::as_const(startingEngines))
        emit attachedToEngine(engine);
    startingEngines.clear();
    for (QJSEngine *engine : std::as_const(stoppingEngines))
        emit detachedFromEngine(engine);
    stoppingEngines.clear();
}

QT_END_NAMESPACE
