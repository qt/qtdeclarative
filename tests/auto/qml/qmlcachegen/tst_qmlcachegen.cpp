// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <QJsonDocument>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QProcess>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QSysInfo>
#include <QLoggingCategory>
#include <private/qqmlcomponent_p.h>
#include <private/qqmljscompilerstats_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qv4compileddata_p.h>
#include <qtranslator.h>
#include <qqmlscriptstring.h>
#include <QString>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include "scriptstringprops.h"

using namespace Qt::StringLiterals;

class tst_qmlcachegen: public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qmlcachegen();

private slots:
    void initTestCase() override;

    void loadGeneratedFile();
    void translationExpressionSupport();
    void signalHandlerParameters();
    void errorOnArgumentsInSignalHandler();
    void aheadOfTimeCompilation();
    void functionExpressions();
    void versionChecksForAheadOfTimeUnits();
    void retainedResources();
    void skippedResources();

    void workerScripts();

    void trickyPaths_data();
    void trickyPaths();

    void qrcScriptImport();
    void fsScriptImport();
    void moduleScriptImport();
    void esModulesViaQJSEngine();

    void enums();

    void sourceFileIndices();

    void reproducibleCache_data();
    void reproducibleCache();

    void parameterAdjustment();
    void inlineComponent();
    void posthocRequired();

    void gracefullyHandleTruncatedCacheFile();

    void scriptStringCachegenInteraction();
    void saveableUnitPointer();

    void aotstatsSerialization();
    void aotstatsGeneration_data();
    void aotstatsGeneration();
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
#if defined(QTEST_CROSS_COMPILED)
    QTest::qFail("You cannot call qmlcachegen on the target.", __FILE__, __LINE__);
    return false;
#endif
    QProcess proc;
    if (capturedStderr == nullptr)
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.setProgram(QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath)
                    + QLatin1String("/qmlcachegen"));
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

tst_qmlcachegen::tst_qmlcachegen()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qmlcachegen::initTestCase()
{
    if (qEnvironmentVariableIsEmpty("QML_DISK_CACHE"))
        qputenv("QML_FORCE_DISK_CACHE", "1");

    QStandardPaths::setTestModeEnabled(true);

    // make sure there's no pre-existing cache dir
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cacheDir.isEmpty())
        //QDir(cacheDir).removeRecursively();
        qDebug() << cacheDir;
    QQmlDataTest::initTestCase();
}

#if QT_CONFIG(process)
static void testWithEnvironment(
        const QString &function, const QString &testFilePath, const QByteArray &value, bool success)
{
    QProcess child;
    child.setProgram(QCoreApplication::applicationFilePath());
    child.setArguments(QStringList(function));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QLatin1String("QML_FORCE_DISK_CACHE"));
    env.insert(QLatin1String("QMLCACHEGEN_TEST_FILE_PATH"), testFilePath);
    env.insert(QLatin1String("QML_DISK_CACHE"), QLatin1String(value));
    child.setProcessEnvironment(env);
    child.start();
    QVERIFY(child.waitForFinished());
    if (success)
        QCOMPARE(child.exitCode(), 0);
    else
        QVERIFY(child.exitCode() != 0);
}
#endif

