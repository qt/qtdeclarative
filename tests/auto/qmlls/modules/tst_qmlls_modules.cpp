// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlls_modules.h"
#include "QtQmlLS/private/qqmllsutils_p.h"

// Check if QTest already has a QTEST_CHECKED macro
#ifndef QTEST_CHECKED
#define QTEST_CHECKED(...) \
do { \
    __VA_ARGS__; \
    if (QTest::currentTestFailed()) \
        return; \
} while (false)
#endif

QT_USE_NAMESPACE
using namespace Qt::StringLiterals;
using namespace QLspSpecification;

static constexpr bool enable_debug_output = true;

tst_qmlls_modules::tst_qmlls_modules()
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
    // m_server.setArguments(QStringList() << u"-v"_s << u"-w"_s << u"8"_s);
    m_protocol.registerPublishDiagnosticsNotificationHandler([](const QByteArray &, auto) {
        // ignoring qmlint notifications
    });
}

void tst_qmlls_modules::initTestCase()
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
    tDoc.typeDefinition = TypeDefinitionClientCapabilities{ false, false };

    PublishDiagnosticsClientCapabilities pDiag;
    tDoc.publishDiagnostics = pDiag;
    pDiag.versionSupport = true;
    clientInfo.capabilities.textDocument = tDoc;
    bool didInit = false;
    m_protocol.requestInitialize(clientInfo, [this, &didInit](const InitializeResult &serverInfo) {
        Q_UNUSED(serverInfo);
        m_protocol.notifyInitialized(InitializedParams());
        didInit = true;
    });
    QTRY_COMPARE_WITH_TIMEOUT(didInit, true, 10000);

    for (const QString &filePath :
         QStringList({ u"completions/Yyy.qml"_s, u"completions/fromBuildDir.qml"_s,
                       u"completions/SomeBase.qml"_s, u"findUsages/jsIdentifierUsages.qml"_s })) {
        QFile file(testFile(filePath));
        QVERIFY(file.open(QIODevice::ReadOnly));
        DidOpenTextDocumentParams oParams;
        TextDocumentItem textDocument;
        QByteArray uri = testFileUrl(filePath).toString().toUtf8();
        textDocument.uri = uri;
        textDocument.text = file.readAll();
        oParams.textDocument = textDocument;
        m_protocol.notifyDidOpenTextDocument(oParams);
        m_uriToClose.append(uri);
    }
}

