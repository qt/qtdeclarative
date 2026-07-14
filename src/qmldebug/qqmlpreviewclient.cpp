// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant


#include "qqmlpreviewclient_p_p.h"
#include <private/qpacket_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlfile.h>

QT_BEGIN_NAMESPACE

QQmlPreviewClient::QQmlPreviewClient(QQmlDebugConnection *connection)
    : QQmlDebugClient(*(new QQmlPreviewClientPrivate(connection)))
{
    Q_D(QQmlPreviewClient);
    connect(&d->replayClient, &QQmlDebugClient::stateChanged, this,
            [this](State replayClientState) {
                if (replayClientState == Enabled)
                    configureEventReplay();
            });
}

void QQmlPreviewClient::messageReceived(const QByteArray &message)
{
    QPacket packet(connection()->currentDataStreamVersion(), message);

    qint8 command;
    packet >> command;

    switch (command) {
    case Error: {
        QString seviceError;
        packet >> seviceError;
        emit error(seviceError);
        break;
    }
    case Request: {
        QString fileName;
        packet >> fileName;
        emit request(fileName);
        break;
    }
    case Fps: {
        FpsInfo info;
        packet >> info.numSyncs >> info.minSync >> info.maxSync >> info.totalSync
               >> info.numRenders >> info.minRender >> info.maxRender >> info.totalRender;
        emit fps(info);
        break;
    }
    case Confirmation: {
        Settings settings;
        packet >> settings.enableInPlaceUpdates;
        emit confirmation(settings);
        break;
    }
    case HotReloadFailure: {
        QString reason;
        packet >> reason;
        emit hotReloadFailure(reason);
        break;
    }
    default:
        emit error(QString::fromLatin1("Unknown command received: %1").arg(command));
        break;
    }
}

void QQmlPreviewClient::sendDirectory(const QString &path, const QStringList &entries)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(Directory) << path << entries;
    sendMessage(packet.data());
}

void QQmlPreviewClient::sendFile(const QString &path, const QByteArray &contents)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(File) << path << contents;
    sendMessage(packet.data());
}

void QQmlPreviewClient::sendError(const QString &path)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(Error) << path;
    sendMessage(packet.data());
}

void QQmlPreviewClient::sendConfiguration(const Settings &config)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(Configuration) << config.enableInPlaceUpdates;
    sendMessage(packet.data());
}

void QQmlPreviewClient::triggerLoad(const QUrl &url)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(Load) << url;
    sendMessage(packet.data());
}

void QQmlPreviewClient::triggerRerun()
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(Rerun);
    sendMessage(packet.data());
}

void QQmlPreviewClient::triggerZoom(float factor)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(Zoom) << factor;
    sendMessage(packet.data());
}

void QQmlPreviewClient::triggerAnimationSpeed(float factor)
{
    QPacket packet(connection()->currentDataStreamVersion());
    packet << static_cast<qint8>(AnimationSpeed) << factor;
    sendMessage(packet.data());
}

void QQmlPreviewClient::configureEventReplay()
{
    Q_D(QQmlPreviewClient);

    Q_ASSERT(connection());
    Q_ASSERT(d->replayClient.state() == Enabled);

    d->recordClient.setFlushInterval(1);
    d->recordClient.setRecording(true);
    d->replayTimer.setInterval(100);
    connect(&d->replayTimer, &QTimer::timeout, this, [this, d]() {
        if (d->eventReceiver.numLoadedEvents() < d->m_numExpectedEvents)
            return;
        triggerAnimationSpeed(1);
        d->replayTimer.stop();
    });
}

void QQmlPreviewClient::replayEvents()
{
    Q_D(QQmlPreviewClient);

    const auto events = d->eventReceiver.events();
    if (events.isEmpty() || d->replayClient.state() != Enabled) {
        d->eventReceiver.clear();
        return;
    }

    const auto types = d->eventReceiver.eventTypes();

    // The replayed events will be recorded again as they are processed. Remember
    // how many we expect so that replayTimer can restore the animation speed
    // once they have all arrived.
    d->m_numExpectedEvents = events.size();

    triggerAnimationSpeed(1000);
    qint64 timestamp = 0;
    for (QQmlProfilerEvent event : events) {
        // Compress the timestamps so that the events are processed in quick succession.
        event.setTimestamp(++timestamp);
        d->replayClient.sendEvent(types[event.typeIndex()], std::move(event));
    }

    d->eventReceiver.clear();
    d->replayTimer.start();
}

void QQmlPreviewClient::replayEvent(const QQmlProfilerEventType &type,
                                    QQmlProfilerEvent &&event)
{
    Q_D(QQmlPreviewClient);
    d->replayClient.sendEvent(type, std::move(event));
}

bool QQmlPreviewClient::saveEvents(const QString &fileName)
{
    Q_D(QQmlPreviewClient);
    return d->eventReceiver.save(fileName);
}

bool QQmlPreviewClient::replayEventsFromFile(const QString &fileName)
{
    Q_D(QQmlPreviewClient);
    if (d->replayClient.state() != Enabled)
        return false;

    triggerAnimationSpeed(1000);
    qsizetype numEvents = 0;
    if (d->replayClient.loadEvents(
                fileName, [&](const QQmlProfilerEventType &type, QQmlProfilerEvent &&event) {
                    // Compress the timestamps so that the events are processed in quick succession.
                    event.setTimestamp(++numEvents);
                    d->replayClient.sendEvent(type, std::move(event));
                })) {

        d->eventReceiver.clear();
        d->m_numExpectedEvents = numEvents;
        d->replayTimer.start();
        return true;
    }

    triggerAnimationSpeed(1);
    return false;
}

bool QQmlPreviewClient::hasRecordedEvents() const
{
    Q_D(const QQmlPreviewClient);
    return d->eventReceiver.numLoadedEvents() > 0;
}

void QQmlPreviewClient::clearRecordedEvents()
{
    Q_D(QQmlPreviewClient);
    d->eventReceiver.clear();
}

QT_END_NAMESPACE

#include "moc_qqmlpreviewclient_p.cpp"