void tst_qmlcachegen::loadGeneratedFile()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QByteArray path = qgetenv("QMLCACHEGEN_TEST_FILE_PATH");
    QString testFilePath;
    if (path.isEmpty()) {
        const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
            QFile f(tempDir.path() + '/' + fileName);
            const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
            Q_ASSERT(ok);
            f.write(contents);
            return f.fileName();
        };

        testFilePath = writeTempFile("test.qml", "import QtQml 2.0\n"
                                                 "QtObject {\n"
                                                 "    property int value: Math.min(100, 42);\n"
                                                 "}");
        QVERIFY(generateCache(testFilePath));
    } else {
        testFilePath = QString::fromUtf8(path);
    }

    const QString cacheFilePath = testFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(cacheFilePath));

    {
        QFile cache(cacheFilePath);
        QVERIFY(cache.open(QIODevice::ReadOnly));
        const QV4::CompiledData::Unit *cacheUnit = reinterpret_cast<const QV4::CompiledData::Unit *>(cache.map(/*offset*/0, sizeof(QV4::CompiledData::Unit)));
        QVERIFY(cacheUnit);
        QVERIFY(cacheUnit->flags & QV4::CompiledData::Unit::StaticData);
        QVERIFY(cacheUnit->flags & QV4::CompiledData::Unit::PendingTypeCompilation);
        QCOMPARE(uint(cacheUnit->sourceFileIndex), uint(0));
    }

    if (path.isEmpty()) {
        QVERIFY(QFile::remove(testFilePath));
    } else {
#if QT_CONFIG(process)
        testWithEnvironment("loadGeneratedFile", testFilePath, "qmlc-read", true);
        testWithEnvironment("loadGeneratedFile", testFilePath, "qmlc-write", false);
        testWithEnvironment("loadGeneratedFile", testFilePath, "qmlc-read,qmlc-read", true);
        testWithEnvironment("loadGeneratedFile", testFilePath, "qmlc", true);
        testWithEnvironment("loadGeneratedFile", testFilePath, "wrong", false);
#endif
    }

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("value").toInt(), 42);

    auto componentPrivate = QQmlComponentPrivate::get(&component);
    QVERIFY(componentPrivate);
    auto compilationUnit = componentPrivate->compilationUnit;
    QVERIFY(compilationUnit);
    auto unitData = compilationUnit->unitData();
    QVERIFY(unitData);
    QVERIFY(unitData->flags & QV4::CompiledData::Unit::StaticData);
}

class QTestTranslator : public QTranslator
{
public:
    QString translate(const char *context, const char *sourceText, const char */*disambiguation*/, int /*n*/) const override
    {
        m_lastContext = QString::fromUtf8(context);
        return QString::fromUtf8(sourceText).toUpper();
    }
    bool isEmpty() const override { return true; }
    mutable QString m_lastContext;
};

void tst_qmlcachegen::translationExpressionSupport()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QTestTranslator translator;
    qApp->installTranslator(&translator);

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
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("text").toString(), QString("ALL Ok"));
    QCOMPARE(translator.m_lastContext, QStringLiteral("test"));
}

void tst_qmlcachegen::signalHandlerParameters()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

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

    {
        QFile cache(cacheFilePath);
        QVERIFY(cache.open(QIODevice::ReadOnly));
        const QV4::CompiledData::Unit *cacheUnit = reinterpret_cast<const QV4::CompiledData::Unit *>(cache.map(/*offset*/0, sizeof(QV4::CompiledData::Unit)));
        QVERIFY(cacheUnit);
    }

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QMetaObject::invokeMethod(obj.data(), "runTest");
    QCOMPARE(obj->property("result").toInt(), 42);

    {
        auto componentPrivate = QQmlComponentPrivate::get(&component);
        QVERIFY(componentPrivate);
        auto compilationUnit = componentPrivate->compilationUnit;
        QVERIFY(compilationUnit);
        QVERIFY(compilationUnit->unitData());

        // Verify that the QML objects don't come from the original data.
        QVERIFY(compilationUnit->objectAt(0) != compilationUnit->unitData()->qmlUnit()->objectAt(0));

        // Typically the final file name is one of those strings that is not in the original
        // pre-compiled qml file's string table, while for example the signal parameter
        // name ("value") is.
        const auto isStringIndexInStringTable = [compilationUnit](uint index) {
            return index < compilationUnit->unitData()->stringTableSize;
        };

        QVERIFY(isStringIndexInStringTable(compilationUnit->objectAt(0)->signalAt(0)->parameterAt(0)->nameIndex));
        QVERIFY(!compilationUnit->baseCompilationUnit()->dynamicStrings.isEmpty());
    }
}

void tst_qmlcachegen::errorOnArgumentsInSignalHandler()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

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
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

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
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QVariant result;
    QMetaObject::invokeMethod(obj.data(), "runTest", Q_RETURN_ARG(QVariant, result));
    QCOMPARE(result.toInt(), 42);
}

static QQmlPrivate::CachedQmlUnit *temporaryModifiedCachedUnit = nullptr;

