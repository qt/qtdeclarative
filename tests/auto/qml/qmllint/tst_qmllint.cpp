/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <util.h>

class TestQmllint: public QQmlDataTest
{
    Q_OBJECT

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

#ifdef QT_QMLJSROOTGEN_PRESENT
    void verifyJsRoot();
#endif

    void autoqmltypes();
    void resources();

    void requiredProperty();

private:
    QString runQmllint(const QString &fileToLint,
                       std::function<void(QProcess &)> handleResult,
                       const QStringList &extraArgs = QStringList());
    QString runQmllint(const QString &fileToLint, bool shouldSucceed,
                       const QStringList &extraArgs = QStringList());

    QString m_qmllintPath;
    QString m_qmljsrootgenPath;
    QString m_qmltyperegistrarPath;
};

void TestQmllint::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmllintPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmllint");
    m_qmljsrootgenPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmljsrootgen");
    m_qmltyperegistrarPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmltyperegistrar");
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
    QVERIFY(output.contains(QString::asprintf("Warning: unqualified access at %s:%d:%d", testFile(filename).toUtf8().constData(), warningLine, warningColumn)));
    QVERIFY(output.contains(warningMessage));
}

void TestQmllint::testUnqualified_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("warningMessage");
    QTest::addColumn<int>("warningLine");
    QTest::addColumn<int>("warningColumn");

    // check for false positive due to and warning about with statement
    QTest::newRow("WithStatement") << QStringLiteral("WithStatement.qml") << QStringLiteral("with statements are strongly discouraged") << 10 << 25;
    // id from nowhere (as with setContextProperty)
    QTest::newRow("IdFromOuterSpaceDirect") << QStringLiteral("IdFromOuterSpace.qml") << "alien.x" << 4 << 8;
    QTest::newRow("IdFromOuterSpaceAccess") << QStringLiteral("IdFromOuterSpace.qml") << "console.log(alien)" << 7 << 21;
    // access property of root object
    QTest::newRow("FromRootDirect") << QStringLiteral("FromRoot.qml") << QStringLiteral("x: root.unqualified") << 9 << 16; // new property
    QTest::newRow("FromRootAccess") << QStringLiteral("FromRoot.qml") << QStringLiteral("property int check: root.x") << 13 << 33;  // builtin property
    // access injected name from signal
    QTest::newRow("SignalHandler1") << QStringLiteral("SignalHandler.qml") << QStringLiteral("onDoubleClicked:  function(mouse) {...") << 5 << 21;
    QTest::newRow("SignalHandler2") << QStringLiteral("SignalHandler.qml") << QStringLiteral("onPositionChanged:  function(mouse) {...") << 10 << 21;
    QTest::newRow("SignalHandlerShort1") << QStringLiteral("SignalHandler.qml") << QStringLiteral("onClicked:  (mouse) =>  {...") << 8 << 29;
    QTest::newRow("SignalHandlerShort2") << QStringLiteral("SignalHandler.qml") << QStringLiteral("onPressAndHold:  (mouse) =>  {...") << 12 << 34;
    // access catch identifier outside catch block
    QTest::newRow("CatchStatement") << QStringLiteral("CatchStatement.qml") << QStringLiteral("err") << 6 << 21;

    QTest::newRow("NonSpuriousParent") << QStringLiteral("nonSpuriousParentWarning.qml") << QStringLiteral("property int x: <id>.parent.x") << 6 << 25;

    QTest::newRow("crashConnections")
        << QStringLiteral("crashConnections.qml")
        << QStringLiteral("target: FirstRunDialog") << 4 << 13;
}

void TestQmllint::testUnknownCausesFail()
{
    const QString unknownNotFound = runQmllint("unknownElement.qml", false);
    QVERIFY(unknownNotFound.contains(
                QStringLiteral("Warning: Unknown was not found. Did you add all import paths?")));
}

void TestQmllint::directoryPassedAsQmlTypesFile()
{
    const QStringList iArg = QStringList() << QStringLiteral("-i") << dataDirectory();
    const QString errorMessages = runQmllint("unknownElement.qml", false, iArg);
    const QString expectedError = QStringLiteral("Warning: QML types file cannot be a directory: ") + dataDirectory();
    QVERIFY2(errorMessages.contains(expectedError), qPrintable(QString::fromLatin1(
        "Expected error to contain \"%1\", but it didn't: %2").arg(expectedError, errorMessages)));
}

