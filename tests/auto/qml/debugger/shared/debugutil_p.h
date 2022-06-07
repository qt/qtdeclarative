// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DEBUGUTIL_P_H
#define DEBUGUTIL_P_H

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

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <private/qqmldebugclient_p.h>

class QQmlDebugProcess;
class QQmlDebugTest : public QQmlDataTest
{
    Q_OBJECT
public:
    QQmlDebugTest(const char *qmlTestDataDir,
        FailOnWarningsPolicy failOnWarningsPolicy = FailOnWarningsPolicy::DoNotFailOnWarnings);

public:
    static bool waitForSignal(QObject *receiver, const char *member, int timeout = 5000);
    static QList<QQmlDebugClient *> createOtherClients(QQmlDebugConnection *connection);
    static QString clientStateString(const QQmlDebugClient *client);
    static QString connectionStateString(const QQmlDebugConnection *connection);

    enum ConnectResult {
        ConnectSuccess,
        ProcessFailed,
        SessionFailed,
        ConnectionFailed,
        ClientsFailed,
        ConnectionTimeout,
        EnableFailed,
        RestrictFailed
    };

    Q_ENUM(ConnectResult)
protected:
    ConnectResult connectTo(const QString &executable, const QString &services,
                          const QString &extraArgs, bool block);

    virtual QQmlDebugProcess *createProcess(const QString &executable);
    virtual QQmlDebugConnection *createConnection();
    virtual QList<QQmlDebugClient *> createClients();

    QQmlDebugProcess *m_process = nullptr;
    QQmlDebugConnection *m_connection = nullptr;
    QList<QQmlDebugClient *> m_clients;

protected slots:
    virtual void cleanup();
};

class QQmlDebugTestClient : public QQmlDebugClient
{
    Q_OBJECT
public:
    QQmlDebugTestClient(const QString &s, QQmlDebugConnection *c);

    QByteArray waitForResponse();

signals:
    void serverMessage(const QByteArray &);

protected:
    void messageReceived(const QByteArray &ba) override;

private:
    QByteArray lastMsg;
};

class QQmlInspectorResultRecipient : public QObject
{
    Q_OBJECT
public:
    QQmlInspectorResultRecipient(QObject *parent = nullptr) :
        QObject(parent), lastResponseId(-1), lastResult(false) {}

    int lastResponseId;
    bool lastResult;

    void recordResponse(int requestId, bool result)
    {
        lastResponseId = requestId;
        lastResult = result;
    }
};

class ClientStateHandler : public QObject
{
    Q_OBJECT
public:
    ClientStateHandler(const QList<QQmlDebugClient *> &clients,
                       const QList<QQmlDebugClient *> &others,
                       QQmlDebugClient::State expectedOthers);

    ~ClientStateHandler();

    bool allEnabled() const { return m_allEnabled; }
    bool othersAsExpected() const { return m_othersAsExpected; }

signals:
    void allOk();

private:
    void checkStates();

    const QList<QQmlDebugClient *> m_clients;
    const QList<QQmlDebugClient *> m_others;
    const QQmlDebugClient::State m_expectedOthers;

    bool m_allEnabled = false;
    bool m_othersAsExpected = false;
};

QString debugJsServerPath(const QString &selfPath);

#endif // DEBUGUTIL_P_H
