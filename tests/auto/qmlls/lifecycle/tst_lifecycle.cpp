// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class TestRigEventHandler
{
public:
    TestRigEventHandler(QLanguageServer *server, QIODevice *device)
        : m_device(device), m_server(server)
    {
        server->finishSetup();
        QObject::connect(device, &QIODevice::readyRead,
                         [this]() { m_server->protocol()->receiveData(m_device->readAll()); });
        QObject::connect(m_server, &QLanguageServer::exit, [this]() { m_hasExited = true; });
    }

    bool hasExited() const { return m_hasExited; }

    QLanguageServer *server() const { return m_server; }

private:
    QIODevice *m_device = nullptr;
    QLanguageServer *m_server = nullptr;
    bool m_hasExited = false;
};

class tst_LifeCycle : public QObject
{
    Q_OBJECT

private slots:
    void lifecycle();
};

void tst_LifeCycle::lifecycle()
{
    QIOPipe pipe;
    pipe.open(QIODevice::ReadWrite);

    QLanguageServer server([&pipe](const QByteArray &data) {
        QMetaObject::invokeMethod(pipe.end1(), [&pipe, data]() { pipe.end1()->write(data); });
    });
    TestRigEventHandler handler(&server, pipe.end1());
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::DidSetup);

    QLanguageServerJsonRpcTransport transport;

    QLanguageServerProtocol protocol([&pipe](const QByteArray &data) { pipe.end2()->write(data); });
    QObject::connect(pipe.end1(), &QIODevice::readyRead,
                     [&pipe, &protocol]() { protocol.receiveData(pipe.end2()->readAll()); });
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::DidSetup);

    enum class RequestStatus {
        NoResponse,
        Success,
        Failure
    } requestStatus = RequestStatus::NoResponse;

    protocol.requestApplyWorkspaceEdit(
            ApplyWorkspaceEditParams(),
            [&requestStatus](const auto &) { requestStatus = RequestStatus::Success; },
            [&requestStatus](const ResponseError &err) {
                QCOMPARE(err.code, int(ErrorCodes::ServerNotInitialized));
                requestStatus = RequestStatus::Failure;
            });
    QTRY_VERIFY(requestStatus != RequestStatus::NoResponse);
    QCOMPARE(requestStatus, RequestStatus::Failure);

    InitializeParams clientInfo;
    clientInfo.rootUri = nullptr;
    clientInfo.processId = nullptr;
    requestStatus = RequestStatus::NoResponse;
    auto requestFailureHandler = [&requestStatus, &protocol](const ResponseError &err) {
        protocol.defaultResponseErrorHandler(err);
        requestStatus = RequestStatus::Failure;
    };
    protocol.requestInitialize(
            clientInfo,
            [&requestStatus](const InitializeResult &serverInfo) {
                Q_UNUSED(serverInfo);
                requestStatus = RequestStatus::Success;
            },
            requestFailureHandler);
    QTRY_VERIFY(requestStatus != RequestStatus::NoResponse);
    QCOMPARE(requestStatus, RequestStatus::Success);
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::DidInitialize);

    protocol.notifyInitialized(InitializedParams());

    requestStatus = RequestStatus::NoResponse;
    protocol.requestShutdown(
            nullptr, [&requestStatus]() { requestStatus = RequestStatus::Success; },
            requestFailureHandler);
    QTRY_VERIFY(requestStatus != RequestStatus::NoResponse);
    QCOMPARE(requestStatus, RequestStatus::Success);

    QVERIFY(!handler.hasExited());
    QCOMPARE(server.runStatus(), QLanguageServer::RunStatus::Stopped);

    protocol.notifyExit(nullptr);

    QTRY_VERIFY(handler.hasExited());
}

QTEST_MAIN(tst_LifeCycle)
#include <tst_lifecycle.moc>
