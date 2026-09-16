// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qlanguageserver_p.h>
#include <private/qqmllanguageserver_p.h>
#include <QtTest/qtest.h>

#include "tst_qqmllanguageserver.h"

using namespace Qt::StringLiterals;
using namespace QLspSpecification;
using namespace QmlLsp;

tst_qqmllanguageserver::tst_qqmllanguageserver() { }

struct ClientAndServer
{
    std::unique_ptr<QLanguageServerProtocol> client;
    std::unique_ptr<QQmlLanguageServer> server;

    ClientAndServer()
    {
        client = std::make_unique<QLanguageServerProtocol>(
                [this](const QByteArray &data) { server->receiveData(data, true); });
        server = std::make_unique<QQmlLanguageServer>(
                [this](const QByteArray &data) { client->receiveData(data); });
    }

    static ClientAndServer createAndInitialize()
    {
        ClientAndServer result;

        bool initializedOk = false;
        InitializeParams initializeParams;
        initializeParams.capabilities.window.emplace().workDoneProgress = true;
        result.client->requestInitialize(
                initializeParams,
                [&initializedOk](const InitializeResult &) { initializedOk = true; });
        [&initializedOk] { QTRY_VERIFY_WITH_TIMEOUT(initializedOk, 3000); }();
        result.client->notifyInitialized({ });

        return result;
    }
};

void tst_qqmllanguageserver::noFreezeAfterRequestToClient()
{
    auto [client, server] = ClientAndServer::createAndInitialize();

    client->registerWorkspaceSemanticTokensRefreshRequestHandler(
            [](const QByteArray, const std::nullptr_t &, auto &&response) {
                response.sendResponse({ });
            });

    bool ok = false;
    server->protocol()->requestWorkspaceSemanticTokensRefresh({ }, [&ok]() { ok = true; });
    QTRY_VERIFY_WITH_TIMEOUT(ok, 5000);

    ok = false;
    client->requestShutdown({ }, [&ok, &c = client]() {
        c->notifyExit({ });
        ok = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(ok, 5000);
}

QTEST_MAIN(tst_qqmllanguageserver)
