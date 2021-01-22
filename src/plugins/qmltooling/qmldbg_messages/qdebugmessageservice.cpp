/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qdebugmessageservice.h"

#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>

QT_BEGIN_NAMESPACE

using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

void DebugMessageHandler(QtMsgType type, const QMessageLogContext &ctxt,
                         const QString &buf)
{
    QQmlDebugConnector::service<QDebugMessageServiceImpl>()->sendDebugMessage(type, ctxt, buf);
}

QDebugMessageServiceImpl::QDebugMessageServiceImpl(QObject *parent) :
    QDebugMessageService(2, parent), oldMsgHandler(nullptr),
    prevState(QQmlDebugService::NotConnected)
{
    // don't execute stateChanged() in parallel
    QMutexLocker lock(&initMutex);
    timer.start();
    if (state() == Enabled) {
        oldMsgHandler = qInstallMessageHandler(DebugMessageHandler);
        prevState = Enabled;
    }
}

void QDebugMessageServiceImpl::sendDebugMessage(QtMsgType type,
                                            const QMessageLogContext &ctxt,
                                            const QString &buf)
{
    //We do not want to alter the message handling mechanism
    //We just eavesdrop and forward the messages to a port
    //only if a client is connected to it.
    QQmlDebugPacket ws;
    ws << QByteArray("MESSAGE") << int(type) << buf.toUtf8();
    ws << QByteArray(ctxt.file) << ctxt.line << QByteArray(ctxt.function);
    ws << QByteArray(ctxt.category) << timer.nsecsElapsed();

    emit messageToClient(name(), ws.data());
    if (oldMsgHandler)
        (*oldMsgHandler)(type, ctxt, buf);
}

void QDebugMessageServiceImpl::stateChanged(State state)
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

void QDebugMessageServiceImpl::synchronizeTime(const QElapsedTimer &otherTimer)
{
    QMutexLocker lock(&initMutex);
    timer = otherTimer;
}

QT_END_NAMESPACE
