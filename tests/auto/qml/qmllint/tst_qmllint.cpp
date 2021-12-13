/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QProcess>
#include <QString>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlLint/private/qqmllinter_p.h>

class TestQmllint: public QQmlDataTest
{
    Q_OBJECT

public:
    TestQmllint();

private Q_SLOTS:
    void initTestCase() override;

    void testUnqualified();
    void testUnqualified_data();

    void cleanQmlCode_data();
    void cleanQmlCode();

    void dirtyQmlCode_data();
    void dirtyQmlCode();

    void testUnknownCausesFail();

    void directoryPassedAsQmlTypesFile();
    void oldQmltypes();

    void qmltypes_data();
    void qmltypes();

    void functionDeclaration();

    void signalHandler();

    void anchors();

#ifdef QT_QMLJSROOTGEN_PRESENT
    void verifyJsRoot();
#endif

    void autoqmltypes();
    void resources();

    void requiredProperty();

    void settingsFile();

    void additionalImplicitImport();
    void listIndices();
    void lazyAndDirect();

    void attachedPropertyReuse();

    void shadowable();
    void tooFewParameters();
    void qQmlV4Function();
    void missingBuiltinsNoCrash();
    void absolutePath();
    void multiGrouped();
    void javascriptVariableArgs();

private:
    enum DefaultIncludeOption { NoDefaultIncludes, UseDefaultIncludes };
    enum ContainOption { StringNotContained, StringContained };
    enum ReplacementOption {
        NoReplacementSearch,
        DoReplacementSearch,
    };

    QString runQmllint(const QString &fileToLint, std::function<void(QProcess &)> handleResult,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addIncludeDirs = true);
    QString runQmllint(const QString &fileToLint, bool shouldSucceed,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addIncludeDirs = true);
    void callQmllint(const QString &fileToLint, bool shouldSucceed, QJsonArray *warnings = nullptr,
                     QStringList includeDirs = {}, QStringList qmltypesFiles = {},
                     QStringList resources = {},
                     DefaultIncludeOption defaultIncludes = UseDefaultIncludes);

    void searchWarnings(const QJsonArray &warnings, const QString &string, const QString &filename,
                        ContainOption shouldContain = StringContained,
                        ReplacementOption searchReplacements = NoReplacementSearch);

    QString m_qmllintPath;
    QString m_qmljsrootgenPath;
    QString m_qmltyperegistrarPath;

    QStringList m_defaultImportPaths;
    QQmlLinter m_linter;
};

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
    QFETCH(QString, warningMessage);
    QFETCH(int, warningLine);
    QFETCH(int, warningColumn);

    const QString output = runQmllint(filename, false);
    QVERIFY(output.contains(QStringLiteral("%1:%2:%3: Unqualified access").arg(testFile(filename)).arg(warningLine).arg(warningColumn)));
    QVERIFY(output.contains(warningMessage));
}

void TestQmllint::testUnqualified_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("warningMessage");
    QTest::addColumn<int>("warningLine");
    QTest::addColumn<int>("warningColumn");

    // id from nowhere (as with setContextProperty)
    QTest::newRow("IdFromOuterSpaceDirect") << QStringLiteral("IdFromOuterSpace.qml") << "alien.x" << 4 << 8;
    QTest::newRow("IdFromOuterSpaceAccess") << QStringLiteral("IdFromOuterSpace.qml") << "console.log(alien)" << 7 << 21;
    // access property of root object
    QTest::newRow("FromRootDirect") << QStringLiteral("FromRoot.qml") << QStringLiteral("x: root.unqualified") << 9 << 16; // new property
    QTest::newRow("FromRootAccess") << QStringLiteral("FromRoot.qml") << QStringLiteral("property int check: root.x") << 13 << 33;  // builtin property
    // access injected name from signal
    QTest::newRow("SignalHandler1")
            << QStringLiteral("SignalHandler.qml")
            << QStringLiteral("onDoubleClicked:  function(mouse) {") << 5 << 21;
    QTest::newRow("SignalHandler2")
            << QStringLiteral("SignalHandler.qml")
            << QStringLiteral("onPositionChanged:  function(mouse) {") << 10 << 21;
    QTest::newRow("SignalHandlerShort1") << QStringLiteral("SignalHandler.qml")
                                         << QStringLiteral("onClicked:  (mouse) => ") << 8 << 29;
    QTest::newRow("SignalHandlerShort2")
            << QStringLiteral("SignalHandler.qml") << QStringLiteral("onPressAndHold:  (mouse) => ")
            << 12 << 34;
    // access catch identifier outside catch block
    QTest::newRow("CatchStatement") << QStringLiteral("CatchStatement.qml") << QStringLiteral("err") << 6 << 21;

    QTest::newRow("NonSpuriousParent") << QStringLiteral("nonSpuriousParentWarning.qml") << QStringLiteral("property int x: <id>.parent.x") << 6 << 25;

    QTest::newRow("crashConnections")
        << QStringLiteral("crashConnections.qml")
        << QStringLiteral("target: FirstRunDialog") << 4 << 13;
}

