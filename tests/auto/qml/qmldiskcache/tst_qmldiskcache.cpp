// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <private/qv4compileddata_p.h>
#include <private/qv4compiler_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4codegen_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qv4executablecompilationunit_p.h>
#include <private/qqmlscriptdata_p.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlFileSelector>
#include <QThread>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDirIterator>

class tst_qmldiskcache: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadLocalAsFallback();
    void regenerateAfterChange();
    void registerImportForImplicitComponent();
    void basicVersionChecks();
    void recompileAfterChange();
    void recompileAfterDirectoryChange();
    void fileSelectors();
    void localAliases();
    void aliasToAlias();
    void cacheResources();
    void stableOrderOfDependentCompositeTypes();
    void singletonDependency();
    void cppRegisteredSingletonDependency();
    void cacheModuleScripts();
    void reuseStaticMappings();
    void invalidateSaveLoadCache();

    void inlineComponentDoesNotCauseConstantInvalidation_data();
    void inlineComponentDoesNotCauseConstantInvalidation();

private:
    QDir m_qmlCacheDirectory;
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
        QTRY_VERIFY_WITH_TIMEOUT(
                    status() == QQmlComponent::Ready || status() == QQmlComponent::Error,
                    32768);
    }
};

static void waitForFileSystem()
{
    // On macOS with HFS+ the precision of file times is measured in seconds, so to ensure that
    // the newly written file has a modification date newer than an existing cache file, we must
    // wait.
    // Similar effects of lacking precision have been observed on some Linux systems.
    static const bool fsHasSubSecondResolution = []() {
        QDateTime mtime = QFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).lastModified();
        // 1:1000 chance of a false negative
        return mtime.toMSecsSinceEpoch() % 1000;
    }();
    if (!fsHasSubSecondResolution)
        QThread::sleep(1);
}

struct TestCompiler
{
    TestCompiler(QQmlEngine *engine)
        : engine(engine)
        , tempDir()
        , currentMapping(nullptr)
    {
        init(tempDir.path());
    }

    void init(const QString &baseDirectory)
    {
        closeMapping();
        testFilePath = baseDirectory + QStringLiteral("/test.qml");
        cacheFilePath = QV4::ExecutableCompilationUnit::localCacheFilePath(
                QUrl::fromLocalFile(testFilePath));
        mappedFile.setFileName(cacheFilePath);
    }

    void reset()
    {
        closeMapping();
        engine->clearComponentCache();
        waitForFileSystem();
    }

    bool writeTestFile(const QByteArray &contents)
    {
        QFile f(testFilePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            lastErrorString = f.errorString();
            return false;
        }
        if (f.write(contents) != contents.size()) {
            lastErrorString = f.errorString();
            return false;
        }
        return true;
    }

    bool loadTestFile()
    {
        CleanlyLoadingComponent component(engine, testFilePath);
        if (!component.isReady()) {
            lastErrorString = component.errorString();
            return false;
        }
        return true;
    }

    bool compile(const QByteArray &contents)
    {
        reset();
        return writeTestFile(contents) && loadTestFile();
    }

    const QV4::CompiledData::Unit *mapUnit()
    {
        if (!mappedFile.open(QIODevice::ReadOnly)) {
            lastErrorString = mappedFile.errorString();
            return nullptr;
        }

        currentMapping = mappedFile.map(/*offset*/0, mappedFile.size());
        if (!currentMapping) {
            lastErrorString = mappedFile.errorString();
            return nullptr;
        }
        QV4::CompiledData::Unit *unitPtr;
        memcpy(&unitPtr, &currentMapping, sizeof(unitPtr));
        return unitPtr;
    }

    typedef void (*HeaderTweakFunction)(QV4::CompiledData::Unit *header);
    bool tweakHeader(HeaderTweakFunction function, const QString &newName)
    {
        closeMapping();

        const QString targetTestFilePath = tempDir.path() + "/" + newName;

        {
            QFile testFile(testFilePath);
            if (!testFile.copy(targetTestFilePath))
                return false;
        }

        const QString targetCacheFilePath = QV4::ExecutableCompilationUnit::localCacheFilePath(
                    QUrl::fromLocalFile(targetTestFilePath));

        QFile source(cacheFilePath);
        if (!source.copy(targetCacheFilePath))
            return false;

        if (!source.open(QIODevice::ReadOnly))
            return false;

        QFile target(targetCacheFilePath);
        if (!target.open(QIODevice::WriteOnly))
            return false;

        QV4::CompiledData::Unit header;
        if (source.read(reinterpret_cast<char *>(&header), sizeof(header)) != sizeof(header))
            return false;
        function(&header);

        return target.write(reinterpret_cast<const char *>(&header), sizeof(header))
                == sizeof(header);
    }

