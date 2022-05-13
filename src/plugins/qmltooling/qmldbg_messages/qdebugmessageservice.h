// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDEBUGMESSAGESERVICE_H
#define QDEBUGMESSAGESERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qlogging.h>
#include <QtCore/qmutex.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QDebugMessageServiceImpl : public QDebugMessageService
{
    Q_OBJECT
public:
    QDebugMessageServiceImpl(QObject *parent = nullptr);

    void sendDebugMessage(QtMsgType type, const QMessageLogContext &ctxt, const QString &buf);
    void synchronizeTime(const QElapsedTimer &otherTimer) override;

protected:
    void stateChanged(State) override;

private:
    friend class QQmlDebuggerServiceFactory;

    QtMessageHandler oldMsgHandler;
    QQmlDebugService::State prevState;
    QMutex initMutex;
    QElapsedTimer timer;
};

QT_END_NAMESPACE

#endif // QDEBUGMESSAGESERVICE_H
