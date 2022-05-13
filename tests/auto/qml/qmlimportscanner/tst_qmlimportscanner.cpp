// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QProcess>
#include <QString>
#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace Qt::StringLiterals;

class TestQmlimportscanner: public QQmlDataTest
{
    Q_OBJECT

public:
    TestQmlimportscanner();

private Q_SLOTS:
    void initTestCase() override;

    void cleanQmlCode_data();
    void cleanQmlCode();
    void rootPath();
    void modules_data();
    void modules();
    void qmldirPreference();

private:
    void runQmlimportscanner(const QString &mode, const QString &fileToScan,
                             const QString &resultFile);

    QString m_qmlimportscannerPath;
};

TestQmlimportscanner::TestQmlimportscanner()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void TestQmlimportscanner::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmlimportscannerPath = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath)
            + QLatin1String("/qmlimportscanner");
#ifdef Q_OS_WIN
    m_qmlimportscannerPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmlimportscannerPath).exists()) {
        QString message = QStringLiteral("qmlimportscanner executable not found (looked for %0)").arg(m_qmlimportscannerPath);
        QFAIL(qPrintable(message));
    }
}

void TestQmlimportscanner::cleanQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("Simple_QML")                << QStringLiteral("Simple.qml");
    QTest::newRow("QML_importing_JS")          << QStringLiteral("importing_js.qml");
    QTest::newRow("JS_with_pragma_and_import") << QStringLiteral("QTBUG-45916.js");
    QTest::newRow("qtQmlOnly")                 << QStringLiteral("qtQmlOnly.qml");
    QTest::newRow("directoryImportWithPrefix") << QStringLiteral("ImportWithPrefix.qml");
    QTest::newRow("localImport")               << QStringLiteral("localImport.qml");
    QTest::newRow("methodsInJavascript")       << QStringLiteral("javascriptMethods.qml");
    QTest::newRow("moduleImportWithPrefix")    << QStringLiteral("Drawer.qml");
    QTest::newRow("localAndModuleImport")      << QStringLiteral("ListProperty.qml");
    QTest::newRow("versionLessLocalImport")    << QStringLiteral("qmldirImportAndDepend.qml");
    QTest::newRow("versionLessModuleImport")   << QStringLiteral("parentEnum.qml");
}

void TestQmlimportscanner::cleanQmlCode()
{
    QFETCH(QString, filename);
    runQmlimportscanner("-qmlFiles", testFile(filename), testFile(filename + ".json"));
}

void TestQmlimportscanner::rootPath()
{
    runQmlimportscanner("-rootPath", dataDirectory(), testFile("rootPath.json"));
}

void TestQmlimportscanner::modules_data()
{
    QTest::addColumn<QString>("name");
    QTest::newRow("CompositeSingleton")        << QStringLiteral("CompositeSingleton");
    QTest::newRow("CompositeWithEnum")         << QStringLiteral("CompositeWithEnum");
    QTest::newRow("CompositeWithinSingleton")  << QStringLiteral("CompositeWithinSingleton");
    QTest::newRow("Imports")                   << QStringLiteral("Imports");
    QTest::newRow("Singleton")                 << QStringLiteral("Singleton");
    QTest::newRow("Things")                    << QStringLiteral("Things");
}

void TestQmlimportscanner::modules()
{
    QFETCH(QString, name);

    QTemporaryFile qmlFile(QDir::tempPath() + "/tst_qmlimportscanner_XXXXXX.qml");
    QVERIFY(qmlFile.open());

    qmlFile.write("import " + name.toUtf8() + "\nQtObject {}");
    qmlFile.close();
    runQmlimportscanner("-qmlFiles", qmlFile.fileName(), testFile(name + ".json"));
}

void TestQmlimportscanner::qmldirPreference()
{
    // ###
    QStringList with  {u"-importPath"_s, testFile("With")};
    QStringList withOut {u"-importPath"_s, testFile("WithOut")};
    QStringList genericArgs {u"-qmlFiles"_s, testFile("qmldirpref.qml"), u"-importPath"_s,
                             QLibraryInfo::path(QLibraryInfo::QmlImportsPath)};


    // found path should not depend  on order of importPath arguments
    QStringList argcombis[2] { genericArgs + with + withOut, genericArgs + withOut + with };
    for (const auto &allArgs: argcombis) {
        QProcess process;
        process.start(m_qmlimportscannerPath, allArgs);
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QVERIFY(process.readAllStandardError().isEmpty());
        auto output = process.readAllStandardOutput();
        // check that the "With" path is used, and the "WithOut" path is ignored
        QVERIFY(output.contains("With/Module"));
        QVERIFY(!output.contains("WithOut/Module"));
    }
}

void TestQmlimportscanner::runQmlimportscanner(const QString &mode, const QString &pathToScan,
                                               const QString &resultFile)
{
    const QString file(pathToScan);
    QStringList args {
        mode, file,
        "-importPath", QLibraryInfo::path(QLibraryInfo::QmlImportsPath), dataDirectory()
    };
    QString errors;
    QProcess process;
    process.start(m_qmlimportscannerPath, args);
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(process.readAllStandardError().isEmpty());

    QJsonParseError error;
    const QJsonDocument generated = QJsonDocument::fromJson(process.readAllStandardOutput(),
                                                            &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(generated.isArray());

    QFile imports(resultFile);
    imports.open(QIODevice::ReadOnly);
    QJsonDocument expected = QJsonDocument::fromJson(imports.readAll(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(expected.isArray());

    const QJsonArray generatedArray = generated.array();
    QJsonArray expectedArray = expected.array();
    for (const QJsonValue value : generatedArray) {
        QVERIFY(value.isObject());
        QJsonObject object = value.toObject();

        // Path is omitted because it's an absolute path, dependent on host system.
        object["path"] = QJsonValue::Undefined;
#ifdef LIBINFIX
#define XSTR(a) STR(a)
#define STR(A) #A
        if (object.contains("plugin")) {
            auto plugin = object["plugin"].toString();
            const auto pos = plugin.lastIndexOf(XSTR(LIBINFIX) "plugin");
            if (pos != -1)
                object["plugin"] = plugin.left(pos) + "plugin";
        }
#endif

        object.remove("components");
        object.remove("scripts");
        bool found = false;
        for (auto it = expectedArray.begin(), end = expectedArray.end(); it != end; ++it) {
            if (*it == object) {
                expectedArray.erase(it);
                found = true;
                break;
            }
        }
        QVERIFY2(found, qPrintable(QDebug::toString(object)));
    }
    QVERIFY(expectedArray.isEmpty());
}

QTEST_MAIN(TestQmlimportscanner)
#include "tst_qmlimportscanner.moc"
