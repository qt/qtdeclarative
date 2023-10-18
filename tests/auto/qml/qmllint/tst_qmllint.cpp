// Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QProcess>
#include <QString>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlCompiler/private/qqmljslinter_p.h>
#include <QtQmlCompiler/private/qqmlsa_p.h>
#include <QtCore/qplugin.h>

Q_IMPORT_PLUGIN(LintPlugin)

using namespace Qt::StringLiterals;

class TestQmllint: public QQmlDataTest
{
    Q_OBJECT

public:
    TestQmllint();

    struct Message
    {
        QString text = QString();
        quint32 line = 0, column = 0;
        QtMsgType severity = QtWarningMsg;
    };

    struct Result
    {
        enum Flag { ExitsNormally = 0x1, NoMessages = 0x2, AutoFixable = 0x4 };

        Q_DECLARE_FLAGS(Flags, Flag)

        static Result clean() { return Result { {}, {}, {}, { NoMessages, ExitsNormally } }; }

        QList<Message> expectedMessages = {};
        QList<Message> badMessages = {};
        QList<Message> expectedReplacements = {};

        Flags flags = {};
    };

private Q_SLOTS:
    void initTestCase() override;

    void testUnqualified();
    void testUnqualified_data();

    void cleanQmlCode_data();
    void cleanQmlCode();

    void dirtyQmlCode_data();
    void dirtyQmlCode();

    void compilerWarnings_data();
    void compilerWarnings();

    void testUnknownCausesFail();

    void directoryPassedAsQmlTypesFile();
    void oldQmltypes();

    void qmltypes_data();
    void qmltypes();

#ifdef QT_QMLJSROOTGEN_PRESENT
    void verifyJsRoot();
#endif

    void autoqmltypes();
    void resources();

    void requiredProperty();

    void settingsFile();

    void additionalImplicitImport();

    void qrcUrlImport();

    void attachedPropertyReuse();

    void missingBuiltinsNoCrash();
    void absolutePath();

    void importMultipartUri();

    void lintModule_data();
    void lintModule();

    void testLineEndings();

#if QT_CONFIG(library)
    void testPlugin();
    void quickPlugin();
#endif
private:
    enum DefaultImportOption { NoDefaultImports, UseDefaultImports };
    enum ContainOption { StringNotContained, StringContained };
    enum ReplacementOption {
        NoReplacementSearch,
        DoReplacementSearch,
    };

    enum LintType { LintFile, LintModule };

    QString runQmllint(const QString &fileToLint, std::function<void(QProcess &)> handleResult,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addImportDirs = true, bool absolutePath = true);
    QString runQmllint(const QString &fileToLint, bool shouldSucceed,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addImportDirs = true, bool absolutePath = true);
    void callQmllint(const QString &fileToLint, bool shouldSucceed, QJsonArray *warnings = nullptr,
                     QStringList importDirs = {}, QStringList qmltypesFiles = {},
                     QStringList resources = {},
                     DefaultImportOption defaultImports = UseDefaultImports,
                     QList<QQmlJS::LoggerCategory> *categories = nullptr, bool autoFixable = false,
                     LintType type = LintFile);

    void searchWarnings(const QJsonArray &warnings, const QString &string,
                        QtMsgType type = QtWarningMsg, quint32 line = 0, quint32 column = 0,
                        ContainOption shouldContain = StringContained,
                        ReplacementOption searchReplacements = NoReplacementSearch);

    template<typename ExpectedMessageFailureHandler, typename BadMessageFailureHandler>
    void checkResult(const QJsonArray &warnings, const Result &result,
                     ExpectedMessageFailureHandler onExpectedMessageFailures,
                     BadMessageFailureHandler onBadMessageFailures);

    void checkResult(const QJsonArray &warnings, const Result &result)
    {
        checkResult(
                warnings, result, [] {}, [] {});
    }

    void runTest(const QString &testFile, const Result &result, QStringList importDirs = {},
                 QStringList qmltypesFiles = {}, QStringList resources = {},
                 DefaultImportOption defaultImports = UseDefaultImports,
                 QList<QQmlJS::LoggerCategory> *categories = nullptr);

    QString m_qmllintPath;
    QString m_qmljsrootgenPath;
    QString m_qmltyperegistrarPath;

    QStringList m_defaultImportPaths;
    QQmlJSLinter m_linter;
};

Q_DECLARE_METATYPE(TestQmllint::Result)

TestQmllint::TestQmllint()
    : QQmlDataTest(QT_QMLTEST_DATADIR),
      m_defaultImportPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath), dataDirectory() }),
      m_linter(m_defaultImportPaths)

{
}

void TestQmllint::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmllintPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmllint");
    m_qmljsrootgenPath = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath)
            + QLatin1String("/qmljsrootgen");
    m_qmltyperegistrarPath = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath)
            + QLatin1String("/qmltyperegistrar");
#ifdef Q_OS_WIN
    m_qmllintPath += QLatin1String(".exe");
    m_qmljsrootgenPath += QLatin1String(".exe");
    m_qmltyperegistrarPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmllintPath).exists()) {
        QString message = QStringLiteral("qmllint executable not found (looked for %0)").arg(m_qmllintPath);
        QFAIL(qPrintable(message));
    }

#ifdef QT_QMLJSROOTGEN_PRESENT
    if (!QFileInfo(m_qmljsrootgenPath).exists()) {
        QString message = QStringLiteral("qmljsrootgen executable not found (looked for %0)").arg(m_qmljsrootgenPath);
        QFAIL(qPrintable(message));
    }
    if (!QFileInfo(m_qmltyperegistrarPath).exists()) {
        QString message = QStringLiteral("qmltypesregistrar executable not found (looked for %0)").arg(m_qmltyperegistrarPath);
        QFAIL(qPrintable(message));
    }
#endif
}

void TestQmllint::testUnqualified()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    runTest(filename, result);
}

void TestQmllint::testUnqualified_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    // id from nowhere (as with setContextProperty)
    QTest::newRow("IdFromOuterSpace")
            << QStringLiteral("IdFromOuterSpace.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 4, 8 },
                          Message { QStringLiteral("Unqualified access"), 7, 21 } } };
    // access property of root object
    QTest::newRow("FromRootDirect")
            << QStringLiteral("FromRoot.qml")
            << Result {
                   {
                           Message { QStringLiteral("Unqualified access"), 9, 16 }, // new property
                           Message { QStringLiteral("Unqualified access"), 13,
                                     33 } // builtin property
                   },
                   {},
                   { { Message { u"root."_s, 9, 16 } }, { Message { u"root."_s, 13, 33 } } }
               };
    // access injected name from signal
    QTest::newRow("SignalHandler")
            << QStringLiteral("SignalHandler.qml")
            << Result { {
                                Message { QStringLiteral("Unqualified access"), 5, 21 },
                                Message { QStringLiteral("Unqualified access"), 10, 21 },
                                Message { QStringLiteral("Unqualified access"), 8, 29 },
                                Message { QStringLiteral("Unqualified access"), 12, 34 },
                        },
                        {},
                        {
                                Message { QStringLiteral("function(mouse)"), 4, 22 },
                                Message { QStringLiteral("function(mouse)"), 9, 24 },
                                Message { QStringLiteral("(mouse) => "), 8, 16 },
                                Message { QStringLiteral("(mouse) => "), 12, 21 },
                        } };
    // access catch identifier outside catch block
    QTest::newRow("CatchStatement")
            << QStringLiteral("CatchStatement.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 6, 21 } } };
    QTest::newRow("NonSpuriousParent")
            << QStringLiteral("nonSpuriousParentWarning.qml")
            << Result { {
                                Message { QStringLiteral("Unqualified access"), 6, 25 },
                        },
                        {},
                        { { Message { u"<id>."_s, 6, 25 } } } };

    QTest::newRow("crashConnections")
            << QStringLiteral("crashConnections.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 4, 13 } } };

    QTest::newRow("delegateContextProperties")
            << QStringLiteral("delegateContextProperties.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 6, 14 },
                          Message { QStringLiteral("Unqualified access"), 7, 15 },
                          Message { QStringLiteral("model is implicitly injected into this "
                                                   "delegate. Add a required property instead.") },
                          Message {
                                  QStringLiteral("index is implicitly injected into this delegate. "
                                                 "Add a required property instead.") } } };
    QTest::newRow("storeSloppy")
            << QStringLiteral("UnqualifiedInStoreSloppy.qml")
            << Result{ { Message{ QStringLiteral("Unqualified access"), 9, 26} } };
    QTest::newRow("storeStrict")
            << QStringLiteral("UnqualifiedInStoreStrict.qml")
            << Result{ { Message{ QStringLiteral("Unqualified access"), 9, 52} } };
}

void TestQmllint::testUnknownCausesFail()
{
    runTest("unknownElement.qml",
            Result { { Message {
                    QStringLiteral("Unknown was not found. Did you add all import paths?"), 4, 5,
                    QtWarningMsg } } });
    runTest("TypeWithUnknownPropertyType.qml",
            Result { { Message {
                    QStringLiteral("Something was not found. Did you add all import paths?"), 4, 5,
                    QtWarningMsg } } });
}

void TestQmllint::directoryPassedAsQmlTypesFile()
{
    runTest("unknownElement.qml",
            Result { { Message { QStringLiteral("QML types file cannot be a directory: ")
                                 + dataDirectory() } } },
            {}, { dataDirectory() });
}

void TestQmllint::oldQmltypes()
{
    runTest("oldQmltypes.qml",
            Result { {
                             Message { QStringLiteral("typeinfo not declared in qmldir file") },
                             Message {
                                     QStringLiteral("Found deprecated dependency specifications") },
                             Message { QStringLiteral(
                                     "Meta object revision and export version differ.") },
                             Message { QStringLiteral(
                                     "Revision 0 corresponds to version 0.0; it should be 1.0.") },
                     },
                     { Message { QStringLiteral(
                             "QQuickItem was not found. Did you add all import paths?") } } });
}