static const char *versionCheckErrorString(QQmlMetaType::CachedUnitLookupError error)
{
    switch (error) {
    case QQmlMetaType::CachedUnitLookupError::NoError:
        return "no error";
    case QQmlMetaType::CachedUnitLookupError::NoUnitFound:
        return "no unit found";
    case QQmlMetaType::CachedUnitLookupError::VersionMismatch:
        return "version mismatch";
    case QQmlMetaType::CachedUnitLookupError::NotFullyTyped:
        return "unit not fully typed";
    }

    return "wat?";
}

void tst_qmlcachegen::versionChecksForAheadOfTimeUnits()
{
    QVERIFY(QFile::exists(":/data/versionchecks.qml"));
    QVERIFY(QFileInfo(":/data/versionchecks.qml").size() > 0);

    Q_ASSERT(!temporaryModifiedCachedUnit);
    QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
    QLoggingCategory::setFilterRules("qt.qml.diskcache.debug=true");
    const QQmlPrivate::CachedQmlUnit *originalUnit = QQmlMetaType::findCachedCompilationUnit(
            QUrl("qrc:/data/versionchecks.qml"), QQmlMetaType::AcceptUntyped, &error);
    QLoggingCategory::setFilterRules(QString());
    QVERIFY2(originalUnit, versionCheckErrorString(error));
    QV4::CompiledData::Unit *tweakedUnit = (QV4::CompiledData::Unit *)malloc(originalUnit->qmlData->unitSize);
    memcpy(reinterpret_cast<void *>(tweakedUnit),
           reinterpret_cast<const void *>(originalUnit->qmlData),
           originalUnit->qmlData->unitSize);
    tweakedUnit->version = QV4_DATA_STRUCTURE_VERSION - 1;

    const auto testHandler = [](const QUrl &url) -> const QQmlPrivate::CachedQmlUnit * {
        if (url == QUrl("qrc:/data/versionchecks.qml"))
            return temporaryModifiedCachedUnit;
        return nullptr;
    };

    const auto dropModifiedUnit = qScopeGuard([&testHandler]() {
        Q_ASSERT(temporaryModifiedCachedUnit);
        free(const_cast<QV4::CompiledData::Unit *>(temporaryModifiedCachedUnit->qmlData));
        delete temporaryModifiedCachedUnit;
        temporaryModifiedCachedUnit = nullptr;

        QQmlMetaType::removeCachedUnitLookupFunction(testHandler);
    });

    temporaryModifiedCachedUnit = new QQmlPrivate::CachedQmlUnit{tweakedUnit, nullptr, nullptr};
    QQmlMetaType::prependCachedUnitLookupFunction(testHandler);

    {
        QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
        QVERIFY(!QQmlMetaType::findCachedCompilationUnit(
                    QUrl("qrc:/data/versionchecks.qml"), QQmlMetaType::AcceptUntyped, &error));
        QCOMPARE(error, QQmlMetaType::CachedUnitLookupError::VersionMismatch);
    }

    {
        QQmlEngine engine;
        CleanlyLoadingComponent component(&engine, QUrl("qrc:/data/versionchecks.qml"));
        QCOMPARE(component.status(), QQmlComponent::Ready);
    }
}

void tst_qmlcachegen::retainedResources()
{
    QFile file(":/Retain.qml");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().startsWith("import QtQml 2.0"));
}

void tst_qmlcachegen::skippedResources()
{
    QFile file(":/not/Skip.qml");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().startsWith("import QtQml 2.0"));

    QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
    const QQmlPrivate::CachedQmlUnit *unit = QQmlMetaType::findCachedCompilationUnit(
            QUrl("qrc:/not/Skip.qml"), QQmlMetaType::AcceptUntyped, &error);
    QCOMPARE(unit, nullptr);
    QCOMPARE(error, QQmlMetaType::CachedUnitLookupError::NoUnitFound);
}

void tst_qmlcachegen::workerScripts()
{
    QVERIFY(QFile::exists(":/workerscripts/data/worker.js"));
    QVERIFY(QFile::exists(":/workerscripts/data/worker.qml"));
    QVERIFY(QFileInfo(":/workerscripts/data/worker.js").size() > 0);

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///workerscripts/data/worker.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_VERIFY(obj->property("success").toBool());
}

