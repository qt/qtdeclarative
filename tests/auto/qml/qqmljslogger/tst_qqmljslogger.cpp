// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QString>
#include <QtQmlCompiler/private/qqmljslogger_p.h>


using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

class TestQQmlJSLogger: public QObject
{
    Q_OBJECT

private slots:
    void printFix();

    void printLink_data();
    void printLink();
};

void TestQQmlJSLogger::printFix()
{
    // This test verifies that we can correctly print out a fixit hint even
    // when there are no associated document edits
    QQmlJSLogger logger;
    logger.setFilePath("test.qml");
    logger.setCode(""_L1);
    QQmlJS::SourceLocation loc = QQmlJS::s_documentOrigin;
    QQmlJSFixSuggestion info("Purely informative"_L1, loc);
    info.setFilename("test.qml"_L1);
    info.setAutoApplicable(false);
    logger.startTransaction();
    logger.log("Test"_L1, qmlSyntax, loc, false, false, info);
    logger.rollback();
    QVERIFY(true); // no assert hit
}

void TestQQmlJSLogger::printLink_data()
{
    QTest::addColumn<QString>("link");
    QTest::addColumn<QString>("message");
    QTest::addColumn<QString>("expectedOutput");

    QTest::addRow("example.com")
            << u"http://example.com"_s << u"This is a link"_s
            << u"\u001B]8;;http://example.com\u001B\\This is a link\u001B]8;;\u001B\\"_s;
}

void TestQQmlJSLogger::printLink()
{
    QFETCH(QString, link);
    QFETCH(QString, message);
    QFETCH(QString, expectedOutput);

    QColorOutput output;

    output.setHyperLinkSupport(false);
    QCOMPARE(output.linkify(link, message), message);
    output.setHyperLinkSupport(true);
    QCOMPARE(output.linkify(link, message), expectedOutput);
}

QT_END_NAMESPACE

QTEST_GUILESS_MAIN(TestQQmlJSLogger)
#include "tst_qqmljslogger.moc"
