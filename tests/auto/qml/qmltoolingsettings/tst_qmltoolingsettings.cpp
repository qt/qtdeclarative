// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtLogging>

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qmltoolingsettings : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qmltoolingsettings();

private Q_SLOTS:
    void searchConfig_data();
    void searchConfig();

    void reportConfigForFiles_data();
    void reportConfigForFiles();

    void searchOptions_data();
    void searchOptions();
};

tst_qmltoolingsettings::tst_qmltoolingsettings() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_qmltoolingsettings::searchConfig_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<QQmlToolingSettings::SearchOptions>("options");
    QTest::addColumn<QQmlToolingSettings::SearchResult>("expectedResult");

    QQmlToolingSettings::SearchOptions options;
    QTest::newRow("sameFolderConfig") << testFile("B/B1/test.qml") << "qmlformat" << options
                                      << QQmlToolingSettings::SearchResult{
                                             QQmlToolingSettings::SearchResult::ResultType::Found,
                                             testFile("B/B1/.qmlformat.ini")
                                         };
    QTest::newRow("parentFolderConfig") << testFile("B/B2/test.qml") << "qmlformat" << options
                                        << QQmlToolingSettings::SearchResult{
                                               QQmlToolingSettings::SearchResult::ResultType::Found,
                                               testFile("B/.qmlformat.ini")
                                           };
    QTest::newRow("parentFolderConfigDifferentTool")
            << testFile("B/B2/test.qml") << "qmlls" << options
            << QQmlToolingSettings::SearchResult{
                   QQmlToolingSettings::SearchResult::ResultType::NotFound, QString()
               };

    QTest::newRow("missingConfig")
            << testFile("A/test.qml") << "qmlformat"
            << QQmlToolingSettings::SearchOptions{ "A/test.ini", false }
            << QQmlToolingSettings::SearchResult{
                   QQmlToolingSettings::SearchResult::ResultType::NotFound, QString()
               };
}

void tst_qmltoolingsettings::searchConfig()
{
    QFETCH(QString, fileName);
    QFETCH(QString, toolName);
    QFETCH(QQmlToolingSettings::SearchOptions, options);
    QFETCH(QQmlToolingSettings::SearchResult, expectedResult);

    QQmlToolingSettings settings(toolName);
    QQmlToolingSettings::SearchResult actualResult = settings.search(fileName, options);

    QCOMPARE(actualResult.type, expectedResult.type);
    QCOMPARE(actualResult.iniFilePath, expectedResult.iniFilePath);
}

void tst_qmltoolingsettings::reportConfigForFiles_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QString>("expectedResultCapture"); // string captured from output

    QStringList files = { testFile("B/B1/test.qml"), testFile("B/B2/test.qml") };
    QTest::newRow("validFiles") << files << "B/B2/test.qml";
}

void tst_qmltoolingsettings::reportConfigForFiles()
{
    QFETCH(QStringList, files);
    QFETCH(QString, expectedResultCapture);

    static QString out;

    QtMessageHandler handler([](QtMsgType type, const QMessageLogContext &, const QString &msg) {
        if (type == QtWarningMsg) {
            QTextStream stream(&out);
            stream << msg << Qt::endl;
        }
    });

    const auto oldMessageHandler = qInstallMessageHandler(handler);
    const auto guard =
            qScopeGuard([&oldMessageHandler]() { qInstallMessageHandler(oldMessageHandler); });

    QQmlToolingSettings settings("qmlformat");
    settings.reportConfigForFiles(files);

    QVERIFY(out.contains("File"));
    QVERIFY(out.contains("Settings File"));
    QVERIFY(out.contains(expectedResultCapture));
    QVERIFY(!settings.isSet("HelloFromB"));
}

void tst_qmltoolingsettings::searchOptions_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<QQmlToolingSettings::SearchOptions>("options");
    QTest::addColumn<QQmlToolingSettings::SearchResult>("expectedResult");

    QTest::newRow("validAlternative")
            << testFile("B/B1/test.qml") << "qmlformat"
            << QQmlToolingSettings::SearchOptions{ testFile("B/.qmlformat.ini"), false }
            << QQmlToolingSettings::SearchResult{
                   QQmlToolingSettings::SearchResult::ResultType::Found,
                   testFile("B/.qmlformat.ini")
               };
    QTest::newRow("invalidAlternative")
            << testFile("B/B1/test.qml") << "qmlformat"
            << QQmlToolingSettings::SearchOptions{ "invalid/.qmlformat.ini", false }
            << QQmlToolingSettings::SearchResult{
                   QQmlToolingSettings::SearchResult::ResultType::NotFound, QString()
               };
}

void tst_qmltoolingsettings::searchOptions()
{
    QFETCH(QString, fileName);
    QFETCH(QString, toolName);
    QFETCH(QQmlToolingSettings::SearchOptions, options);
    QFETCH(QQmlToolingSettings::SearchResult, expectedResult);

    QQmlToolingSettings settings(toolName);
    QQmlToolingSettings::SearchResult actualResult = settings.search(fileName, options);

    QCOMPARE(actualResult.type, expectedResult.type);
    QCOMPARE(actualResult.iniFilePath, expectedResult.iniFilePath);
}

QTEST_MAIN(tst_qmltoolingsettings)
#include "tst_qmltoolingsettings.moc"
