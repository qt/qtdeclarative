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
};

void TestQmllint::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmllintPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmllint");
#ifdef Q_OS_WIN
    m_qmllintPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmllintPath).exists()) {
        QString message = QStringLiteral("qmllint executable not found (looked for %0)").arg(m_qmllintPath);
        QFAIL(qPrintable(message));
    }
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

    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml")
            << QStringLiteral("%1:4 : Expected token `:'")
            << QString();
    QTest::newRow("Invalid_syntax_JS")
            << QStringLiteral("failure1.js")
            << QStringLiteral("%1:4 : Expected token `;'")
            << QString();
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << QString("Warning: unqualified access at %1:12:36")
            << QStringLiteral("no matching signal found");
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << QString("Warning: Property \"foo\" not found on type \"QtObject\" at %1:6:31")
            << QString();
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << QString("Warning: Property \"foo2\" not found on type \"Methods\" at %1:5:25")
            << QString();
    QTest::newRow("badAlias1")
            << QStringLiteral("badAlias.qml")
            << QString("Warning: 3:1: Cannot deduce type of alias \"wrong\"")
            << QString();
    QTest::newRow("badAlias2")
            << QStringLiteral("badAlias.qml")
            << QString("Warning: unqualified access at %1:4:27")
            << QString();
    QTest::newRow("badAliasProperty1")
            << QStringLiteral("badAliasProperty.qml")
            << QString("Warning: 3:1: Cannot deduce type of alias \"wrong\"")
            << QString();
    QTest::newRow("badAliasProperty2")
            << QStringLiteral("badAliasProperty.qml")
            << QString("Warning: Property \"nowhere\" not found on type \"QtObject\" at %1:5:32")
            << QString();
    QTest::newRow("badAliasExpression")
            << QStringLiteral("badAliasExpression.qml")
            << QString("Warning: 5:26: Invalid alias expression. Only IDs and field member "
                       "expressions can be aliased")
            << QString();
    QTest::newRow("aliasCycle1")
            << QStringLiteral("aliasCycle.qml")
            << QString("Warning: 3:1: Alias \"b\" is part of an alias cycle")
            << QString();
    QTest::newRow("aliasCycle2")
            << QStringLiteral("aliasCycle.qml")
            << QString("Warning: 3:1: Alias \"a\" is part of an alias cycle")
            << QString();
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << QString("Warning: Property \"rrr\" not found on type \"Item\" at %1:5:34")
            << QString();
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << QString("Warning: Property \"progress\" not found on type \"QQuickItem\" at %1:7:39")
            << QString();
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << QString("Warning: Property \"rrr\" not found on type \"Item\" at %1:5:39")
            << QString();
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << QString("Warning: Type \"QPalette\" of base \"palette\" not found when accessing member \"weDontKnowIt\" at %1:5:34")
            << QString();
    QTest::newRow("inheritanceCylce")
            << QStringLiteral("Cycle1.qml")
            << QString("Warning: Cycle2 is part of an inheritance cycle: Cycle2 -> Cycle3 -> Cycle1 -> Cycle2")
            << QString();
    QTest::newRow("badQmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/bad.qml")
            << QString("Warning: Item was not found. Did you add all import paths?")
            << QString();
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleBad.qml")
            << QString("Warning: Property \"unknownFunc\" not found on type \"Foo\"")
            << QString();
    QTest::newRow("badEnumFromQtQml")
            << QStringLiteral("badEnumFromQtQml.qml")
            << QString("Warning: Property \"Linear123\" not found on type \"QQmlEasingEnums\"")
            << QString();
    QTest::newRow("anchors3")
            << QStringLiteral("anchors3.qml")
            << QString()
            << QString();
    QTest::newRow("nanchors1")
            << QStringLiteral("nanchors1.qml")
            << QString()
            << QString();
    QTest::newRow("nanchors2")
            << QStringLiteral("nanchors2.qml")
            << QString("unknown grouped property scope nanchors.")
            << QString();
    QTest::newRow("nanchors3")
            << QStringLiteral("nanchors3.qml")
            << QString("unknown grouped property scope nanchors.")
            << QString();
    QTest::newRow("badAliasObject")
            << QStringLiteral("badAliasObject.qml")
            << QString("Warning: Property \"wrongwrongwrong\" not found on type \"QtObject\"")
            << QString();
    QTest::newRow("badScript")
            << QStringLiteral("badScript.qml")
            << QString("Warning: Property \"stuff\" not found on type \"Empty\"")
            << QString();
    QTest::newRow("brokenNamespace")
            << QStringLiteral("brokenNamespace.qml")
            << QString("Warning: type not found in namespace at %1:4:17")
            << QString();
    // TODO: This fails but currently for the wrong reasons, make sure to add a warning message requirement
    // once it does fail properly in order to avoid regressions.
    QTest::newRow("segFault (bad)")
            << QStringLiteral("SegFault.bad.qml")
            << QString()
            << QString();
    QTest::newRow("VariableUsedBeforeDeclaration")
            << QStringLiteral("useBeforeDeclaration.qml")
            << QStringLiteral("Variable \"argq\" is used before its declaration at 5:9. "
                              "The declaration is at 6:13.")
            << QString();
    QTest::newRow("SignalParameterMismatch")
            << QStringLiteral("namedSignalParameters.qml")
            << QStringLiteral("Parameter 1 to signal handler for \"onSig\" is called \"argarg\". "
                              "The signal has a parameter of the same name in position 2.")
            << QStringLiteral("onSig2");
    QTest::newRow("TooManySignalParameters")
            << QStringLiteral("tooManySignalParameters.qml")
            << QStringLiteral("Signal handler for \"onSig\" has more formal parameters "
                              "than the signal it handles.")
            << QString();
    QTest::newRow("OnAssignment")
            << QStringLiteral("onAssignment.qml")
            << QStringLiteral("Property \"loops\" not found on type \"bool\"")
            << QString();
    QTest::newRow("BadAttached")
            << QStringLiteral("badAttached.qml")
            << QStringLiteral("unknown attached property scope WrongAttached.")
            << QString();
    QTest::newRow("BadBinding")
            << QStringLiteral("badBinding.qml")
            << QStringLiteral("Binding assigned to \"doesNotExist\", but no property "
                              "\"doesNotExist\" exists in the current element.")
            << QString();
    QTest::newRow("BadPropertyType")
            << QStringLiteral("badPropertyType.qml")
            << QStringLiteral("No type found for property \"bad\". This may be due to a missing "
                              "import statement or incomplete qmltypes files.")
            << QString();
    QTest::newRow("Deprecation (Property, with reason)")
            << QStringLiteral("deprecatedPropertyReason.qml")
            << QStringLiteral("Property \"deprecated\" is deprecated (Reason: Test)")
            << QString();
    QTest::newRow("Deprecation (Property, no reason)")
            << QStringLiteral("deprecatedProperty.qml")
            << QStringLiteral("Property \"deprecated\" is deprecated")
            << QString();
    QTest::newRow("Deprecation (Type, with reason)")
            << QStringLiteral("deprecatedTypeReason.qml")
            << QStringLiteral("Type \"TypeDeprecatedReason\" is deprecated (Reason: Test)")
            << QString();
    QTest::newRow("Deprecation (Type, no reason)")
            << QStringLiteral("deprecatedType.qml")
            << QStringLiteral("Type \"TypeDeprecated\" is deprecated")
            << QString();
    QTest::newRow("MissingDefaultProperty")
            << QStringLiteral("defaultPropertyWithoutKeyword.qml")
            << QStringLiteral("Cannot assign to non-existent default property") << QString();
    QTest::newRow("DoubleAssignToDefaultProperty")
            << QStringLiteral("defaultPropertyWithDoubleAssignment.qml")
            << QStringLiteral("Cannot assign multiple objects to a default non-list property")
            << QString();
    QTest::newRow("DefaultPropertyWithWrongType(string)")
            << QStringLiteral("defaultPropertyWithWrongType.qml")
            << QStringLiteral("Cannot assign to default property of incompatible type")
            << QStringLiteral("Cannot assign to non-existent default property");
    QTest::newRow("DefaultPropertyWithWrongType(var)")
            << QStringLiteral("defaultPropertyWithWrongType2.qml")
            << QStringLiteral("Cannot assign to default property of incompatible type")
            << QStringLiteral("Cannot assign to non-existent default property");
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(QString, warningMessage);
    QFETCH(QString, notContained);
    if (warningMessage.contains(QLatin1String("%1")))
        warningMessage = warningMessage.arg(testFile(filename));

    const QString output = runQmllint(filename, [&](QProcess &process) {
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QEXPECT_FAIL("anchors3", "We don't see that QQuickItem cannot be assigned to QQuickAnchorLine", Abort);
        QEXPECT_FAIL("nanchors1", "Invalid grouped properties are not always detected", Abort);
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
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);
    const QString warnings = runQmllint(filename, true);
    QEXPECT_FAIL("segFault", "This property exists and should not produce a warning", Abort);
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

        QEXPECT_FAIL("segFault", "This property exists and should not produce a warning", Abort);
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
