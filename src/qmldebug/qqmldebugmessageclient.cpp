// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugmessageclient_p.h"

#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

/*!
  \class QQmlDebugMessageClient
  \internal

  \brief Client for the debug message service

  The QQmlDebugMessageClient receives debug messages routed through the QML
  debug connection via QDebugMessageService.
 */

QQmlDebugMessageClient::QQmlDebugMessageClient(QQmlDebugConnection *client)
    : QQmlDebugClient(QLatin1String("DebugMessages"), client)
{
}

void QQmlDebugMessageClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == "MESSAGE") {
        int type;
        int line;
        QByteArray debugMessage;
        QByteArray file;
        QByteArray function;
        ds >> type >> debugMessage >> file >> line >> function;
        QQmlDebugContextInfo info;
        info.line = line;
        info.file = QString::fromUtf8(file);
        info.function = QString::fromUtf8(function);
        info.timestamp = -1;
        if (!ds.atEnd()) {
            QByteArray category;
            ds >> category;
            info.category = QString::fromUtf8(category);
            if (!ds.atEnd())
                ds >> info.timestamp;
        }
        emit message(QtMsgType(type), QString::fromUtf8(debugMessage), info);
    }
}

QT_END_NAMESPACE

#include "moc_qqmldebugmessageclient_p.cpp"
