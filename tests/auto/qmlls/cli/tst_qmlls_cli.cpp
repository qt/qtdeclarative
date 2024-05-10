// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_cli.h"

using namespace Qt::StringLiterals;

void tst_qmlls_cli::initTestCase()
{
    QQmlDataTest::initTestCase();

    m_qmllsPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlls");
#ifdef Q_OS_WIN
    m_qmllsPath += QLatin1String(".exe");
#endif
    // allow overriding of the executable, to be able to use a qmlEcho script (as described in
    // qmllanguageservertool.cpp)
    m_qmllsPath = qEnvironmentVariable("QMLLS", m_qmllsPath);
    m_server.setProgram(m_qmllsPath);
}

void tst_qmlls_cli::cleanup()
{
    m_server.closeWriteChannel();
    m_server.waitForFinished();
    QTRY_COMPARE(m_server.state(), QProcess::NotRunning);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

// Helper structs to avoid confusions between expected and unexpected messages and between expected
// and unexpected diagnostics.
struct ExpectedMessages : public QStringList
{
    using QStringList::QStringList;
};
struct UnexpectedMessages : public QStringList
{
    using QStringList::QStringList;
};
struct ExpectedDiagnostics : public QStringList
{
    using QStringList::QStringList;
};
struct UnexpectedDiagnostics : public QStringList
{
    using QStringList::QStringList;
};

// Extra environment variables to be added to qmlls's environment.
struct Environment : public QList<QPair<QString, QString>>
{
    using QList<QPair<QString, QString>>::QList;
};

void tst_qmlls_cli::warnings_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<Environment>("environment");
    QTest::addColumn<QString>("filePath");
    // messages are printed to stderr and not shown in editor:
    QTest::addColumn<ExpectedMessages>("expectedMessages");
    QTest::addColumn<UnexpectedMessages>("unexpectedMessages");
    // diagnostics are passed via LSP to be shown in editor:
    QTest::addColumn<ExpectedDiagnostics>("expectedDiagnostics");
    QTest::addColumn<UnexpectedDiagnostics>("unexpectedDiagnostics");

    const Environment defaultEnv;
    const QString dir1 = testFile(u"ImportPath1"_s);
    const QString dir2 = testFile(u"ImportPath2"_s);
    const QString notDir = testFile(u"ImportPath1/SomeModule/qmldir"_s);
    const QString wrongDir = testFile(u"ImportPathInexistent"_s);

    const QString fileImportingDir1 = testFile(u"sourceFolder/ImportFromImportPath1.qml"_s);
    const QString fileImportingBothDirs = testFile(u"sourceFolder/ImportFromBothPaths.qml"_s);

    const QString importWarningDir1 = u"Warnings occurred while importing module \"SomeModule\""_s;
    const QString importWarningDir2 = u"Warnings occurred while importing module \"AnotherModule\""_s;

    const UnexpectedMessages noUnexpectedMessages;
    const QString warnAboutQmllsIniFiles{
        u"Using the build directories found in the .qmlls.ini file. Your build folder might not be found if no .qmlls.ini files are present in the root source folder."_s
    };

    QTest::addRow("2-build-dirs")
            << QStringList{ u"--build-dir"_s, dir1, u"-b"_s, dir2 } << defaultEnv
            << fileImportingDir1
            << ExpectedMessages{ u"Using build directories passed by -b: \"%1\", \"%2\"."_s.arg(
                       dir1, dir2) }
            << UnexpectedMessages{ warnAboutQmllsIniFiles } << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{ importWarningDir1 };

    QTest::addRow("build-dir-not-dir")
            << QStringList{ u"--build-dir"_s, notDir, u"-b"_s, dir2 } << defaultEnv
            << fileImportingBothDirs
            << ExpectedMessages{ u"Argument \"%1\" passed to -b is not a directory."_s.arg(notDir) }
            << UnexpectedMessages{ warnAboutQmllsIniFiles }
            << ExpectedDiagnostics{ importWarningDir1 }
            << UnexpectedDiagnostics{ importWarningDir2 };

    QTest::addRow("build-dir-not-existing")
            << QStringList{ u"--build-dir"_s, wrongDir, u"-b"_s, dir2 } << defaultEnv
            << fileImportingBothDirs
            << ExpectedMessages{ u"Argument \"%1\" passed to -b does not exist."_s.arg(wrongDir) }
            << UnexpectedMessages{ warnAboutQmllsIniFiles } << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{};

    QTest::addRow("build-dir-from-environment")
            << QStringList{}
            << Environment{ { u"QMLLS_BUILD_DIRS"_s,
                              u"%1%2%3"_s.arg(dir1, QDir::listSeparator(), dir2) } }
            << fileImportingBothDirs
            << ExpectedMessages{ u"Using build directories passed from environment variable \"QMLLS_BUILD_DIRS\": \"%1\", \"%2\"."_s
                                         .arg(dir1, dir2) }
            << UnexpectedMessages{ warnAboutQmllsIniFiles } << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{ importWarningDir1, importWarningDir2 };

    QTest::addRow("build-dir-from-environment-not-existing")
            << QStringList{}
            << Environment{ { u"QMLLS_BUILD_DIRS"_s,
                              QStringList{ dir1, wrongDir, notDir }.join(QDir::listSeparator()) } }
            << fileImportingDir1
            << ExpectedMessages{ u"Argument \"%1\" from environment variable \"QMLLS_BUILD_DIRS\" does not exist."_s
                                         .arg(wrongDir),
                                 u"Argument \"%1\" from environment variable \"QMLLS_BUILD_DIRS\" is not a directory."_s
                                         .arg(notDir) }
            << UnexpectedMessages{ warnAboutQmllsIniFiles } << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{ importWarningDir1, importWarningDir2 };

    QTest::addRow("ignore-environment-with-option")
            << QStringList{ u"--build-dir"_s, dir1 }
            << Environment{ { u"QMLLS_BUILD_DIRS"_s, dir2 } } << fileImportingBothDirs
            << ExpectedMessages{ u"Using build directories passed by -b: \"%1\"."_s.arg(dir1) }
            << UnexpectedMessages{ dir2, warnAboutQmllsIniFiles }
            << ExpectedDiagnostics{ importWarningDir2 }
            << UnexpectedDiagnostics{ importWarningDir1 };

    QTest::addRow("loadFromConfigFile")
            << QStringList{} << Environment{} << fileImportingDir1
            << ExpectedMessages{ warnAboutQmllsIniFiles } << UnexpectedMessages{}
            << ExpectedDiagnostics{ importWarningDir1 } << UnexpectedDiagnostics{};

    QTest::addRow("2-import-paths")
            << QStringList{ u"-I"_s, dir1, u"-I"_s, dir2 } << Environment{} << fileImportingBothDirs
            << ExpectedMessages{ u"Using import directories passed by -I: \"%1\", \"%2\"."_s.arg(
                       dir1, dir2) }
            << UnexpectedMessages{} << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{ importWarningDir1, importWarningDir2 };

    QTest::addRow("import-paths-ignore-env")
            << QStringList{ u"-I"_s, dir1, } << Environment{ { u"QML_IMPORT_PATH"_s, dir2 } }
            << fileImportingBothDirs
            << ExpectedMessages{ u"Using import directories passed by -I: \"%1\"."_s.arg(dir1) }
            << UnexpectedMessages{ u"Using import directories passed from environment variable \"QML_IMPORT_PATH\": \"%1\"."_s.arg(dir2)}
            << ExpectedDiagnostics{importWarningDir2} << UnexpectedDiagnostics{ importWarningDir1 };

    QTest::addRow("2-import-paths-mixed")
            << QStringList{ u"-I"_s, dir1, u"-E"_s }
            << Environment{ { u"QML_IMPORT_PATH"_s, dir2 } } << fileImportingBothDirs
            << ExpectedMessages{ u"Using import directories passed by -I: \"%1\"."_s.arg(dir1),
                                 u"Using import directories passed from environment variable \"QML_IMPORT_PATH\": \"%1\"."_s
                                         .arg(dir2) }
            << UnexpectedMessages{} << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{ importWarningDir1, importWarningDir2 };

    QTest::addRow("2-import-paths-deprecated")
            << QStringList{ u"-I"_s, dir1, u"-E"_s }
            << Environment{ { u"QML2_IMPORT_PATH"_s, dir2 } } << fileImportingBothDirs
            << ExpectedMessages{ u"Using import directories passed by -I: \"%1\"."_s.arg(dir1),
                                 u"Using import directories passed from the deprecated environment variable \"QML2_IMPORT_PATH\": \"%1\"."_s
                                         .arg(dir2) }
            << UnexpectedMessages{} << ExpectedDiagnostics{}
            << UnexpectedDiagnostics{ importWarningDir1, importWarningDir2 };
}