void TestQmllint::qmltypes_data()
{
    QTest::addColumn<QString>("file");

    const QString importsPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QDirIterator it(importsPath, { "*.qmltypes" },
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        QTest::addRow("%s", qPrintable(it.next().mid(importsPath.size()))) << it.filePath();
}

void TestQmllint::qmltypes()
{
    QFETCH(QString, file);
    // pass the warnings in, so that callQmllint() would show errors if any
    QJsonArray warnings;
    callQmllint(file, true, &warnings);
}

#ifdef QT_QMLJSROOTGEN_PRESENT
void TestQmllint::verifyJsRoot()
{
    QProcess process;

    const QString importsPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QDirIterator it(importsPath, { "jsroot.qmltypes" },
                    QDir::Files, QDirIterator::Subdirectories);

    QVERIFY(it.hasNext());

    QString currentJsRootPath = it.next();

    QTemporaryDir dir;

    QProcess jsrootProcess;
    connect(&jsrootProcess, &QProcess::errorOccurred, [&](QProcess::ProcessError error) {
        qWarning() << error << jsrootProcess.errorString();
    });
    jsrootProcess.setWorkingDirectory(dir.path());
    jsrootProcess.start(m_qmljsrootgenPath, {"jsroot.json"});

    jsrootProcess.waitForFinished();

    QCOMPARE(jsrootProcess.exitStatus(), QProcess::NormalExit);
    QCOMPARE(jsrootProcess.exitCode(), 0);


    QProcess typeregistrarProcess;
    typeregistrarProcess.setWorkingDirectory(dir.path());
    typeregistrarProcess.start(m_qmltyperegistrarPath, {"jsroot.json", "--generate-qmltypes", "jsroot.qmltypes"});

    typeregistrarProcess.waitForFinished();

    QCOMPARE(typeregistrarProcess.exitStatus(), QProcess::NormalExit);
    QCOMPARE(typeregistrarProcess.exitCode(), 0);

    QString currentJsRootContent, generatedJsRootContent;

    QFile currentJsRoot(currentJsRootPath);
    QVERIFY(currentJsRoot.open(QFile::ReadOnly | QIODevice::Text));
    currentJsRootContent = QString::fromUtf8(currentJsRoot.readAll());
    currentJsRoot.close();

    QFile generatedJsRoot(dir.path() + QDir::separator() + "jsroot.qmltypes");
    QVERIFY(generatedJsRoot.open(QFile::ReadOnly | QIODevice::Text));
    generatedJsRootContent = QString::fromUtf8(generatedJsRoot.readAll());
    generatedJsRoot.close();

    // If any of the following asserts fail you need to update jsroot.qmltypes using the following commands:
    //
    // qmljsrootgen jsroot.json
    // qmltyperegistrar jsroot.json --generate-qmltypes src/imports/builtins/jsroot.qmltypes
    QStringList currentLines = currentJsRootContent.split(QLatin1Char('\n'));
    QStringList generatedLines = generatedJsRootContent.split(QLatin1Char('\n'));

    QCOMPARE(currentLines.size(), generatedLines.size());

    for (qsizetype i = 0; i < currentLines.size(); i++) {
        QCOMPARE(currentLines[i], generatedLines[i]);
    }
}
#endif

void TestQmllint::autoqmltypes()
{
    QProcess process;
    process.setWorkingDirectory(testFile("autoqmltypes"));
    process.start(m_qmllintPath, { QStringLiteral("test.qml") });

    process.waitForFinished();

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QVERIFY(process.exitCode() != 0);

    QVERIFY(process.readAllStandardError()
                .contains("is not a qmldir file. Assuming qmltypes"));
    QVERIFY(process.readAllStandardOutput().isEmpty());
}

void TestQmllint::resources()
{
    {
        // We need to clear the import cache before we add a qrc file with different
        // contents for the same paths.
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });

        callQmllint(testFile("resource.qml"), true, nullptr, {}, {}, { testFile("resource.qrc") });
        callQmllint(testFile("badResource.qml"), false, nullptr, {}, {}, { testFile("resource.qrc") });
    }


    callQmllint(testFile("resource.qml"), false);
    callQmllint(testFile("badResource.qml"), true);

    {
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
        callQmllint(testFile("T/b.qml"), true, nullptr, {}, {}, { testFile("T/a.qrc") });
    }

    {
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
        callQmllint(testFile("relPathQrc/Foo/Thing.qml"), true, nullptr, {}, {},
                { testFile("relPathQrc/resources.qrc") });
    }
}

