// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLENGINECONTROLSERVICE_H
#define QQMLENGINECONTROLSERVICE_H

#include <QMutex>
#include <private/qqmldebugserviceinterfaces_p.h>

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

QT_BEGIN_NAMESPACE

class QQmlEngineControlServiceImpl : public QQmlEngineControlService
{
public:
    enum MessageType {
        EngineAboutToBeAdded,
        EngineAdded,
        EngineAboutToBeRemoved,
        EngineRemoved
    };

    enum CommandType {
        StartWaitingEngine,
        StopWaitingEngine
    };

    QQmlEngineControlServiceImpl(QObject *parent = nullptr);

protected:
    friend class QQmlProfilerServiceFactory;

    QMutex dataMutex;
    QList<QJSEngine *> startingEngines;
    QList<QJSEngine *> stoppingEngines;
    bool blockingMode;

    void messageReceived(const QByteArray &) override;
    void engineAboutToBeAdded(QJSEngine *) override;
    void engineAboutToBeRemoved(QJSEngine *) override;
    void engineAdded(QJSEngine *) override;
    void engineRemoved(QJSEngine *) override;

    void sendMessage(MessageType type, QJSEngine *engine);

    void stateChanged(State) override;
};

QT_END_NAMESPACE

#endif // QQMLENGINECONTROLSERVICE_H
