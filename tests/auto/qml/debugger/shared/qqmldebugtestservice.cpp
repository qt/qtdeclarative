// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmldebugtestservice.h"
#include <QThread>

QQmlDebugTestService::QQmlDebugTestService(const QString &s, float version, QObject *parent)
    : QQmlDebugService(s, version, parent)
{
}

void QQmlDebugTestService::messageReceived(const QByteArray &ba)
{
    Q_ASSERT(QThread::currentThread() != thread());
    emit messageToClient(name(), ba);
}

void QQmlDebugTestService::stateAboutToBeChanged(QQmlDebugService::State)
{
    Q_ASSERT(QThread::currentThread() != thread());
}

void QQmlDebugTestService::stateChanged(State)
{
    Q_ASSERT(QThread::currentThread() != thread());
}