void TestQmllint::oldQmltypes()
{
    const QString errors = runQmllint("oldQmltypes.qml", false);
    QVERIFY(errors.contains(QStringLiteral("Warning: typeinfo not declared in qmldir file")));
    QVERIFY(!errors.contains(QStringLiteral("Warning: QQuickItem was not found. Did you add all import paths?")));
    QVERIFY(errors.contains(QStringLiteral("Warning: Found deprecated dependency specifications")));

    // Checking for both lines separately so that we don't have to mess with the line endings.b
    QVERIFY(errors.contains(QStringLiteral("Meta object revision and export version differ, ignoring the revision.")));
    QVERIFY(errors.contains(QStringLiteral("Revision 0 corresponds to version 0.0; it should be 1.0.")));
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
    runQmllint(file, true);
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
    jsrootProcess.setWorkingDirectory(dir.path());
    jsrootProcess.start(m_qmljsrootgenPath, {"jsroot.json"});

    jsrootProcess.waitForFinished();

    QCOMPARE(jsrootProcess.exitStatus(), QProcess::NormalExit);
    QCOMPARE(jsrootProcess.exitCode(), 0);


    QProcess typeregistrarProcess;
    typeregistrarProcess.setWorkingDirectory(dir.path());
    typeregistrarProcess.start(m_qmltyperegistrarPath, {"jsroot.json", "--generate-qmltypes", "jsroot.qmltypes", "--import-name", "QJSEngine", "--major-version", "1", "--minor-version", "0"});

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
    // qmltyperegistrar jsroot.json --generate-qmltypes src/imports/builtins/jsroot.qmltypes --import-name QJSEngine --major-version 1 --minor-version 0
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
    QCOMPARE(process.exitCode(), 0);

    QVERIFY(process.readAllStandardError().isEmpty());
    QVERIFY(process.readAllStandardOutput().isEmpty());
}

void TestQmllint::resources()
{
    runQmllint(testFile("resource.qml"), true,
               {QStringLiteral("--resource"), testFile("resource.qrc")});
    runQmllint(testFile("badResource.qml"), false,
               {QStringLiteral("--resource"), testFile("resource.qrc")});
    runQmllint(testFile("resource.qml"), false, {});
    runQmllint(testFile("badResource.qml"), true, {});
}

