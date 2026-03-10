// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_code_action.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlLS/private/qqmllanguageserver_p.h>
#include <QtTest/qtest.h>

using namespace QmlLsp;
using namespace QLspSpecification;

#include <QLibraryInfo>

// TODO move to qmlls / qml test utils??
class LSPClient : public QLanguageServerProtocol
{
public:
    using QLanguageServerProtocol::QLanguageServerProtocol;

    // TODO need to be careful about the timeouts(?)
    InitializeResult
    sendInitializeRequest(const InitializeParams &initializeParams = InitializeParams{})
    {
        InitializeResult response;
        requestInitialize(initializeParams,
                          [&response](const InitializeResult &result) { response = result; });
        return response;
    }

    QByteArray open(TextDocumentItem doc)
    {
        DidOpenTextDocumentParams oParams;
        oParams.textDocument = doc;
        notifyDidOpenTextDocument(oParams);
        return doc.uri;
    }
};

struct LSPSession
{
    std::unique_ptr<LSPClient> client;
    std::unique_ptr<QQmlLanguageServer> server;

    LSPSession()
    {
        client = std::make_unique<LSPClient>(
                [this](const QByteArray &data) { server->receiveData(data, true); });
        server = std::make_unique<QQmlLanguageServer>(
                [this](const QByteArray &data) { client->receiveData(data); });

        // to find QtQuick...
        server->codeModelManager()->setImportPaths(
                QLibraryInfo::paths(QLibraryInfo::QmlImportsPath));
    }

    static LSPSession
    createAndInitialize(const InitializeParams &initializeParams = InitializeParams{})
    {
        LSPSession session;
        // TODO need to be careful about the timeouts(?)
        session.client->sendInitializeRequest(std::move(initializeParams));
        session.client->notifyInitialized({});

        return session;
    }
};

static TextDocumentItem fakeTextDocument(/*QString?*/ const QByteArray &content,
                                         const QByteArray &uri = "file://tst.qml")
{
    TextDocumentItem textDocument;
    textDocument.uri = uri;
    textDocument.text = content;
    return textDocument;
}

using CodeActions = QList<std::variant<Command, CodeAction>>;

void tst_qmlls_code_action::tst()
{
    auto [client, server] = LSPSession::createAndInitialize();

    QLspSpecification::CodeActionParams params;
    params.textDocument.uri = client->open(fakeTextDocument("import QtQuick; Item{}"));

    bool didFinish = false;
    QList<std::variant<Command, CodeAction>> result;
    auto responseHandler = [&result, &didFinish](auto res) {
        result = res.value_or(CodeActions{});
        didFinish = true;
    };

    client->requestCodeAction(params, responseHandler);

    QTRY_VERIFY_WITH_TIMEOUT(didFinish, 10000);

    QVERIFY(result.isEmpty());
}

QTEST_MAIN(tst_qmlls_code_action)
