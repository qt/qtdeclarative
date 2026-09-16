// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLANGUAGESERVER_P_H
#define QLANGUAGESERVER_P_H

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

#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtLanguageServer/private/qlspnotifysignals_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

class QLanguageServer;
class QLanguageServerPrivate;
Q_DECLARE_LOGGING_CATEGORY(lspServerLog)

class QLanguageServerModule : public QObject
{
    Q_OBJECT
public:
    QLanguageServerModule(QObject *parent = nullptr) : QObject(parent) { }
    virtual void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) = 0;
    virtual void setupCapabilities(QLspSpecification::ServerCapabilities &) = 0;
};

class QLanguageServer : public QObject
{
    Q_OBJECT
public:
    QLanguageServer(const QJsonRpcTransport::DataHandler &h, QObject *parent = nullptr);
    enum class RunStatus {
        NotInitialized,
        Initialized, // normal state of execution
        Stopping,
        WaitingForExit,
        Stopped
    };
    Q_ENUM(RunStatus)

    QLanguageServerProtocol *protocol();
    void registerHandlers(QLanguageServerProtocol *protocol);
    void registerModule(QLanguageServerModule *serverModule);
    QLspNotifySignals *notifySignals();

    // API
    RunStatus runStatus() const;
    const QLspSpecification::InitializeParams &clientInfo() const;

public Q_SLOTS:
    void receiveData(const QByteArray &d, bool isEndOfMessage);
Q_SIGNALS:
    void clientInitialized(QLanguageServer *server);
    void exit();
    void lifecycleError();
    void readNextMessage();

private:
    void registerMethods(QJsonRpc::TypedRpc &typedRpc);
    void executeShutdown();
    Q_DECLARE_PRIVATE(QLanguageServer)
};

QT_END_NAMESPACE

#endif // QLANGUAGESERVER_P_H