auto tst_qmlls_cli::startServerRAII()
{
    startServerImpl();
    return qScopeGuard([this]() { this->stopServerImpl(); });
}

void tst_qmlls_cli::startServerImpl()
{
    m_protocol = std::make_unique<QLanguageServerProtocol>(
            [this](const QByteArray &data) { m_server.write(data); });

    connect(&m_server, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_server.readAllStandardOutput();
        m_protocol->receiveData(data);
    });

    m_server.start();

    QLspSpecification::InitializeParams clientInfo;
    clientInfo.rootUri = QUrl::fromLocalFile(dataDirectory() + "/default").toString().toUtf8();

    QLspSpecification::TextDocumentClientCapabilities tDoc;
    tDoc.typeDefinition = QLspSpecification::TypeDefinitionClientCapabilities{ false, false };

    QLspSpecification::PublishDiagnosticsClientCapabilities pDiag;
    tDoc.publishDiagnostics = pDiag;
    pDiag.versionSupport = true;
    clientInfo.capabilities.textDocument = tDoc;
    bool didInit = false;
    m_protocol->requestInitialize(
            clientInfo, [this, &didInit](const QLspSpecification::InitializeResult &serverInfo) {
                Q_UNUSED(serverInfo);
                m_protocol->notifyInitialized(QLspSpecification::InitializedParams());
                didInit = true;
            });
    QTRY_COMPARE_WITH_TIMEOUT(didInit, true, 10000);
}

