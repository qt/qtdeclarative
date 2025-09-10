// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdebugmessageservice.h"

#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>

QT_BEGIN_NAMESPACE

using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

void DebugMessageHandler(QtMsgType type, const QMessageLogContext &ctxt,
                         const QString &buf)
{
    if (QDebugMessageServiceImpl *service = QQmlDebugConnector::service<QDebugMessageServiceImpl>())
        service->sendDebugMessage(type, ctxt, buf);
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

QDebugMessageServiceImpl::~QDebugMessageServiceImpl()
{
    if (oldMsgHandler)
        restoreOldMessageHandler();
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

    if (state != Enabled && prevState == Enabled)
        restoreOldMessageHandler();
    else if (state == Enabled && prevState != Enabled)
        oldMsgHandler = qInstallMessageHandler(DebugMessageHandler);

    prevState = state;
}

void QDebugMessageServiceImpl::restoreOldMessageHandler()
{
    QtMessageHandler handler = qInstallMessageHandler(oldMsgHandler);

    // has our handler been overwritten in between?
    // In that case, leave the other one alone and try again on destruction.
    if (handler == DebugMessageHandler)
        oldMsgHandler = nullptr;
    else
        qInstallMessageHandler(handler);
}

void QDebugMessageServiceImpl::synchronizeTime(const QElapsedTimer &otherTimer)
{
    QMutexLocker lock(&initMutex);
    timer = otherTimer;
}

QT_END_NAMESPACE

#include "moc_qdebugmessageservice.cpp"
