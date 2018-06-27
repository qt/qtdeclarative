/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qtest.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QProcess>
#include <QLibraryInfo>
#include <QSysInfo>
#include <QLoggingCategory>
#include <private/qqmlcomponent_p.h>

class tst_qmlcachegen: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void loadGeneratedFile();
    void translationExpressionSupport();
    void signalHandlerParameters();
    void errorOnArgumentsInSignalHandler();
    void aheadOfTimeCompilation();
    void functionExpressions();
    void versionChecksForAheadOfTimeUnits();

    void workerScripts();

    void trickyPaths_data();
    void trickyPaths();

    void scriptImport();

    void enums();
};

// A wrapper around QQmlComponent to ensure the temporary reference counts
// on the type data as a result of the main thread <> loader thread communication
// are dropped. Regular Synchronous loading will leave us with an event posted
// to the gui thread and an extra refcount that will only be dropped after the
// event delivery. A plain sendPostedEvents() however is insufficient because
// we can't be sure that the event is posted after the constructor finished.
class CleanlyLoadingComponent : public QQmlComponent
{
public:
    CleanlyLoadingComponent(QQmlEngine *engine, const QUrl &url)
        : QQmlComponent(engine, url, QQmlComponent::Asynchronous)
    { waitForLoad(); }
    CleanlyLoadingComponent(QQmlEngine *engine, const QString &fileName)
        : QQmlComponent(engine, fileName, QQmlComponent::Asynchronous)
    { waitForLoad(); }

    void waitForLoad()
    {
        QTRY_VERIFY(status() == QQmlComponent::Ready || status() == QQmlComponent::Error);
    }
};

static bool generateCache(const QString &qmlFileName, QByteArray *capturedStderr = nullptr)
{
    QProcess proc;
    if (capturedStderr == nullptr)
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.setProgram(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QDir::separator() + QLatin1String("qmlcachegen"));
    proc.setArguments(QStringList() << qmlFileName);
    proc.start();
    if (!proc.waitForFinished())
        return false;

    if (capturedStderr)
        *capturedStderr = proc.readAllStandardError();

    if (proc.exitStatus() != QProcess::NormalExit)
        return false;
    return proc.exitCode() == 0;
}

void tst_qmlcachegen::initTestCase()
{
    qputenv("QML_FORCE_DISK_CACHE", "1");
}

void tst_qmlcachegen::loadGeneratedFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    const QString testFilePath = writeTempFile("test.qml", "import QtQml 2.0\n"
                                                           "QtObject {\n"
                                                           "    property int value: Math.min(100, 42);\n"
                                                           "}");

    QVERIFY(generateCache(testFilePath));

    const QString cacheFilePath = testFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(cacheFilePath));

    {
        QFile cache(cacheFilePath);
        QVERIFY(cache.open(QIODevice::ReadOnly));
        const QV4::CompiledData::Unit *cacheUnit = reinterpret_cast<const QV4::CompiledData::Unit *>(cache.map(/*offset*/0, sizeof(QV4::CompiledData::Unit)));
        QVERIFY(cacheUnit);
        QVERIFY(cacheUnit->flags & QV4::CompiledData::Unit::StaticData);
        QVERIFY(cacheUnit->flags & QV4::CompiledData::Unit::PendingTypeCompilation);
    }

    QVERIFY(QFile::remove(testFilePath));

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("value").toInt(), 42);

    auto componentPrivate = QQmlComponentPrivate::get(&component);
    QVERIFY(componentPrivate);
    auto compilationUnit = componentPrivate->compilationUnit;
    QVERIFY(compilationUnit);
    QVERIFY(compilationUnit->data);
    QVERIFY(!(compilationUnit->data->flags & QV4::CompiledData::Unit::StaticData));
}

