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

void tst_qmlls_code_action::wrapComponentInLoader_data()
{
    QTest::addColumn<TextDocumentItem>("document");
    QTest::addColumn<Range>("range");
    QTest::addColumn<CodeActions>("expected");

    using TextEdits = decltype(std::declval<TextDocumentEdit>().edits);
    const auto codeAction = [](TextEdits &&edits) -> CodeAction {
        TextDocumentEdit textDocEdit;
        textDocEdit.textDocument = { { "file://tst.qml" }, 0 };
        textDocEdit.edits = std::move(edits);

        WorkspaceEdit workspaceEdit;
        workspaceEdit.documentChanges = { textDocEdit };

        CodeAction codeAction;
        codeAction.kind = CodeActionKind::RefactorRewrite;
        codeAction.title = "Wrap Component in Loader";
        codeAction.edit = workspaceEdit;
        return codeAction;
    };

    //Note items to be wraped are correctly found only based on the "end" position of the range
    // TODO rewrite after adding apply(TextEdits) to test LSP Client

    const auto singleItemDoc = fakeTextDocument("import QtQuick; Item{}");
    QTest::newRow("emptyRange") << singleItemDoc << Range{} << CodeActions{};
    QTest::newRow("import") << singleItemDoc << Range{ { 0, 4 }, { 0, 14 } } << CodeActions{};
    QTest::newRow("rootItem") << singleItemDoc << Range{ { 0, 16 }, { 0, 22 } } << CodeActions{};

    const auto propertyDoc = fakeTextDocument("import QtQuick; Item{ property var p }");
    QTest::newRow("property_def") << propertyDoc << Range{ { 0, 22 }, { 0, 36 } } << CodeActions{};
    QTest::newRow("property_name") << propertyDoc << Range{ { 0, 35 }, { 0, 36 } } << CodeActions{};

    const auto itemWithBindingDoc =
            fakeTextDocument("import QtQuick; Item{ property var p: Item {} }");
    QTest::newRow("binding") << itemWithBindingDoc << Range{ { 0, 39 }, { 0, 45 } }
                             << CodeActions{};

    {
        const auto itemInsideBindingDoc =
                fakeTextDocument("import QtQuick; Item{ property var p: Item { Item{} } }");

        TextEdits edits{
            TextEdit{ Range{ { 0, 45 }, { 0, 45 } },
                      "// TODO: Move position bindings from the component to the Loader.\n"
                      "//       Check all uses of 'parent' inside the root element of the "
                      "component.\n" },
            TextEdit{ Range{ { 0, 50 }, { 0, 50 } }, "\n" },
            TextEdit{ Range{ { 0, 45 }, { 0, 45 } }, "Component {\n    id: component_Item\n" },
            TextEdit{ Range{ { 0, 51 }, { 0, 51 } }, "\n}\n" },
            TextEdit{ Range{ { 0, 51 }, { 0, 51 } },
                      "Loader {\n    id: loader_Item\n    sourceComponent: component_Item\n}\n" }
        };

        QTest::newRow("inside_binding") << itemInsideBindingDoc << Range{ { 0, 45 }, { 0, 50 } }
                                        << CodeActions{ codeAction(std::move(edits)) };
    }

    const auto twoItemsDoc = fakeTextDocument("import QtQuick; Item{ Item{} }");
    QTest::newRow("rootItem2") << twoItemsDoc << Range{ { 0, 16 }, { 0, 30 } } << CodeActions{};

    {
        TextEdits edits{
            TextEdit{ Range{ { 0, 22 }, { 0, 22 } },
                      "// TODO: Move position bindings from the component to the Loader.\n"
                      "//       Check all uses of 'parent' inside the root element of the "
                      "component.\n" },
            TextEdit{ Range{ { 0, 27 }, { 0, 27 } }, "\n" },
            TextEdit{ Range{ { 0, 22 }, { 0, 22 } }, "Component {\n    id: component_Item\n" },
            TextEdit{ Range{ { 0, 28 }, { 0, 28 } }, "\n}\n" },
            TextEdit{ Range{ { 0, 28 }, { 0, 28 } },
                      "Loader {\n    id: loader_Item\n    sourceComponent: component_Item\n}\n" }
        };

        QTest::newRow("childItem") << twoItemsDoc << Range{ { 0, 22 }, { 0, 28 } }
                                   << CodeActions{ codeAction(std::move(edits)) };
    }

    const auto nestedDoc = fakeTextDocument("import QtQuick;\n"
                                            "Item {\n"
                                            "   Item {\n"
                                            "       id: item1\n"
                                            "       Item {\n"
                                            "           id: item2\n"
                                            "           Item {id: item3}\n"
                                            "       }\n"
                                            "   }\n"
                                            "}");
    QTest::newRow("nestedDoc_RootItem")
            << nestedDoc << Range{ { 1, 1 }, { 9, 0 } } << CodeActions{};

    {
        TextEdits edits{
            TextEdit{ Range{ { 2, 3 }, { 2, 3 } },
                      "// TODO: Move position bindings from the component to the Loader.\n"
                      "//       Check all uses of 'parent' inside the root element of the "
                      "component.\n"
                      "//       Rename all outer uses of the id \"item1\" to "
                      "\"loader_item1.item\".\n"
                      "//       Rename all outer uses of the id \"item2\" "
                      "to \"loader_item1.item.item2\".\n"
                      "//       Rename all outer uses of the id "
                      "\"item3\" to \"loader_item1.item.item3\".\n" },
            TextEdit{ Range{ { 2, 9 }, { 2, 9 } },
                      "\n"
                      "property alias item2: inner_item2\n"
                      "property alias item3: inner_item3\n" },
            TextEdit{ Range{ { 5, 15 }, { 5, 15 } }, "inner_" },
            TextEdit{ Range{ { 6, 21 }, { 6, 21 } }, "inner_" },
            TextEdit{ Range{ { 2, 3 }, { 2, 3 } }, "Component {\n    id: component_item1\n" },
            TextEdit{ Range{ { 8, 4 }, { 8, 4 } }, "\n}\n" },
            TextEdit{
                    Range{ { 8, 4 }, { 8, 4 } },
                    "Loader {\n    id: loader_item1\n    sourceComponent: component_item1\n}\n" }
        };

        QTest::newRow("nestedDoc_Item1") << nestedDoc << Range{ { 2, 4 }, { 8, 3 } }
                                         << CodeActions{ codeAction(std::move(edits)) };
    }
}

void tst_qmlls_code_action::wrapComponentInLoader()
{
    auto [client, server] = LSPSession::createAndInitialize();

    QFETCH(TextDocumentItem, document);
    QFETCH(Range, range);
    QFETCH(CodeActions, expected);

    CodeActionParams params;
    params.textDocument.uri = client->open(document);
    params.range = range;

    bool didFinish = false;
    CodeActions result;
    auto responseHandler = [&result, &didFinish](auto res) {
        result = res.value_or(CodeActions{});
        didFinish = true;
    };

    client->requestCodeAction(params, responseHandler);

    QTRY_VERIFY_WITH_TIMEOUT(didFinish, 1000);

    const auto resultJson = QTypedJson::toJsonValue(result);
    const auto expectedJson = QTypedJson::toJsonValue(expected);
    QCOMPARE(resultJson, expectedJson);
}

QTEST_MAIN(tst_qmlls_code_action)