void tst_qmlls_cli::stopServerImpl()
{
    m_server.closeWriteChannel();
    m_server.waitForFinished();
    QTRY_COMPARE(m_server.state(), QProcess::NotRunning);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

void tst_qmlls_cli::warnings()
{
    QFETCH(QStringList, args);
    QFETCH(Environment, environment);
    QFETCH(ExpectedMessages, expectedMessages);
    QFETCH(UnexpectedMessages, unexpectedMessages);
    QFETCH(QString, filePath);
    QFETCH(ExpectedDiagnostics, expectedDiagnostics);
    QFETCH(UnexpectedDiagnostics, unexpectedDiagnostics);

    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    for (const auto &entry : environment)
        processEnvironment.insert(entry.first, entry.second);
    m_server.setProcessEnvironment(processEnvironment);
    m_server.setArguments(args);

    QList<int> countExpectedMessages(expectedMessages.size(), 0);
    QList<int> countUnexpectedMessages(unexpectedMessages.size(), 0);
    QList<int> countExpectedDiagnostics(expectedDiagnostics.size(), 0);
    QList<int> countUnexpectedDiagnostics(unexpectedDiagnostics.size(), 0);

    auto guard = qScopeGuard([this]() {
        // note: the lambda used in the "connect"-call references local variables, so disconnect the
        // lambda via QScopedGuard to avoid its captured references to dangle
        disconnect(&m_server, &QProcess::readyReadStandardOutput, nullptr, nullptr);
    });
    connect(&m_server, &QProcess::readyReadStandardError, this,
            [this, &expectedMessages, &countExpectedMessages, &unexpectedMessages,
             &countUnexpectedMessages]() {
                const auto data = QString::fromUtf8(m_server.readAllStandardError());
                if (data.isEmpty())
                    return;

                for (int i = 0; i < expectedMessages.size(); ++i) {
                    if (data.contains(expectedMessages[i]))
                        ++countExpectedMessages[i];
                }
                for (int i = 0; i < unexpectedMessages.size(); ++i) {
                    if (data.contains(unexpectedMessages[i]))
                        ++countUnexpectedMessages[i];
                }
            });

    auto guard2 = startServerRAII();

    // each expected message should appear exactly one time
    QTRY_COMPARE_WITH_TIMEOUT(countExpectedMessages, QList<int>(expectedMessages.size(), 1), 500);
    // each unexpected message should appear exactly zero times
    QCOMPARE(countUnexpectedMessages, QList<int>(unexpectedMessages.size(), 0));

    bool diagnosticOk = false;
    m_protocol->registerPublishDiagnosticsNotificationHandler(
            [&diagnosticOk, &expectedDiagnostics, &countExpectedDiagnostics, &unexpectedDiagnostics,
             &countUnexpectedDiagnostics](const QByteArray &,
                                          const QLspSpecification::PublishDiagnosticsParams &p) {
                for (const auto &d : p.diagnostics) {
                    const QString message = QString::fromUtf8(d.message);
                    for (int i = 0; i < expectedDiagnostics.size(); ++i) {
                        if (message.contains(expectedDiagnostics[i]))
                            ++countExpectedDiagnostics[i];
                    }
                    for (int i = 0; i < unexpectedDiagnostics.size(); ++i) {
                        if (message.contains(unexpectedDiagnostics[i]))
                            ++countUnexpectedDiagnostics[i];
                    }
                }
                diagnosticOk = true;
            });

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));

    QLspSpecification::DidOpenTextDocumentParams oParams;
    QLspSpecification::TextDocumentItem textDocument;
    QByteArray uri = QUrl::fromLocalFile(filePath).toEncoded();
    textDocument.uri = uri;
    textDocument.text = file.readAll();
    oParams.textDocument = textDocument;
    m_protocol->notifyDidOpenTextDocument(oParams);

    QTRY_VERIFY_WITH_TIMEOUT(diagnosticOk, 3000);
    // each expected diagnostic should appear exactly one time
    QCOMPARE(countExpectedDiagnostics, QList<int>(expectedDiagnostics.size(), 1));
    // each unexpected diagnostic should appear exactly zero times
    QCOMPARE(countUnexpectedDiagnostics, QList<int>(unexpectedDiagnostics.size(), 0));

}

QTEST_MAIN(tst_qmlls_cli)
