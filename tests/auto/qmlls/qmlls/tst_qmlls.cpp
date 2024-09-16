// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

#include <iostream>

using namespace Qt::StringLiterals;
using namespace QLspSpecification;

class DiagnosticsHandler
{
public:
    void handleNotification(const PublishDiagnosticsParams &params)
    {
        m_received.append(PublishDiagnosticsParams(params));
    }

    bool contains(const QString &uri, int line1, int column1, int line2, int column2) const
    {
        for (const auto &params : m_received) {
            if (params.uri != uri)
                continue;
            for (const auto &diagnostic : params.diagnostics) {
                const auto range = diagnostic.range;
                if (range.start.line == line1 && range.start.character == column1
                    && range.end.line == line2 && range.end.character == column2) {
                    return true;
                }
            }
        }
        return false;
    }

    int numDiagnostics(const QByteArray &uri) const
    {
        int num = 0;
        for (const auto &params : m_received) {
            if (params.uri == uri)
                num += params.diagnostics.size();
        }
        return num;
    }

    QList<Diagnostic> diagnostics(const QByteArray &uri) const
    {
        QList<Diagnostic> result;
        for (const auto &params : m_received) {
            if (params.uri == uri)
                result << params.diagnostics;
        }

        return result;
    }

    void clear() { m_received.clear(); }

private:
    QList<PublishDiagnosticsParams> m_received;
};

class tst_Qmlls : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_Qmlls();
private slots:
    void initTestCase() final;
    void didOpenTextDocument();
    void testWorkspace();
    void cleanupTestCase();

private:
    QProcess m_server;
    QLanguageServerProtocol m_protocol;
    DiagnosticsHandler m_diagnosticsHandler;
    QString m_qmllsPath;
    QList<RegistrationParams> m_registrations;
};

tst_Qmlls::tst_Qmlls()
    : QQmlDataTest(QT_QMLTEST_DATADIR),
      m_protocol([this](const QByteArray &data) { m_server.write(data); })
{
    connect(&m_server, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_server.readAllStandardOutput();
        m_protocol.receiveData(data);
    });

    connect(&m_server, &QProcess::readyReadStandardError, this,
            [this]() {
        QProcess::ProcessChannel tmp = m_server.readChannel();
        m_server.setReadChannel(QProcess::StandardError);
        while (m_server.canReadLine())
            std::cerr << m_server.readLine().constData();
        m_server.setReadChannel(tmp);
    });

    m_qmllsPath =
            QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlls");
#ifdef Q_OS_WIN
    m_qmllsPath += QLatin1String(".exe");
#endif
    // allow overriding of the executable, to be able to use a qmlEcho script (as described in
    // qmllanguageservertool.cpp)
    m_qmllsPath = qEnvironmentVariable("QMLLS", m_qmllsPath);
    m_server.setProgram(m_qmllsPath);
    m_protocol.registerPublishDiagnosticsNotificationHandler(
            [this](const QByteArray &, auto params) {
                m_diagnosticsHandler.handleNotification(params);
            });
}

void tst_Qmlls::initTestCase()
{
    QQmlDataTest::initTestCase();
    if (!QFileInfo::exists(m_qmllsPath)) {
        QString message =
                QStringLiteral("qmlls executable not found (looked for %0)").arg(m_qmllsPath);
        QSKIP(qPrintable(message)); // until we add a feature for this we avoid failing here
    }
    m_server.start();
    InitializeParams clientInfo;
    clientInfo.rootUri = QUrl::fromLocalFile(dataDirectory() + "/default").toString().toUtf8();
    TextDocumentClientCapabilities tDoc;
    PublishDiagnosticsClientCapabilities pDiag;
    tDoc.publishDiagnostics = pDiag;
    pDiag.versionSupport = true;
    clientInfo.capabilities.textDocument = tDoc;
    QJsonObject workspace({ { u"didChangeWatchedFiles"_s,
                              QJsonObject({ { u"dynamicRegistration"_s, true } }) } });
    clientInfo.capabilities.workspace = workspace;
    bool didInit = false;
    m_protocol.registerRegistrationRequestHandler([this](const QByteArray &,
                                                         const RegistrationParams &params,
                                                         LSPResponse<std::nullptr_t> &&response) {
        m_registrations.append(params);
        response.sendResponse();
    });
    m_protocol.requestInitialize(clientInfo, [this, &didInit](const InitializeResult &serverInfo) {
        Q_UNUSED(serverInfo);
        m_protocol.notifyInitialized(InitializedParams());
        didInit = true;
    });
    QTRY_COMPARE_WITH_TIMEOUT(didInit, true, 10000);
}