void TestQmllint::testUnknownCausesFail()
{
    {
        QJsonArray warnings;
        callQmllint("unknownElement.qml", false, &warnings);
        searchWarnings(
                warnings,
                QStringLiteral(
                        "Warning: %1:4:5: Unknown was not found. Did you add all import paths?")
                        .arg(testFile("unknownElement.qml")),
                "unknownElement.qml");
    }
    {
        QJsonArray warnings;
        callQmllint("TypeWithUnknownPropertyType.qml", false, &warnings);
        searchWarnings(
                warnings,
                QStringLiteral(
                        "Warning: %1:4:5: Something was not found. Did you add all import paths?")
                        .arg(testFile("TypeWithUnknownPropertyType.qml")),
                "TypeWithUnknownPropertyType.qml");
    }
}

void TestQmllint::directoryPassedAsQmlTypesFile()
{
    QJsonArray warnings;
    callQmllint("unknownElement.qml", false, &warnings, {}, { dataDirectory() });
    searchWarnings(warnings,
                   QStringLiteral("QML types file cannot be a directory: ") + dataDirectory(),
                   "unknownElement.qml");
}

void TestQmllint::oldQmltypes()
{
    QJsonArray warnings;
    callQmllint("oldQmltypes.qml", false, &warnings);
    searchWarnings(warnings, QStringLiteral("typeinfo not declared in qmldir file"),
                   "oldQmltypes.qml");
    searchWarnings(warnings,
                   QStringLiteral("QQuickItem was not found. Did you add all import paths?"),
                   "oldQmltypes.qml", StringNotContained);
    searchWarnings(warnings, QStringLiteral("Found deprecated dependency specifications"),
                   "oldQmltypes.qml");

    // Checking for both lines separately so that we don't have to mess with the line endings.b
    searchWarnings(
            warnings,
            QStringLiteral(
                    "Meta object revision and export version differ, ignoring the revision."),
            "oldQmltypes.qml");
    searchWarnings(warnings,
                   QStringLiteral("Revision 0 corresponds to version 0.0; it should be 1.0."),
                   "oldQmltypes.qml");
}