    bool verify(const QString &fileName = QString())
    {
        const QString path = fileName.isEmpty() ? testFilePath : tempDir.path() + "/" + fileName;

        QQmlRefPointer<QV4::ExecutableCompilationUnit> unit
                = QV4::ExecutableCompilationUnit::create();
        return unit->loadFromDisk(QUrl::fromLocalFile(path),
                                  QFileInfo(path).lastModified(), &lastErrorString);
    }

    quintptr unitData()
    {
        QQmlRefPointer<QV4::ExecutableCompilationUnit> unit
                = QV4::ExecutableCompilationUnit::create();
        return unit->loadFromDisk(QUrl::fromLocalFile(testFilePath),
                                  QFileInfo(testFilePath).lastModified(), &lastErrorString)
                ? quintptr(unit->unitData())
                : 0;
    }


    void closeMapping()
    {
        if (currentMapping) {
            mappedFile.unmap(currentMapping);
            currentMapping = nullptr;
        }
        mappedFile.close();
    }

    void clearCache(const QString &fileName = QString())
    {
        const QString path = fileName.isEmpty() ? testFilePath : tempDir.path() + "/" + fileName;
        closeMapping();
        QFile::remove(path);
    }

    QQmlEngine *engine;
    const QTemporaryDir tempDir;
    QString testFilePath;
    QString cacheFilePath;
    QString lastErrorString;
    QFile mappedFile;
    uchar *currentMapping;
};

void tst_qmldiskcache::initTestCase()
{
    qputenv("QML_FORCE_DISK_CACHE", "1");
    QStandardPaths::setTestModeEnabled(true);

    const QString cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_qmlCacheDirectory.setPath(cacheDirectory + QLatin1String("/qmlcache"));
    if (m_qmlCacheDirectory.exists())
        QVERIFY(m_qmlCacheDirectory.removeRecursively());
    QVERIFY(QDir::root().mkpath(m_qmlCacheDirectory.absolutePath()));
}

void tst_qmldiskcache::cleanupTestCase()
{
    m_qmlCacheDirectory.removeRecursively();
}

void tst_qmldiskcache::loadLocalAsFallback()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

    // Create an invalid side-by-side .qmlc
    {
        QFile f(testCompiler.tempDir.path() + "/test.qmlc");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QV4::CompiledData::Unit unit = {};
        memcpy(unit.magic, QV4::CompiledData::magic_str, sizeof(unit.magic));
        unit.version = QV4_DATA_STRUCTURE_VERSION;
        unit.qtVersion = QT_VERSION;
        unit.sourceTimeStamp = testCompiler.mappedFile.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch();
        unit.unitSize = ~0U;    // make the size a silly number
        // write something to the library hash that should cause it not to be loaded
        memset(unit.libraryVersionHash, 'z', sizeof(unit.libraryVersionHash));
        memset(unit.md5Checksum, 0, sizeof(unit.md5Checksum));

        // leave the other fields unset, since they don't matter

        f.write(reinterpret_cast<const char *>(&unit), sizeof(unit));
    }

    QQmlRefPointer<QV4::ExecutableCompilationUnit> unit = QV4::ExecutableCompilationUnit::create();
    bool loaded = unit->loadFromDisk(QUrl::fromLocalFile(testCompiler.testFilePath),
                                     QFileInfo(testCompiler.testFilePath).lastModified(),
                                     &testCompiler.lastErrorString);
    QVERIFY2(loaded, qPrintable(testCompiler.lastErrorString));
    QCOMPARE(unit->objectCount(), 1);
}

