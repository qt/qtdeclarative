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
};

void TestQQmlJSLogger::printFix()
{
    // This test verifies that we can correctly print out a fixit hint even
    // when there's no valid replacement text
    QQmlJSLogger logger;
    logger.setFilePath("test.qml");
    logger.setCode(""_L1);
    QQmlJS::SourceLocation loc(0, 0, 1, 1);
    QQmlJSFixSuggestion info("Purely informative"_L1, loc, ""_L1);
    info.setFilename("test.qml"_L1);
    info.setAutoApplicable(false);
    info.setHint("Just a hint"_L1);
    logger.startTransaction();
    logger.log("Test"_L1, qmlSyntax, loc, false, false, info);
    logger.rollback();;
    QVERIFY(true); // no assert hit
}

QT_END_NAMESPACE

QTEST_GUILESS_MAIN(TestQQmlJSLogger)
#include "tst_qqmljslogger.moc"