void TestQmllint::dirtyQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml")
            << Result { { Message { QStringLiteral("Expected token `:'"), 4, 8, QtCriticalMsg } } };
    QTest::newRow("Invalid_syntax_JS") << QStringLiteral("failure1.js")
                                       << Result { { Message { QStringLiteral("Expected token `;'"),
                                                               4, 12, QtCriticalMsg } } };
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 12, 36 } } };
    QTest::newRow("AutomatchedSignalHandler2")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << Result { { Message {
                       QStringLiteral("Implicitly defining onClicked as signal handler"), 0, 0,
                       QtInfoMsg } } };
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << Result { { Message {
                       QStringLiteral("Member \"foo\" not found on type \"QtObject\""), 6,
                       31 } } };
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << Result { { Message {
                       QStringLiteral("Member \"foo2\" not found on type \"Methods\""), 5,
                       25 } } };
    QTest::newRow("badAlias")
            << QStringLiteral("badAlias.qml")
            << Result { { Message { QStringLiteral("Cannot resolve alias \"wrong\""), 3, 1 } } };
    QTest::newRow("badAliasProperty1")
            << QStringLiteral("badAliasProperty.qml")
            << Result { { Message { QStringLiteral("Cannot resolve alias \"wrong\""), 3, 1 } } };
    QTest::newRow("badAliasExpression")
            << QStringLiteral("badAliasExpression.qml")
            << Result { { Message {
                       QStringLiteral("Invalid alias expression. Only IDs and field member "
                                      "expressions can be aliased"),
                       5, 26 } } };
    QTest::newRow("badAliasNotAnExpression")
            << QStringLiteral("badAliasNotAnExpression.qml")
            << Result { { Message {
                                  QStringLiteral("Invalid alias expression. Only IDs and field member "
                                                 "expressions can be aliased"),
                                  4, 30 } } };
    QTest::newRow("aliasCycle1") << QStringLiteral("aliasCycle.qml")
                                 << Result { { Message {
                                            QStringLiteral("Alias \"b\" is part of an alias cycle"),
                                            3, 1 } } };
    QTest::newRow("aliasCycle2") << QStringLiteral("aliasCycle.qml")
                                 << Result { { Message {
                                            QStringLiteral("Alias \"a\" is part of an alias cycle"),
                                            3, 1 } } };
    QTest::newRow("invalidAliasTarget1") << QStringLiteral("invalidAliasTarget.qml")
                                         << Result { { Message {
                                            QStringLiteral("Invalid alias expression – an initalizer is needed."),
                                            6, 18 } } };
    QTest::newRow("invalidAliasTarget2") << QStringLiteral("invalidAliasTarget.qml")
                                         << Result { { Message {
                                            QStringLiteral("Invalid alias expression. Only IDs and field member expressions can be aliased"),
                                            7, 30 } } };
    QTest::newRow("invalidAliasTarget3") << QStringLiteral("invalidAliasTarget.qml")
                                         << Result { { Message {
                                            QStringLiteral("Invalid alias expression. Only IDs and field member expressions can be aliased"),
                                            9, 34 } } };
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << Result { { Message { QStringLiteral("Member \"rrr\" not found on type \"Item\""),
                                    5, 34 } } };
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << Result { { Message {
                       QStringLiteral("Member \"progress\" not found on type \"QQuickItem\""), 7,
                       39 } } };
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << Result { { Message {
                       QStringLiteral("Member \"rrr\" not found on type \"QQuickItem\""), 5,
                       39 } } };
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << Result { { Message {
                       QStringLiteral("Type \"QPalette\" of property \"palette\" not found"), 5,
                       26 } } };
    QTest::newRow("incompleteQmltypes2")
            << QStringLiteral("incompleteQmltypes2.qml")
            << Result { { Message { QStringLiteral("Member \"weDontKnowIt\" "
                                                   "not found on type \"CustomPalette\""),
                                    5, 35 } } };
    QTest::newRow("incompleteQmltypes3")
            << QStringLiteral("incompleteQmltypes3.qml")
            << Result { { Message {
                       QStringLiteral("Type \"QPalette\" of property \"palette\" not found"), 5,
                       21 } } };
    QTest::newRow("inheritanceCycle")
            << QStringLiteral("Cycle1.qml")
            << Result { { Message {
                       QStringLiteral("Cycle1 is part of an inheritance cycle: Cycle2 -> Cycle3 "
                                      "-> Cycle1 -> Cycle2"),
                       2, 1 } } };
    QTest::newRow("badQmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/bad.qml")
            << Result { { Message {
                       QStringLiteral("Item was not found. Did you add all import paths?"), 3,
                       1 } } };
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleBad.qml")
            << Result { { Message {
                       QStringLiteral("Member \"unknownFunc\" not found on type \"Foo\""), 5,
                       21 } } };
    QTest::newRow("badEnumFromQtQml")
            << QStringLiteral("badEnumFromQtQml.qml")
            << Result { { Message { QStringLiteral("Member \"Linear123\" not "
                                                   "found on type \"QQmlEasingEnums\""),
                                    4, 30 } } };
    QTest::newRow("anchors3")
            << QStringLiteral("anchors3.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign binding of type QQuickItem to QQuickAnchorLine") } } };
    QTest::newRow("nanchors1") << QStringLiteral("nanchors1.qml")
                               << Result { { Message { QStringLiteral(
                                          "unknown grouped property scope nanchors.") } } };
    QTest::newRow("nanchors2") << QStringLiteral("nanchors2.qml")
                               << Result { { Message { QStringLiteral(
                                          "unknown grouped property scope nanchors.") } } };
    QTest::newRow("nanchors3") << QStringLiteral("nanchors3.qml")
                               << Result { { Message { QStringLiteral(
                                          "unknown grouped property scope nanchors.") } } };
    QTest::newRow("badAliasObject")
            << QStringLiteral("badAliasObject.qml")
            << Result { { Message { QStringLiteral("Member \"wrongwrongwrong\" not "
                                                   "found on type \"QtObject\""),
                                    8, 40 } } };
    QTest::newRow("badScript") << QStringLiteral("badScript.qml")
                               << Result { { Message {
                                          QStringLiteral(
                                                  "Member \"stuff\" not found on type \"Empty\""),
                                          5, 21 } } };
    QTest::newRow("badScriptOnAttachedProperty")
            << QStringLiteral("badScript.attached.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 3, 26 } } };
    QTest::newRow("brokenNamespace")
            << QStringLiteral("brokenNamespace.qml")
            << Result { { Message { QStringLiteral("Type not found in namespace"), 4, 19 } } };
    QTest::newRow("segFault (bad)")
            << QStringLiteral("SegFault.bad.qml")
            << Result { { Message { QStringLiteral(
                       "Member \"foobar\" not found on type \"QQuickScreenAttached\"") } } };
    QTest::newRow("VariableUsedBeforeDeclaration")
            << QStringLiteral("useBeforeDeclaration.qml")
            << Result { { Message {
                       QStringLiteral("Variable \"argq\" is used here before its declaration. "
                                      "The declaration is at 6:13."),
                       5, 9 } } };
    QTest::newRow("SignalParameterMismatch")
            << QStringLiteral("namedSignalParameters.qml")
            << Result { { Message { QStringLiteral(
                                "Parameter 1 to signal handler for \"onSig\" is called \"argarg\". "
                                "The signal has a parameter of the same name in position 2.") } },
                        { Message { QStringLiteral("onSig2") } } };
    QTest::newRow("TooManySignalParameters")
            << QStringLiteral("tooManySignalParameters.qml")
            << Result { { Message {
                       QStringLiteral("Signal handler for \"onSig\" has more formal parameters "
                                      "than the signal it handles.") } } };
    QTest::newRow("OnAssignment") << QStringLiteral("onAssignment.qml")
                                  << Result { { Message { QStringLiteral(
                                             "Member \"loops\" not found on type \"bool\"") } } };
    QTest::newRow("BadAttached") << QStringLiteral("badAttached.qml")
                                 << Result { { Message { QStringLiteral(
                                            "unknown attached property scope WrongAttached.") } } };
    QTest::newRow("BadBinding") << QStringLiteral("badBinding.qml")
                                << Result { { Message { QStringLiteral(
                                           "Binding assigned to \"doesNotExist\", but no property "
                                           "\"doesNotExist\" exists in the current element.") } } };
    QTest::newRow("bad template literal (simple)")
            << QStringLiteral("badTemplateStringSimple.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("bad constant number to string")
            << QStringLiteral("numberToStringProperty.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign literal of type double to QString") } } };
    QTest::newRow("bad unary minus to string")
            << QStringLiteral("unaryMinusToStringProperty.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign literal of type double to QString") } } };
    QTest::newRow("bad tranlsation binding (qsTr)") << QStringLiteral("bad_qsTr.qml") << Result {};
    QTest::newRow("bad string binding (QT_TR_NOOP)")
            << QStringLiteral("bad_QT_TR_NOOP.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("BadScriptBindingOnGroup")
            << QStringLiteral("badScriptBinding.group.qml")
            << Result { { Message {
                       QStringLiteral("Binding assigned to \"bogusProperty\", but no "
                                      "property \"bogusProperty\" exists in the current element."),
                       3, 10 } } };
    QTest::newRow("BadScriptBindingOnAttachedType")
            << QStringLiteral("badScriptBinding.attached.qml")
            << Result { { Message {
                       QStringLiteral("Binding assigned to \"bogusProperty\", but no "
                                      "property \"bogusProperty\" exists in the current element."),
                       5, 12 } } };
    QTest::newRow("BadScriptBindingOnAttachedSignalHandler")
            << QStringLiteral("badScriptBinding.attachedSignalHandler.qml")
            << Result { { Message {
                       QStringLiteral("no matching signal found for handler \"onBogusSignal\""), 3,
                       10 } } };
    QTest::newRow("BadPropertyType")
            << QStringLiteral("badPropertyType.qml")
            << Result { { Message { QStringLiteral(
                       "No type found for property \"bad\". This may be due to a missing "
                       "import statement or incomplete qmltypes files.") } } };
    QTest::newRow("Deprecation (Property, with reason)")
            << QStringLiteral("deprecatedPropertyReason.qml")
            << Result { { Message {
                       QStringLiteral("Property \"deprecated\" is deprecated (Reason: Test)") } } };
    QTest::newRow("Deprecation (Property, no reason)")
            << QStringLiteral("deprecatedProperty.qml")
            << Result { { Message { QStringLiteral("Property \"deprecated\" is deprecated") } } };
    QTest::newRow("Deprecation (Property binding, with reason)")
            << QStringLiteral("deprecatedPropertyBindingReason.qml")
            << Result { { Message { QStringLiteral(
                       "Binding on deprecated property \"deprecatedReason\" (Reason: Test)") } } };
    QTest::newRow("Deprecation (Property binding, no reason)")
            << QStringLiteral("deprecatedPropertyBinding.qml")
            << Result { { Message {
                       QStringLiteral("Binding on deprecated property \"deprecated\"") } } };
    QTest::newRow("Deprecation (Type, with reason)")
            << QStringLiteral("deprecatedTypeReason.qml")
            << Result { { Message { QStringLiteral(
                       "Type \"TypeDeprecatedReason\" is deprecated (Reason: Test)") } } };
    QTest::newRow("Deprecation (Type, no reason)")
            << QStringLiteral("deprecatedType.qml")
            << Result { { Message { QStringLiteral("Type \"TypeDeprecated\" is deprecated") } } };
    QTest::newRow("MissingDefaultProperty")
            << QStringLiteral("defaultPropertyWithoutKeyword.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to non-existent default property") } } };
    QTest::newRow("MissingDefaultPropertyDefinedInTheSameType")
            << QStringLiteral("defaultPropertyWithinTheSameType.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to non-existent default property") } } };
    QTest::newRow("DoubleAssignToDefaultProperty")
            << QStringLiteral("defaultPropertyWithDoubleAssignment.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign multiple objects to a default non-list property") } } };
    QTest::newRow("DefaultPropertyWithWrongType(string)")
            << QStringLiteral("defaultPropertyWithWrongType.qml")
            << Result { { Message { QStringLiteral(
                                "Cannot assign to default property of incompatible type") } },
                        { Message { QStringLiteral(
                                "Cannot assign to non-existent default property") } } };
    QTest::newRow("MultiDefaultPropertyWithWrongType")
            << QStringLiteral("multiDefaultPropertyWithWrongType.qml")
            << Result { { Message { QStringLiteral(
                                "Cannot assign to default property of incompatible type") } },
                        { Message { QStringLiteral(
                                "Cannot assign to non-existent default property") } } };
    QTest::newRow("DefaultPropertyLookupInUnknownType")
        << QStringLiteral("unknownParentDefaultPropertyCheck.qml")
        << Result { { Message {  QStringLiteral(
                "Alien was not found. Did you add all import paths?") } } };
    QTest::newRow("InvalidImport")
            << QStringLiteral("invalidImport.qml")
            << Result { { Message { QStringLiteral(
                       "Failed to import FooBar. Are your import paths set up properly?") } } };
    QTest::newRow("Unused Import (simple)")
            << QStringLiteral("unused_simple.qml")
            << Result { { Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg } },
                        {},
                        {},
                        Result::ExitsNormally };
    QTest::newRow("Unused Import (prefix)")
            << QStringLiteral("unused_prefix.qml")
            << Result { { Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg } },
                        {},
                        {},
                        Result::ExitsNormally };
    QTest::newRow("TypePropertAccess") << QStringLiteral("typePropertyAccess.qml") << Result {};
    QTest::newRow("badAttachedProperty")
            << QStringLiteral("badAttachedProperty.qml")
            << Result { { Message {
                       QStringLiteral("Member \"progress\" not found on type \"TestType\"") } } };
    QTest::newRow("badAttachedPropertyNested")
            << QStringLiteral("badAttachedPropertyNested.qml")
            << Result { { Message { QStringLiteral(
                                            "Member \"progress\" not found on type \"QObject\""),
                                    12, 41 } },
                        { Message { QString("Member \"progress\" not found on type \"QObject\""),
                                    6, 37 } } };
    QTest::newRow("badAttachedPropertyTypeString")
            << QStringLiteral("badAttachedPropertyTypeString.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("badAttachedPropertyTypeQtObject")
            << QStringLiteral("badAttachedPropertyTypeQtObject.qml")
            << Result { { Message { QStringLiteral(
                       "Property \"count\" of type \"int\" is assigned an incompatible type "
                       "\"QtObject\"") } } };
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyAccess")
            << QStringLiteral("goodAttachedPropertyAccess.qml") << Result::clean();
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyNested")
            << QStringLiteral("goodAttachedPropertyNested.qml") << Result::clean();
    QTest::newRow("deprecatedFunction")
            << QStringLiteral("deprecatedFunction.qml")
            << Result { { Message { QStringLiteral(
                       "Method \"deprecated(foobar)\" is deprecated (Reason: No particular "
                       "reason.)") } } };
    QTest::newRow("deprecatedFunctionInherited")
            << QStringLiteral("deprecatedFunctionInherited.qml")
            << Result { { Message { QStringLiteral(
                       "Method \"deprecatedInherited(c, d)\" is deprecated (Reason: This "
                       "deprecation should be visible!)") } } };

    QTest::newRow("duplicated id")
            << QStringLiteral("duplicateId.qml")
            << Result { { Message {
                       QStringLiteral("Found a duplicated id. id root was first declared "), 0, 0,
                       QtCriticalMsg } } };

    QTest::newRow("string as id") << QStringLiteral("stringAsId.qml")
                                  << Result { { Message { QStringLiteral(
                                             "ids do not need quotation marks") } } };
    QTest::newRow("stringIdUsedInWarning")
            << QStringLiteral("stringIdUsedInWarning.qml")
            << Result { { Message {
                                QStringLiteral("i is a member of a parent element"),
                        } },
                        {},
                        { Message { QStringLiteral("stringy.") } } };
    QTest::newRow("Invalid_id_expression")
            << QStringLiteral("invalidId1.qml")
            << Result { { Message { QStringLiteral("Failed to parse id") } } };
    QTest::newRow("Invalid_id_blockstatement")
            << QStringLiteral("invalidId2.qml")
            << Result { { Message { QStringLiteral("id must be followed by an identifier") } } };
    QTest::newRow("multilineString")
            << QStringLiteral("multilineString.qml")
            << Result { { Message { QStringLiteral("String contains unescaped line terminator "
                                                   "which is deprecated."),
                                    0, 0, QtInfoMsg } },
                        {},
                        { Message { "`Foo\nmultiline\\`\nstring`", 4, 32 },
                          Message { "`another\\`\npart\nof it`", 6, 11 },
                          Message { R"(`
quote: " \\" \\\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \' \\' \\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`)",
                                    10, 28 },
                          Message {
                                  R"(`
quote: " \" \\" \\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \\' \\\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`)",
                                  16, 27 } },
                        { Result::ExitsNormally, Result::AutoFixable } };
    QTest::newRow("unresolvedType")
            << QStringLiteral("unresolvedType.qml")
            << Result { { Message { QStringLiteral(
                                "UnresolvedType was not found. Did you add all import paths?") } },
                        { Message { QStringLiteral("incompatible type") } } };
    QTest::newRow("invalidInterceptor")
            << QStringLiteral("invalidInterceptor.qml")
            << Result { { Message { QStringLiteral(
                       "On-binding for property \"angle\" has wrong type \"Item\"") } } };
    QTest::newRow("2Interceptors")
            << QStringLiteral("2interceptors.qml")
            << Result { { Message { QStringLiteral("Duplicate interceptor on property \"x\"") } } };
    QTest::newRow("ValueSource+2Interceptors")
            << QStringLiteral("valueSourceBetween2interceptors.qml")
            << Result { { Message { QStringLiteral("Duplicate interceptor on property \"x\"") } } };
    QTest::newRow("2ValueSources") << QStringLiteral("2valueSources.qml")
                                   << Result { { Message { QStringLiteral(
                                              "Duplicate value source on property \"x\"") } } };
    QTest::newRow("ValueSource+Value")
            << QStringLiteral("valueSource_Value.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot combine value source and binding on property \"obj\"") } } };
    QTest::newRow("ValueSource+ListValue")
            << QStringLiteral("valueSource_listValue.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot combine value source and binding on property \"objs\"") } } };
    QTest::newRow("NonExistentListProperty")
            << QStringLiteral("nonExistentListProperty.qml")
            << Result { { Message { QStringLiteral("Property \"objs\" does not exist") } } };
    QTest::newRow("QtQuick.Window 2.0")
            << QStringLiteral("qtquickWindow20.qml")
            << Result { { Message { QStringLiteral(
                       "Member \"window\" not found on type \"QQuickWindow\"") } } };
    QTest::newRow("unresolvedAttachedType")
            << QStringLiteral("unresolvedAttachedType.qml")
            << Result { { Message { QStringLiteral(
                                "unknown attached property scope UnresolvedAttachedType.") } },
                        { Message { QStringLiteral("Property \"property\" does not exist") } } };
    QTest::newRow("nestedInlineComponents")
            << QStringLiteral("nestedInlineComponents.qml")
            << Result { { Message {
                       QStringLiteral("Nested inline components are not supported") } } };
    QTest::newRow("inlineComponentNoComponent")
            << QStringLiteral("inlineComponentNoComponent.qml")
            << Result { { Message {
                          QStringLiteral("Inline component declaration must be followed by a typename"),
                         3, 2 } } };
    QTest::newRow("WithStatement") << QStringLiteral("WithStatement.qml")
                                   << Result { { Message { QStringLiteral(
                                              "with statements are strongly discouraged") } } };
    QTest::newRow("BadLiteralBinding")
            << QStringLiteral("badLiteralBinding.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("BadLiteralBindingDate")
            << QStringLiteral("badLiteralBindingDate.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign binding of type QString to QDateTime") } } };
    QTest::newRow("BadModulePrefix")
            << QStringLiteral("badModulePrefix.qml")
            << Result { { Message {
                       QStringLiteral("Cannot access singleton as a property of an object") } } };
    QTest::newRow("BadModulePrefix2")
            << QStringLiteral("badModulePrefix2.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot use a non-QObject type QRectF to access prefixed import") } } };
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to read-only property activeFocus") } } };
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty2.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to read-only property activeFocus") } } };
    QTest::newRow("cachedDependency")
            << QStringLiteral("cachedDependency.qml")
            << Result { { Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg } },
                        { Message { QStringLiteral(
                                "Cannot assign binding of type QQuickItem to QObject") } },
                        {},
                        Result::ExitsNormally };
    QTest::newRow("cycle in import")
            << QStringLiteral("cycleHead.qml")
            << Result { { Message { QStringLiteral(
                       "MenuItem is part of an inheritance cycle: MenuItem -> MenuItem") } } };
    QTest::newRow("badGeneralizedGroup1")
            << QStringLiteral("badGeneralizedGroup1.qml")
            << Result { { Message { QStringLiteral(
                       "Binding assigned to \"aaaa\", "
                       "but no property \"aaaa\" exists in the current element") } } };
    QTest::newRow("badGeneralizedGroup2")
            << QStringLiteral("badGeneralizedGroup2.qml")
            << Result { { Message { QStringLiteral("unknown grouped property scope aself") } } };
    QTest::newRow("missingQmltypes")
            << QStringLiteral("missingQmltypes.qml")
            << Result { { Message { QStringLiteral("QML types file does not exist") } } };
    QTest::newRow("enumInvalid")
            << QStringLiteral("enumInvalid.qml")
            << Result { { Message {
                       QStringLiteral("Member \"red\" not found on type \"QtObject\"") } } };
    QTest::newRow("inaccessibleId")
            << QStringLiteral("inaccessibleId.qml")
            << Result { { Message {
                       QStringLiteral("Member \"objectName\" not found on type \"int\"") } } };
    QTest::newRow("inaccessibleId2")
            << QStringLiteral("inaccessibleId2.qml")
            << Result { { Message {
                       QStringLiteral("Member \"objectName\" not found on type \"int\"") } } };
    QTest::newRow("unknownTypeCustomParser")
            << QStringLiteral("unknownTypeCustomParser.qml")
            << Result { { Message { QStringLiteral("TypeDoesNotExist was not found.") } } };
    QTest::newRow("nonNullStored")
            << QStringLiteral("nonNullStored.qml")
            << Result { { Message { QStringLiteral(
                                "Member \"objectName\" not found on type \"Foozle\"") } },
                        { Message { QStringLiteral("Unqualified access") } } };
    QTest::newRow("cppPropertyChangeHandlers-wrong-parameters-size-bindable")
            << QStringLiteral("badCppPropertyChangeHandlers1.qml")
            << Result { { Message { QStringLiteral(
                       "Signal handler for \"onAChanged\" has more formal parameters than "
                       "the signal it handles") } } };
    QTest::newRow("cppPropertyChangeHandlers-wrong-parameters-size-notify")
            << QStringLiteral("badCppPropertyChangeHandlers2.qml")
            << Result { { Message { QStringLiteral(
                       "Signal handler for \"onBChanged\" has more formal parameters than "
                       "the signal it handles") } } };
    QTest::newRow("cppPropertyChangeHandlers-no-property")
            << QStringLiteral("badCppPropertyChangeHandlers3.qml")
            << Result { { Message {
                       QStringLiteral("no matching signal found for handler \"onXChanged\"") } } };
    QTest::newRow("cppPropertyChangeHandlers-not-a-signal")
            << QStringLiteral("badCppPropertyChangeHandlers4.qml")
            << Result { { Message { QStringLiteral(
                       "no matching signal found for handler \"onWannabeSignal\"") } } };
    QTest::newRow("didYouMean(binding)")
            << QStringLiteral("didYouMeanBinding.qml")
            << Result {
                   { Message { QStringLiteral(
                           "Binding assigned to \"witdh\", but no property \"witdh\" exists in "
                           "the current element.") } },
                   {},
                   { Message { QStringLiteral("width") } }
               };
    QTest::newRow("didYouMean(unqualified)")
            << QStringLiteral("didYouMeanUnqualified.qml")
            << Result { { Message { QStringLiteral("Unqualified access") } },
                        {},
                        { Message { QStringLiteral("height") } } };
    QTest::newRow("didYouMean(unqualifiedCall)")
            << QStringLiteral("didYouMeanUnqualifiedCall.qml")
            << Result { { Message { QStringLiteral("Unqualified access") } },
                        {},
                        { Message { QStringLiteral("func") } } };
    QTest::newRow("didYouMean(property)")
            << QStringLiteral("didYouMeanProperty.qml")
            << Result { { Message { QStringLiteral(
                                  "Member \"hoight\" not found on type \"Rectangle\"") },
                          {},
                          { Message { QStringLiteral("height") } } } };
    QTest::newRow("didYouMean(propertyCall)")
            << QStringLiteral("didYouMeanPropertyCall.qml")
            << Result {
                   { Message { QStringLiteral("Member \"lgg\" not found on type \"Console\"") },
                     {},
                     { Message { QStringLiteral("log") } } }
               };
    QTest::newRow("didYouMean(component)")
            << QStringLiteral("didYouMeanComponent.qml")
            << Result { { Message { QStringLiteral(
                                  "Itym was not found. Did you add all import paths?") },
                          {},
                          { Message { QStringLiteral("Item") } } } };
    QTest::newRow("didYouMean(enum)")
            << QStringLiteral("didYouMeanEnum.qml")
            << Result { { Message { QStringLiteral(
                                  "Member \"Readx\" not found on type \"QQuickImage\"") },
                          {},
                          { Message { QStringLiteral("Ready") } } } };
    QTest::newRow("nullBinding") << QStringLiteral("nullBinding.qml")
                                 << Result{ { Message{ QStringLiteral(
                                            "Cannot assign literal of type null to double") } } };
    QTest::newRow("missingRequiredAlias")
            << QStringLiteral("missingRequiredAlias.qml")
            << Result { { Message {
                       QStringLiteral("Component is missing required property requiredAlias from "
                                      "RequiredWithRootLevelAlias") } } };
    QTest::newRow("missingSingletonPragma")
            << QStringLiteral("missingSingletonPragma.qml")
            << Result { { Message { QStringLiteral(
                       "Type MissingPragma declared as singleton in qmldir but missing "
                       "pragma Singleton") } } };
    QTest::newRow("missingSingletonQmldir")
            << QStringLiteral("missingSingletonQmldir.qml")
            << Result { { Message { QStringLiteral(
                       "Type MissingQmldirSingleton not declared as singleton in qmldir but using "
                       "pragma Singleton") } } };
    QTest::newRow("jsVarDeclarationsWriteConst")
            << QStringLiteral("jsVarDeclarationsWriteConst.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to read-only property constProp") } } };
    QTest::newRow("shadowedSignal")
            << QStringLiteral("shadowedSignal.qml")
            << Result { { Message {
                       QStringLiteral("Signal \"pressed\" is shadowed by a property.") } } };
    QTest::newRow("shadowedSignalWithId")
            << QStringLiteral("shadowedSignalWithId.qml")
            << Result { { Message {
                       QStringLiteral("Signal \"pressed\" is shadowed by a property") } } };
    QTest::newRow("shadowedSlot") << QStringLiteral("shadowedSlot.qml")
                                  << Result { { Message { QStringLiteral(
                                             "Slot \"move\" is shadowed by a property") } } };
    QTest::newRow("shadowedMethod") << QStringLiteral("shadowedMethod.qml")
                                    << Result { { Message { QStringLiteral(
                                               "Method \"foo\" is shadowed by a property.") } } };
    QTest::newRow("callVarProp")
            << QStringLiteral("callVarProp.qml")
            << Result { { Message { QStringLiteral(
                       "Property \"foo\" is a variant property. It may or may not be a "
                       "method. Use a regular function instead.") } } };
    QTest::newRow("callJSValue")
            << QStringLiteral("callJSValueProp.qml")
            << Result { { Message { QStringLiteral(
                       "Property \"gradient\" is a QJSValue property. It may or may not be "
                       "a method. Use a regular Q_INVOKABLE instead.") } } };
    QTest::newRow("assignNonExistingTypeToVarProp")
            << QStringLiteral("assignNonExistingTypeToVarProp.qml")
            << Result { { Message { QStringLiteral(
                       "NonExistingType was not found. Did you add all import paths?") } } };
    QTest::newRow("unboundComponents")
            << QStringLiteral("unboundComponents.qml")
            << Result { {
                         Message { QStringLiteral("Unqualified access"), 10, 25 },
                         Message { QStringLiteral("Unqualified access"), 14, 33 }
               } };
    QTest::newRow("badlyBoundComponents")
            << QStringLiteral("badlyBoundComponents.qml")
            << Result{ { Message{ QStringLiteral("Unqualified access"), 18, 36 } } };
    QTest::newRow("NotScopedEnumCpp")
            << QStringLiteral("NotScopedEnumCpp.qml")
            << Result{ { Message{
                       QStringLiteral("You cannot access unscoped enum \"V1\" from here."), 5,
                       57 } } };

    QTest::newRow("unresolvedArrayBinding")
            << QStringLiteral("unresolvedArrayBinding.qml")
            << Result{ { Message{ QStringLiteral(u"Declaring an object which is not an Qml object"
                          " as a list member.") } } };
    QTest::newRow("duplicatedPropertyName")
            << QStringLiteral("duplicatedPropertyName.qml")
            << Result{ { Message{ QStringLiteral("Duplicated property name \"cat\"."), 5, 5 } } };
    QTest::newRow("duplicatedSignalName")
            << QStringLiteral("duplicatedPropertyName.qml")
            << Result{ { Message{ QStringLiteral("Duplicated signal name \"clicked\"."), 8, 5 } } };
    QTest::newRow("missingComponentBehaviorBound")
            << QStringLiteral("missingComponentBehaviorBound.qml")
            << Result {
                    {  Message{ QStringLiteral("Unqualified access"), 8, 31  } },
                    {},
                    {  Message{ QStringLiteral("Set \"pragma ComponentBehavior: Bound\" in "
                                               "order to use IDs from outer components "
                                               "in nested components."), 0, 0, QtInfoMsg } },
                    Result::AutoFixable
                };
    QTest::newRow("IsNotAnEntryOfEnum")
            << QStringLiteral("IsNotAnEntryOfEnum.qml")
            << Result{ {
                         Message {
                                  QStringLiteral("Member \"Mode\" not found on type \"Item\""), 12,
                                  29, QtWarningMsg },
                          Message{
                                  QStringLiteral("\"Hour\" is not an entry of enum \"Mode\"."), 13,
                                  62, QtInfoMsg }
                       },
                       {},
                       { Message{ QStringLiteral("Hours") } }
               };

    QTest::newRow("StoreNameMethod")
            << QStringLiteral("storeNameMethod.qml")
            << Result{ { Message{ QStringLiteral("Cannot assign to method foo") } } };

    QTest::newRow("lowerCaseQualifiedImport")
            << QStringLiteral("lowerCaseQualifiedImport.qml")
            << Result{ {
                       Message{ u"Import qualifier 'test' must start with a capital letter."_s },
                       Message{
                               u"Namespace 'test' of 'test.Rectangle' must start with an upper case letter."_s },
               } };
    QTest::newRow("lowerCaseQualifiedImport2")
            << QStringLiteral("lowerCaseQualifiedImport2.qml")
            << Result{ {
                       Message{ u"Import qualifier 'test' must start with a capital letter."_s },
                       Message{
                               u"Namespace 'test' of 'test.Item' must start with an upper case letter."_s },
                       Message{
                               u"Namespace 'test' of 'test.Rectangle' must start with an upper case letter."_s },
                       Message{
                               u"Namespace 'test' of 'test.color' must start with an upper case letter."_s },
                       Message{
                               u"Namespace 'test' of 'test.Grid' must start with an upper case letter."_s },
               } };
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    QJsonArray warnings;

    QEXPECT_FAIL("attachedPropertyAccess", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("attachedPropertyNested", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("BadLiteralBindingDate",
                 "We're currently not able to verify any non-trivial QString conversion that "
                 "requires QQmlStringConverters",
                 Abort);
    QEXPECT_FAIL("bad tranlsation binding (qsTr)", "We currently do not check translation binding",
                 Abort);

    callQmllint(filename, result.flags.testFlag(Result::ExitsNormally), &warnings, {}, {}, {},
                UseDefaultImports, nullptr, result.flags.testFlag(Result::Flag::AutoFixable));

    checkResult(
            warnings, result,
            [] {
                QEXPECT_FAIL("BadLiteralBindingDate",
                             "We're currently not able to verify any non-trivial QString "
                             "conversion that "
                             "requires QQmlStringConverters",
                             Abort);
            },
            [] {
                QEXPECT_FAIL("badAttachedPropertyNested",
                             "We cannot discern between types and instances", Abort);
            });
}

void TestQmllint::cleanQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("Simple_QML")                << QStringLiteral("Simple.qml");
    QTest::newRow("QML_importing_JS")          << QStringLiteral("importing_js.qml");
    QTest::newRow("JS_with_pragma_and_import") << QStringLiteral("QTBUG-45916.js");
    QTest::newRow("uiQml")                     << QStringLiteral("FormUser.qml");
    QTest::newRow("methodInScope")             << QStringLiteral("MethodInScope.qml");
    QTest::newRow("importWithPrefix")          << QStringLiteral("ImportWithPrefix.qml");
    QTest::newRow("catchIdentifier")           << QStringLiteral("catchIdentifierNoWarning.qml");
    QTest::newRow("qmldirAndQmltypes")         << QStringLiteral("qmldirAndQmltypes.qml");
    QTest::newRow("forLoop")                   << QStringLiteral("forLoop.qml");
    QTest::newRow("esmodule")                  << QStringLiteral("esmodule.mjs");
    QTest::newRow("methodsInJavascript")       << QStringLiteral("javascriptMethods.qml");
    QTest::newRow("goodAlias")                 << QStringLiteral("goodAlias.qml");
    QTest::newRow("goodParent")                << QStringLiteral("goodParent.qml");
    QTest::newRow("goodTypeAssertion")         << QStringLiteral("goodTypeAssertion.qml");
    QTest::newRow("AttachedProps")             << QStringLiteral("AttachedProps.qml");
    QTest::newRow("unknownBuiltinFont")        << QStringLiteral("ButtonLoader.qml");
    QTest::newRow("confusingImport")           << QStringLiteral("Dialog.qml");
    QTest::newRow("qualifiedAttached")         << QStringLiteral("Drawer.qml");
    QTest::newRow("EnumAccess1") << QStringLiteral("EnumAccess1.qml");
    QTest::newRow("EnumAccess2") << QStringLiteral("EnumAccess2.qml");
    QTest::newRow("ListProperty") << QStringLiteral("ListProperty.qml");
    QTest::newRow("AttachedType") << QStringLiteral("AttachedType.qml");
    QTest::newRow("qmldirImportAndDepend") << QStringLiteral("qmldirImportAndDepend/good.qml");
    QTest::newRow("ParentEnum") << QStringLiteral("parentEnum.qml");
    QTest::newRow("Signals") << QStringLiteral("Signal.qml");
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleGood.qml");
    QTest::newRow("enumFromQtQml") << QStringLiteral("enumFromQtQml.qml");
    QTest::newRow("anchors1") << QStringLiteral("anchors1.qml");
    QTest::newRow("anchors2") << QStringLiteral("anchors2.qml");
    QTest::newRow("defaultImport") << QStringLiteral("defaultImport.qml");
    QTest::newRow("goodAliasObject") << QStringLiteral("goodAliasObject.qml");
    QTest::newRow("jsmoduleimport") << QStringLiteral("jsmoduleimport.qml");
    QTest::newRow("overridescript") << QStringLiteral("overridescript.qml");
    QTest::newRow("multiExtension") << QStringLiteral("multiExtension.qml");
    QTest::newRow("segFault") << QStringLiteral("SegFault.qml");
    QTest::newRow("grouped scope failure") << QStringLiteral("groupedScope.qml");
    QTest::newRow("layouts depends quick") << QStringLiteral("layouts.qml");
    QTest::newRow("attached") << QStringLiteral("attached.qml");
    QTest::newRow("enumProperty") << QStringLiteral("enumProperty.qml");
    QTest::newRow("externalEnumProperty") << QStringLiteral("externalEnumProperty.qml");
    QTest::newRow("shapes") << QStringLiteral("shapes.qml");
    QTest::newRow("var") << QStringLiteral("var.qml");
    QTest::newRow("defaultProperty") << QStringLiteral("defaultProperty.qml");
    QTest::newRow("defaultPropertyList") << QStringLiteral("defaultPropertyList.qml");
    QTest::newRow("defaultPropertyComponent") << QStringLiteral("defaultPropertyComponent.qml");
    QTest::newRow("defaultPropertyComponent2") << QStringLiteral("defaultPropertyComponent.2.qml");
    QTest::newRow("defaultPropertyListModel") << QStringLiteral("defaultPropertyListModel.qml");
    QTest::newRow("defaultPropertyVar") << QStringLiteral("defaultPropertyVar.qml");
    QTest::newRow("multiDefaultProperty") << QStringLiteral("multiDefaultPropertyOk.qml");
    QTest::newRow("propertyDelegate") << QStringLiteral("propertyDelegate.qml");
    QTest::newRow("duplicateQmldirImport") << QStringLiteral("qmldirImport/duplicate.qml");
    QTest::newRow("Used imports") << QStringLiteral("used.qml");
    QTest::newRow("Unused imports (multi)") << QStringLiteral("unused_multi.qml");
    QTest::newRow("Unused static module") << QStringLiteral("unused_static.qml");
    QTest::newRow("compositeSingleton") << QStringLiteral("compositesingleton.qml");
    QTest::newRow("stringLength") << QStringLiteral("stringLength.qml");
    QTest::newRow("stringLength2") << QStringLiteral("stringLength2.qml");
    QTest::newRow("stringLength3") << QStringLiteral("stringLength3.qml");
    QTest::newRow("attachedPropertyAssignments")
            << QStringLiteral("attachedPropertyAssignments.qml");
    QTest::newRow("groupedPropertyAssignments") << QStringLiteral("groupedPropertyAssignments.qml");
    QTest::newRow("goodAttachedProperty") << QStringLiteral("goodAttachedProperty.qml");
    QTest::newRow("objectBindingOnVarProperty") << QStringLiteral("objectBoundToVar.qml");
    QTest::newRow("Unversioned change signal without arguments") << QStringLiteral("unversionChangedSignalSansArguments.qml");
    QTest::newRow("deprecatedFunctionOverride") << QStringLiteral("deprecatedFunctionOverride.qml");
    QTest::newRow("multilineStringEscaped") << QStringLiteral("multilineStringEscaped.qml");
    QTest::newRow("propertyOverride") << QStringLiteral("propertyOverride.qml");
    QTest::newRow("propertyBindingValue") << QStringLiteral("propertyBindingValue.qml");
    QTest::newRow("customParser") << QStringLiteral("customParser.qml");
    QTest::newRow("customParser.recursive") << QStringLiteral("customParser.recursive.qml");
    QTest::newRow("2Behavior") << QStringLiteral("2behavior.qml");
    QTest::newRow("interceptor") << QStringLiteral("interceptor.qml");
    QTest::newRow("valueSource") << QStringLiteral("valueSource.qml");
    QTest::newRow("interceptor+valueSource") << QStringLiteral("interceptor_valueSource.qml");
    QTest::newRow("groupedProperty (valueSource+interceptor)")
            << QStringLiteral("groupedProperty_valueSource_interceptor.qml");
    QTest::newRow("QtQuick.Window 2.1") << QStringLiteral("qtquickWindow21.qml");
    QTest::newRow("attachedTypeIndirect") << QStringLiteral("attachedTypeIndirect.qml");
    QTest::newRow("objectArray") << QStringLiteral("objectArray.qml");
    QTest::newRow("aliasToList") << QStringLiteral("aliasToList.qml");
    QTest::newRow("QVariant") << QStringLiteral("qvariant.qml");
    QTest::newRow("Accessible") << QStringLiteral("accessible.qml");
    QTest::newRow("qjsroot") << QStringLiteral("qjsroot.qml");
    QTest::newRow("InlineComponent") << QStringLiteral("inlineComponent.qml");
    QTest::newRow("InlineComponentWithComponents") << QStringLiteral("inlineComponentWithComponents.qml");
    QTest::newRow("InlineComponentsChained") << QStringLiteral("inlineComponentsChained.qml");
    QTest::newRow("ignoreWarnings") << QStringLiteral("ignoreWarnings.qml");
    QTest::newRow("BindingBeforeDeclaration") << QStringLiteral("bindingBeforeDeclaration.qml");
    QTest::newRow("CustomParserUnqualifiedAccess")
            << QStringLiteral("customParserUnqualifiedAccess.qml");
    QTest::newRow("ImportQMLModule") << QStringLiteral("importQMLModule.qml");
    QTest::newRow("ImportDirectoryQmldir") << QStringLiteral("Things/LintDirectly.qml");
    QTest::newRow("BindingsOnGroupAndAttachedProperties")
            << QStringLiteral("goodBindingsOnGroupAndAttached.qml");
    QTest::newRow("QQmlEasingEnums::Type") << QStringLiteral("animationEasing.qml");
    QTest::newRow("ValidLiterals") << QStringLiteral("validLiterals.qml");
    QTest::newRow("GoodModulePrefix") << QStringLiteral("goodModulePrefix.qml");
    QTest::newRow("required property in Component") << QStringLiteral("requiredPropertyInComponent.qml");
    QTest::newRow("bytearray") << QStringLiteral("bytearray.qml");
    QTest::newRow("initReadonly") << QStringLiteral("initReadonly.qml");
    QTest::newRow("connectionNoParent") << QStringLiteral("connectionNoParent.qml"); // QTBUG-97600
    QTest::newRow("goodGeneralizedGroup") << QStringLiteral("goodGeneralizedGroup.qml");
    QTest::newRow("on binding in grouped property") << QStringLiteral("onBindingInGroupedProperty.qml");
    QTest::newRow("declared property of JS object") << QStringLiteral("bareQt.qml");
    QTest::newRow("ID overrides property") << QStringLiteral("accessibleId.qml");
    QTest::newRow("matchByName") << QStringLiteral("matchByName.qml");
    QTest::newRow("QObject.hasOwnProperty") << QStringLiteral("qobjectHasOwnProperty.qml");
    QTest::newRow("cppPropertyChangeHandlers")
            << QStringLiteral("goodCppPropertyChangeHandlers.qml");
    QTest::newRow("unexportedCppBase") << QStringLiteral("unexportedCppBase.qml");
    QTest::newRow("requiredWithRootLevelAlias") << QStringLiteral("RequiredWithRootLevelAlias.qml");
    QTest::newRow("jsVarDeclarations") << QStringLiteral("jsVarDeclarations.qml");
    QTest::newRow("qmodelIndex") << QStringLiteral("qmodelIndex.qml");
    QTest::newRow("boundComponents") << QStringLiteral("boundComponents.qml");
    QTest::newRow("prefixedAttachedProperty") << QStringLiteral("prefixedAttachedProperty.qml");
    QTest::newRow("callLater") << QStringLiteral("callLater.qml");
    QTest::newRow("listPropertyMethods") << QStringLiteral("listPropertyMethods.qml");
    QTest::newRow("v4SequenceMethods") << QStringLiteral("v4SequenceMethods.qml");
    QTest::newRow("stringToByteArray") << QStringLiteral("stringToByteArray.qml");
    QTest::newRow("jsLibrary") << QStringLiteral("jsLibrary.qml");
    QTest::newRow("nullBindingFunction") << QStringLiteral("nullBindingFunction.qml");
    QTest::newRow("BindingTypeMismatchFunction") << QStringLiteral("bindingTypeMismatchFunction.qml");
    QTest::newRow("BindingTypeMismatch") << QStringLiteral("bindingTypeMismatch.qml");
    QTest::newRow("template literal (substitution)") << QStringLiteral("templateStringSubstitution.qml");
    QTest::newRow("enumsOfScrollBar") << QStringLiteral("enumsOfScrollBar.qml");
    QTest::newRow("optionalChainingCall") << QStringLiteral("optionalChainingCall.qml");
    QTest::newRow("EnumAccessCpp") << QStringLiteral("EnumAccessCpp.qml");
    QTest::newRow("qtquickdialog") << QStringLiteral("qtquickdialog.qml");
    QTest::newRow("callBase") << QStringLiteral("callBase.qml");
    QTest::newRow("propertyWithOn") << QStringLiteral("switcher.qml");
    QTest::newRow("constructorProperty") << QStringLiteral("constructorProperty.qml");
    QTest::newRow("onlyMajorVersion") << QStringLiteral("onlyMajorVersion.qml");
    QTest::newRow("attachedImportUse") << QStringLiteral("attachedImportUse.qml");
    QTest::newRow("VariantMapGetPropertyLookup") << QStringLiteral("variantMapLookup.qml");
    QTest::newRow("StringToDateTime") << QStringLiteral("stringToDateTime.qml");
    QTest::newRow("ScriptInTemplate") << QStringLiteral("scriptInTemplate.qml");
    QTest::newRow("AddressableValue") << QStringLiteral("addressableValue.qml");
    QTest::newRow("WriteListProperty") << QStringLiteral("writeListProperty.qml");
    QTest::newRow("dontConfuseMemberPrintWithGlobalPrint") << QStringLiteral("findMemberPrint.qml");
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);

    QJsonArray warnings;

    runTest(filename, Result::clean());
}