void tst_qmldiskcache::regenerateAfterChange()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

    {
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        const QV4::CompiledData::QmlUnit *qmlUnit = testUnit->qmlUnit();

        QCOMPARE(quint32(qmlUnit->nObjects), quint32(1));

        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(1));
        QCOMPARE(obj->bindingTable()->type(), QV4::CompiledData::Binding::Type_Script);
        QCOMPARE(quint32(obj->bindingTable()->value.compiledScriptIndex), quint32(0));

        QCOMPARE(quint32(testUnit->functionTableSize), quint32(1));

        const QV4::CompiledData::Function *bindingFunction = testUnit->functionAt(0);
        QCOMPARE(testUnit->stringAtInternal(bindingFunction->nameIndex), QString("expression for blah")); // check if we have the correct function
        QVERIFY(bindingFunction->codeSize > 0);
        QVERIFY(bindingFunction->codeOffset < testUnit->unitSize);
    }

    {
        const QByteArray newContents = QByteArrayLiteral("import QtQml 2.0\n"
                                                         "QtObject {\n"
                                                         "    property string blah: Qt.platform;\n"
                                                         "    property int secondProperty: 42;\n"
                                                         "}");

        QVERIFY2(testCompiler.compile(newContents), qPrintable(testCompiler.lastErrorString));
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        const QV4::CompiledData::QmlUnit *qmlUnit = testUnit->qmlUnit();

        QCOMPARE(quint32(qmlUnit->nObjects), quint32(1));

        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(2));
        QCOMPARE(obj->bindingTable()->type(), QV4::CompiledData::Binding::Type_Number);

        const QV4::Value value(testUnit->constants()[obj->bindingTable()->value.constantValueIndex]);
        QCOMPARE(value.doubleValue(), double(42));

        QCOMPARE(quint32(testUnit->functionTableSize), quint32(1));

        const QV4::CompiledData::Function *bindingFunction = testUnit->functionAt(0);
        QCOMPARE(testUnit->stringAtInternal(bindingFunction->nameIndex), QString("expression for blah")); // check if we have the correct function
        QVERIFY(bindingFunction->codeSize > 0);
        QVERIFY(bindingFunction->codeOffset < testUnit->unitSize);
    }
}

void tst_qmldiskcache::registerImportForImplicitComponent()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQuick 2.0\n"
                                                  "Loader {\n"
                                                   "    sourceComponent: Item {}\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
    {
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        const QV4::CompiledData::QmlUnit *qmlUnit = testUnit->qmlUnit();
        QCOMPARE(quint32(qmlUnit->nImports), quint32(2));
        QCOMPARE(testUnit->stringAtInternal(qmlUnit->importAt(0)->uriIndex), QStringLiteral("QtQuick"));

        QQmlType componentType = QQmlMetaType::qmlType(
                    &QQmlComponent::staticMetaObject, QStringLiteral("QML"),
                    QTypeRevision::fromVersion(1, 0));

        QCOMPARE(testUnit->stringAtInternal(qmlUnit->importAt(1)->uriIndex), QString(componentType.module()));
        QCOMPARE(testUnit->stringAtInternal(qmlUnit->importAt(1)->qualifierIndex), QStringLiteral("QML"));

        QCOMPARE(quint32(qmlUnit->nObjects), quint32(3));

        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(1));
        QCOMPARE(obj->bindingTable()->type(), QV4::CompiledData::Binding::Type_Object);

        const QV4::CompiledData::Object *implicitComponent = qmlUnit->objectAt(obj->bindingTable()->value.objectIndex);
        QCOMPARE(testUnit->stringAtInternal(implicitComponent->inheritedTypeNameIndex), QStringLiteral("QML.") + componentType.elementName());
    }
}

void tst_qmldiskcache::basicVersionChecks()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

        const QString qtVersionFile = QStringLiteral("qtversion.qml");
        QVERIFY(testCompiler.tweakHeader([](QV4::CompiledData::Unit *header) {
            header->qtVersion = 0;
        }, qtVersionFile));

        QVERIFY(!testCompiler.verify(qtVersionFile));
        QCOMPARE(testCompiler.lastErrorString, QString::fromUtf8("Qt version mismatch. Found 0 expected %1").arg(QT_VERSION, 0, 16));
        testCompiler.clearCache(qtVersionFile);
    }

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

        const QString versionFile = QStringLiteral("version.qml");
        QVERIFY(testCompiler.tweakHeader([](QV4::CompiledData::Unit *header) {
            header->version = 0;
        }, versionFile));

        QVERIFY(!testCompiler.verify(versionFile));
        QCOMPARE(testCompiler.lastErrorString, QString::fromUtf8("V4 data structure version mismatch. Found 0 expected %1").arg(QV4_DATA_STRUCTURE_VERSION, 0, 16));
        testCompiler.clearCache(versionFile);
    }
}

class TypeVersion1 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged);
public:


    int m_value = 0;
    int value() const { return m_value; }
    void setValue(int v) { m_value = v; emit valueChanged(); }

signals:
    void valueChanged();
};