void tst_qmlls_modules::completions_data()
{
    QTest::addColumn<QByteArray>("uri");
    QTest::addColumn<int>("lineNr");
    QTest::addColumn<int>("character");
    QTest::addColumn<ExpectedCompletions>("expected");
    QTest::addColumn<QStringList>("notExpected");

    QByteArray uri = testFileUrl("completions/Yyy.qml").toString().toUtf8();

    QTest::newRow("objEmptyLine") << uri << 8 << 0
                                  << ExpectedCompletions({
                                             { u"Rectangle"_s, CompletionItemKind::Class },
                                             { u"property"_s, CompletionItemKind::Keyword },
                                             { u"width"_s, CompletionItemKind::Property },
                                             { u"function"_s, CompletionItemKind::Keyword },
                                     })
                                  << QStringList({ u"QtQuick"_s, u"vector4d"_s });

    QTest::newRow("inBindingLabel") << uri << 5 << 9
                                    << ExpectedCompletions({
                                               { u"Rectangle"_s, CompletionItemKind::Class },
                                               { u"property"_s, CompletionItemKind::Keyword },
                                               { u"width"_s, CompletionItemKind::Property },
                                       })
                                    << QStringList({ u"QtQuick"_s, u"vector4d"_s });

    QTest::newRow("afterBinding") << uri << 5 << 10
                                  << ExpectedCompletions({
                                             { u"Rectangle"_s, CompletionItemKind::Field },
                                             { u"width"_s, CompletionItemKind::Field },
                                             { u"vector4d"_s, CompletionItemKind::Field },
                                     })
                                  << QStringList({ u"QtQuick"_s, u"property"_s });

    // suppress?
    QTest::newRow("afterId") << uri << 4 << 7
                             << ExpectedCompletions({
                                        { u"import"_s, CompletionItemKind::Keyword },
                                })
                             << QStringList({ u"QtQuick"_s, u"property"_s, u"Rectangle"_s,
                                              u"width"_s, u"vector4d"_s });

    QTest::newRow("fileStart") << uri << 0 << 0
                               << ExpectedCompletions({
                                          { u"Rectangle"_s, CompletionItemKind::Class },
                                          { u"import"_s, CompletionItemKind::Keyword },
                                  })
                               << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s });

    QTest::newRow("importImport") << uri << 0 << 3
                                  << ExpectedCompletions({
                                             { u"Rectangle"_s, CompletionItemKind::Class },
                                             { u"import"_s, CompletionItemKind::Keyword },
                                     })
                                  << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s });

    QTest::newRow("importModuleStart")
            << uri << 0 << 7
            << ExpectedCompletions({
                       { u"QtQuick"_s, CompletionItemKind::Module },
               })
            << QStringList({ u"vector4d"_s, u"width"_s, u"Rectangle"_s, u"import"_s });

    QTest::newRow("importVersionStart")
            << uri << 0 << 15
            << ExpectedCompletions({
                       { u"2"_s, CompletionItemKind::Constant },
                       { u"as"_s, CompletionItemKind::Keyword },
               })
            << QStringList({ u"Rectangle"_s, u"import"_s, u"vector4d"_s, u"width"_s });

    // QTest::newRow("importVersionMinor")
    //         << uri << 0 << 17
    //         << ExpectedCompletions({
    //                    { u"15"_s, CompletionItemKind::Constant },
    //            })
    //         << QStringList({ u"as"_s, u"Rectangle"_s, u"import"_s, u"vector4d"_s, u"width"_s });

    QTest::newRow("inScript") << uri << 6 << 14
                              << ExpectedCompletions({
                                         { u"Rectangle"_s, CompletionItemKind::Field },
                                         { u"vector4d"_s, CompletionItemKind::Field },
                                         { u"lala()"_s, CompletionItemKind::Function },
                                         { u"longfunction()"_s, CompletionItemKind::Function },
                                         { u"documentedFunction()"_s,
                                           CompletionItemKind::Function },
                                         { u"lala()"_s, CompletionItemKind { 0 } },
                                         { u"width"_s, CompletionItemKind::Field },
                                 })
                              << QStringList({ u"import"_s });

    QTest::newRow("expandBase1") << uri << 9 << 23
                                 << ExpectedCompletions({
                                            { u"width"_s, CompletionItemKind::Field },
                                            { u"foo"_s, CompletionItemKind::Field },
                                    })
                                 << QStringList({ u"import"_s, u"Rectangle"_s });

    QTest::newRow("expandBase2") << uri << 10 << 29
                                 << ExpectedCompletions({
                                            { u"width"_s, CompletionItemKind::Field },
                                            { u"color"_s, CompletionItemKind::Field },
                                    })
                                 << QStringList({ u"foo"_s, u"import"_s, u"Rectangle"_s });

    QTest::newRow("asCompletions")
            << uri << 25 << 8
            << ExpectedCompletions({
                       { u"Rectangle"_s, CompletionItemKind::Field },
               })
            << QStringList({ u"foo"_s, u"import"_s, u"lala()"_s, u"width"_s });
}

