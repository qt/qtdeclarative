// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


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

QT_END_NAMESPACE

#include "moc_qqmlpreviewclient_p.cpp"