// Same as TypeVersion1 except the property type changed!
class TypeVersion2 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged);
public:


    QString m_value;
    QString value() const { return m_value; }
    void setValue(QString v) { m_value = v; emit valueChanged(); }

signals:
    void valueChanged();
};

void tst_qmldiskcache::recompileAfterChange()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import TypeTest 1.0\n"
                                                  "TypeThatWillChange {\n"
                                                   "}");

    qmlRegisterType<TypeVersion1>("TypeTest", 1, 0, "TypeThatWillChange");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    QDateTime initialCacheTimeStamp = QFileInfo(testCompiler.cacheFilePath).lastModified();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<TypeVersion1> obj(qobject_cast<TypeVersion1*>(component.create()));
        QVERIFY(!obj.isNull());
        QCOMPARE(QFileInfo(testCompiler.cacheFilePath).lastModified(), initialCacheTimeStamp);
    }

    engine.clearComponentCache();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<TypeVersion1> obj(qobject_cast<TypeVersion1*>(component.create()));
        QVERIFY(!obj.isNull());
        QCOMPARE(QFileInfo(testCompiler.cacheFilePath).lastModified(), initialCacheTimeStamp);
    }

    engine.clearComponentCache();
    qmlClearTypeRegistrations();
    qmlRegisterType<TypeVersion2>("TypeTest", 1, 0, "TypeThatWillChange");

    waitForFileSystem();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<TypeVersion2> obj(qobject_cast<TypeVersion2*>(component.create()));
        QVERIFY(!obj.isNull());
        QVERIFY(QFileInfo(testCompiler.cacheFilePath).lastModified() > initialCacheTimeStamp);
    }
}

void tst_qmldiskcache::recompileAfterDirectoryChange()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    QVERIFY(QDir(testCompiler.tempDir.path()).mkdir("source1"));
    testCompiler.init(testCompiler.tempDir.path() + QLatin1String("/source1"));

    {
        const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                      "QtObject {\n"
                                                       "    property int blah: 42;\n"
                                                       "}");

        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
        const QV4::CompiledData::Unit *unit = testCompiler.mapUnit();
        QVERIFY(unit->sourceFileIndex != 0);
        const QString expectedPath = QUrl::fromLocalFile(testCompiler.testFilePath).toString();
        QCOMPARE(unit->stringAtInternal(unit->sourceFileIndex), expectedPath);
        testCompiler.closeMapping();
    }

    const QDateTime initialCacheTimeStamp = QFileInfo(testCompiler.cacheFilePath).lastModified();

    QDir(testCompiler.tempDir.path()).rename(QStringLiteral("source1"), QStringLiteral("source2"));
    waitForFileSystem();

    testCompiler.init(testCompiler.tempDir.path() + QLatin1String("/source2"));

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("blah").toInt(), 42);
    }

    QFile cacheFile(testCompiler.cacheFilePath);
    QVERIFY2(cacheFile.exists(), qPrintable(cacheFile.fileName()));
    QVERIFY(QFileInfo(testCompiler.cacheFilePath).lastModified() > initialCacheTimeStamp);
}

void tst_qmldiskcache::fileSelectors()
{
    QQmlEngine engine;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString testFilePath = tempDir.path() + "/test.qml";
    {
        QFile f(testFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 42 }"));
    }

    const QString selector = QStringLiteral("testSelector");
    const QString selectorPath = tempDir.path() + "/+" + selector;
    const QString selectedTestFilePath = selectorPath + "/test.qml";
    {
        QVERIFY(QDir::root().mkpath(selectorPath));
        QFile f(selectorPath + "/test.qml");
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 100 }"));
    }

    {
        QQmlComponent component(&engine, testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 42);

        QFile cacheFile(QV4::ExecutableCompilationUnit::localCacheFilePath(
                QUrl::fromLocalFile(testFilePath)));
        QVERIFY2(cacheFile.exists(), qPrintable(cacheFile.fileName()));
    }

    QQmlFileSelector qmlSelector(&engine);
    qmlSelector.setExtraSelectors(QStringList() << selector);

    engine.clearComponentCache();

    {
        QQmlComponent component(&engine, testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 100);

        QFile cacheFile(QV4::ExecutableCompilationUnit::localCacheFilePath(
                QUrl::fromLocalFile(selectedTestFilePath)));
        QVERIFY2(cacheFile.exists(), qPrintable(cacheFile.fileName()));
    }
}