void tst_qmlls_modules::checkCompletions(QByteArray uri, int lineNr, int character,
                                            ExpectedCompletions expected, QStringList notExpected)
{
    CompletionParams cParams;
    cParams.position.line = lineNr;
    cParams.position.character = character;
    cParams.textDocument.uri = uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol.requestCompletion(
            cParams,
            [clean, uri, expected, notExpected](auto res) {
                QScopeGuard cleanup(clean);
                const QList<CompletionItem> *cItems = std::get_if<QList<CompletionItem>>(&res);

                if (!cItems) {
                    return;
                }

                QSet<QString> labels;
                QDuplicateTracker<QByteArray> modulesTracker;
                QDuplicateTracker<QByteArray> keywordsTracker;
                QDuplicateTracker<QByteArray> classesTracker;
                QDuplicateTracker<QByteArray> fieldsTracker;
                QDuplicateTracker<QByteArray> propertiesTracker;

                for (const CompletionItem &c : *cItems) {
                    if (c.kind->toInt() == int(CompletionItemKind::Module)) {
                        QVERIFY2(!modulesTracker.hasSeen(c.label), "Duplicate module: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Keyword)) {
                        QVERIFY2(!keywordsTracker.hasSeen(c.label),
                                 "Duplicate keyword: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Class)) {
                        QVERIFY2(!classesTracker.hasSeen(c.label), "Duplicate class: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Field)) {
                        QVERIFY2(!fieldsTracker.hasSeen(c.label), "Duplicate field: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Property)) {
                        QVERIFY2(!propertiesTracker.hasSeen(c.label),
                                 "Duplicate property: " + c.label);
                        QVERIFY2(c.insertText == c.label + u": "_s,
                                 "a property should end with a colon with a space for "
                                 "'insertText', for better coding experience");
                    }
                    labels << c.label;
                }

                for (const ExpectedCompletion &exp : expected) {
                    QVERIFY2(labels.contains(exp.first),
                             u"no %1 in %2"_s
                                     .arg(exp.first,
                                          QStringList(labels.begin(), labels.end()).join(u", "_s))
                                     .toUtf8());
                    if (labels.contains(exp.first)) {
                        for (const CompletionItem &c : *cItems) {
                            const auto kind = static_cast<CompletionItemKind>(c.kind->toInt());

                            bool foundEntry = false;
                            bool hasCorrectKind = false;
                            for (const ExpectedCompletion &e : expected) {
                                if (c.label == e.first) {
                                    foundEntry = true;
                                    hasCorrectKind |= kind == e.second;
                                }
                            }

                            // Ignore QVERIFY for those completions not in the expected list.
                            if (!foundEntry)
                                continue;

                            QVERIFY2(hasCorrectKind,
                                     qPrintable(
                                             QString::fromLatin1(
                                                     "Completion item '%1' has wrong kind '%2'")
                                                     .arg(c.label)
                                                     .arg(QMetaEnum::fromType<CompletionItemKind>()
                                                                  .valueToKey(int(kind)))));
                        }
                    }
                }
                for (const QString &nexp : notExpected) {
                    QVERIFY2(!labels.contains(nexp),
                             u"found unexpected completion  %1"_s.arg(nexp).toUtf8());
                }
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

void tst_qmlls_modules::completions()
{
    QFETCH(QByteArray, uri);
    QFETCH(int, lineNr);
    QFETCH(int, character);
    QFETCH(ExpectedCompletions, expected);
    QFETCH(QStringList, notExpected);

    QTEST_CHECKED(checkCompletions(uri, lineNr, character, expected, notExpected));
}

void tst_qmlls_modules::function_documentations_data()
{
    QTest::addColumn<QByteArray>("uri");
    QTest::addColumn<int>("lineNr");
    QTest::addColumn<int>("character");
    QTest::addColumn<ExpectedDocumentations>("expectedDocs");

    QByteArray uri = testFileUrl("completions/Yyy.qml").toString().toUtf8();

    QTest::newRow("longfunction")
            << uri << 5 << 14
            << ExpectedDocumentations{
                   std::make_tuple(u"lala()"_s, u"returns void"_s, u"lala()"_s),
                   std::make_tuple(u"longfunction()"_s, u"returns string"_s,
                                   uR"(longfunction(a, b, c = "c", d = "d"))"_s),
                   std::make_tuple(u"documentedFunction()"_s, u"returns string"_s,
                                   uR"(// documentedFunction: is documented
// returns 'Good'
documentedFunction(arg1, arg2 = "Qt"))"_s),
               };
}

void tst_qmlls_modules::function_documentations()
{
    QFETCH(QByteArray, uri);
    QFETCH(int, lineNr);
    QFETCH(int, character);
    QFETCH(ExpectedDocumentations, expectedDocs);

    CompletionParams cParams;
    cParams.position.line = lineNr;
    cParams.position.character = character;
    cParams.textDocument.uri = uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol.requestCompletion(
            cParams,
            [clean, uri, expectedDocs](auto res) {
                const QList<CompletionItem> *cItems = std::get_if<QList<CompletionItem>>(&res);

                if (!cItems) {
                    return;
                }

                for (const ExpectedDocumentation &exp : expectedDocs) {
                    bool hasFoundExpected = false;
                    const auto expectedLabel = std::get<0>(exp);
                    for (const CompletionItem &c : *cItems) {
                        if (c.kind->toInt() != int(CompletionItemKind::Function)) {
                            // Only check functions.
                            continue;
                        }

                        if (c.label == expectedLabel) {
                            hasFoundExpected = true;
                        }
                    }

                    QVERIFY2(hasFoundExpected,
                             qPrintable(u"expected completion label '%1' wasn't found"_s.arg(
                                     expectedLabel)));
                }

                for (const CompletionItem &c : *cItems) {
                    if (c.kind->toInt() != int(CompletionItemKind::Function)) {
                        // Only check functions.
                        continue;
                    }

                    QVERIFY(c.documentation != std::nullopt);
                    // We currently don't support 'MarkupContent', change this when we do.
                    QVERIFY(c.documentation->index() == 0);
                    const QByteArray cDoc = std::get<0>(*c.documentation);

                    for (const ExpectedDocumentation &exp : expectedDocs) {
                        const auto &[label, details, docs] = exp;

                        if (c.label != label)
                            continue;

                        QVERIFY2(c.detail == details,
                                 qPrintable(u"Completion item '%1' has wrong details '%2'"_s
                                                    .arg(label).arg(*c.detail)));
                        QVERIFY2(cDoc == docs,
                                 qPrintable(u"Completion item '%1' has wrong documentation '%2'"_s
                                                    .arg(label).arg(cDoc)));
                    }
                }
                clean();
            },
            [clean](const ResponseError &err) {
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
                clean();
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

void tst_qmlls_modules::buildDir()
{
    QString filePath = u"completions/fromBuildDir.qml"_s;
    QByteArray uri = testFileUrl(filePath).toString().toUtf8();
    QTEST_CHECKED(checkCompletions(uri, 3, 0,
                     ExpectedCompletions({
                             { u"property"_s, CompletionItemKind::Keyword },
                             { u"function"_s, CompletionItemKind::Keyword },
                             { u"Rectangle"_s, CompletionItemKind::Class },
                     }),
                     QStringList({ u"BuildDirType"_s, u"QtQuick"_s, u"width"_s, u"vector4d"_s })));
    Notifications::AddBuildDirsParams bDirs;
    UriToBuildDirs ub;
    ub.baseUri = uri;
    ub.buildDirs.append(testFile("buildDir").toUtf8());
    bDirs.buildDirsToSet.append(ub);
    m_protocol.typedRpc()->sendNotification(QByteArray(Notifications::AddBuildDirsMethod), bDirs);
    DidChangeTextDocumentParams didChange;
    didChange.textDocument.uri = uri;
    didChange.textDocument.version = 2;
    TextDocumentContentChangeEvent change;
    QFile file(testFile(filePath));
    QVERIFY(file.open(QIODevice::ReadOnly));
    change.text = file.readAll();
    didChange.contentChanges.append(change);
    m_protocol.notifyDidChangeTextDocument(didChange);
    QTEST_CHECKED(checkCompletions(uri, 3, 0,
                     ExpectedCompletions({
                             { u"BuildDirType"_s, CompletionItemKind::Class },
                             { u"Rectangle"_s, CompletionItemKind::Class },
                             { u"property"_s, CompletionItemKind::Keyword },
                             { u"width"_s, CompletionItemKind::Property },
                             { u"function"_s, CompletionItemKind::Keyword },
                     }),
                     QStringList({ u"QtQuick"_s, u"vector4d"_s })));
}
void tst_qmlls_modules::cleanupTestCase()
{
    for (const QByteArray &uri : m_uriToClose) {
        DidCloseTextDocumentParams closeP;
        closeP.textDocument.uri = uri;
        m_protocol.notifyDidCloseTextDocument(closeP);
    }
    m_server.closeWriteChannel();
    QTRY_COMPARE(m_server.state(), QProcess::NotRunning);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

void tst_qmlls_modules::goToTypeDefinition_data()
{
    QTest::addColumn<QByteArray>("uri");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QByteArray>("expectedUri");
    QTest::addColumn<int>("expectedStartLine");
    QTest::addColumn<int>("expectedStartCharacter");
    QTest::addColumn<int>("expectedEndLine");
    QTest::addColumn<int>("expectedEndCharacter");

    QByteArray yyyUri = testFileUrl("completions/Yyy.qml").toString().toUtf8();
    QByteArray zzzUri = testFileUrl("completions/Zzz.qml").toString().toUtf8();
    QByteArray someBaseUri = testFileUrl("completions/SomeBase.qml").toString().toUtf8();

    QTest::newRow("BaseOfYyy") << yyyUri << 3 << 1 << zzzUri << 2 << 0 << 9 << 1;
    QTest::newRow("BaseOfIC") << yyyUri << 29 << 19 << zzzUri << 2 << 0 << 9 << 1;

    QTest::newRow("PropertyType") << yyyUri << 30 << 14 << someBaseUri << 2 << 0 << 4 << 1;

    QTest::newRow("TypeInIC") << yyyUri << 29 << 36 << someBaseUri << 2 << 0 << 4 << 1;
    QTest::newRow("ICTypeDefinition") << yyyUri << 29 << 15 << yyyUri << 29 << 18 << 29 << 48;
}

void tst_qmlls_modules::goToTypeDefinition()
{
    QFETCH(QByteArray, uri);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QByteArray, expectedUri);
    QFETCH(int, expectedStartLine);
    QFETCH(int, expectedStartCharacter);
    QFETCH(int, expectedEndLine);
    QFETCH(int, expectedEndCharacter);

    QVERIFY(uri.startsWith("file://"_ba));

    // TODO
    TypeDefinitionParams params;
    params.position.line = line;
    params.position.character = character;
    params.textDocument.uri = uri;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol.requestTypeDefinition(
            params,
            [&](auto res) {
                QScopeGuard cleanup(clean);
                auto *result = std::get_if<QList<Location>>(&res);

                QVERIFY(result);

                QCOMPARE(result->size(), 1);

                Location l = result->front();
                QCOMPARE(l.uri, expectedUri);
                QCOMPARE(l.range.start.line, expectedStartLine);
                QCOMPARE(l.range.start.character, expectedStartCharacter);
                QCOMPARE(l.range.end.line, expectedEndLine);
                QCOMPARE(l.range.end.character, expectedEndCharacter);
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

// startLine and startCharacter start at 1, not 0
static QLspSpecification::Location locationFrom(const QByteArray fileName, const QString &code,
                                                quint32 startLine, quint32 startCharacter,
                                                quint32 length)
{
    QLspSpecification::Location location;
    location.uri = QQmlLSUtils::qmlUrlToLspUri(fileName);
    // the LSP works with lines and characters starting at 0
    location.range.start.line = startLine - 1;
    location.range.start.character = startCharacter - 1;

    quint32 startOffset = QQmlLSUtils::textOffsetFrom(code, startLine - 1, startCharacter - 1);
    auto end = QQmlLSUtils::textRowAndColumnFrom(code, startOffset + length);
    location.range.end.line = end.line;
    location.range.end.character = end.character;

    return location;
}

void tst_qmlls_modules::findUsages_data()
{
    QTest::addColumn<QByteArray>("uri");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QList<QLspSpecification::Location>>("expectedUsages");

    QByteArray jsIdentifierUsagesUri = testFileUrl("findUsages/jsIdentifierUsages.qml").toEncoded();

    QString jsIdentifierUsagesContent;
    {
        QFile file(testFile("findUsages/jsIdentifierUsages.qml").toUtf8());
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        jsIdentifierUsagesContent = QString::fromUtf8(file.readAll());
    }

    // line and character start at 1!
    const QList<QLspSpecification::Location> sumUsages = {
        locationFrom(jsIdentifierUsagesUri, jsIdentifierUsagesContent, 8, 13, strlen("sum")),
        locationFrom(jsIdentifierUsagesUri, jsIdentifierUsagesContent, 10, 13, strlen("sum")),
        locationFrom(jsIdentifierUsagesUri, jsIdentifierUsagesContent, 10, 19, strlen("sum")),
    };
    QVERIFY(sumUsages.front().uri.startsWith("file://"_ba));

    // line and character start at 1!
    QTest::addRow("sumUsagesFromUsage") << jsIdentifierUsagesUri << 10 << 14 << sumUsages;
    QTest::addRow("sumUsagesFromUsage2") << jsIdentifierUsagesUri << 10 << 20 << sumUsages;
    QTest::addRow("sumUsagesFromDefinition") << jsIdentifierUsagesUri << 8 << 14 << sumUsages;
}

static bool operator==(const QLspSpecification::Location &a, const QLspSpecification::Location &b)
{
    return std::tie(a.uri, a.range.start.character, a.range.start.line, a.range.end.character,
                    a.range.end.line)
            == std::tie(b.uri, b.range.start.character, b.range.start.line, b.range.end.character,
                        b.range.end.line);
}

static bool locationListsAreEqual(const QList<QLspSpecification::Location> &a,
                                  const QList<QLspSpecification::Location> &b)
{
    return std::equal(
            a.cbegin(), a.cend(), b.cbegin(), b.cend(),
            [](const QLspSpecification::Location &a, const QLspSpecification::Location &b) {
                return a == b; // as else std::equal will not find the above implementation of
                               // operator==
            });
}

static QString locationToString(const QLspSpecification::Location &l)
{
    QString s = u"%1: (%2, %3) - (%4, %5)"_s.arg(l.uri)
                        .arg(l.range.start.line)
                        .arg(l.range.start.character)
                        .arg(l.range.end.line)
                        .arg(l.range.end.character);
    return s;
}

void tst_qmlls_modules::findUsages()
{
    QFETCH(QByteArray, uri);
    // line and character start at 1!
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QList<QLspSpecification::Location>, expectedUsages);

    QVERIFY(uri.startsWith("file://"_ba));

    ReferenceParams params;
    params.position.line = line - 1;
    params.position.character = character - 1;
    params.textDocument.uri = uri;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol.requestReference(
            params,
            [&](auto res) {
                QScopeGuard cleanup(clean);
                auto *result = std::get_if<QList<Location>>(&res);

                QVERIFY(result);
                if constexpr (enable_debug_output) {
                    if (!locationListsAreEqual(*result, expectedUsages)) {
                        qDebug() << "Got following locations:";
                        for (auto &x : *result) {
                            qDebug() << locationToString(x);
                        }
                        qDebug() << "But expected:";
                        for (auto &x : expectedUsages) {
                            qDebug() << locationToString(x);
                        }
                    }
                }

                QVERIFY(locationListsAreEqual(*result, expectedUsages));
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 3000);
}

QTEST_MAIN(tst_qmlls_modules)

#include <tst_qmlls_modules.moc>
