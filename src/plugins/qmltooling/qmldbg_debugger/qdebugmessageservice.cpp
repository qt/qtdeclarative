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

#include "qdebugmessageservice.h"
#include <private/qqmldebugconnector_p.h>

#include <QDataStream>

QT_BEGIN_NAMESPACE

const QString QDebugMessageService::s_key = QStringLiteral("DebugMessages");

void DebugMessageHandler(QtMsgType type, const QMessageLogContext &ctxt,
                         const QString &buf)
{
    QQmlDebugConnector::service<QDebugMessageService>()->sendDebugMessage(type, ctxt, buf);
}

QDebugMessageService::QDebugMessageService(QObject *parent) :
    QQmlDebugService(s_key, 2, parent), oldMsgHandler(0),
    prevState(QQmlDebugService::NotConnected)
{
    // don't execute stateChanged() in parallel
    QMutexLocker lock(&initMutex);
    if (state() == Enabled) {
        oldMsgHandler = qInstallMessageHandler(DebugMessageHandler);
        prevState = Enabled;
    }
}

void QDebugMessageService::sendDebugMessage(QtMsgType type,
                                            const QMessageLogContext &ctxt,
                                            const QString &buf)
{
    //We do not want to alter the message handling mechanism
    //We just eavesdrop and forward the messages to a port
    //only if a client is connected to it.
    QByteArray message;
    QQmlDebugStream ws(&message, QIODevice::WriteOnly);
    ws << QByteArray("MESSAGE") << type << buf.toUtf8();
    ws << QString::fromLatin1(ctxt.file).toUtf8();
    ws << ctxt.line << QString::fromLatin1(ctxt.function).toUtf8();

    emit messageToClient(name(), message);
    if (oldMsgHandler)
        (*oldMsgHandler)(type, ctxt, buf);
}

void QDebugMessageService::stateChanged(State state)
{
    QMutexLocker lock(&initMutex);

    if (state != Enabled && prevState == Enabled) {
        QtMessageHandler handler = qInstallMessageHandler(oldMsgHandler);
        // has our handler been overwritten in between?
        if (handler != DebugMessageHandler)
            qInstallMessageHandler(handler);

    } else if (state == Enabled && prevState != Enabled) {
        oldMsgHandler = qInstallMessageHandler(DebugMessageHandler);
    }

    prevState = state;
}

QT_END_NAMESPACE