void tst_qmldiskcache::localAliases()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                  "    id: root\n"
                                                  "    property int prop: 100\n"
                                                  "    property alias dummy1: root.prop\n"
                                                  "    property alias dummy2: root.prop\n"
                                                  "    property alias dummy3: root.prop\n"
                                                  "    property alias dummy4: root.prop\n"
                                                  "    property alias dummy5: root.prop\n"
                                                  "    property alias foo: root.prop\n"
                                                  "    property alias bar: root.foo\n"
                                                   "}");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("bar").toInt(), 100);
    }

    engine.clearComponentCache();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("bar").toInt(), 100);
    }
}

void tst_qmldiskcache::aliasToAlias()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral(R"(
        import QML
        QtObject {
            id: foo
            readonly property alias myAlias: bar.prop

            property QtObject o: QtObject {
                id: bar

                property QtObject o: QtObject {
                    id: baz
                    readonly property int value: 100
                }

                readonly property alias prop: baz.value
            }
        }
    )");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("myAlias").toInt(), 100);
    }

    engine.clearComponentCache();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("myAlias").toInt(), 100);
    }
}

static QSet<QString> entrySet(const QDir &dir)
{
    const auto &list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    return QSet<QString>(list.cbegin(), list.cend());
}

static QSet<QString> entrySet(const QDir &dir, const QStringList &filters)
{
    const auto &list = dir.entryList(filters);
    return QSet<QString>(list.cbegin(), list.cend());
}

void tst_qmldiskcache::cacheResources()
{
    const QSet<QString> existingFiles = entrySet(m_qmlCacheDirectory);

    QQmlEngine engine;

    {
        CleanlyLoadingComponent component(&engine, QUrl("qrc:/test.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 20);
    }

    const QSet<QString> entries = entrySet(m_qmlCacheDirectory).subtract(existingFiles);
    QCOMPARE(entries.size(), 1);

    QDateTime cacheFileTimeStamp;

    {
        QFile cacheFile(m_qmlCacheDirectory.absoluteFilePath(*entries.cbegin()));
        QVERIFY2(cacheFile.open(QIODevice::ReadOnly), qPrintable(cacheFile.errorString()));
        QV4::CompiledData::Unit unit;
        QVERIFY(cacheFile.read(reinterpret_cast<char *>(&unit), sizeof(unit)) == sizeof(unit));

        cacheFileTimeStamp = QFileInfo(cacheFile.fileName()).lastModified();

        QDateTime referenceTimeStamp = QFileInfo(":/test.qml").lastModified();
        if (!referenceTimeStamp.isValid())
            referenceTimeStamp = QFileInfo(QCoreApplication::applicationFilePath()).lastModified();
        QCOMPARE(qint64(unit.sourceTimeStamp), referenceTimeStamp.toMSecsSinceEpoch());
    }

    waitForFileSystem();

    {
        CleanlyLoadingComponent component(&engine, QUrl("qrc:///test.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 20);
    }

    {
        const QSet<QString> entries = entrySet(m_qmlCacheDirectory).subtract(existingFiles);
        QCOMPARE(entries.size(), 1);

        QCOMPARE(QFileInfo(m_qmlCacheDirectory.absoluteFilePath(*entries.cbegin())).lastModified().toMSecsSinceEpoch(),
                           cacheFileTimeStamp.toMSecsSinceEpoch());
    }
}

void tst_qmldiskcache::stableOrderOfDependentCompositeTypes()
{
    QQmlEngine engine;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    {
        const QString depFilePath = tempDir.path() + "/FirstDependentType.qml";
        QFile f(depFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 42 }"));
    }

    {
        const QString depFilePath = tempDir.path() + "/SecondDependentType.qml";
        QFile f(depFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 100 }"));
    }

    const QString testFilePath = tempDir.path() + "/main.qml";
    {
        QFile f(testFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject {\n"
                                  "    property QtObject dep1: FirstDependentType{}\n"
                                  "    property QtObject dep2: SecondDependentType{}\n"
                                  "    property int value: dep1.value + dep2.value\n"
                                  "}"));
    }

    QByteArray firstDependentTypeClassName;
    QByteArray secondDependentTypeClassName;

    {
        CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 142);

        firstDependentTypeClassName = qvariant_cast<QObject *>(obj->property("dep1"))->metaObject()->className();
        secondDependentTypeClassName = qvariant_cast<QObject *>(obj->property("dep2"))->metaObject()->className();
    }

    QVERIFY(firstDependentTypeClassName != secondDependentTypeClassName);
    QVERIFY2(firstDependentTypeClassName.contains("QMLTYPE"), firstDependentTypeClassName.constData());
    QVERIFY2(secondDependentTypeClassName.contains("QMLTYPE"), secondDependentTypeClassName.constData());

    const QString testFileCachePath = QV4::ExecutableCompilationUnit::localCacheFilePath(
            QUrl::fromLocalFile(testFilePath));
    QVERIFY(QFile::exists(testFileCachePath));
    QDateTime initialCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();

    engine.clearComponentCache();
    waitForFileSystem();

    // Creating the test component a second time should load it from the cache (same time stamp),
    // despite the class names of the dependent composite types differing.
    {
        CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 142);

        QVERIFY(qvariant_cast<QObject *>(obj->property("dep1"))->metaObject()->className() != firstDependentTypeClassName);
        QVERIFY(qvariant_cast<QObject *>(obj->property("dep2"))->metaObject()->className() != secondDependentTypeClassName);
    }

    {
        QVERIFY(QFile::exists(testFileCachePath));
        QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
        QCOMPARE(newCacheTimeStamp, initialCacheTimeStamp);
    }

    // Now change the first dependent QML type and see if we correctly re-generate the
    // caches.
    engine.clearComponentCache();
    waitForFileSystem();
    {
        const QString depFilePath = tempDir.path() + "/FirstDependentType.qml";
        QFile f(depFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 40 }"));
    }

    {
        CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 140);
    }

    {
        QVERIFY(QFile::exists(testFileCachePath));
        QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
        QVERIFY2(newCacheTimeStamp > initialCacheTimeStamp, qPrintable(newCacheTimeStamp.toString()));
    }
}

void tst_qmldiskcache::singletonDependency()
{
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 42 }");
    writeTempFile("qmldir", "singleton MySingleton 1.0 MySingleton.qml");
    const QString testFilePath = writeTempFile("main.qml", "import QtQml 2.0\nimport \".\"\nQtObject {\n"
                                                           "    property int value: MySingleton.value\n"
                                                           "}");

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 42);
    }

    const QString testFileCachePath = QV4::ExecutableCompilationUnit::localCacheFilePath(
            QUrl::fromLocalFile(testFilePath));
    QVERIFY(QFile::exists(testFileCachePath));
    QDateTime initialCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();

    engine.reset(new QQmlEngine);
    waitForFileSystem();

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 100 }");
    waitForFileSystem();

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 100);
    }

    {
        QVERIFY(QFile::exists(testFileCachePath));
        QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
        QVERIFY2(newCacheTimeStamp > initialCacheTimeStamp, qPrintable(newCacheTimeStamp.toString()));
    }
}

