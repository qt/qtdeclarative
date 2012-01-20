/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdebugmessageservice_p.h"
#include "qdeclarativedebugservice_p_p.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QDebugMessageService, declarativeDebugMessageService)

void DebugMessageHandler(QtMsgType type, const char *buf)
{
    QDebugMessageService::instance()->sendDebugMessage(type, buf);
}

class QDebugMessageServicePrivate : public QDeclarativeDebugServicePrivate
{
public:
    QDebugMessageServicePrivate()
        : oldMsgHandler(0)
        , prevStatus(QDeclarativeDebugService::NotConnected)
    {
    }

    QtMsgHandler oldMsgHandler;
    QDeclarativeDebugService::Status prevStatus;
};

QDebugMessageService::QDebugMessageService(QObject *parent) :
    QDeclarativeDebugService(*(new QDebugMessageServicePrivate()),
                                   QLatin1String("DebugMessages"), 2, parent)
{
    Q_D(QDebugMessageService);

    registerService();
    if (status() == Enabled) {
        d->oldMsgHandler = qInstallMsgHandler(DebugMessageHandler);
        d->prevStatus = Enabled;
    }
}

QDebugMessageService *QDebugMessageService::instance()
{
    return declarativeDebugMessageService();
}

void QDebugMessageService::sendDebugMessage(QtMsgType type, const char *buf)
{
    Q_D(QDebugMessageService);

    //We do not want to alter the message handling mechanism
    //We just eavesdrop and forward the messages to a port
    //only if a client is connected to it.
    QByteArray message;
    QDataStream ws(&message, QIODevice::WriteOnly);
    ws << QByteArray("MESSAGE") << type << QString::fromLocal8Bit(buf).toUtf8();

    sendMessage(message);
    if (d->oldMsgHandler)
        (*d->oldMsgHandler)(type, buf);
}

void QDebugMessageService::statusChanged(Status status)
{
    Q_D(QDebugMessageService);

    if (status != Enabled && d->prevStatus == Enabled) {
        QtMsgHandler handler = qInstallMsgHandler(d->oldMsgHandler);
        // has our handler been overwritten in between?
        if (handler != DebugMessageHandler)
            qInstallMsgHandler(handler);

    } else if (status == Enabled && d->prevStatus != Enabled) {
        d->oldMsgHandler = qInstallMsgHandler(DebugMessageHandler);

    }

    d->prevStatus = status;
}

QT_END_NAMESPACE
