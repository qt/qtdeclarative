/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
    virtual QString name() const = 0;
    virtual void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) = 0;
    virtual void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                                   QLspSpecification::InitializeResult &) = 0;
};

class QLanguageServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RunStatus runStatus READ runStatus NOTIFY runStatusChanged)
    Q_PROPERTY(bool isInitialized READ isInitialized)
public:
    QLanguageServer(const QJsonRpcTransport::DataHandler &h, QObject *parent = nullptr);
    enum class RunStatus {
        NotSetup,
        SettingUp,
        DidSetup,
        Initializing,
        DidInitialize, // normal state of execution
        WaitPending,
        Stopping,
        Stopped
    };
    Q_ENUM(RunStatus)

    QLanguageServerProtocol *protocol();
    void finishSetup();
    void registerHandlers(QLanguageServerProtocol *protocol);
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &serverInfo);
    void addServerModule(QLanguageServerModule *serverModule);
    QLanguageServerModule *moduleByName(const QString &n) const;
    QLspNotifySignals *notifySignals();

    // API
    RunStatus runStatus() const;
    bool isInitialized() const;
    bool isRequestCanceled(const QJsonRpc::IdType &id) const;
    const QLspSpecification::InitializeParams &clientInfo() const;
    const QLspSpecification::InitializeResult &serverInfo() const;

public slots:
    void receiveData(const QByteArray &d);
signals:
    void runStatusChanged(RunStatus);
    void clientInitialized(QLanguageServer *server);
    void shutdown();
    void exit();
    void lifecycleError();

private:
    void registerMethods(QJsonRpc::TypedRpc &typedRpc);
    void executeShutdown();
    Q_DECLARE_PRIVATE(QLanguageServer)
};

QT_END_NAMESPACE

#endif // QLANGUAGESERVER_P_H