void tst_qmldiskcache::cppRegisteredSingletonDependency()
{
    qmlClearTypeRegistrations();
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 42 }");

    qmlRegisterSingletonType(QUrl::fromLocalFile(tempDir.path() + QLatin1String("/MySingleton.qml")), "CppRegisteredSingletonDependency", 1, 0, "Singly");

    const QString testFilePath = writeTempFile("main.qml", "import QtQml 2.0\nimport CppRegisteredSingletonDependency 1.0\nQtObject {\n"
                                                           "    function getValue() { return Singly.value; }\n"
                                                           "}");

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QVariant value;
        QVERIFY(QMetaObject::invokeMethod(obj.data(), "getValue", Q_RETURN_ARG(QVariant, value)));
        QCOMPARE(value.toInt(), 42);
    }

    const QString testFileCachePath = QV4::ExecutableCompilationUnit::localCacheFilePath(
            QUrl::fromLocalFile(testFilePath));
    QVERIFY(QFile::exists(testFileCachePath));
    QDateTime initialCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();

    engine.reset(new QQmlEngine);
    waitForFileSystem();

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 100 }");
    waitForFileSystem();

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());

        {
            QVERIFY(QFile::exists(testFileCachePath));
            QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
            QVERIFY2(newCacheTimeStamp > initialCacheTimeStamp, qPrintable(newCacheTimeStamp.toString()));
        }

        QVariant value;
        QVERIFY(QMetaObject::invokeMethod(obj.data(), "getValue", Q_RETURN_ARG(QVariant, value)));
        QCOMPARE(value.toInt(), 100);
    }
}