void tst_qmlcachegen::translationExpressionSupport()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    const QString testFilePath = writeTempFile("test.qml", "import QtQml.Models 2.2\n"
                                                           "import QtQml 2.2\n"
                                                           "QtObject {\n"
                                                           "    property ListModel model: ListModel {\n"
                                                           "        ListElement {\n"
                                                           "            text: qsTr(\"All\")\n"
                                                           "        }\n"
                                                           "        ListElement {\n"
                                                           "            text: QT_TR_NOOP(\"Ok\")\n"
                                                           "        }\n"
                                                           "    }\n"
                                                           "    property string text: model.get(0).text + \" \" + model.get(1).text\n"
                                                           "}");


    QVERIFY(generateCache(testFilePath));

    const QString cacheFilePath = testFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(cacheFilePath));
    QVERIFY(QFile::remove(testFilePath));

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("text").toString(), QString("All Ok"));
}

void tst_qmlcachegen::signalHandlerParameters()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    const QString testFilePath = writeTempFile("test.qml", "import QtQml 2.0\n"
                                                           "QtObject {\n"
                                                           "    property real result: 0\n"
                                                           "    signal testMe(real value);\n"
                                                           "    onTestMe: result = value;\n"
                                                           "    function runTest() { testMe(42); }\n"
                                                           "}");

    QVERIFY(generateCache(testFilePath));

    const QString cacheFilePath = testFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(cacheFilePath));
    QVERIFY(QFile::remove(testFilePath));

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QMetaObject::invokeMethod(obj.data(), "runTest");
    QCOMPARE(obj->property("result").toInt(), 42);
}

void tst_qmlcachegen::errorOnArgumentsInSignalHandler()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    const QString testFilePath = writeTempFile("test.qml", "import QtQml 2.2\n"
                                                           "QtObject {\n"
                                                           "    signal mySignal(var arguments);\n"
                                                           "    onMySignal: console.log(arguments);\n"
                                                           "}");


    QByteArray errorOutput;
    QVERIFY(!generateCache(testFilePath, &errorOutput));
    QVERIFY2(errorOutput.contains("error: The use of eval() or the use of the arguments object in signal handlers is"), errorOutput);
}

void tst_qmlcachegen::aheadOfTimeCompilation()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    const QString testFilePath = writeTempFile("test.qml", "import QtQml 2.0\n"
                                                           "QtObject {\n"
                                                           "    function runTest() { var x = 0; while (x < 42) { ++x }; return x; }\n"
                                                           "}");

    QVERIFY(generateCache(testFilePath));

    const QString cacheFilePath = testFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(cacheFilePath));
    QVERIFY(QFile::remove(testFilePath));

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QVariant result;
    QMetaObject::invokeMethod(obj.data(), "runTest", Q_RETURN_ARG(QVariant, result));
    QCOMPARE(result.toInt(), 42);
}

static QQmlPrivate::CachedQmlUnit *temporaryModifiedCachedUnit = nullptr;

void tst_qmlcachegen::versionChecksForAheadOfTimeUnits()
{
    QVERIFY(QFile::exists(":/versionchecks.qml"));
    QCOMPARE(QFileInfo(":/versionchecks.qml").size(), 0);

    Q_ASSERT(!temporaryModifiedCachedUnit);
    QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
    const QV4::CompiledData::Unit *originalUnit = QQmlMetaType::findCachedCompilationUnit(QUrl("qrc:/versionchecks.qml"), &error);
    QVERIFY(originalUnit);
    QV4::CompiledData::Unit *tweakedUnit = (QV4::CompiledData::Unit *)malloc(originalUnit->unitSize);
    memcpy(reinterpret_cast<void *>(tweakedUnit), reinterpret_cast<const void *>(originalUnit), originalUnit->unitSize);
    tweakedUnit->version = QV4_DATA_STRUCTURE_VERSION - 1;
    temporaryModifiedCachedUnit = new QQmlPrivate::CachedQmlUnit{tweakedUnit, nullptr, nullptr};

    auto testHandler = [](const QUrl &url) -> const QQmlPrivate::CachedQmlUnit * {
        if (url == QUrl("qrc:/versionchecks.qml"))
            return temporaryModifiedCachedUnit;
        return nullptr;
    };
    QQmlMetaType::prependCachedUnitLookupFunction(testHandler);

    {
        QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
        QVERIFY(!QQmlMetaType::findCachedCompilationUnit(QUrl("qrc:/versionchecks.qml"), &error));
        QCOMPARE(error, QQmlMetaType::CachedUnitLookupError::VersionMismatch);
    }

    {
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl("qrc:/versionchecks.qml"));
        QCOMPARE(component.status(), QQmlComponent::Error);
        QCOMPARE(component.errorString(), QString("qrc:/versionchecks.qml:-1 File was compiled ahead of time with an incompatible version of Qt and the original file cannot be found. Please recompile\n"));
    }

    Q_ASSERT(temporaryModifiedCachedUnit);
    free(const_cast<QV4::CompiledData::Unit *>(temporaryModifiedCachedUnit->qmlData));
    delete temporaryModifiedCachedUnit;
    temporaryModifiedCachedUnit = nullptr;

    QQmlMetaType::removeCachedUnitLookupFunction(testHandler);
}