void TestQmllint::qmltypes_data()
{
    QTest::addColumn<QString>("file");

    const QString importsPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QDirIterator it(importsPath, { "*.qmltypes" },
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        QTest::addRow("%s", qPrintable(it.next().mid(importsPath.length()))) << it.filePath();
}

void TestQmllint::qmltypes()
{
    QFETCH(QString, file);
    callQmllint(file, true);
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
    QVERIFY(currentJsRoot.open(QFile::ReadOnly));
    currentJsRootContent = QString::fromUtf8(currentJsRoot.readAll());
    currentJsRoot.close();

    QFile generatedJsRoot(dir.path() + QDir::separator() + "jsroot.qmltypes");
    QVERIFY(generatedJsRoot.open(QFile::ReadOnly));
    generatedJsRootContent = QString::fromUtf8(generatedJsRoot.readAll());
    generatedJsRoot.close();

    // If any of the following asserts fail you need to update jsroot.qmltypes using the following commands:
    //
    // qmljsrootgen jsroot.json
    // qmltyperegistrar jsroot.json --generate-qmltypes src/imports/builtins/jsroot.qmltypes
    QStringList currentLines = currentJsRootContent.split(QLatin1Char('\n'));
    QStringList generatedLines = generatedJsRootContent.split(QLatin1Char('\n'));

    QCOMPARE(currentLines.count(), generatedLines.count());

    for (qsizetype i = 0; i < currentLines.count(); i++) {
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
    callQmllint(testFile("resource.qml"), true, nullptr, {}, {}, { testFile("resource.qrc") });
    callQmllint(testFile("badResource.qml"), false, nullptr, {}, {}, { testFile("resource.qrc") });
    callQmllint(testFile("resource.qml"), false);
    callQmllint(testFile("badResource.qml"), true);
    callQmllint(testFile("T/b.qml"), true, nullptr, {}, {}, { testFile("T/a.qrc") });
}

void TestQmllint::dirtyQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("warningMessage");
    QTest::addColumn<QString>("notContained");
    QTest::addColumn<QString>("replacement");
    QTest::addColumn<bool>("exitsNormally");

    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml") << QStringLiteral("%1:4:8: Expected token `:'")
            << QString() << QString() << false;
    QTest::newRow("Invalid_syntax_JS")
            << QStringLiteral("failure1.js") << QStringLiteral("%1:4:12: Expected token `;'")
            << QString() << QString() << false;
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << QString("Warning: %1:12:36: Unqualified access") << QString() << QString() << false;
    QTest::newRow("AutomatchedSignalHandler2")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << QString("Info: Implicitly defining onClicked as signal handler") << QString()
            << QString() << false;
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << QString("Warning: %1:6:31: Property \"foo\" not found on type \"QtObject\"")
            << QString() << QString() << false;
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << QString("Warning: %1:5:25: Property \"foo2\" not found on type \"Methods\"")
            << QString() << QString() << false;
    QTest::newRow("badAlias") << QStringLiteral("badAlias.qml")
                              << QString("Warning: %1:3:1: Cannot resolve alias \"wrong\"")
                              << QString() << QString() << false;
    QTest::newRow("badAliasProperty1") << QStringLiteral("badAliasProperty.qml")
                                       << QString("Warning: %1:3:1: Cannot resolve alias \"wrong\"")
                                       << QString() << QString() << false;
    QTest::newRow("badAliasExpression")
            << QStringLiteral("badAliasExpression.qml")
            << QString("Warning: %1:5:26: Invalid alias expression. Only IDs and field member "
                       "expressions can be aliased")
            << QString() << QString() << false;
    QTest::newRow("aliasCycle1") << QStringLiteral(
            "aliasCycle.qml") << QString("Warning: %1:3:1: Alias \"b\" is part of an alias cycle")
                                 << QString() << QString() << false;
    QTest::newRow("aliasCycle2") << QStringLiteral(
            "aliasCycle.qml") << QString("Warning: %1:3:1: Alias \"a\" is part of an alias cycle")
                                 << QString() << QString() << false;
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << QString("Warning: %1:5:34: Property \"rrr\" not found on type \"Item\"") << QString()
            << QString() << false;
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << QString("Warning: %1:7:39: Property \"progress\" not found on type \"QQuickItem\"")
            << QString() << QString() << false;
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << QString("Warning: %1:5:39: Property \"rrr\" not found on type \"QQuickItem\"")
            << QString() << QString() << false;
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << QString("Warning: %1:5:26: Type \"QPalette\" of property \"palette\" not found")
            << QString() << QString() << false;
    QTest::newRow("incompleteQmltypes2") << QStringLiteral("incompleteQmltypes2.qml")
                                         << QString("Warning: %1:5:35: Property \"weDontKnowIt\" "
                                                    "not found on type \"CustomPalette\"")
                                         << QString() << QString() << false;
    QTest::newRow("inheritanceCylce") << QStringLiteral("Cycle1.qml")
                                      << QString("Warning: %1: Cycle2 is part of an inheritance "
                                                 "cycle: Cycle2 -> Cycle3 -> Cycle1 -> Cycle2")
                                      << QString() << QString() << false;
    QTest::newRow("badQmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/bad.qml")
            << QString("Warning: %1:3:1: Item was not found. Did you add all import paths?")
            << QString() << QString() << false;
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleBad.qml")
            << QString("Warning: %1:5:21: Property \"unknownFunc\" not found on type \"Foo\"")
            << QString() << QString() << false;
    QTest::newRow("badEnumFromQtQml") << QStringLiteral("badEnumFromQtQml.qml")
                                      << QString("Warning: %1:4:30: Property \"Linear123\" not "
                                                 "found on type \"QQmlEasingEnums\"")
                                      << QString() << QString() << false;
    QTest::newRow("anchors3")
            << QStringLiteral("anchors3.qml")
            << QString("Cannot assign binding of type QQuickItem to QQuickAnchorLine") << QString()
            << QString() << false;
    QTest::newRow("nanchors1") << QStringLiteral("nanchors1.qml")
                               << QString("unknown grouped property scope nanchors.") << QString()
                               << QString() << false;
    QTest::newRow("nanchors2") << QStringLiteral("nanchors2.qml")
                               << QString("unknown grouped property scope nanchors.") << QString()
                               << QString() << false;
    QTest::newRow("nanchors3") << QStringLiteral("nanchors3.qml")
                               << QString("unknown grouped property scope nanchors.") << QString()
                               << QString() << false;
    QTest::newRow("badAliasObject") << QStringLiteral("badAliasObject.qml")
                                    << QString("Warning: %1:8:40: Property \"wrongwrongwrong\" not "
                                               "found on type \"QtObject\"")
                                    << QString() << QString() << false;
    QTest::newRow("badScript")
            << QStringLiteral("badScript.qml")
            << QString("Warning: %1:5:21: Property \"stuff\" not found on type \"Empty\"")
            << QString() << QString() << false;
    QTest::newRow("badScriptOnAttachedProperty")
            << QStringLiteral("badScript.attached.qml")
            << QString("Warning: %1:3:26: Unqualified access") << QString() << QString() << false;
    QTest::newRow("brokenNamespace") << QStringLiteral("brokenNamespace.qml")
                                     << QString("Warning: %1:4:19: Type not found in namespace")
                                     << QString() << QString() << false;
    QTest::newRow("segFault (bad)")
            << QStringLiteral("SegFault.bad.qml")
            << QStringLiteral("Property \"foobar\" not found on type \"QQuickScreenAttached\"")
            << QString() << QString() << false;
    QTest::newRow("VariableUsedBeforeDeclaration")
            << QStringLiteral("useBeforeDeclaration.qml")
            << QStringLiteral("%1:5:9: Variable \"argq\" is used here before its declaration. "
                              "The declaration is at 6:13.")
            << QString() << QString() << false;
    QTest::newRow("SignalParameterMismatch")
            << QStringLiteral("namedSignalParameters.qml")
            << QStringLiteral("Parameter 1 to signal handler for \"onSig\" is called \"argarg\". "
                              "The signal has a parameter of the same name in position 2.")
            << QStringLiteral("onSig2") << QString() << false;
    QTest::newRow("TooManySignalParameters")
            << QStringLiteral("tooManySignalParameters.qml")
            << QStringLiteral("Signal handler for \"onSig\" has more formal parameters "
                              "than the signal it handles.")
            << QString() << QString() << false;
    QTest::newRow("OnAssignment") << QStringLiteral("onAssignment.qml")
                                  << QStringLiteral("Property \"loops\" not found on type \"bool\"")
                                  << QString() << QString() << false;
    QTest::newRow("BadAttached") << QStringLiteral("badAttached.qml")
                                 << QStringLiteral("unknown attached property scope WrongAttached.")
                                 << QString() << QString() << false;
    QTest::newRow("BadBinding") << QStringLiteral("badBinding.qml")
                                << QStringLiteral(
                                           "Binding assigned to \"doesNotExist\", but no property "
                                           "\"doesNotExist\" exists in the current element.")
                                << QString() << QString() << false;
    QTest::newRow("bad template literal (simple)")
            << QStringLiteral("badTemplateStringSimple.qml")
            << QStringLiteral("Cannot assign binding of type string to int") << QString()
            << QString() << false;
    QTest::newRow("bad template literal (substitution)")
            << QStringLiteral("badTemplateStringSubstitution.qml")
            << QStringLiteral("Cannot assign binding of type QString to int") << QString()
            << QString() << false;
    QTest::newRow("bad constant number to string")
            << QStringLiteral("numberToStringProperty.qml")
            << QStringLiteral("Cannot assign a numeric constant to a string property") << QString()
            << QString() << false;
    QTest::newRow("bad unary minus to string")
            << QStringLiteral("unaryMinusToStringProperty.qml")
            << QStringLiteral("Cannot assign a numeric constant to a string property") << QString()
            << QString() << false;
    QTest::newRow("bad tranlsation binding (qsTr)")
            << QStringLiteral("bad_qsTr.qml") << QStringLiteral("") << QString() << QString()
            << false;
    QTest::newRow("bad string binding (QT_TR_NOOP)")
            << QStringLiteral("bad_QT_TR_NOOP.qml")
            << QStringLiteral("Cannot assign binding of type string to int") << QString()
            << QString() << false;
    QTest::newRow("BadScriptBindingOnGroup")
            << QStringLiteral("badScriptBinding.group.qml")
            << QStringLiteral("Warning: %1:3:10: Binding assigned to \"bogusProperty\", but no "
                              "property \"bogusProperty\" exists in the current element.")
            << QString() << QString() << false;
    QTest::newRow("BadScriptBindingOnAttachedType")
            << QStringLiteral("badScriptBinding.attached.qml")
            << QStringLiteral("Warning: %1:5:12: Binding assigned to \"bogusProperty\", but no "
                              "property \"bogusProperty\" exists in the current element.")
            << QString() << QString() << false;
    QTest::newRow("BadScriptBindingOnAttachedSignalHandler")
            << QStringLiteral("badScriptBinding.attachedSignalHandler.qml")
            << QStringLiteral(
                       "Warning: %1:3:10: no matching signal found for handler \"onBogusSignal\"")
            << QString() << QString() << false;
    QTest::newRow("BadPropertyType")
            << QStringLiteral("badPropertyType.qml")
            << QStringLiteral("No type found for property \"bad\". This may be due to a missing "
                              "import statement or incomplete qmltypes files.")
            << QString() << QString() << false;
    QTest::newRow("Deprecation (Property, with reason)")
            << QStringLiteral("deprecatedPropertyReason.qml")
            << QStringLiteral("Property \"deprecated\" is deprecated (Reason: Test)") << QString()
            << QString() << false;
    QTest::newRow("Deprecation (Property, no reason)")
            << QStringLiteral("deprecatedProperty.qml")
            << QStringLiteral("Property \"deprecated\" is deprecated") << QString() << QString()
            << false;
    QTest::newRow("Deprecation (Property binding, with reason)")
            << QStringLiteral("deprecatedPropertyBindingReason.qml")
            << QStringLiteral("Binding on deprecated property \"deprecatedReason\" (Reason: Test)")
            << QString() << QString() << false;
    QTest::newRow("Deprecation (Property binding, no reason)")
            << QStringLiteral("deprecatedPropertyBinding.qml")
            << QStringLiteral("Binding on deprecated property \"deprecated\"") << QString()
            << QString() << false;
    QTest::newRow("Deprecation (Type, with reason)")
            << QStringLiteral("deprecatedTypeReason.qml")
            << QStringLiteral("Type \"TypeDeprecatedReason\" is deprecated (Reason: Test)")
            << QString() << QString() << false;
    QTest::newRow("Deprecation (Type, no reason)")
            << QStringLiteral("deprecatedType.qml")
            << QStringLiteral("Type \"TypeDeprecated\" is deprecated") << QString() << QString()
            << false;
    QTest::newRow("MissingDefaultProperty")
            << QStringLiteral("defaultPropertyWithoutKeyword.qml")
            << QStringLiteral("Cannot assign to non-existent default property") << QString()
            << QString() << false;
    QTest::newRow("MissingDefaultPropertyDefinedInTheSameType")
            << QStringLiteral("defaultPropertyWithinTheSameType.qml")
            << QStringLiteral("Cannot assign to non-existent default property") << QString()
            << QString() << false;
    QTest::newRow("DoubleAssignToDefaultProperty")
            << QStringLiteral("defaultPropertyWithDoubleAssignment.qml")
            << QStringLiteral("Cannot assign multiple objects to a default non-list property")
            << QString() << QString() << false;
    QTest::newRow("DefaultPropertyWithWrongType(string)")
            << QStringLiteral("defaultPropertyWithWrongType.qml")
            << QStringLiteral("Cannot assign to default property of incompatible type")
            << QStringLiteral("Cannot assign to non-existent default property") << QString()
            << false;
    QTest::newRow("MultiDefaultPropertyWithWrongType")
            << QStringLiteral("multiDefaultPropertyWithWrongType.qml")
            << QStringLiteral("Cannot assign to default property of incompatible type")
            << QStringLiteral("Cannot assign to non-existent default property") << QString()
            << false;
    QTest::newRow("InvalidImport")
            << QStringLiteral("invalidImport.qml")
            << QStringLiteral("Failed to import FooBar. Are your include paths set up properly?")
            << QString() << QString() << false;
    QTest::newRow("Unused Import (simple)")
            << QStringLiteral("unused_simple.qml") << QStringLiteral("Unused import at %1:1:1")
            << QString() << QString() << true;
    QTest::newRow("Unused Import (prefix)")
            << QStringLiteral("unused_prefix.qml") << QStringLiteral("Unused import at %1:1:1")
            << QString() << QString() << true;
    QTest::newRow("TypePropertAccess") << QStringLiteral("typePropertyAccess.qml") << QString()
                                       << QString() << QString() << false;
    QTest::newRow("badAttachedProperty")
            << QStringLiteral("badAttachedProperty.qml")
            << QString("Property \"progress\" not found on type \"TestType\"") << QString()
            << QString() << false;
    QTest::newRow("badAttachedPropertyNested")
            << QStringLiteral("badAttachedPropertyNested.qml")
            << QString("12:41: Property \"progress\" not found on type \"QObject\"")
            << QString("6:37: Property \"progress\" not found on type \"QObject\"") << QString()
            << false;
    QTest::newRow("badAttachedPropertyTypeString")
            << QStringLiteral("badAttachedPropertyTypeString.qml")
            << QString("Cannot assign binding of type string to int") << QString() << QString()
            << false;
    QTest::newRow("badAttachedPropertyTypeQtObject")
            << QStringLiteral("badAttachedPropertyTypeQtObject.qml")
            << QString("Property \"count\" of type \"int\" is assigned an incompatible type "
                       "\"QtObject\"")
            << QString() << QString() << false;
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyAccess") << QStringLiteral("goodAttachedPropertyAccess.qml")
                                            << QString() << QString() << QString() << true;
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyNested") << QStringLiteral("goodAttachedPropertyNested.qml")
                                            << QString() << QString() << QString() << true;
    QTest::newRow("deprecatedFunction")
            << QStringLiteral("deprecatedFunction.qml")
            << QStringLiteral("Method \"deprecated(foobar)\" is deprecated (Reason: No particular "
                              "reason.)")
            << QString() << QString() << false;
    QTest::newRow("deprecatedFunctionInherited")
            << QStringLiteral("deprecatedFunctionInherited.qml")
            << QStringLiteral("Method \"deprecatedInherited(c, d)\" is deprecated (Reason: This "
                              "deprecation should be visible!)")
            << QString() << QString() << false;

    QTest::newRow("string as id") << QStringLiteral("stringAsId.qml")
                                  << QStringLiteral("ids do not need quotation marks") << QString()
                                  << QString() << false;
    QTest::newRow("stringIdUsedInWarning") << QStringLiteral("stringIdUsedInWarning.qml")
                                           << QStringLiteral("i is a member of a parent element")
                                           << QString() << QStringLiteral("stringy.") << false;
    QTest::newRow("Invalid id (expression)")
            << QStringLiteral("invalidId1.qml") << QStringLiteral("Failed to parse id") << QString()
            << QString() << false;
    QTest::newRow("multilineString")
            << QStringLiteral("multilineString.qml")
            << QStringLiteral("String contains unescaped line terminator which is deprecated. Use "
                              "a template literal instead.")
            << QString() << QString() << true;
    QTest::newRow("unresolvedType")
            << QStringLiteral("unresolvedType.qml")
            << QStringLiteral("UnresolvedType was not found. Did you add all import paths?")
            << QStringLiteral("incompatible type") << QString() << false;
    QTest::newRow("invalidInterceptor")
            << QStringLiteral("invalidInterceptor.qml")
            << QStringLiteral("On-binding for property \"angle\" has wrong type \"Item\"")
            << QString() << QString() << false;
    QTest::newRow("2Interceptors") << QStringLiteral("2interceptors.qml")
                                   << QStringLiteral("Duplicate interceptor on property \"x\"")
                                   << QString() << QString() << false;
    QTest::newRow("ValueSource+2Interceptors")
            << QStringLiteral("valueSourceBetween2interceptors.qml")
            << QStringLiteral("Duplicate interceptor on property \"x\"") << QString() << QString()
            << false;
    QTest::newRow("2ValueSources") << QStringLiteral("2valueSources.qml")
                                   << QStringLiteral("Duplicate value source on property \"x\"")
                                   << QString() << QString() << false;
    QTest::newRow("ValueSource+Value")
            << QStringLiteral("valueSource_Value.qml")
            << QStringLiteral("Cannot combine value source and binding on property \"obj\"")
            << QString() << QString() << false;
    QTest::newRow("ValueSource+ListValue")
            << QStringLiteral("valueSource_listValue.qml")
            << QStringLiteral("Cannot combine value source and binding on property \"objs\"")
            << QString() << QString() << false;
    QTest::newRow("NonExistentListProperty")
            << QStringLiteral("nonExistentListProperty.qml")
            << QStringLiteral("Property \"objs\" is invalid or does not exist") << QString()
            << QString() << false;
    QTest::newRow("QtQuick.Window 2.0")
            << QStringLiteral("qtquickWindow20.qml")
            << QStringLiteral("Property \"window\" not found on type \"QQuickWindow\"") << QString()
            << QString() << false;
    QTest::newRow("unresolvedAttachedType")
            << QStringLiteral("unresolvedAttachedType.qml")
            << QStringLiteral("unknown attached property scope UnresolvedAttachedType.")
            << QStringLiteral("Property \"property\" is invalid or does not exist") << QString()
            << false;
    QTest::newRow("nestedInlineComponents")
            << QStringLiteral("nestedInlineComponents.qml")
            << QStringLiteral("Nested inline components are not supported") << QString()
            << QString() << false;
    QTest::newRow("WithStatement") << QStringLiteral("WithStatement.qml")
                                   << QStringLiteral("with statements are strongly discouraged")
                                   << QString() << QString() << false;
    QTest::newRow("BindingTypeMismatch")
            << QStringLiteral("bindingTypeMismatch.qml")
            << QStringLiteral("Cannot assign binding of type QString to int") << QString()
            << QString() << false;
    QTest::newRow("BindingTypeMismatchFunction")
            << QStringLiteral("bindingTypeMismatchFunction.qml")
            << QStringLiteral("Cannot assign binding of type QString to int") << QString()
            << QString() << false;
    QTest::newRow("BadLiteralBinding")
            << QStringLiteral("badLiteralBinding.qml")
            << QStringLiteral("Cannot assign binding of type string to int") << QString()
            << QString() << false;
    QTest::newRow("BadLiteralBindingDate")
            << QStringLiteral("badLiteralBindingDate.qml")
            << QStringLiteral("Cannot assign binding of type QString to QDateTime") << QString()
            << QString() << false;
    QTest::newRow("BadModulePrefix")
            << QStringLiteral("badModulePrefix.qml")
            << QStringLiteral("Cannot load singleton as property of object") << QString()
            << QString() << false;
    QTest::newRow("BadModulePrefix2")
            << QStringLiteral("badModulePrefix2.qml")
            << QStringLiteral("Cannot use non-reference type QRectF as base "
                              "of namespaced attached type")
            << QString() << QString() << false;
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty.qml")
            << QStringLiteral("Cannot assign to read-only property activeFocus") << QString()
            << QString() << false;
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty2.qml")
            << QStringLiteral("Cannot assign to read-only property activeFocus") << QString()
            << QString() << false;
    QTest::newRow("DeferredPropertyID")
            << QStringLiteral("deferredPropertyID.qml")
            << QStringLiteral(
                       "Cannot defer property assignment to "
                       "\"contentData\". Assigning an id to an object or one of its sub-objects "
                       "bound to a deferred property will make the assignment immediate.")
            << QString() << QString() << false;
    QTest::newRow("DeferredPropertyNestedID")
            << QStringLiteral("deferredPropertyNestedID.qml")
            << QStringLiteral(
                       "Cannot defer property assignment to "
                       "\"contentData\". Assigning an id to an object or one of its sub-objects "
                       "bound to a deferred property will make the assignment immediate.")
            << QString() << QString() << false;
    QTest::newRow("cachedDependency")
            << QStringLiteral("cachedDependency.qml") << QStringLiteral("Unused import at %1:1:1")
            << QStringLiteral("Cannot assign binding of type QQuickItem to QObject") << QString()
            << true;
    QTest::newRow("cycle in import")
            << QStringLiteral("cycleHead.qml")
            << QStringLiteral("MenuItem is part of an inheritance cycle: MenuItem -> MenuItem")
            << QString() << QString() << false;
    QTest::newRow("badGeneralizedGroup1")
            << QStringLiteral("badGeneralizedGroup1.qml")
            << QStringLiteral("Binding assigned to \"aaaa\", "
                              "but no property \"aaaa\" exists in the current element")
            << QString() << QString() << false;
    QTest::newRow("badGeneralizedGroup2") << QStringLiteral("badGeneralizedGroup2.qml")
                                          << QStringLiteral("unknown grouped property scope aself")
                                          << QString() << QString() << false;
    QTest::newRow("missingQmltypes")
            << QStringLiteral("missingQmltypes.qml")
            << QStringLiteral("QML types file does not exist") << QString() << QString() << false;
    QTest::newRow("enumInvalid") << QStringLiteral(
            "enumInvalid.qml") << QStringLiteral("Property \"red\" not found on type \"QtObject\"")
                                 << QString() << QString() << false;
    QTest::newRow("inaccessibleId")
            << QStringLiteral("inaccessibleId.qml")
            << QStringLiteral("Property \"objectName\" not found on type \"int\"") << QString()
            << QString() << false;
    QTest::newRow("inaccessibleId2")
            << QStringLiteral("inaccessibleId2.qml")
            << QStringLiteral("Property \"objectName\" not found on type \"int\"") << QString()
            << QString() << false;
    QTest::newRow("unknownTypeCustomParser")
            << QStringLiteral("unknownTypeCustomParser.qml")
            << QStringLiteral("TypeDoesNotExist was not found.") << QString() << QString() << false;
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(QString, warningMessage);
    QFETCH(QString, notContained);
    QFETCH(QString, replacement);
    QFETCH(bool, exitsNormally);

    if (warningMessage.contains(QLatin1String("%1")))
        warningMessage = warningMessage.arg(testFile(filename));

    QJsonArray warnings;

    QEXPECT_FAIL("attachedPropertyAccess", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("attachedPropertyNested", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("BadLiteralBindingDate",
                 "We're currently not able to verify any non-trivial QString conversion that "
                 "requires QQmlStringConverters",
                 Abort);
    QEXPECT_FAIL("bad tranlsation binding (qsTr)", "We currently do not check translation binding",
                 Abort);

    callQmllint(filename, exitsNormally, &warnings);

    if (!warningMessage.isEmpty()) {
        // output.contains() expect fails:
        QEXPECT_FAIL("BadLiteralBindingDate",
                     "We're currently not able to verify any non-trivial QString conversion that "
                     "requires QQmlStringConverters",
                     Abort);

        searchWarnings(warnings, warningMessage, filename);
    }

    if (!notContained.isEmpty()) {
        // !output.contains() expect fails:
        QEXPECT_FAIL("badAttachedPropertyNested", "We cannot discern between types and instances",
                     Abort);

        searchWarnings(warnings, notContained, filename, StringNotContained);
    }

    if (!replacement.isEmpty())
        searchWarnings(warnings, replacement, filename, StringContained, DoReplacementSearch);
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
    QTest::newRow("optionalImport") << QStringLiteral("optionalImport.qml");
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
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);

    QJsonArray warnings;

    callQmllint(filename, true, &warnings);
    QVERIFY2(warnings.isEmpty(), qPrintable(QJsonDocument(warnings).toJson()));
}

QString TestQmllint::runQmllint(const QString &fileToLint,
                                std::function<void(QProcess &)> handleResult,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addIncludeDirs)
{
    auto qmlImportDir = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QStringList args;

    args << (QFileInfo(fileToLint).isAbsolute() ? fileToLint : testFile(fileToLint));

    if (addIncludeDirs) {
        args << QStringLiteral("-I") << qmlImportDir
             << QStringLiteral("-I") << dataDirectory();
    }

    if (ignoreSettings)
        QStringLiteral("--ignore-settings");

    args << extraArgs;
    args << QStringLiteral("--silent");
    QString errors;
    auto verify = [&](bool isSilent) {
        QProcess process;
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
                                bool addIncludeDirs)
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
            extraArgs, ignoreSettings, addIncludeDirs);
}

void TestQmllint::callQmllint(const QString &fileToLint, bool shouldSucceed, QJsonArray *warnings,
                              QStringList includeDirs, QStringList qmltypesFiles,
                              QStringList resources, DefaultIncludeOption defaultIncludes)
{
    QJsonArray jsonOutput;

    bool success = m_linter.lintFile(
            QFileInfo(fileToLint).isAbsolute() ? fileToLint : testFile(fileToLint), nullptr, true,
            warnings ? &jsonOutput : nullptr,
            defaultIncludes == UseDefaultIncludes ? m_defaultImportPaths + includeDirs
                                                  : includeDirs,
            qmltypesFiles, resources, {});
    QVERIFY2(success == shouldSucceed, QJsonDocument(jsonOutput).toJson());

    if (warnings) {
        QVERIFY2(jsonOutput.size() == 1, QJsonDocument(jsonOutput).toJson());
        *warnings = jsonOutput.at(0)[u"warnings"_qs].toArray();
    }

    QCOMPARE(success, shouldSucceed);
}

void TestQmllint::searchWarnings(const QJsonArray &warnings, const QString &substring,
                                 const QString &filename, ContainOption shouldContain,
                                 ReplacementOption searchReplacements)
{
    bool contains = false;

    auto toDisplayWarningType = [](const QString &warningType) {
        return warningType.first(1).toUpper() + warningType.mid(1);
    };

    for (const QJsonValue &warning : warnings) {
        // We're currently recreating the output that the logger would write to the console,
        // so we can keep using our existing test data without modification.
        // In the future our test data should be replaced with a structured representation of the
        // warning it expects, possibly as a QQmlJS::DiagnosticMessage.
        const QString warningMessage =
                u"%1: %2:%3 %4"_qs
                        .arg(toDisplayWarningType(warning[u"type"].toString()), testFile(filename))
                        .arg(warning[u"line"].isUndefined()
                                     ? u""_qs
                                     : u"%1:%2:"_qs.arg(warning[u"line"].toInt())
                                               .arg(warning[u"column"].toInt()))
                        .arg(warning[u"message"].toString());
        if (warningMessage.contains(substring)) {
            contains = true;
            break;
        }

        for (const QJsonValue &fix : warning[u"suggestions"].toArray()) {
            const QString jsonFixMessage = u"Info: "_qs + fix[u"message"].toString();
            if (jsonFixMessage.contains(substring)) {
                contains = true;
                break;
            }
            if (searchReplacements == DoReplacementSearch
                && fix[u"replacement"].toString().contains(substring)) {
                contains = true;
                break;
            }
        }
    }

    const auto toDescription = [](const QJsonArray &warnings, const QString &substring,
                                  bool must = true) {
        // Note: this actually produces a very poorly formatted multi-line
        // description, but this is how we also do it in cleanQmlCode test case,
        // so this should suffice. in any case this mainly aids the debugging
        // and CI stays (or should stay) clean.
        return QStringLiteral("qmllint output '%1' %2 contain '%3'")
                .arg(QString::fromUtf8(QJsonDocument(warnings).toJson(QJsonDocument::Compact)),
                     must ? u"must" : u"must NOT", substring);
    };

    if (shouldContain == StringContained)
        QVERIFY2(contains, qPrintable(toDescription(warnings, substring)));
    else
        QVERIFY2(!contains, qPrintable(toDescription(warnings, substring, false)));
}

void TestQmllint::requiredProperty()
{
    QVERIFY(runQmllint("requiredProperty.qml", true).isEmpty());

    {
        QJsonArray warnings;
        callQmllint("requiredMissingProperty.qml", false, &warnings);
        searchWarnings(
                warnings,
                QStringLiteral("Property \"foo\" was marked as required but does not exist."),
                "requiredMissingProperty.qml");
    }

    QVERIFY(runQmllint("requiredPropertyBindings.qml", true).isEmpty());

    {
        QJsonArray warnings;
        callQmllint("requiredPropertyBindingsNow.qml", false, &warnings);

        searchWarnings(
                warnings,
                QStringLiteral(
                        "Component is missing required property required_now_string from Base"),
                "requiredPropertyBindingsNow.qml");
        searchWarnings(warnings,
                       QStringLiteral("Component is missing required property "
                                      "required_defined_here_string from here"),
                       "requiredPropertyBindingsNow.qml");
    }

    {
        QJsonArray warnings;
        callQmllint("requiredPropertyBindingsLater.qml", false, &warnings);
        searchWarnings(
                warnings,
                QStringLiteral("Component is missing required property required_later_string from "
                               "Base (marked as required by Derived)"),
                "requiredPropertyBindingsLater.qml");
        searchWarnings(
                warnings,
                QStringLiteral("Component is missing required property required_even_later_string "
                               "from Base (marked as required by here)"),
                "requiredPropertyBindingsLater.qml");
    }
}

void TestQmllint::settingsFile()
{
    QVERIFY(runQmllint("settings/unqualifiedSilent/unqualified.qml", true, QStringList(), false)
                    .isEmpty());
    QVERIFY(runQmllint("settings/unusedImportWarning/unused.qml", false, QStringList(), false)
                    .contains(QStringLiteral("Info: %1:2:1: Unused import at %1:2:1")
                                      .arg(testFile("settings/unusedImportWarning/unused.qml"))));
    QVERIFY(runQmllint("settings/bare/bare.qml", false, { "--bare" }, false, false)
                    .contains(QStringLiteral("Failed to find the following builtins: "
                                             "builtins.qmltypes, jsroot.qmltypes")));
    QVERIFY(runQmllint("settings/qmltypes/qmltypes.qml", false, QStringList(), false)
                    .contains(QStringLiteral("not a qmldir file. Assuming qmltypes.")));
}

void TestQmllint::additionalImplicitImport()
{
    QJsonArray warnings;
    callQmllint("additionalImplicitImport.qml", true, &warnings, {}, {},
                { testFile("implicitImportResource.qrc") });
    QVERIFY(warnings.isEmpty());
}

void TestQmllint::listIndices()
{
    QVERIFY(runQmllint("listIndices.qml", true, {"--compiler=warning"}, false).isEmpty());
}

void TestQmllint::lazyAndDirect()
{
    QVERIFY(runQmllint("LazyAndDirect/Lazy.qml", true, {"--compiler=warning"}, false).isEmpty());
}

void TestQmllint::functionDeclaration()
{
    QVERIFY(runQmllint("functionDeclarations.qml", false, { "--controls-sanity=warning" }, false)
                    .contains(u"Declared function \"add\""_qs));
}

void TestQmllint::signalHandler()
{
    QVERIFY(runQmllint("signalHandlers.qml", false, { "--controls-sanity=warning" }, false)
                    .contains(u"Declared signal handler \"onCompleted\""_qs));
}

void TestQmllint::anchors()
{
    QVERIFY(runQmllint("anchors.qml", false, { "--controls-sanity=warning" }, false)
                    .contains(u"Using anchors here"_qs));
}

void TestQmllint::attachedPropertyReuse()
{
    QVERIFY(runQmllint("attachedPropNotReused.qml", false,
                       QStringList() << "--multiple-attached-objects"
                                     << "warning",
                       false)
                    .contains(QStringLiteral("Using attached type QQuickKeyNavigationAttached "
                                             "already initialized in a parent "
                                             "scope")));
    QVERIFY(runQmllint("attachedPropEnum.qml", true,
                       QStringList() << "--multiple-attached-objects"
                                     << "warning")
                    .isEmpty());
}

void TestQmllint::shadowable()
{
    QVERIFY(runQmllint("shadowable.qml", false, {"--compiler=warning"}, false).contains(
            QStringLiteral("with type NotSoSimple can be shadowed")));
}

void TestQmllint::tooFewParameters()
{
    QVERIFY(runQmllint("tooFewParams.qml", false, {"--compiler=warning"}, false).contains(
            QStringLiteral("No matching override found")));
}

void TestQmllint::qQmlV4Function()
{
    QVERIFY(runQmllint("varargs.qml", true, {"--compiler=warning"}, false).isEmpty());
}

void TestQmllint::missingBuiltinsNoCrash()
{
    QVERIFY(runQmllint("missingBuiltinsNoCrash.qml", false, { "--bare" }, false, false)
                    .contains(QStringLiteral("Failed to find the following builtins: "
                                             "builtins.qmltypes, jsroot.qmltypes")));
}

void TestQmllint::absolutePath()
{
    const QString absolutePath = QFileInfo(testFile("memberNotFound.qml")).absoluteFilePath();
    QVERIFY(runQmllint(absolutePath, false, { "--absolute-path" }).contains(absolutePath));
}

void TestQmllint::multiGrouped()
{
    QVERIFY(runQmllint("multiGrouped.qml", true, {"--compiler=warning"}).isEmpty());
}

void TestQmllint::javascriptVariableArgs()
{
    QVERIFY(runQmllint("javascriptVariableArgs.qml", false, { "--compiler", "warning" })
            .contains(QStringLiteral("Function expects 0 arguments, but 2 were provided")));
}

QTEST_MAIN(TestQmllint)
#include "tst_qmllint.moc"