void tst_qmlcachegen::functionExpressions()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

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
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
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
    QVERIFY(QFileInfo(filePath).size() > 0);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc" + filePath));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("success").toInt(), 42);
}

void tst_qmlcachegen::qrcScriptImport()
{
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///data/jsimport.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_COMPARE(obj->property("value").toInt(), 42);
#if QT_CONFIG(process)
    if (qEnvironmentVariableIsEmpty("QMLCACHEGEN_TEST_FILE_PATH")) {
        testWithEnvironment("qrcScriptImport", ":/data/jsimport.qml", "aot-native", false);
        testWithEnvironment("qrcScriptImport", ":/data/jsimport.qml", "aot-bytecode", true);
        testWithEnvironment("qrcScriptImport", ":/data/jsimport.qml", "aot-bytecode,aot-native", true);
        testWithEnvironment("qrcScriptImport", ":/data/jsimport.qml", "aot", true);
        testWithEnvironment("qrcScriptImport", ":/data/jsimport.qml", "wrong", false);
    }
#endif

    auto componentPrivate = QQmlComponentPrivate::get(&component);
    QVERIFY(componentPrivate);
    auto compilationUnit = componentPrivate->compilationUnit;
    QVERIFY(compilationUnit);
    auto unitData = compilationUnit->unitData();
    QVERIFY(unitData);
    QVERIFY(unitData->flags & QV4::CompiledData::Unit::StaticData);
}

void tst_qmlcachegen::fsScriptImport()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

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
            "import QtQml 2.0\n"
            "import \"test.js\" as ScriptTest\n"
            "QtObject {\n"
            "    property int value: ScriptTest.value\n"
            "}\n");

    const QString scriptFilePath = writeTempFile(
            "test.js",
            "var value = 42"
            );

    QVERIFY(generateCache(scriptFilePath));
    QVERIFY(generateCache(testFilePath));

    const QString scriptCacheFilePath = scriptFilePath + QLatin1Char('c');
    QVERIFY(QFile::exists(scriptFilePath));

    {
        QFile cache(scriptCacheFilePath);
        QVERIFY(cache.open(QIODevice::ReadOnly));
        const QV4::CompiledData::Unit *cacheUnit = reinterpret_cast<const QV4::CompiledData::Unit *>(cache.map(/*offset*/0, sizeof(QV4::CompiledData::Unit)));
        QVERIFY(cacheUnit);
        QVERIFY(cacheUnit->flags & QV4::CompiledData::Unit::StaticData);
        QVERIFY(!(cacheUnit->flags & QV4::CompiledData::Unit::PendingTypeCompilation));
        QCOMPARE(uint(cacheUnit->sourceFileIndex), uint(0));
    }

    // Remove source code to make sure that when loading succeeds, it is because we loaded
    // the existing cache files.
    QVERIFY(QFile::remove(testFilePath));
    QVERIFY(QFile::remove(scriptFilePath));

    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("value").toInt(), 42);
}

void tst_qmlcachegen::moduleScriptImport()
{
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///data/jsmoduleimport.qml"));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_VERIFY(obj->property("ok").toBool());

    QVERIFY(QFile::exists(":/data/script.mjs"));
    QVERIFY(QFileInfo(":/data/script.mjs").size() > 0);

    {
        auto componentPrivate = QQmlComponentPrivate::get(&component);
        QVERIFY(componentPrivate);
        auto compilationUnit = componentPrivate->compilationUnit->dependentScriptsPtr()
                                       ->first()->compilationUnit();
        QVERIFY(compilationUnit);
        auto unitData = compilationUnit->unitData();
        QVERIFY(unitData);
        QVERIFY(unitData->flags & QV4::CompiledData::Unit::StaticData);
        QVERIFY(unitData->flags & QV4::CompiledData::Unit::IsESModule);

        QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
        const QQmlPrivate::CachedQmlUnit *unitFromResources = QQmlMetaType::findCachedCompilationUnit(
                QUrl("qrc:/data/script.mjs"), QQmlMetaType::AcceptUntyped, &error);
        QVERIFY(unitFromResources);

        QCOMPARE(unitFromResources->qmlData, compilationUnit->unitData());
    }
}

