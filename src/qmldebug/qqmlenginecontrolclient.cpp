// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlenginecontrolclient_p.h"
#include "qqmlenginecontrolclient_p_p.h"
#include "qqmldebugconnection_p.h"

#include <private/qpacket_p.h>

QT_BEGIN_NAMESPACE

QQmlEngineControlClient::QQmlEngineControlClient(QQmlDebugConnection *connection) :
    QQmlDebugClient(*(new QQmlEngineControlClientPrivate(connection)))
{
}

QQmlEngineControlClient::QQmlEngineControlClient(QQmlEngineControlClientPrivate &dd) :
    QQmlDebugClient(dd)
{
}

/*!
 * Block the starting or stopping of the engine with id \a engineId for now. By calling
 * releaseEngine later the block can be lifted again. In the debugged application the engine control
 * server waits until a message is received before continuing. So by not sending a message here we
 * delay the process. Blocks add up. You have to call releaseEngine() as often as you've called
 * blockEngine before. The intention of that is to allow different debug clients to use the same
 * engine control and communicate with their respective counterparts before the QML engine starts or
 * shuts down.
 */
void QQmlEngineControlClient::blockEngine(int engineId)
{
    Q_D(QQmlEngineControlClient);
    Q_ASSERT(d->blockedEngines.contains(engineId));
    d->blockedEngines[engineId].blockers++;
}

/*!
 * Release the engine with id \a engineId. If no other blocks are present, depending on what the
 * engine is waiting for, the start or stop command is sent to the process being debugged.
 */
void QQmlEngineControlClient::releaseEngine(int engineId)
{
    Q_D(QQmlEngineControlClient);
    Q_ASSERT(d->blockedEngines.contains(engineId));

    QQmlEngineControlClientPrivate::EngineState &state = d->blockedEngines[engineId];
    if (--state.blockers == 0) {
        Q_ASSERT(state.releaseCommand != QQmlEngineControlClientPrivate::InvalidCommand);
        d->sendCommand(state.releaseCommand, engineId);
        d->blockedEngines.remove(engineId);
    }
}

QList<int> QQmlEngineControlClient::blockedEngines() const
{
    Q_D(const QQmlEngineControlClient);
    return d->blockedEngines.keys();
}

void QQmlEngineControlClient::messageReceived(const QByteArray &data)
{
    Q_D(QQmlEngineControlClient);
    QPacket stream(d->connection->currentDataStreamVersion(), data);
    qint32 message;
    qint32 id;
    QString name;

    stream >> message >> id;

    if (!stream.atEnd())
        stream >> name;

    auto handleWaiting = [&](
            QQmlEngineControlClientPrivate::CommandType command, std::function<void()> emitter) {
        QQmlEngineControlClientPrivate::EngineState &state = d->blockedEngines[id];
        Q_ASSERT(state.blockers == 0);
        Q_ASSERT(state.releaseCommand == QQmlEngineControlClientPrivate::InvalidCommand);
        state.releaseCommand = command;
        emitter();
        if (state.blockers == 0) {
            d->sendCommand(state.releaseCommand, id);
            d->blockedEngines.remove(id);
        }
    };

    switch (message) {
    case QQmlEngineControlClientPrivate::EngineAboutToBeAdded:
        handleWaiting(QQmlEngineControlClientPrivate::StartWaitingEngine, [&](){
            emit engineAboutToBeAdded(id, name);
        });
        break;
    case QQmlEngineControlClientPrivate::EngineAdded:
        emit engineAdded(id, name);
        break;
    case QQmlEngineControlClientPrivate::EngineAboutToBeRemoved:
        handleWaiting(QQmlEngineControlClientPrivate::StopWaitingEngine, [&](){
            emit engineAboutToBeRemoved(id, name);
        });
        break;
    case QQmlEngineControlClientPrivate::EngineRemoved:
        emit engineRemoved(id, name);
        break;
    }
}

QQmlEngineControlClientPrivate::QQmlEngineControlClientPrivate(QQmlDebugConnection *connection) :
    QQmlDebugClientPrivate(QLatin1String("EngineControl"), connection)
{
}

void QQmlEngineControlClientPrivate::sendCommand(
        QQmlEngineControlClientPrivate::CommandType command, int engineId)
{
    Q_Q(QQmlEngineControlClient);
    QPacket stream(connection->currentDataStreamVersion());
    stream << static_cast<qint32>(command) << engineId;
    q->sendMessage(stream.data());
}

QT_END_NAMESPACE

#include "moc_qqmlenginecontrolclient_p.cpp"