void tst_qmldiskcache::cacheModuleScripts()
{
    const QSet<QString> existingFiles = entrySet(m_qmlCacheDirectory);

    QQmlEngine engine;

    {
        CleanlyLoadingComponent component(&engine, QUrl("qrc:/importmodule.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QVERIFY(obj->property("ok").toBool());

        auto componentPrivate = QQmlComponentPrivate::get(&component);
        QVERIFY(componentPrivate);
        auto compilationUnit = componentPrivate->compilationUnit->dependentScripts.first()->compilationUnit();
        QVERIFY(compilationUnit);
        auto unitData = compilationUnit->unitData();
        QVERIFY(unitData);
        QVERIFY(unitData->flags & QV4::CompiledData::Unit::StaticData);
        QVERIFY(unitData->flags & QV4::CompiledData::Unit::IsESModule);
        QVERIFY(compilationUnit->backingFile);
    }

    const QSet<QString> entries = entrySet(m_qmlCacheDirectory, QStringList("*.mjsc"));

    QCOMPARE(entries.size(), 1);

    QDateTime cacheFileTimeStamp;

    {
        QFile cacheFile(m_qmlCacheDirectory.absoluteFilePath(*entries.cbegin()));
        QVERIFY2(cacheFile.open(QIODevice::ReadOnly), qPrintable(cacheFile.errorString()));
        QV4::CompiledData::Unit unit;
        QVERIFY(cacheFile.read(reinterpret_cast<char *>(&unit), sizeof(unit)) == sizeof(unit));

        QVERIFY(unit.flags & QV4::CompiledData::Unit::IsESModule);
    }
}

void tst_qmldiskcache::reuseStaticMappings()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    testCompiler.reset();
    QVERIFY(testCompiler.writeTestFile("import QtQml\nQtObject { objectName: 'foobar' }\n"));
    QVERIFY(testCompiler.loadTestFile());

    const quintptr data1 = testCompiler.unitData();
    QVERIFY(data1 != 0);
    QCOMPARE(testCompiler.unitData(), data1);

    testCompiler.reset();
    QVERIFY(testCompiler.loadTestFile());

    QCOMPARE(testCompiler.unitData(), data1);
}

class AParent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x MEMBER x)
public:
    AParent(QObject *parent = nullptr) : QObject(parent) {}
    int x = 25;
};

class BParent : public QObject
{
    Q_OBJECT

    // Insert y before x, to change the property index of x
    Q_PROPERTY(int y MEMBER y)

    Q_PROPERTY(int x MEMBER x)
public:
    BParent(QObject *parent = nullptr) : QObject(parent) {}
    int y = 13;
    int x = 25;
};

static QString writeTempFile(
        const QTemporaryDir &tempDir, const QString &fileName, const char *contents) {
    QFile f(tempDir.path() + '/' + fileName);
    const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    Q_ASSERT(ok);
    f.write(contents);
    return f.fileName();
};

void tst_qmldiskcache::invalidateSaveLoadCache()
{
    qmlRegisterType<AParent>("Base", 1, 0, "Parent");
    std::unique_ptr<QQmlEngine> e = std::make_unique<QQmlEngine>();

    // If you store a CU to a .qmlc file at run time, the .qmlc file will contain
    // alias entries with the encodedMetaPropertyIndex pre-resolved. That's in
    // contrast to .qmlc files generated ahead of time. Exploit that to cause
    // a need to recompile the file.

    QTemporaryDir tempDir;
    writeTempFile(
        tempDir, QLatin1String("B.qml"),
        R"(
            import QML
            QtObject {
                component C: QtObject {}
            }
        )");

    const QString fileName = writeTempFile(
        tempDir, QLatin1String("a.qml"),
        R"(
            import Base
            Parent {
                id: self
                property alias z: self.x
                component C: Parent {}
                property C c: C {}
                property B.C d: B.C {}
            }
        )");
    const QUrl url = QUrl::fromLocalFile(fileName);
    waitForFileSystem();

    {
        QQmlComponent a(e.get(), url);
        QVERIFY2(a.isReady(), qPrintable(a.errorString()));
        QScopedPointer<QObject> ao(a.create());
        QVERIFY(!ao.isNull());
        AParent *ap = qobject_cast<AParent *>(ao.data());
        QCOMPARE(ap->property("z").toInt(), ap->x);
    }

    QString errorString;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> oldUnit
            = QV4::ExecutableCompilationUnit::create();
    QVERIFY2(oldUnit->loadFromDisk(url, QFileInfo(fileName).lastModified(), &errorString), qPrintable(errorString));

    // Produce a checksum mismatch.
    e->clearComponentCache();
    qmlClearTypeRegistrations();
    qmlRegisterType<BParent>("Base", 1, 0, "Parent");
    e = std::make_unique<QQmlEngine>();

    {
        QQmlComponent b(e.get(), url);
        QVERIFY2(b.isReady(), qPrintable(b.errorString()));
        QScopedPointer<QObject> bo(b.create());
        QVERIFY(!bo.isNull());
        BParent *bp = qobject_cast<BParent *>(bo.data());
        QCOMPARE(bp->property("z").toInt(), bp->x);
    }

    // Make it recompile again. If we ever get rid of the metaobject indices in compilation units,
    // the above test will not test the save/load cache anymore. Therefore, in order to make really
    // sure that we get a new CU that invalidates the save/load cache, modify the file in place.

    e->clearComponentCache();
    {
        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Append);
        file.write(" ");
    }
    waitForFileSystem();

    {
        QQmlComponent b(e.get(), url);
        QVERIFY2(b.isReady(), qPrintable(b.errorString()));
        QScopedPointer<QObject> bo(b.create());
        QVERIFY(!bo.isNull());
        BParent *bp = qobject_cast<BParent *>(bo.data());
        QCOMPARE(bp->property("z").toInt(), bp->x);
    }

    // Verify that the mapped unit data is actually different now.
    // The cache should have been invalidated after all.
    // So, now we should be able to load a freshly written CU.

    QQmlRefPointer<QV4::ExecutableCompilationUnit> unit
            = QV4::ExecutableCompilationUnit::create();
    QVERIFY2(unit->loadFromDisk(url, QFileInfo(fileName).lastModified(), &errorString), qPrintable(errorString));

    QVERIFY(unit->unitData() != oldUnit->unitData());
}

