/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdebugmessageservice_p.h"
#include "qqmldebugservice_p_p.h"

#include <QDataStream>
#include <QMutex>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QDebugMessageService, qmlDebugMessageService)

void DebugMessageHandler(QtMsgType type, const QMessageLogContext &ctxt,
                         const char *buf)
{
    QDebugMessageService::instance()->sendDebugMessage(type, ctxt, buf);
}

class QDebugMessageServicePrivate : public QQmlDebugServicePrivate
{
public:
    QDebugMessageServicePrivate()
        : oldMsgHandler(0)
        , prevState(QQmlDebugService::NotConnected)
    {
    }

    QMessageHandler oldMsgHandler;
    QQmlDebugService::State prevState;
    QMutex initMutex;
};

QDebugMessageService::QDebugMessageService(QObject *parent) :
    QQmlDebugService(*(new QDebugMessageServicePrivate()),
                                   QStringLiteral("DebugMessages"), 2, parent)
{
    Q_D(QDebugMessageService);

    // don't execute stateChanged() in parallel
    QMutexLocker lock(&d->initMutex);
    registerService();
    if (state() == Enabled) {
        d->oldMsgHandler = qInstallMessageHandler(DebugMessageHandler);
        d->prevState = Enabled;
    }
}

QDebugMessageService *QDebugMessageService::instance()
{
    return qmlDebugMessageService();
}

void QDebugMessageService::sendDebugMessage(QtMsgType type,
                                            const QMessageLogContext &ctxt,
                                            const char *buf)
{
    Q_D(QDebugMessageService);

    //We do not want to alter the message handling mechanism
    //We just eavesdrop and forward the messages to a port
    //only if a client is connected to it.
    QByteArray message;
    QDataStream ws(&message, QIODevice::WriteOnly);
    ws << QByteArray("MESSAGE") << type << QString::fromLocal8Bit(buf).toUtf8();
    ws << QString::fromLatin1(ctxt.file).toUtf8();
    ws << ctxt.line << QString::fromLatin1(ctxt.function).toUtf8();

    sendMessage(message);
    if (d->oldMsgHandler)
        (*d->oldMsgHandler)(type, ctxt, buf);
}

void QDebugMessageService::stateChanged(State state)
{
    Q_D(QDebugMessageService);
    QMutexLocker lock(&d->initMutex);

    if (state != Enabled && d->prevState == Enabled) {
        QMessageHandler handler = qInstallMessageHandler(d->oldMsgHandler);
        // has our handler been overwritten in between?
        if (handler != DebugMessageHandler)
            qInstallMessageHandler(handler);

    } else if (state == Enabled && d->prevState != Enabled) {
        d->oldMsgHandler = qInstallMessageHandler(DebugMessageHandler);
    }

    d->prevState = state;
}

QT_END_NAMESPACE