void tst_qmlcachegen::workerScripts()
{
    QVERIFY(QFile::exists(":/workerscripts/worker.js"));
    QVERIFY(QFile::exists(":/workerscripts/worker.qml"));
    QCOMPARE(QFileInfo(":/workerscripts/worker.js").size(), 0);

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///workerscripts/worker.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_VERIFY(obj->property("success").toBool());
}

void tst_qmlcachegen::functionExpressions()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    const QString testFilePath = writeTempFile(
                "test.qml",
                "import QtQuick 2.0\n"
                "Item {\n"
                "    id: di\n"
                "    \n"
                "    property var f\n"
                "    property bool f_called: false\n"
                "    f : function() { f_called = true }\n"
                "    \n"
                "    signal g\n"
                "    property bool g_handler_called: false\n"
                "    onG: function() { g_handler_called = true }\n"
                "    \n"
                "    signal h(int i)\n"
                "    property bool h_connections_handler_called: false\n"
                "    Connections {\n"
                "        target: di\n"
                "        onH: function(magic) { h_connections_handler_called = (magic == 42)\n }\n"
                "    }\n"
                "    \n"
                "    function runTest() { \n"
                "        f()\n"
                "        g()\n"
                "        h(42)\n"
                "    }\n"
                "}");

    QVERIFY(generateCache(testFilePath));

    const QString cacheFilePath = testFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(cacheFilePath));
    QVERIFY(QFile::remove(testFilePath));

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());

    QCOMPARE(obj->property("f_called").toBool(), false);
    QCOMPARE(obj->property("g_handler_called").toBool(), false);
    QCOMPARE(obj->property("h_connections_handler_called").toBool(), false);

    QMetaObject::invokeMethod(obj.data(), "runTest");

    QCOMPARE(obj->property("f_called").toBool(), true);
    QCOMPARE(obj->property("g_handler_called").toBool(), true);
    QCOMPARE(obj->property("h_connections_handler_called").toBool(), true);
}

void tst_qmlcachegen::trickyPaths_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::newRow("path with spaces") << QStringLiteral(":/directory with spaces/file name with spaces.qml");
    QTest::newRow("version style suffix 1") << QStringLiteral(":/directory with spaces/versionStyleSuffix-1.2-core-yc.qml");
    QTest::newRow("version style suffix 2") << QStringLiteral(":/directory with spaces/versionStyleSuffix-1.2-more.qml");

    // QTBUG-46375
#if !defined(Q_OS_WIN)
    QTest::newRow("path with umlaut") << QStringLiteral(":/Bäh.qml");
#endif
}

void tst_qmlcachegen::trickyPaths()
{
    QFETCH(QString, filePath);
    QVERIFY2(QFile::exists(filePath), qPrintable(filePath));
    QCOMPARE(QFileInfo(filePath).size(), 0);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc" + filePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("success").toInt(), 42);
}

void tst_qmlcachegen::scriptImport()
{
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///jsimport.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_COMPARE(obj->property("value").toInt(), 42);
}

void tst_qmlcachegen::enums()
{
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///Enums.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_COMPARE(obj->property("value").toInt(), 200);
}

QTEST_GUILESS_MAIN(tst_qmlcachegen)

#include "tst_qmlcachegen.moc"