void TestQmllint::dirtyQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("warningMessage");
    QTest::addColumn<QString>("notContained");
    QTest::addColumn<bool>("exitsNormally");

    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml")
            << QStringLiteral("%1:4 : Expected token `:'")
            << QString()
            << false;
    QTest::newRow("Invalid_syntax_JS")
            << QStringLiteral("failure1.js")
            << QStringLiteral("%1:4 : Expected token `;'")
            << QString()
            << false;
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << QString("Warning: unqualified access at %1:12:36")
            << QStringLiteral("no matching signal found")
            << false;
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << QString("Warning: Property \"foo\" not found on type \"QtObject\" at %1:6:31")
            << QString()
            << false;
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << QString("Warning: Property \"foo2\" not found on type \"Methods\" at %1:5:25")
            << QString()
            << false;
    QTest::newRow("badAlias1")
            << QStringLiteral("badAlias.qml")
            << QString("Warning: 3:1: Cannot deduce type of alias \"wrong\"")
            << QString()
            << false;
    QTest::newRow("badAlias2")
            << QStringLiteral("badAlias.qml")
            << QString("Warning: unqualified access at %1:4:27")
            << QString()
            << false;
    QTest::newRow("badAliasProperty1")
            << QStringLiteral("badAliasProperty.qml")
            << QString("Warning: 3:1: Cannot deduce type of alias \"wrong\"")
            << QString()
            << false;
    QTest::newRow("badAliasProperty2")
            << QStringLiteral("badAliasProperty.qml")
            << QString("Warning: Property \"nowhere\" not found on type \"QtObject\" at %1:5:32")
            << QString()
            << false;
    QTest::newRow("badAliasExpression")
            << QStringLiteral("badAliasExpression.qml")
            << QString("Warning: 5:26: Invalid alias expression. Only IDs and field member "
                       "expressions can be aliased")
            << QString()
            << false;
    QTest::newRow("aliasCycle1")
            << QStringLiteral("aliasCycle.qml")
            << QString("Warning: 3:1: Alias \"b\" is part of an alias cycle")
            << QString()
            << false;
    QTest::newRow("aliasCycle2")
            << QStringLiteral("aliasCycle.qml")
            << QString("Warning: 3:1: Alias \"a\" is part of an alias cycle")
            << QString()
            << false;
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << QString("Warning: Property \"rrr\" not found on type \"Item\" at %1:5:34")
            << QString()
            << false;
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << QString("Warning: Property \"progress\" not found on type \"QQuickItem\" at %1:7:39")
            << QString()
            << false;
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << QString("Warning: Property \"rrr\" not found on type \"Item\" at %1:5:39")
            << QString()
            << false;
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << QString("Warning: Type \"QPalette\" of base \"palette\" not found when accessing member \"weDontKnowIt\" at %1:5:34")
            << QString()
            << false;
    QTest::newRow("inheritanceCylce")
            << QStringLiteral("Cycle1.qml")
            << QString("Warning: Cycle2 is part of an inheritance cycle: Cycle2 -> Cycle3 -> Cycle1 -> Cycle2")
            << QString()
            << false;
    QTest::newRow("badQmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/bad.qml")
            << QString("Warning: Item was not found. Did you add all import paths?")
            << QString()
            << false;
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleBad.qml")
            << QString("Warning: Property \"unknownFunc\" not found on type \"Foo\"")
            << QString()
            << false;
    QTest::newRow("badEnumFromQtQml")
            << QStringLiteral("badEnumFromQtQml.qml")
            << QString("Warning: Property \"Linear123\" not found on type \"QQmlEasingEnums\"")
            << QString()
            << false;
    QTest::newRow("anchors3")
            << QStringLiteral("anchors3.qml")
            << QString()
            << QString()
            << false;
    QTest::newRow("nanchors1")
            << QStringLiteral("nanchors1.qml")
            << QString()
            << QString()
            << false;
    QTest::newRow("nanchors2")
            << QStringLiteral("nanchors2.qml")
            << QString("unknown grouped property scope nanchors.")
            << QString()
            << false;
    QTest::newRow("nanchors3")
            << QStringLiteral("nanchors3.qml")
            << QString("unknown grouped property scope nanchors.")
            << QString()
            << false;
    QTest::newRow("badAliasObject")
            << QStringLiteral("badAliasObject.qml")
            << QString("Warning: Property \"wrongwrongwrong\" not found on type \"QtObject\"")
            << QString()
            << false;
    QTest::newRow("badScript")
            << QStringLiteral("badScript.qml")
            << QString("Warning: Property \"stuff\" not found on type \"Empty\"")
            << QString()
            << false;
    QTest::newRow("brokenNamespace")
            << QStringLiteral("brokenNamespace.qml")
            << QString("Warning: type not found in namespace at %1:4:17")
            << QString()
            << false;
    QTest::newRow("segFault (bad)")
            << QStringLiteral("SegFault.bad.qml")
            << QStringLiteral("Property \"foobar\" not found on type \"QQuickScreenAttached\"")
            << QString()
            << false;
    QTest::newRow("VariableUsedBeforeDeclaration")
            << QStringLiteral("useBeforeDeclaration.qml")
            << QStringLiteral("Variable \"argq\" is used before its declaration at 5:9. "
                              "The declaration is at 6:13.")
            << QString()
            << false;
    QTest::newRow("SignalParameterMismatch")
            << QStringLiteral("namedSignalParameters.qml")
            << QStringLiteral("Parameter 1 to signal handler for \"onSig\" is called \"argarg\". "
                              "The signal has a parameter of the same name in position 2.")
            << QStringLiteral("onSig2")
            << false;
    QTest::newRow("TooManySignalParameters")
            << QStringLiteral("tooManySignalParameters.qml")
            << QStringLiteral("Signal handler for \"onSig\" has more formal parameters "
                              "than the signal it handles.")
            << QString()
            << false;
    QTest::newRow("OnAssignment")
            << QStringLiteral("onAssignment.qml")
            << QStringLiteral("Property \"loops\" not found on type \"bool\"")
            << QString()
            << false;
    QTest::newRow("BadAttached")
            << QStringLiteral("badAttached.qml")
            << QStringLiteral("unknown attached property scope WrongAttached.")
            << QString()
            << false;
    QTest::newRow("BadBinding")
            << QStringLiteral("badBinding.qml")
            << QStringLiteral("Binding assigned to \"doesNotExist\", but no property "
                              "\"doesNotExist\" exists in the current element.")
            << QString()
            << false;
    QTest::newRow("BadPropertyType")
            << QStringLiteral("badPropertyType.qml")
            << QStringLiteral("No type found for property \"bad\". This may be due to a missing "
                              "import statement or incomplete qmltypes files.")
            << QString()
            << false;
    QTest::newRow("Deprecation (Property, with reason)")
            << QStringLiteral("deprecatedPropertyReason.qml")
            << QStringLiteral("Property \"deprecated\" is deprecated (Reason: Test)")
            << QString()
            << false;
    QTest::newRow("Deprecation (Property, no reason)")
            << QStringLiteral("deprecatedProperty.qml")
            << QStringLiteral("Property \"deprecated\" is deprecated")
            << QString()
            << false;
    QTest::newRow("Deprecation (Type, with reason)")
            << QStringLiteral("deprecatedTypeReason.qml")
            << QStringLiteral("Type \"TypeDeprecatedReason\" is deprecated (Reason: Test)")
            << QString()
            << false;
    QTest::newRow("Deprecation (Type, no reason)")
            << QStringLiteral("deprecatedType.qml")
            << QStringLiteral("Type \"TypeDeprecated\" is deprecated")
            << QString()
            << false;
    QTest::newRow("MissingDefaultProperty")
            << QStringLiteral("defaultPropertyWithoutKeyword.qml")
            << QStringLiteral("Cannot assign to non-existent default property") << QString() << false;

    QTest::newRow("DoubleAssignToDefaultProperty")
            << QStringLiteral("defaultPropertyWithDoubleAssignment.qml")
            << QStringLiteral("Cannot assign multiple objects to a default non-list property")
            << QString()
            << false;
    QTest::newRow("DefaultPropertyWithWrongType(string)")
            << QStringLiteral("defaultPropertyWithWrongType.qml")
            << QStringLiteral("Cannot assign to default property of incompatible type")
            << QStringLiteral("Cannot assign to non-existent default property")
            << false;
    QTest::newRow("DefaultPropertyWithWrongType(var)")
            << QStringLiteral("defaultPropertyWithWrongType2.qml")
            << QStringLiteral("Cannot assign to default property of incompatible type")
            << QStringLiteral("Cannot assign to non-existent default property")
            << false;
    QTest::newRow("InvalidImport")
            << QStringLiteral("invalidImport.qml")
            << QStringLiteral("Failed to import FooBar. Are your include paths set up properly?")
            << QString()
            << false;
    QTest::newRow("Unused Import (simple)")
            << QStringLiteral("unused_simple.qml")
            << QStringLiteral("Unused import at %1:1:1")
            << QString()
            << true;
    QTest::newRow("Unused Import (prefix)")
            << QStringLiteral("unused_prefix.qml")
            << QStringLiteral("Unused import at %1:1:1")
            << QString()
            << true;
    QTest::newRow("TypePropertAccess")
            << QStringLiteral("typePropertyAccess.qml")
            << QString()
            << QString()
            << false;
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(QString, warningMessage);
    QFETCH(QString, notContained);
    QFETCH(bool, exitsNormally);

    if (warningMessage.contains(QLatin1String("%1")))
        warningMessage = warningMessage.arg(testFile(filename));

    const QString output = runQmllint(filename, [&](QProcess &process) {
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QEXPECT_FAIL("anchors3", "We don't see that QQuickItem cannot be assigned to QQuickAnchorLine", Abort);
        QEXPECT_FAIL("nanchors1", "Invalid grouped properties are not always detected", Abort);
        QEXPECT_FAIL("TypePropertAccess", "We cannot discern between types and instances", Abort);

        if (exitsNormally)
            QVERIFY(process.exitCode() == 0);
        else
            QVERIFY(process.exitCode() != 0);
    });

    QVERIFY(output.contains(warningMessage));
    if (!notContained.isEmpty())
        QVERIFY(!output.contains(notContained));
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
    QTest::newRow("duplicateQmldirImport") << QStringLiteral("qmldirImport/duplicate.qml");
    QTest::newRow("Used imports") << QStringLiteral("used.qml");
    QTest::newRow("Unused imports (multi)") << QStringLiteral("unused_multi.qml");
    QTest::newRow("compositeSingleton") << QStringLiteral("compositesingleton.qml");
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);
    const QString warnings = runQmllint(filename, true);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings));
}