void TestQmllint::compilerWarnings_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");
    QTest::addColumn<bool>("enableCompilerWarnings");

    QTest::newRow("listIndices") << QStringLiteral("listIndices.qml") << Result::clean() << true;
    QTest::newRow("lazyAndDirect")
            << QStringLiteral("LazyAndDirect/Lazy.qml") << Result::clean() << true;
    QTest::newRow("qQmlV4Function") << QStringLiteral("varargs.qml") << Result::clean() << true;
    QTest::newRow("multiGrouped") << QStringLiteral("multiGrouped.qml") << Result::clean() << true;

    QTest::newRow("shadowable") << QStringLiteral("shadowable.qml")
                                << Result { { Message { QStringLiteral(
                                           "with type NotSoSimple can be shadowed") } } }
                                << true;
    QTest::newRow("tooFewParameters")
            << QStringLiteral("tooFewParams.qml")
            << Result { { Message { QStringLiteral("No matching override found") } } } << true;
    QTest::newRow("javascriptVariableArgs")
            << QStringLiteral("javascriptVariableArgs.qml")
            << Result { { Message {
                       QStringLiteral("Function expects 0 arguments, but 2 were provided") } } }
            << true;
    QTest::newRow("unknownTypeInRegister")
            << QStringLiteral("unknownTypeInRegister.qml")
            << Result { { Message {
                       QStringLiteral("Functions without type annotations won't be compiled") } } }
            << true;
    QTest::newRow("pragmaStrict")
            << QStringLiteral("pragmaStrict.qml")
            << Result { { { QStringLiteral(
                       "Functions without type annotations won't be compiled") } } }
            << true;
    QTest::newRow("generalizedGroupHint")
            << QStringLiteral("generalizedGroupHint.qml")
            << Result { { { QStringLiteral(
                       "Cannot resolve property type  for binding on myColor. "
                       "You may want use ID-based grouped properties here.") } } }
            << true;
    QTest::newRow("invalidIdLookup")
            << QStringLiteral("invalidIdLookup.qml")
            << Result { { {
                    QStringLiteral("Cannot retrieve a non-object type by ID: stateMachine")
               } } }
            << true;
}