void tst_qmlcachegen::esModulesViaQJSEngine()
{
    QJSEngine engine;
    QJSValue module = engine.importModule(":/data/module.mjs");
    QJSValue result = module.property("entry").call();
    QCOMPARE(result.toString(), "ok");
}

void tst_qmlcachegen::enums()
{
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///data/Enums.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QTRY_COMPARE(obj->property("value").toInt(), 200);
}

void tst_qmlcachegen::sourceFileIndices()
{
    QVERIFY(QFile::exists(":/data/versionchecks.qml"));
    QVERIFY(QFileInfo(":/data/versionchecks.qml").size() > 0);

    QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
    const QQmlPrivate::CachedQmlUnit *unitFromResources = QQmlMetaType::findCachedCompilationUnit(
            QUrl("qrc:/data/versionchecks.qml"), QQmlMetaType::AcceptUntyped, &error);
    QVERIFY(unitFromResources);
    QVERIFY(unitFromResources->qmlData->flags & QV4::CompiledData::Unit::PendingTypeCompilation);
    QCOMPARE(uint(unitFromResources->qmlData->sourceFileIndex), uint(0));
}

void tst_qmlcachegen::reproducibleCache_data()
{
    QTest::addColumn<QString>("filePath");

    QDir dir(dataDirectory());
    for (const QString &entry : dir.entryList((QStringList() << "*.qml" << "*.js" << "*.mjs"), QDir::Files)) {
        QTest::newRow(entry.toUtf8().constData()) << dir.filePath(entry);
    }
}

void tst_qmlcachegen::reproducibleCache()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

    QFETCH(QString, filePath);

    QFile file(filePath);
    QVERIFY(file.exists());

    auto generate = [](const QString &path) {
        if (!generateCache(path))
            return QByteArray();
        QFile generated(path + 'c');
        [&](){ QVERIFY(generated.open(QIODevice::ReadOnly)); }();
        const QByteArray result = generated.readAll();
        generated.remove();
        return result;
    };

    const QByteArray contents1 = generate(file.fileName());
    const QByteArray contents2 = generate(file.fileName());
    QCOMPARE(contents1, contents2);
}

