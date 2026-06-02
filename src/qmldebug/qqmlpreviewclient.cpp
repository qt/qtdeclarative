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
    connect(this, &QQmlPreviewClient::confirmation, this,
            [this](const QQmlPreviewClient::Settings &) { configureEventReplay(); });
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
    Q_ASSERT(connection());
    if (!d_func()->m_recordingData) {
        d_func()->m_recordingData =
                std::make_unique<QQmlPreviewClientPrivate::PreviewRecordingData>(connection());
    }

    recordClient().setFlushInterval(1);
    recordClient().setRecording(true);
    auto &timer = replayTimer();
    timer.setInterval(100);
    connect(&timer, &QTimer::timeout, this, [this, &timer]() {
        if (eventReceiver().numLoadedEvents() < numExpectedEvents())
            return;
        triggerAnimationSpeed(1);
        timer.stop();
    });

    // We want to start the replay as soon as possible after the configuration is confirmed.
    if (eventReceiver().numLoadedEvents() > 0 && replayClient().state() == Enabled) {
        replayEventsForUrl(QUrl());
    }
}

void QQmlPreviewClient::replayEventsForUrl(const QUrl &url)
{
    const auto events = eventReceiver().events();
    const auto types = eventReceiver().eventTypes();
    triggerAnimationSpeed(1000);
    triggerLoad(url);
    for (const auto &event : events)
        replayClient().sendEvent(types[event.typeIndex()], event);
    eventReceiver().clear();
    replayTimer().start();
}

void QQmlPreviewClient::loadUrl(const QUrl &url)
{
    if (!d_func()->m_recordingData)
        return triggerLoad(url);

    setNumExpectedEvents(eventReceiver().numLoadedEvents());
    if (numExpectedEvents() > 0 && replayClient().state() == Enabled) {
        replayEventsForUrl(url);
    } else {
        eventReceiver().clear();
        triggerLoad(url);
    }
}

void QQmlPreviewClient::setNumExpectedEvents(qsizetype eventCount)
{
    Q_D(QQmlPreviewClient);
    d->m_numExpectedEvents = eventCount;
}

qsizetype QQmlPreviewClient::numExpectedEvents() const
{
    Q_D(const QQmlPreviewClient);
    return d->m_numExpectedEvents;
}

QTimer &QQmlPreviewClient::replayTimer() const
{
    Q_D(const QQmlPreviewClient);
    return d->m_recordingData->replayTimer;
}

QQmlProfilerClient &QQmlPreviewClient::recordClient() const
{
    Q_D(const QQmlPreviewClient);
    return d->m_recordingData->recordClient;
}

QQuickEventReplayClient &QQmlPreviewClient::replayClient() const
{
    Q_D(const QQmlPreviewClient);
    return d->m_recordingData->replayClient;
}

QQmlProfilerQtdWriter &QQmlPreviewClient::eventReceiver() const
{
    Q_D(const QQmlPreviewClient);
    return d->m_recordingData->eventReceiver;
}

QT_END_NAMESPACE

#include "moc_qqmlpreviewclient_p.cpp"