void TestQmllint::compilerWarnings()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);
    QFETCH(bool, enableCompilerWarnings);

    QJsonArray warnings;

    auto categories = QQmlJSLogger::defaultCategories();

    auto category = std::find_if(categories.begin(), categories.end(), [](const QQmlJS::LoggerCategory& category) {
        return category.id() == qmlCompiler;
    });
    Q_ASSERT(category != categories.end());

    if (enableCompilerWarnings) {
        category->setLevel(QtWarningMsg);
        category->setIgnored(false);
    }

    runTest(filename, result, {}, {}, {}, UseDefaultImports, &categories);
}

QString TestQmllint::runQmllint(const QString &fileToLint,
                                std::function<void(QProcess &)> handleResult,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addImportDirs, bool absolutePath)
{
    auto qmlImportDir = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QStringList args;

    QString absoluteFilePath =
            QFileInfo(fileToLint).isAbsolute() ? fileToLint : testFile(fileToLint);

    args << QFileInfo(absoluteFilePath).fileName();

    if (addImportDirs) {
        args << QStringLiteral("-I") << qmlImportDir
             << QStringLiteral("-I") << dataDirectory();
    }

    if (ignoreSettings)
        args << QStringLiteral("--ignore-settings");

    if (absolutePath)
        args << QStringLiteral("--absolute-path");

    args << extraArgs;
    args << QStringLiteral("--silent");
    QString errors;
    auto verify = [&](bool isSilent) {
        QProcess process;
        process.setWorkingDirectory(QFileInfo(absoluteFilePath).absolutePath());
        process.start(m_qmllintPath, args);
        handleResult(process);
        errors = process.readAllStandardError();

        QStringList lines = errors.split(u'\n', Qt::SkipEmptyParts);

        auto end = std::remove_if(lines.begin(), lines.end(), [](const QString &line) {
            return !line.startsWith("Warning: ") && !line.startsWith("Error: ");
        });

        std::sort(lines.begin(), end);
        auto it = std::unique(lines.begin(), end);
        if (it != end) {
            qDebug() << "The warnings and errors were generated more than once:";
            do {
                qDebug() << *it;
            } while (++it != end);
            QTest::qFail("Duplicate warnings and errors", __FILE__, __LINE__);
        }

        if (isSilent) {
            QTest::qVerify(errors.isEmpty(), "errors.isEmpty()", "Silent mode outputs messages",
                           __FILE__, __LINE__);
        }

        if (QTest::currentTestFailed()) {
            qDebug().noquote() << "Command:" << process.program() << args.join(u' ');
            qDebug() << "Exit status:" << process.exitStatus();
            qDebug() << "Exit code:" << process.exitCode();
            qDebug() << "stderr:" << errors;
            qDebug() << "stdout:" << process.readAllStandardOutput();
        }
    };
    verify(true);
    args.removeLast();
    verify(false);
    return errors;
}

