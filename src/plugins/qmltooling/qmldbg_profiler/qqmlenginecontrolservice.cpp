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

#include "qqmlenginecontrolservice.h"
#include "qqmldebugpacket.h"
#include <QJSEngine>

QT_BEGIN_NAMESPACE

QQmlEngineControlServiceImpl::QQmlEngineControlServiceImpl(QObject *parent) :
    QQmlEngineControlService(1, parent)
{
}

void QQmlEngineControlServiceImpl::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(&dataMutex);
    QQmlDebugPacket d(message);
    int command;
    int engineId;
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
    if (state() == Enabled) {
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
    if (state() == Enabled) {
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

void QQmlEngineControlServiceImpl::sendMessage(QQmlEngineControlServiceImpl::MessageType type, QJSEngine *engine)
{
    QQmlDebugPacket d;
    d << type << idForObject(engine);
    emit messageToClient(name(), d.data());
}

void QQmlEngineControlServiceImpl::stateChanged(State)
{
    // We flush everything for any kind of state change, to avoid complicated timing issues.
    QMutexLocker lock(&dataMutex);
    foreach (QJSEngine *engine, startingEngines)
        emit attachedToEngine(engine);
    startingEngines.clear();
    foreach (QJSEngine *engine, stoppingEngines)
        emit detachedFromEngine(engine);
    stoppingEngines.clear();
}

QT_END_NAMESPACE
