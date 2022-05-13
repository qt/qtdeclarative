// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugtranslationclient_p.h"
#include "qqmldebugconnection_p.h"

#include <QDebug>

#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>

QT_BEGIN_NAMESPACE

QQmlDebugTranslationClient::QQmlDebugTranslationClient(QQmlDebugConnection *client)
    : QQmlDebugClient(QLatin1String("DebugTranslation"), client)
{
}

void QQmlDebugTranslationClient::messageReceived(const QByteArray &message)
{
    QVersionedPacket<QQmlDebugConnector> packet(message);
    QQmlDebugTranslation::Reply type;

    packet >> type;
    switch (type) {
    case QQmlDebugTranslation::Reply::TranslationIssues: {
        packet >> translationIssues;
        break;
    }
    case QQmlDebugTranslation::Reply::LanguageChanged: {
        languageChanged = true;
        break;
    }
    case QQmlDebugTranslation::Reply::TranslatableTextOccurrences: {
        packet >> qmlElements;
        break;
    }
    case QQmlDebugTranslation::Reply::StateList: {
        packet >> qmlStates;
        break;
    }

    default:
        qWarning() << "TestDebugTranslationClient: received unknown command: " << static_cast<int>(type);
        break;
    }
}

QT_END_NAMESPACE

#include "moc_qqmldebugtranslationclient_p.cpp"