QString TestQmllint::runQmllint(const QString &fileToLint, bool shouldSucceed,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addImportDirs, bool absolutePath)
{
    return runQmllint(
            fileToLint,
            [&](QProcess &process) {
                QVERIFY(process.waitForFinished());
                QCOMPARE(process.exitStatus(), QProcess::NormalExit);

                if (shouldSucceed)
                    QCOMPARE(process.exitCode(), 0);
                else
                    QVERIFY(process.exitCode() != 0);
            },
            extraArgs, ignoreSettings, addImportDirs, absolutePath);
}

void TestQmllint::callQmllint(const QString &fileToLint, bool shouldSucceed, QJsonArray *warnings,
                              QStringList importPaths, QStringList qmldirFiles,
                              QStringList resources, DefaultImportOption defaultImports,
                              QList<QQmlJS::LoggerCategory> *categories, bool autoFixable,
                              LintType type)
{
    QJsonArray jsonOutput;

    const QFileInfo info = QFileInfo(fileToLint);
    const QString lintedFile = info.isAbsolute() ? fileToLint : testFile(fileToLint);

    QQmlJSLinter::LintResult lintResult;

    const QStringList resolvedImportPaths = defaultImports == UseDefaultImports
            ? m_defaultImportPaths + importPaths
            : importPaths;
    if (type == LintFile) {
        const QList<QQmlJS::LoggerCategory> resolvedCategories =
                categories != nullptr ? *categories : QQmlJSLogger::defaultCategories();
        lintResult = m_linter.lintFile(
                    lintedFile, nullptr, true, &jsonOutput, resolvedImportPaths, qmldirFiles,
                    resources, resolvedCategories);
    } else {
        lintResult =
                m_linter.lintModule(fileToLint, true, &jsonOutput, resolvedImportPaths, resources);
    }

    bool success = lintResult == QQmlJSLinter::LintSuccess;
    QEXPECT_FAIL("qtquickdialog", "Will fail until QTBUG-104091 is implemented", Abort);
    QVERIFY2(success == shouldSucceed, QJsonDocument(jsonOutput).toJson());

    if (warnings) {
        QVERIFY2(jsonOutput.size() == 1, QJsonDocument(jsonOutput).toJson());
        *warnings = jsonOutput.at(0)[u"warnings"_s].toArray();
    }

    QCOMPARE(success, shouldSucceed);

    if (lintResult == QQmlJSLinter::LintSuccess || lintResult == QQmlJSLinter::HasWarnings) {
        QString fixedCode;
        QQmlJSLinter::FixResult fixResult = m_linter.applyFixes(&fixedCode, true);

        if (autoFixable) {
            QCOMPARE(fixResult, QQmlJSLinter::FixSuccess);
            // Check that the fixed version of the file actually passes qmllint now
            QTemporaryDir dir;
            QVERIFY(dir.isValid());
            QFile file(dir.filePath("Fixed.qml"));
            QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
            file.write(fixedCode.toUtf8());
            file.flush();
            file.close();

            callQmllint(QFileInfo(file).absoluteFilePath(), true, nullptr, importPaths, qmldirFiles,
                        resources, defaultImports, categories, false);

            const QString fixedPath = testFile(info.baseName() + u".fixed.qml"_s);

            if (QFileInfo(fixedPath).exists()) {
                QFile fixedFile(fixedPath);
                fixedFile.open(QFile::ReadOnly);
                QString fixedFileContents = QString::fromUtf8(fixedFile.readAll());
#ifdef Q_OS_WIN
                fixedCode = fixedCode.replace(u"\r\n"_s, u"\n"_s);
                fixedFileContents = fixedFileContents.replace(u"\r\n"_s, u"\n"_s);
#endif

                QCOMPARE(fixedCode, fixedFileContents);
            }
        } else {
            if (shouldSucceed)
                QCOMPARE(fixResult, QQmlJSLinter::NothingToFix);
            else
                QVERIFY(fixResult == QQmlJSLinter::FixSuccess
                        || fixResult == QQmlJSLinter::NothingToFix);
        }
    }
}

