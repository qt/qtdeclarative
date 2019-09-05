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
    void test();
    void test_data();
    void testUnqualified();
    void testUnqualified_data();
    void testUnqualifiedNoSpuriousParentWarning();
    void catchIdentifierNoFalsePositive();
private:
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

void TestQmllint::test_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<bool>("isValid");

    // Valid files:
    QTest::newRow("Simple_QML") << QStringLiteral("Simple.qml") << true;
    QTest::newRow("QML_importing_JS") << QStringLiteral("importing_js.qml") << true;
    QTest::newRow("QTBUG-45916_JS_with_pragma_and_import") << QStringLiteral("QTBUG-45916.js") << true;

    // Invalid files:
    QTest::newRow("Invalid_syntax_QML") << QStringLiteral("failure1.qml") << false;
    QTest::newRow("Invalid_syntax_JS") << QStringLiteral("failure1.js") << false;
}

void TestQmllint::testUnqualified()
{
    auto qmlImportDir = QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
    QFETCH(QString, filename);
    QFETCH(QString, warningMessage);
    QFETCH(int, warningLine);
    QFETCH(int, warningColumn);
    QStringList args;
    args << QStringLiteral("-U") << testFile(filename) << QStringLiteral("-I") << qmlImportDir;

    QProcess process;
    process.start(m_qmllintPath, args);
    QVERIFY(process.waitForFinished());
    QVERIFY(process.exitStatus() == QProcess::NormalExit);
    QVERIFY(process.exitCode());
    QString output = process.readAllStandardError();
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
}

void TestQmllint::testUnqualifiedNoSpuriousParentWarning()
{
    auto qmlImportDir = QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
    {
        QString filename = testFile("spuriousParentWarning.qml");
        QStringList args;
        args << QStringLiteral("-U") << filename << QStringLiteral("-I") << qmlImportDir;
        QProcess process;
        process.start(m_qmllintPath, args);
        QVERIFY(process.waitForFinished());
        QVERIFY(process.exitStatus() == QProcess::NormalExit);
        QVERIFY(process.exitCode() == 0);
    }
    {
        QString filename = testFile("nonSpuriousParentWarning.qml");
        QStringList args;
        args << QStringLiteral("-U") << filename << QStringLiteral("-I") << qmlImportDir;
        QProcess process;
        process.start(m_qmllintPath, args);
        QVERIFY(process.waitForFinished());
        QVERIFY(process.exitStatus() == QProcess::NormalExit);
        QVERIFY(process.exitCode());
    }
}

void TestQmllint::catchIdentifierNoFalsePositive()
{
    auto qmlImportDir = QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
    QString filename = QLatin1String("catchIdentifierNoWarning.qml");
    filename.prepend(QStringLiteral("data/"));
    QStringList args;
    args << QStringLiteral("-U") << filename << QStringLiteral("-I") << qmlImportDir;
    QProcess process;
    process.start(m_qmllintPath, args);
    QVERIFY(process.waitForFinished());
    QVERIFY(process.exitStatus() == QProcess::NormalExit);
    QVERIFY(process.exitCode() == 0);
}

void TestQmllint::test()
{
    QFETCH(QString, filename);
    QFETCH(bool, isValid);
    QStringList args;
    args << QStringLiteral("--silent") << testFile(filename);

    bool success = QProcess::execute(m_qmllintPath, args) == 0;
    QCOMPARE(success, isValid);
}

QTEST_MAIN(TestQmllint)
#include "tst_qmllint.moc"