void tst_qmlcachegen::parameterAdjustment()
{
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, QUrl("qrc:///data/parameterAdjustment.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull()); // Doesn't crash
}


void tst_qmlcachegen::inlineComponent()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

    QByteArray errors;
    bool ok = generateCache(testFile("inlineComponentWithId.qml"), &errors);
    QVERIFY2(ok, errors);
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, testFileUrl("inlineComponentWithId.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "42");
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
}

void tst_qmlcachegen::posthocRequired()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

    bool ok = generateCache(testFile("posthocrequired.qml"));
    QVERIFY(ok);
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, testFileUrl("posthocrequired.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj.isNull() && component.isError());
    QVERIFY2(component.errorString().contains(
                 QStringLiteral("Required property x was not initialized")),
             qPrintable(component.errorString()));
}

void tst_qmlcachegen::gracefullyHandleTruncatedCacheFile()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

    bool ok = generateCache(testFile("truncateTest.qml"));
    QVERIFY(ok);
    const QString qmlcFile = testFile("truncateTest.qmlc");
    QVERIFY(QFile::exists(qmlcFile));
    QFile::resize(qmlcFile, QFileInfo(qmlcFile).size() / 2);
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, testFileUrl("truncateTest.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
}

void tst_qmlcachegen::scriptStringCachegenInteraction()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif

    bool ok = generateCache(testFile("scriptstring.qml"));
    QVERIFY(ok);
    QQmlEngine engine;
    CleanlyLoadingComponent component(&engine, testFileUrl("scriptstring.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(!root.isNull(), qPrintable(component.errorString()));
    auto scripty = qobject_cast<ScriptStringProps *>(root.get());
    QVERIFY(scripty);

    QVERIFY(scripty->m_undef.isUndefinedLiteral());
    QVERIFY(scripty->m_nul.isNullLiteral());
    QCOMPARE(scripty->m_str.stringLiteral(), u"hello"_s);
    QCOMPARE(scripty->m_num.numberLiteral(&ok), 42);
    ok = false;
    scripty->m_bol.booleanLiteral(&ok);
    QVERIFY(ok);
}

void tst_qmlcachegen::saveableUnitPointer()
{
    QV4::CompiledData::Unit unit;
    unit.flags = QV4::CompiledData::Unit::StaticData | QV4::CompiledData::Unit::IsJavascript;
    const auto flags = unit.flags;

    QV4::CompiledData::SaveableUnitPointer pointer(&unit);

    QVERIFY(pointer.saveToDisk<char>([](const char *, quint32) { return true; }));
    QCOMPARE(unit.flags, flags);
}

void tst_qmlcachegen::aotstatsSerialization()
{
    const auto createEntry = [](const auto &d, const auto &n, const auto &e, const auto &l,
                                const auto &c, const auto &s) -> QQmlJS::AotStatsEntry {
        QQmlJS::AotStatsEntry entry;
        entry.codegenDuration = d;
        entry.functionName = n;
        entry.errorMessage = e;
        entry.line = l;
        entry.column = c;
        entry.codegenSuccessful = s;
        return entry;
    };

    const auto equal = [](const auto &e1, const auto &e2) -> bool {
        return e1.codegenDuration == e2.codegenDuration && e1.functionName == e2.functionName
                && e1.errorMessage == e2.errorMessage && e1.line == e2.line
                && e1.column == e2.column && e1.codegenSuccessful == e2.codegenSuccessful;
    };

    // AotStats
    // +-ModuleA
    // | +-File1
    // | | +-e1
    // | | +-e2
    // | +-File2
    // | | +-e3
    // +-ModuleB
    // | +-File3
    // | | +-e4

    QQmlJS::AotStats original;
    QQmlJS::AotStatsEntry e1 = createEntry(std::chrono::microseconds(500), "f1", "", 1, 1, true);
    QQmlJS::AotStatsEntry e2 = createEntry(std::chrono::microseconds(200), "f2", "err1", 5, 4, false);
    QQmlJS::AotStatsEntry e3 = createEntry(std::chrono::microseconds(750), "f3", "", 20, 4, true);
    QQmlJS::AotStatsEntry e4 = createEntry(std::chrono::microseconds(300), "f4", "err2", 5, 8, false);
    original.addEntry("ModuleA", "File1", e1);
    original.addEntry("ModuleA", "File1", e2);
    original.addEntry("ModuleA", "File2", e3);
    original.addEntry("ModuleB", "File3", e4);

    const auto parsed = QQmlJS::AotStats::fromJsonDocument(original.toJsonDocument());
    QCOMPARE(parsed.entries().size(), original.entries().size());

    const auto &parsedA = parsed.entries()["ModuleA"];
    const auto &originalA = original.entries()["ModuleA"];
    QCOMPARE(parsedA.size(), originalA.size());
    QCOMPARE(parsedA["File1"].size(), originalA["File1"].size());
    QVERIFY(equal(parsedA["File1"][0], originalA["File1"][0]));
    QVERIFY(equal(parsedA["File1"][1], originalA["File1"][1]));
    QCOMPARE(parsedA["File2"].size(), originalA["File2"].size());
    QVERIFY(equal(parsedA["File2"][0], originalA["File2"][0]));

    const auto &parsedB = parsed.entries()["ModuleB"];
    const auto &originalB = original.entries()["ModuleB"];
    QCOMPARE(parsedB.size(), originalB.size());
    QCOMPARE(parsedB["File3"].size(), originalB["File3"].size());
    QVERIFY(equal(parsedB["File3"][0], originalB["File3"][0]));
}

struct FunctionEntry
{
    QString name;
    QString errorMessage;
    bool codegenSuccessful;
};

void tst_qmlcachegen::aotstatsGeneration_data()
{
    QTest::addColumn<QString>("qmlFile");
    QTest::addColumn<QList<FunctionEntry>>("entries");

    QTest::addRow("clean") << "AotstatsClean.qml"
                           << QList<FunctionEntry>{ { "j", "", true }, { "s", "", true } };

    const QString fError = "function without return type annotation returns int. This may prevent "
                           "proper compilation to Cpp.";
    const QString sError = "method g cannot be resolved.";
    QTest::addRow("mixed") << "AotstatsMixed.qml"
                           << QList<FunctionEntry>{ { "i", "", true },
                                                    { "f", fError, false },
                                                    { "s", sError, false } };
}

void tst_qmlcachegen::aotstatsGeneration()
{
#if defined(QTEST_CROSS_COMPILED)
    QSKIP("Cannot call qmlcachegen on cross-compiled target.");
#endif
    QFETCH(QString, qmlFile);
    QFETCH(QList<FunctionEntry>, entries);

    QTemporaryDir dir;
    QProcess proc;
    proc.setProgram(QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath) + "/qmlcachegen"_L1);
    const QString cppOutput = dir.filePath(qmlFile + ".cpp");
    const QString aotstatsOutput = cppOutput + ".aotstats";
    proc.setArguments({ "--bare",
                        "--resource-path", "/cachegentest/data/aotstats/" + qmlFile,
                        "-i", testFile("aotstats/qmldir"),
                        "--resource", testFile("aotstats/cachegentest.qrc"),
                        "--dump-aot-stats",
                        "--module-id=Aotstats",
                        "-o", cppOutput,
                        testFile("aotstats/" + qmlFile) });
    proc.start();
    QVERIFY(proc.waitForFinished() && proc.exitStatus() == QProcess::NormalExit);

    QVERIFY(QFileInfo::exists(aotstatsOutput));
    QFile aotstatsFile(aotstatsOutput);
    QVERIFY(aotstatsFile.open(QIODevice::Text | QIODevice::ReadOnly));
    const auto document = QJsonDocument::fromJson(aotstatsFile.readAll());
    const auto aotstats = QQmlJS::AotStats::fromJsonDocument(document);
    QVERIFY(aotstats.entries().size() == 1);  // One module
    const auto &moduleEntries = aotstats.entries()["Aotstats"];
    QVERIFY(moduleEntries.size() == 1);     // Only one qml file was compiled
    const auto &fileEntries = moduleEntries[moduleEntries.keys().first()];

    for (const auto &entry : entries) {
        const auto it = std::find_if(fileEntries.cbegin(), fileEntries.cend(),
                                     [&](const auto &e) { return e.functionName == entry.name; });
        QVERIFY(it != fileEntries.cend());
        QVERIFY(it->codegenSuccessful == entry.codegenSuccessful);
        QVERIFY(it->errorMessage == entry.errorMessage);
    }
}

const QQmlScriptString &ScriptStringProps::undef() const
{
    return m_undef;
}



void ScriptStringProps::setUndef(const QQmlScriptString &newUndef)
{
    if (m_undef == newUndef)
        return;
    m_undef = newUndef;
    emit undefChanged();
}



const QQmlScriptString &ScriptStringProps::nul() const
{
    return m_nul;
}



void ScriptStringProps::setNul(const QQmlScriptString &newNul)
{
    if (m_nul == newNul)
        return;
    m_nul = newNul;
    emit nulChanged();
}



const QQmlScriptString &ScriptStringProps::str() const
{
    return m_str;
}



void ScriptStringProps::setStr(const QQmlScriptString &newStr)
{
    if (m_str == newStr)
        return;
    m_str = newStr;
    emit strChanged();
}



const QQmlScriptString &ScriptStringProps::num() const
{
    return m_num;
}



void ScriptStringProps::setNum(const QQmlScriptString &newNum)
{
    if (m_num == newNum)
        return;
    m_num = newNum;
    emit numChanged();
}



const QQmlScriptString &ScriptStringProps::bol() const
{
    return m_bol;
}



void ScriptStringProps::setBol(const QQmlScriptString &newBol)
{
    if (m_bol == newBol)
        return;
    m_bol = newBol;
    emit bolChanged();
}

QTEST_GUILESS_MAIN(tst_qmlcachegen)

#include "tst_qmlcachegen.moc"