void TestQmllint::runTest(const QString &testFile, const Result &result, QStringList importDirs,
                          QStringList qmltypesFiles, QStringList resources,
                          DefaultImportOption defaultImports,
                          QList<QQmlJS::LoggerCategory> *categories)
{
    QJsonArray warnings;
    callQmllint(testFile, result.flags.testFlag(Result::Flag::ExitsNormally), &warnings, importDirs,
                qmltypesFiles, resources, defaultImports, categories,
                result.flags.testFlag(Result::Flag::AutoFixable));
    checkResult(warnings, result);
}

template<typename ExpectedMessageFailureHandler, typename BadMessageFailureHandler>
void TestQmllint::checkResult(const QJsonArray &warnings, const Result &result,
                              ExpectedMessageFailureHandler onExpectedMessageFailures,
                              BadMessageFailureHandler onBadMessageFailures)
{
    if (result.flags.testFlag(Result::Flag::NoMessages))
        QVERIFY2(warnings.isEmpty(), qPrintable(QJsonDocument(warnings).toJson()));

    for (const Message &msg : result.expectedMessages) {
        // output.contains() expect fails:
        onExpectedMessageFailures();

        searchWarnings(warnings, msg.text, msg.severity, msg.line, msg.column);
    }

    for (const Message &msg : result.badMessages) {
        // !output.contains() expect fails:
        onBadMessageFailures();

        searchWarnings(warnings, msg.text, msg.severity, msg.line, msg.column, StringNotContained);
    }

    for (const Message &replacement : result.expectedReplacements) {
        searchWarnings(warnings, replacement.text, replacement.severity, replacement.line,
                       replacement.column, StringContained, DoReplacementSearch);
    }
}

void TestQmllint::searchWarnings(const QJsonArray &warnings, const QString &substring,
                                 QtMsgType type, quint32 line, quint32 column,
                                 ContainOption shouldContain, ReplacementOption searchReplacements)
{
    bool contains = false;

    auto typeStringToMsgType = [](const QString &type) -> QtMsgType {
        if (type == u"debug")
            return QtDebugMsg;
        if (type == u"info")
            return QtInfoMsg;
        if (type == u"warning")
            return QtWarningMsg;
        if (type == u"critical")
            return QtCriticalMsg;
        if (type == u"fatal")
            return QtFatalMsg;

        Q_UNREACHABLE();
    };

    for (const QJsonValueConstRef warning : warnings) {
        QString warningMessage = warning[u"message"].toString();
        quint32 warningLine = warning[u"line"].toInt();
        quint32 warningColumn = warning[u"column"].toInt();
        QtMsgType warningType = typeStringToMsgType(warning[u"type"].toString());

        if (warningMessage.contains(substring)) {
            if (warningType != type) {
                continue;
            }
            if (line != 0 || column != 0) {
                if (warningLine != line || warningColumn != column) {
                    continue;
                }
            }

            contains = true;
            break;
        }

        for (const QJsonValueConstRef fix : warning[u"suggestions"].toArray()) {
            const QString fixMessage = fix[u"message"].toString();
            if (fixMessage.contains(substring)) {
                contains = true;
                break;
            }

            if (searchReplacements == DoReplacementSearch) {
                QString replacement = fix[u"replacement"].toString();
#ifdef Q_OS_WIN
                // Replacements can contain native line endings
                // but we need them to be uniform in order for them to conform to our test data
                replacement = replacement.replace(u"\r\n"_s, u"\n"_s);
#endif

                if (replacement.contains(substring)) {
                    quint32 fixLine = fix[u"line"].toInt();
                    quint32 fixColumn = fix[u"column"].toInt();
                    if (line != 0 || column != 0) {
                        if (fixLine != line || fixColumn != column) {
                            continue;
                        }
                    }
                    contains = true;
                    break;
                }
            }
        }
    }

    const auto toDescription = [](const QJsonArray &warnings, const QString &substring,
                                  quint32 line, quint32 column, bool must = true) {
        QString msg = QStringLiteral("qmllint output:\n%1\nIt %2 contain '%3'")
                              .arg(QString::fromUtf8(
                                           QJsonDocument(warnings).toJson(QJsonDocument::Indented)),
                                   must ? u"must" : u"must NOT", substring);
        if (line != 0 || column != 0)
            msg += u" (%1:%2)"_s.arg(line).arg(column);

        return msg;
    };

    if (shouldContain == StringContained) {
        if (!contains)
            qWarning().noquote() << toDescription(warnings, substring, line, column);
        QVERIFY(contains);
    } else {
        if (contains)
            qWarning().noquote() << toDescription(warnings, substring, line, column, false);
        QVERIFY(!contains);
    }
}

void TestQmllint::requiredProperty()
{
    runTest("requiredProperty.qml", Result::clean());

    runTest("requiredMissingProperty.qml",
            Result { { Message { QStringLiteral(
                    "Property \"foo\" was marked as required but does not exist.") } } });

    runTest("requiredPropertyBindings.qml", Result::clean());
    runTest("requiredPropertyBindingsNow.qml",
            Result { { Message { QStringLiteral("Component is missing required property "
                                                "required_now_string from Base") },
                       Message { QStringLiteral("Component is missing required property "
                                                "required_defined_here_string from here") } } });
    runTest("requiredPropertyBindingsLater.qml",
            Result { { Message { QStringLiteral("Component is missing required property "
                                                "required_later_string from "
                                                "Base") },
                       Message { QStringLiteral("Property marked as required in Derived") },
                       Message { QStringLiteral("Component is missing required property "
                                                "required_even_later_string "
                                                "from Base (marked as required by here)") } } });
}

void TestQmllint::settingsFile()
{
    QVERIFY(runQmllint("settings/unqualifiedSilent/unqualified.qml", true, QStringList(), false)
                    .isEmpty());
    QVERIFY(runQmllint("settings/unusedImportWarning/unused.qml", false, QStringList(), false)
                    .contains(QStringLiteral("Warning: %1:2:1: Unused import")
                                      .arg(testFile("settings/unusedImportWarning/unused.qml"))));
    QVERIFY(runQmllint("settings/bare/bare.qml", false, {}, false, false)
                    .contains(QStringLiteral("Failed to find the following builtins: "
                                             "builtins.qmltypes, jsroot.qmltypes")));
    QVERIFY(runQmllint("settings/qmltypes/qmltypes.qml", false, QStringList(), false)
                    .contains(QStringLiteral("not a qmldir file. Assuming qmltypes.")));
    QVERIFY(runQmllint("settings/qmlimports/qmlimports.qml", true, QStringList(), false).isEmpty());
}

void TestQmllint::additionalImplicitImport()
{
    // We're polluting the resource file system here, so let's clean up afterwards.
    const auto guard = qScopeGuard([this]() {m_linter.clearCache(); });
    runTest("additionalImplicitImport.qml", Result::clean(), {}, {},
            { testFile("implicitImportResource.qrc") });
}

void TestQmllint::qrcUrlImport()
{
    const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });

    QJsonArray warnings;
    callQmllint(testFile("untitled/main.qml"), true, &warnings, {}, {},
                { testFile("untitled/qrcUrlImport.qrc") });
    checkResult(warnings, Result::clean());
}

void TestQmllint::attachedPropertyReuse()
{
    auto categories = QQmlJSLogger::defaultCategories();
    auto category = std::find_if(categories.begin(), categories.end(), [](const QQmlJS::LoggerCategory& category) {
        return category.id() == qmlAttachedPropertyReuse;
    });
    Q_ASSERT(category != categories.end());

    category->setLevel(QtWarningMsg);
    category->setIgnored(false);
    runTest("attachedPropNotReused.qml",
            Result { { Message { QStringLiteral("Using attached type QQuickKeyNavigationAttached "
                                                "already initialized in a parent "
                                                "scope") } } },
            {}, {}, {}, UseDefaultImports, &categories);

    runTest("attachedPropEnum.qml", Result::clean(), {}, {}, {}, UseDefaultImports, &categories);
    runTest("MyStyle/ToolBar.qml", Result {
        {
            Message {
                "Using attached type MyStyle already initialized in a parent scope"_L1,
                10,
                16
            }
        },
        {},
        {
            Message {
                "Reference it by id instead"_L1,
                10,
                16
            }
        },
        Result::AutoFixable
    });
}

void TestQmllint::missingBuiltinsNoCrash()
{
    // We cannot use the normal linter here since the other tests might have cached the builtins
    // alread
    QQmlJSLinter linter(m_defaultImportPaths);

    QJsonArray jsonOutput;
    QJsonArray warnings;

    bool success = linter.lintFile(testFile("missingBuiltinsNoCrash.qml"), nullptr, true,
                                   &jsonOutput, {}, {}, {}, {})
            == QQmlJSLinter::LintSuccess;
    QVERIFY2(!success, QJsonDocument(jsonOutput).toJson());

    QVERIFY2(jsonOutput.size() == 1, QJsonDocument(jsonOutput).toJson());
    warnings = jsonOutput.at(0)[u"warnings"_s].toArray();

    checkResult(warnings,
                Result { { Message { QStringLiteral("Failed to find the following builtins: "
                                                    "builtins.qmltypes, jsroot.qmltypes") } } });
}

void TestQmllint::absolutePath()
{
    QString absPathOutput = runQmllint("memberNotFound.qml", false, {}, true, true, true);
    QString relPathOutput = runQmllint("memberNotFound.qml", false, {}, true, true, false);
    const QString absolutePath = QFileInfo(testFile("memberNotFound.qml")).absoluteFilePath();

    QVERIFY(absPathOutput.contains(absolutePath));
    QVERIFY(!relPathOutput.contains(absolutePath));
}

void TestQmllint::importMultipartUri()
{
    runTest("here.qml", Result::clean(), {}, { testFile("Elsewhere/qmldir") });
}