void tst_qmldiskcache::inlineComponentDoesNotCauseConstantInvalidation_data()
{
    QTest::addColumn<QByteArray>("code");

    QTest::addRow("simple") << QByteArray(R"(
        import QtQml
        QtObject {
            component Test: QtObject {
                property int i: 28
            }
            property Test test: Test {
                objectName: "foobar"
            }
            property int k: test.i
        }
    )");

    QTest::addRow("with function") << QByteArray(R"(
        import QtQml
        QtObject {
            component Test : QtObject {
                id: self
                property int i: 2
                property alias j: self.i
            }
            property Test test: Test {
                function updateValue() {}
                objectName: 'foobar'
                j: 28
            }
            property int k: test.j
        }
    )");

    QTest::addRow("in nested") << QByteArray(R"(
        import QtQuick
        Item {
            Item {
                component Line: Item {
                    property alias endY: pathLine.y
                    Item {
                        Item {
                            id: pathLine
                        }
                    }
                }
            }
            Line {
                id: primaryLine
                endY: 28
            }
            property int k: primaryLine.endY
        }
    )");

    QTest::addRow("with revision") << QByteArray(R"(
        import QtQuick
        ListView {
            Item {
                id: scrollBar
            }
            delegate: Image {
                mipmap: true
            }
            Item {
                id: refreshNodesIndicator
            }
            property int k: delegate.createObject().mipmap ? 28 : 4
        }
    )");
}

void tst_qmldiskcache::inlineComponentDoesNotCauseConstantInvalidation()
{
    QFETCH(QByteArray, code);

    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    auto check = [&](){
        QQmlComponent c(&engine, QUrl::fromLocalFile(testCompiler.testFilePath));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
        QCOMPARE(o->property("k"), QVariant::fromValue<int>(28));
    };

    testCompiler.reset();
    QVERIFY(testCompiler.writeTestFile(code));

    QVERIFY(testCompiler.loadTestFile());

    const quintptr data1 = testCompiler.unitData();
    QVERIFY(data1 != 0);
    QCOMPARE(testCompiler.unitData(), data1);
    check();

    engine.clearComponentCache();

    // inline component does not invalidate cache
    QVERIFY(testCompiler.loadTestFile());
    QCOMPARE(testCompiler.unitData(), data1);
    check();

    testCompiler.reset();
    QVERIFY(testCompiler.writeTestFile(R"(
        import QtQml
        QtObject {
            component Test : QtObject {
                property double d: 2
            }
            property Test test: Test {
                objectName: 'foobar'
            }
        })"));
    QVERIFY(testCompiler.loadTestFile());
    const quintptr data2 = testCompiler.unitData();
    QVERIFY(data2);
    QVERIFY(data1 != data2);
}

QTEST_MAIN(tst_qmldiskcache)

#include "tst_qmldiskcache.moc"