QString TestQmllint::runQmllint(const QString &fileToLint,
                                std::function<void(QProcess &)> handleResult,
                                const QStringList &extraArgs)
{
    auto qmlImportDir = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QStringList args;

    args << (QFileInfo(fileToLint).isAbsolute() ? fileToLint : testFile(fileToLint))
         << QStringLiteral("-I") << qmlImportDir
         << QStringLiteral("-I") << dataDirectory();
    args << extraArgs;
    args << QStringLiteral("--silent");
    QString errors;
    auto verify = [&](bool isSilent) {
        QProcess process;
        process.start(m_qmllintPath, args);
        handleResult(process);
        errors = process.readAllStandardError();

        if (isSilent)
            QVERIFY(errors.isEmpty());

        if (QTest::currentTestFailed()) {
            qDebug() << "Command:" << process.program() << args.join(u' ');
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

QString TestQmllint::runQmllint(const QString &fileToLint, bool shouldSucceed, const QStringList &extraArgs)
{
    return runQmllint(fileToLint, [&](QProcess &process) {
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);

        if (shouldSucceed)
            QCOMPARE(process.exitCode(), 0);
        else
            QVERIFY(process.exitCode() != 0);
    }, extraArgs);
}

void TestQmllint::requiredProperty()
{
    QVERIFY(runQmllint("requiredProperty.qml", true).isEmpty());

    const QString errors = runQmllint("requiredMissingProperty.qml", false);
    QVERIFY(errors.contains(QStringLiteral("Property \"foo\" was marked as required but does not exist.")));
}


QTEST_MAIN(TestQmllint)
#include "tst_qmllint.moc"