void TestQmllint::lintModule_data()
{
    QTest::addColumn<QString>("module");
    QTest::addColumn<QStringList>("importPaths");
    QTest::addColumn<QStringList>("resources");
    QTest::addColumn<Result>("result");

    QTest::addRow("Things")
            << u"Things"_s
            << QStringList()
            << QStringList()
            << Result {
                   { Message {
                             u"Type \"QPalette\" not found. Used in SomethingEntirelyStrange.palette"_s,
                     },
                     Message {
                             u"Type \"CustomPalette\" is not fully resolved. Used in SomethingEntirelyStrange.palette2"_s } }
               };
    QTest::addRow("missingQmltypes")
            << u"Fake5Compat.GraphicalEffects.private"_s
            << QStringList()
            << QStringList()
            << Result { { Message { u"QML types file does not exist"_s } } };

    QTest::addRow("moduleWithQrc")
            << u"moduleWithQrc"_s
            << QStringList({ testFile("hidden") })
            << QStringList({
                               testFile("hidden/qmake_moduleWithQrc.qrc"),
                               testFile("hidden/moduleWithQrc_raw_qml_0.qrc")
                           })
            << Result::clean();
}

void TestQmllint::lintModule()
{
    QFETCH(QString, module);
    QFETCH(QStringList, importPaths);
    QFETCH(QStringList, resources);
    QFETCH(Result, result);

    QJsonArray warnings;
    callQmllint(module, result.flags & Result::ExitsNormally, &warnings, importPaths, {}, resources,
                UseDefaultImports, nullptr, false, LintModule);
    checkResult(warnings, result);
}

void TestQmllint::testLineEndings()
{
    {
        const auto textWithLF = QString::fromUtf16(u"import QtQuick 2.0\nimport QtTest 2.0 // qmllint disable unused-imports\n"
            "import QtTest 2.0 // qmllint disable\n\nItem {\n    @Deprecated {}\n    property string deprecated\n\n    "
            "property string a: root.a // qmllint disable unqualifi77777777777777777777777777777777777777777777777777777"
            "777777777777777777777777777777777777ed\n    property string b: root.a // qmllint di000000000000000000000000"
            "000000000000000000inyyyyyyyyg c: root.a\n    property string d: root.a\n    // qmllint enable unqualified\n\n    "
            "//qmllint d       4isable\n    property string e: root.a\n    Component.onCompleted: {\n        console.log"
            "(deprecated);\n    }\n    // qmllint enable\n\n}\n");

        const auto lintResult = m_linter.lintFile( {}, &textWithLF, true, nullptr, {}, {}, {}, {});

        QCOMPARE(lintResult, QQmlJSLinter::LintResult::HasWarnings);
    }
    {
        const auto textWithCRLF = QString::fromUtf16(u"import QtQuick 2.0\nimport QtTest 2.0 // qmllint disable unused-imports\n"
        "import QtTest 2.0 // qmllint disable\n\nItem {\n    @Deprecated {}\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r"
        "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\n    property string deprecated\n\n    property string a: root.a "
        "// qmllint disable unqualifi77777777777777777777777777777777777777777777777777777777777777777777777777777777777777777ed\n    "
        "property string b: root.a // qmllint di000000000000000000000000000000000000000000inyyyyyyyyg c: root.a\n    property string d: "
        "root.a\n    // qmllint enable unqualified\n\n    //qmllint d       4isable\n    property string e: root.a\n    Component.onCompleted: "
        "{\n        console.log(deprecated);\n    }\n    // qmllint enable\n\n}\n");

        const auto lintResult = m_linter.lintFile( {}, &textWithCRLF, true, nullptr, {}, {}, {}, {});

        QCOMPARE(lintResult, QQmlJSLinter::LintResult::HasWarnings);
    }
}

#if QT_CONFIG(library)
void TestQmllint::testPlugin()
{
    bool pluginFound = false;
    for (const QQmlJSLinter::Plugin &plugin : m_linter.plugins()) {
        if (plugin.name() == "testPlugin") {
            pluginFound = true;
            QCOMPARE(plugin.author(), u"Qt"_s);
            QCOMPARE(plugin.description(), u"A test plugin for tst_qmllint"_s);
            QCOMPARE(plugin.version(), u"1.0"_s);
            break;
        }
    }
    QVERIFY(pluginFound);

    runTest("elementpass_pluginTest.qml", Result { { Message { u"ElementTest OK"_s, 4, 5 } } });
    runTest("propertypass_pluginTest.qml",
            Result {
                    { // Specific binding for specific property
                      Message {
                              u"Saw binding on Text property text with value NULL (and type 3) in scope Text"_s },

                      // Property on any type
                      Message { u"Saw read on Text property x in scope Text"_s },
                      Message {
                              u"Saw binding on Text property x with value NULL (and type 2) in scope Text"_s },
                      Message { u"Saw read on Text property x in scope Item"_s },
                      Message { u"Saw write on Text property x with value int in scope Item"_s },
                      Message {
                              u"Saw binding on Item property x with value NULL (and type 2) in scope Item"_s },
                      // ListModel
                      Message {
                              u"Saw binding on ListView property model with value ListModel (and type 8) in scope ListView"_s },
                      Message {
                              u"Saw binding on ListView property height with value NULL (and type 2) in scope ListView"_s } } });
    runTest("controlsWithQuick_pluginTest.qml",
            Result { { Message { u"QtQuick.Controls, QtQuick and QtQuick.Window present"_s } } });
    runTest("controlsWithoutQuick_pluginTest.qml",
            Result { { Message { u"QtQuick.Controls and NO QtQuick present"_s } } });
    // Verify that none of the passes do anything when they're not supposed to
    runTest("nothing_pluginTest.qml", Result::clean());

    QVERIFY(runQmllint("settings/plugin/elemenpass_pluginSettingTest.qml", true, QStringList(), false)
                    .isEmpty());
}

// TODO: Eventually tests for (real) plugins need to be moved into a separate file
void TestQmllint::quickPlugin()
{
    const auto &plugins = m_linter.plugins();

    const bool pluginFound =
            std::find_if(plugins.cbegin(), plugins.cend(),
                         [](const auto &plugin) { return plugin.name() == "Quick"; })
            != plugins.cend();
    QVERIFY(pluginFound);

    runTest("pluginQuick_anchors.qml",
            Result{ { Message{
                              u"Cannot specify left, right, and horizontalCenter anchors at the same time."_s },
                      Message {
                              u"Cannot specify top, bottom, and verticalCenter anchors at the same time."_s },
                      Message{
                              u"Baseline anchor cannot be used in conjunction with top, bottom, or verticalCenter anchors."_s },
                      Message { u"Cannot assign literal of type null to QQuickAnchorLine"_s, 5,
                                35 },
                      Message { u"Cannot assign literal of type null to QQuickAnchorLine"_s, 6,
                                33 } } });
    runTest("pluginQuick_anchorsUndefined.qml", Result::clean());
    runTest("pluginQuick_layoutChildren.qml",
            Result {
                    { Message {
                              u"Detected anchors on an item that is managed by a layout. This is undefined behavior; use Layout.alignment instead."_s },
                      Message {
                              u"Detected x on an item that is managed by a layout. This is undefined behavior; use Layout.leftMargin or Layout.rightMargin instead."_s },
                      Message {
                              u"Detected y on an item that is managed by a layout. This is undefined behavior; use Layout.topMargin or Layout.bottomMargin instead."_s },
                      Message {
                              u"Detected height on an item that is managed by a layout. This is undefined behavior; use implictHeight or Layout.preferredHeight instead."_s },
                      Message {
                              u"Detected width on an item that is managed by a layout. This is undefined behavior; use implicitWidth or Layout.preferredWidth instead."_s },
                      Message {
                              u"Cannot specify anchors for items inside Grid. Grid will not function."_s },
                      Message {
                              u"Cannot specify x for items inside Grid. Grid will not function."_s },
                      Message {
                              u"Cannot specify y for items inside Grid. Grid will not function."_s },
                      Message {
                              u"Cannot specify anchors for items inside Flow. Flow will not function."_s },
                      Message {
                              u"Cannot specify x for items inside Flow. Flow will not function."_s },
                      Message {
                              u"Cannot specify y for items inside Flow. Flow will not function."_s } } });
    runTest("pluginQuick_attached.qml",
            Result {
                    { Message { u"ToolTip must be attached to an Item"_s },
                      Message { u"SplitView attached property only works with Items"_s },
                      Message { u"ScrollIndicator must be attached to a Flickable"_s },
                      Message { u"ScrollBar must be attached to a Flickable or ScrollView"_s },
                      Message { u"Accessible must be attached to an Item"_s },
                      Message { u"EnterKey attached property only works with Items"_s },
                      Message {
                              u"LayoutDirection attached property only works with Items and Windows"_s },
                      Message { u"Layout must be attached to Item elements"_s },
                      Message { u"StackView attached property only works with Items"_s },
                      Message { u"TextArea must be attached to a Flickable"_s },
                      Message { u"StackLayout must be attached to an Item"_s },
                      Message {
                              u"Tumbler: attached properties of Tumbler must be accessed through a delegate item"_s },
                      Message {
                              u"Attached properties of SwipeDelegate must be accessed through an Item"_s },
                      Message { u"SwipeView must be attached to an Item"_s } } });

    runTest("pluginQuick_swipeDelegate.qml",
            Result { {
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with contentItem; unable to layout the item."_s,
                             6, 43 },
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with background; unable to layout the item."_s,
                             7, 43 },
                         Message { u"SwipeDelegate: Cannot set both behind and left/right properties"_s,
                                   9, 9 },
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with contentItem; unable to layout the item."_s,
                             13, 47 },
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with background; unable to layout the item."_s,
                             14, 42 },
                         Message { u"SwipeDelegate: Cannot set both behind and left/right properties"_s,
                                   16, 9 },
                     } });

    runTest("pluginQuick_varProp.qml",
            Result {
                    { Message {
                              u"Unexpected type for property \"contentItem\" expected QQuickPathView, QQuickListView got QQuickItem"_s },
                      Message {
                              u"Unexpected type for property \"columnWidthProvider\" expected function got null"_s },
                      Message {
                              u"Unexpected type for property \"textFromValue\" expected function got null"_s },
                      Message {
                              u"Unexpected type for property \"valueFromText\" expected function got int"_s },
                      Message {
                              u"Unexpected type for property \"rowHeightProvider\" expected function got int"_s } } });
    runTest("pluginQuick_varPropClean.qml", Result::clean());
    runTest("pluginQuick_attachedClean.qml", Result::clean());
    runTest("pluginQuick_attachedIgnore.qml", Result::clean());
    runTest("pluginQuick_noCrashOnUneresolved.qml", Result {}); // we don't care about the specific warnings

    runTest("pluginQuick_propertyChangesParsed.qml",
            Result { {
                Message {
                      u"Property \"myColor\" is custom-parsed in PropertyChanges. "
                       "You should phrase this binding as \"foo.myColor: Qt.rgba(0.5, ...\""_s,
                      12, 30
                },
                Message {
                      u"Unknown property \"notThere\" in PropertyChanges."_s,
                      13, 31
                }
            } });
    runTest("pluginQuick_propertyChangesInvalidTarget.qml", Result {}); // we don't care about the specific warnings
}
#endif

QTEST_MAIN(TestQmllint)
#include "tst_qmllint.moc"