void tst_Qmlls::didOpenTextDocument()
{
    QFile file(testFile("default/Yyy.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    DidOpenTextDocumentParams oParams;
    TextDocumentItem textDocument;
    QByteArray uri = testFileUrl("default/Yyy.qml").toString().toUtf8();
    textDocument.uri = uri;
    textDocument.text = file.readAll().replace("width", "wildth");
    oParams.textDocument = textDocument;
    m_protocol.notifyDidOpenTextDocument(oParams);

    QTRY_VERIFY_WITH_TIMEOUT(m_diagnosticsHandler.numDiagnostics(uri) != 0, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(m_diagnosticsHandler.contains(uri, 3, 4, 3, 10), 10000);

    auto diagnostics = m_diagnosticsHandler.diagnostics(uri);

    CodeActionParams codeActionParams;
    codeActionParams.textDocument = { textDocument.uri };
    codeActionParams.context.diagnostics = diagnostics;
    codeActionParams.range.start = Position { 0, 0 };
    codeActionParams.range.end =
            Position { static_cast<int>(textDocument.text.split(u'\n').size()), 0 };

    bool success = false;
    m_protocol.requestCodeAction(
            codeActionParams,
            [&](const std::variant<QList<std::variant<Command, CodeAction>>, std::nullptr_t>
                        &response) {
                using ListType = QList<std::variant<Command, CodeAction>>;

                QVERIFY(std::holds_alternative<ListType>(response));

                auto list = std::get<ListType>(response);

                struct ReplacementData
                {
                    QString replacement;
                    Range range;
                };

                QHash<QString, ReplacementData> expectedData = {
                    { QLatin1StringView("Did you mean \"width\"?"),
                      { QLatin1StringView("width"),
                        Range { Position { 3, 4 }, Position { 3, 10 } } } },
                    { QLatin1StringView("Did you mean \"z\"?"),
                      { QLatin1StringView("z"),
                        Range { Position {
                                        3,
                                        12,
                                },
                                Position { 3, 15 } } } }
                };
                QCOMPARE(list.size(), expectedData.size());

                for (const auto &entry : list) {
                    QVERIFY(std::holds_alternative<CodeAction>(entry));
                    CodeAction action = std::get<CodeAction>(entry);

                    QString title = QString::fromUtf8(action.title);
                    QVERIFY(action.kind.has_value());
                    QCOMPARE(QString::fromUtf8(action.kind.value()), QLatin1StringView("quickfix"));
                    QVERIFY(action.edit.has_value());
                    WorkspaceEdit edit = action.edit.value();

                    QVERIFY(edit.documentChanges.has_value());
                    auto documentChanges = edit.documentChanges.value();
                    QCOMPARE(documentChanges.size(), 1);

                    QVERIFY(std::holds_alternative<TextDocumentEdit>(documentChanges.first()));
                    TextDocumentEdit textDocEdit
                            = std::get<TextDocumentEdit>(documentChanges.first());
                    QCOMPARE(textDocEdit.textDocument.uri, textDocument.uri);
                    QVERIFY(std::holds_alternative<int>(textDocEdit.textDocument.version));

                    QCOMPARE(textDocEdit.edits.size(), 1);
                    auto editVariant = textDocEdit.edits.first();
                    QVERIFY(std::holds_alternative<TextEdit>(editVariant));

                    TextEdit textEdit = std::get<TextEdit>(editVariant);
                    QString replacement = QString::fromUtf8(textEdit.newText);
                    const Range &range = textEdit.range;

                    QVERIFY2(expectedData.contains(title),
                             qPrintable(QLatin1String("Unexpected fix \"%1\"").arg(title)));
                    QCOMPARE(replacement, expectedData[title].replacement);
                    QCOMPARE(range.start.line, expectedData[title].range.start.line);
                    QCOMPARE(range.start.character, expectedData[title].range.start.character);
                    QCOMPARE(range.end.line, expectedData[title].range.end.line);
                    QCOMPARE(range.end.character, expectedData[title].range.end.character);
                    // Make sure every expected entry only occurs once
                    expectedData.remove(title);
                }

                success = true;
            },
            [](const QLspSpecification::ResponseError &error) {
                qWarning() << "CodeAction Error:" << QString::fromUtf8(error.message);
            });

    QTRY_VERIFY_WITH_TIMEOUT(success, 10000);
    m_diagnosticsHandler.clear();

    DidChangeTextDocumentParams cParams;
    cParams.textDocument.uri = uri;
    cParams.textDocument.version = 2;
    TextDocumentContentChangeEvent change;
    change.text = file.readAll().replace("wildth", "wid");
    cParams.contentChanges.append(change);
    m_protocol.notifyDidChangeTextDocument(cParams);

    QTRY_VERIFY_WITH_TIMEOUT(m_diagnosticsHandler.numDiagnostics(uri) != 0, 30000);
    m_diagnosticsHandler.clear();

    DidCloseTextDocumentParams closeP;
    closeP.textDocument.uri = uri;
    m_protocol.notifyDidCloseTextDocument(closeP);
}

void tst_Qmlls::testWorkspace()
{
    QTRY_VERIFY_WITH_TIMEOUT(!m_registrations.isEmpty(), 10000);
    QByteArray uri = testFileUrl("default/Zzz.qml").toString().toUtf8();
    DidChangeWatchedFilesParams fChanges;
    FileEvent fEvent;
    fEvent.uri = uri;
    fEvent.type = int(FileChangeType::Changed);
    fChanges.changes.append(fEvent);
    m_protocol.notifyDidChangeWatchedFiles(fChanges);
    DidChangeWorkspaceFoldersParams dChanges;
    WorkspaceFolder dir;
    dir.name = "default";
    dir.uri = uri.mid(uri.lastIndexOf('/'));
    dChanges.event.added.append(dir);
    dChanges.event.removed.append(dir);
    m_protocol.notifyDidChangeWorkspaceFolders(dChanges);

    DidOpenTextDocumentParams oParams;
    TextDocumentItem textDocument;
    textDocument.uri = uri;
    QFile file(testFile("default/Yyy.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    textDocument.text = file.readAll().replace("width", "wildth");
    oParams.textDocument = textDocument;
    m_protocol.notifyDidOpenTextDocument(oParams);

    QTRY_VERIFY_WITH_TIMEOUT(m_diagnosticsHandler.numDiagnostics(uri) != 0, 30000);
    m_diagnosticsHandler.clear();

    DidCloseTextDocumentParams closeP;
    closeP.textDocument.uri = uri;
    m_protocol.notifyDidCloseTextDocument(closeP);
}

void tst_Qmlls::cleanupTestCase()
{
    m_server.closeWriteChannel();
    QTRY_COMPARE_WITH_TIMEOUT(m_server.state(), QProcess::NotRunning, 10000);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

QTEST_MAIN(tst_Qmlls)

#include <tst_qmlls.moc>
