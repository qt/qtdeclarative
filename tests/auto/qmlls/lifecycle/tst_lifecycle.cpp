// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qiopipe.h"

#include <QtLanguageServer/private/qlanguageserverjsonrpctransport_p.h>

#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>

#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQmlLS/private/qlanguageserver_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

using namespace QLspSpecification;
using namespace Qt::StringLiterals;

class TestRigEventHandler
{
public:
    TestRigEventHandler(QLanguageServer *server, QIODevice *device)
        : m_device(device), m_server(server)
    {
        server->finishSetup();
        QObject::connect(device, &QIODevice::readyRead, m_server,
                         [this]() { m_server->protocol()->receiveData(m_device->readAll()); });
        QObject::connect(m_server, &QLanguageServer::exit, m_server,
                         [this] { m_hasExited = true; });
    }

    bool hasExited() const { return m_hasExited; }

    QLanguageServer *server() const { return m_server; }

private:
    QIODevice *m_device = nullptr;
    QLanguageServer *m_server = nullptr;
    bool m_hasExited = false;
};

enum class RequestStatus { NoResponse, Success, Failure };

struct State {
    QIOPipe pipe;
    QLanguageServer server;
    TestRigEventHandler handler;
    QLanguageServerJsonRpcTransport transport;
    QLanguageServerProtocol protocol;
    RequestStatus requestStatus = RequestStatus::NoResponse;

    State();

    QLanguageServerProtocol::ResponseErrorHandler getRequestFailureHandler()
    {
        return [this](const ResponseError &err) {
            protocol.defaultResponseErrorHandler(err);
            requestStatus = RequestStatus::Failure;
        };
    }
};

class tst_LifeCycle : public QObject
{
    Q_OBJECT

    static QByteArray addHeaderToRequest(const QByteArray &rawRequest)
    {
        QByteArray result{ "Content-Length: " };
        result += QByteArray::number(rawRequest.size());
        result += "\r\n\r\n";
        result += rawRequest;
        return result;
    }

    const QByteArray shutdownRequest = addHeaderToRequest(
            "{\"id\":\"{126974d0-927b-402e-8207-c43e6d344536}\",\"jsonrpc\":\"2.0\","
            "\"method\": \"shutdown\",\"params\":null}");
    const QByteArray exitNotification =
            addHeaderToRequest("{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":null}");

private slots:
    void lifecycle();
    void separateShutdownAndExit();
    void joinedShutdownAndExit();

};

State::State()
    : server([this](const QByteArray &data) {
          QMetaObject::invokeMethod(pipe.end1(), [this, data]() { pipe.end1()->write(data); });
      }),
      handler(&server, pipe.end1()),
      protocol([this](const QByteArray &data) { pipe.end2()->write(data); })
{
    pipe.open(QIODevice::ReadWrite);
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::DidSetup);

    QObject::connect(pipe.end1(), &QIODevice::readyRead, &pipe,
                     [this]() { protocol.receiveData(pipe.end2()->readAll()); });
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::DidSetup);

    protocol.requestApplyWorkspaceEdit(
            ApplyWorkspaceEditParams(),
            [this](const auto &) { requestStatus = RequestStatus::Success; },
            [this](const ResponseError &err) {
                QCOMPARE(err.code, int(ErrorCodes::ServerNotInitialized));
                requestStatus = RequestStatus::Failure;
            });
    QTRY_VERIFY(requestStatus != RequestStatus::NoResponse);
    QCOMPARE(requestStatus, RequestStatus::Failure);

    InitializeParams clientInfo;
    clientInfo.rootUri = nullptr;
    clientInfo.processId = nullptr;
    requestStatus = RequestStatus::NoResponse;
    protocol.requestInitialize(
            clientInfo,
            [this](const InitializeResult &serverInfo) {
                Q_UNUSED(serverInfo);
                requestStatus = RequestStatus::Success;
            },
            getRequestFailureHandler());
    QTRY_VERIFY(requestStatus != RequestStatus::NoResponse);
    QCOMPARE(requestStatus, RequestStatus::Success);
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::DidInitialize);

    protocol.notifyInitialized(InitializedParams());
}

void tst_LifeCycle::lifecycle()
{
    State s;
    s.protocol.requestShutdown(
            nullptr, [&s]() { s.requestStatus = RequestStatus::Success; },
            s.getRequestFailureHandler());
    QTRY_VERIFY(s.requestStatus != RequestStatus::NoResponse);
    QCOMPARE(s.requestStatus, RequestStatus::Success);

    QVERIFY(!s.handler.hasExited());
    QCOMPARE(s.server.runStatus(), QLanguageServer::RunStatus::WaitingForExit);

    s.protocol.notifyExit(nullptr);

    QTRY_VERIFY(s.handler.hasExited());
    QCOMPARE(s.server.runStatus(), QLanguageServer::RunStatus::Stopped);
}

void tst_LifeCycle::separateShutdownAndExit()
{
    State s;

    QSignalSpy didRequestNextMessage(&s.server, &QLanguageServer::readNextMessage);
    QVERIFY(didRequestNextMessage.isValid());

    s.server.receiveData(shutdownRequest, true);

    QTRY_VERIFY(s.requestStatus != RequestStatus::NoResponse);
    QCOMPARE(s.requestStatus, RequestStatus::Success);

    QVERIFY(!s.handler.hasExited());
    QCOMPARE(s.server.runStatus(), QLanguageServer::RunStatus::WaitingForExit);
    // trigger requestNextMessage() to read the exit notification before actually shutting down
    QCOMPARE(didRequestNextMessage.size(), 1);

    s.server.receiveData(exitNotification, true);

    QTRY_VERIFY(s.handler.hasExited());
    QCOMPARE(s.server.runStatus(), QLanguageServer::RunStatus::Stopped);

    // don't read anything anymore after the exit notification
    QCOMPARE(didRequestNextMessage.size(), 1);
}

void tst_LifeCycle::joinedShutdownAndExit()
{
    State s;

    QSignalSpy didRequestNextMessage(&s.server, &QLanguageServer::readNextMessage);
    QVERIFY(didRequestNextMessage.isValid());

    s.server.receiveData(shutdownRequest + exitNotification, true);

    QTRY_VERIFY(s.requestStatus != RequestStatus::NoResponse);
    QCOMPARE(s.requestStatus, RequestStatus::Success);

    // don't trigger requestNextMessage() to read the exit notification before actually shutting down
    QCOMPARE(didRequestNextMessage.size(), 0);

    QCOMPARE(s.server.runStatus(), QLanguageServer::RunStatus::Stopped);
    QVERIFY(s.handler.hasExited());
}



QTEST_MAIN(tst_LifeCycle)
#include <tst_lifecycle.moc>
