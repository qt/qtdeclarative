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

private:
    QString runQmllint(const QString &fileToLint, bool shouldSucceed);

    QString m_qmllintPath;
};

void TestQmllint::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmllintPath = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/qmllint");
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
    QVERIFY(output.contains(QString::asprintf("Warning: unqualified access at %d:%d", warningLine, warningColumn)));
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
}

void TestQmllint::testUnknownCausesFail()
{
    const QString unknownNotFound = runQmllint("unknownElement.qml", false);
    QVERIFY(unknownNotFound.contains(
                QStringLiteral("warning: Unknown was not found. Did you add all import paths?")));
}

void TestQmllint::dirtyQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("warningMessage");
    QTest::addColumn<QString>("notContained");

    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml")
            << QStringLiteral("failure1.qml:4 : Expected token `:'")
            << QString();
    QTest::newRow("Invalid_syntax_JS")
            << QStringLiteral("failure1.js")
            << QStringLiteral("failure1.js:4 : Expected token `;'")
            << QString();
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << QString("Warning: unqualified access at 12:36")
            << QStringLiteral("no matching signal found");
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << QString("Warning: Property \"foo\" not found on type \"QtObject\" at 6:31")
            << QString();
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << QString("Warning: Property \"foo2\" not found on type \"Methods\" at 5:25")
            << QString();
    QTest::newRow("badAlias")
            << QStringLiteral("badAlias.qml")
            << QString("Warning: unqualified access at 4:27")
            << QString();
    QTest::newRow("badAliasProperty")
            << QStringLiteral("badAliasProperty.qml")
            << QString("Warning: Property \"nowhere\" not found on type \"QtObject\" at 5:32")
            << QString();
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << QString("Warning: Property \"rrr\" not found on type \"Item\" at 5:34")
            << QString();
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << QString("Warning: Property \"progress\" not found on type \"QQuickItem\" at 7:39")
            << QString();
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << QString("Warning: Property \"rrr\" not found on type \"Item\" at 5:39")
            << QString();
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << QString("Warning: Type \"QPalette\" of member \"palette\" not found at 5:26")
            << QString();
    QTest::newRow("inheritanceCylce")
            << QStringLiteral("Cycle1.qml")
            << QString("Warning: Cycle2 is part of an inheritance cycle: Cycle2 -> Cycle3 -> Cycle1 -> Cycle2")
            << QString();
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(QString, warningMessage);
    QFETCH(QString, notContained);

    const QString output = runQmllint(filename, false);
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
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);
    const QString warnings = runQmllint(filename, true);
    QVERIFY(warnings.isEmpty());
}

QString TestQmllint::runQmllint(const QString &fileToLint, bool shouldSucceed)
{
    auto qmlImportDir = QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
    QStringList args;
    args << QStringLiteral("-U") << testFile(fileToLint)
         << QStringLiteral("-I") << qmlImportDir
         << QStringLiteral("-I") << dataDirectory()
         << QStringLiteral("--silent");
    QString errors;
    auto verify = [&](bool isSilent) {
        QProcess process;
        process.start(m_qmllintPath, args);
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        if (shouldSucceed)
            QCOMPARE(process.exitCode(), 0);
        else
            QVERIFY(process.exitCode() != 0);
        errors = process.readAllStandardError();

        if (isSilent)
            QVERIFY(errors.isEmpty());
    };
    verify(true);
    args.removeLast();
    verify(false);
    return errors;
}

QTEST_MAIN(TestQmllint)
#include "tst_qmllint.moc"
