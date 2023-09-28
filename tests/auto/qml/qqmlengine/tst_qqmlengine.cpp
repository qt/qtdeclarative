// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlEngine>
#include <QQmlContext>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QDir>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QDebug>
#include <QBuffer>
#include <QCryptographicHash>
#include <QQmlComponent>
#include <QQmlNetworkAccessManagerFactory>
#include <QQmlExpression>
#include <QQmlIncubationController>
#include <QTemporaryDir>
 #include <QQmlEngineExtensionPlugin>
#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <QQmlAbstractUrlInterceptor>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include "declarativelyregistered.h"

Q_IMPORT_QML_PLUGIN(OnlyDeclarativePlugin)

class tst_qqmlengine : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlengine() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void rootContext();
    void networkAccessManager();
    void synchronousNetworkAccessManager();
    void baseUrl();
    void contextForObject();
    void offlineStoragePath();
    void offlineDatabaseStoragePath();
    void clearComponentCache();
    void trimComponentCache();
    void trimComponentCache_data();
    void clearSingletons();
    void repeatedCompilation();
    void failedCompilation();
    void failedCompilation_data();
    void outputWarningsToStandardError();
    void objectOwnership();
    void multipleEngines();
    void qtqmlModule_data();
    void qtqmlModule();
    void urlInterceptor_data();
    void urlInterceptor();
    void qmlContextProperties();
    void testGCCorruption();
    void testGroupedPropertyRevisions();
    void componentFromEval();
    void qrcUrls();
    void cppSignalAndEval();
    void singletonInstance();
    void aggressiveGc();
    void cachedGetterLookup_qtbug_75335();
    void createComponentOnSingletonDestruction();
    void uiLanguage();
    void markCurrentFunctionAsTranslationBinding();
    void executeRuntimeFunction();
    void captureQProperty();
    void listWrapperAsListReference();
    void attachedObjectAsObject();
    void listPropertyAsQJSValue();
    void stringToColor();
    void qobjectToString();
    void qtNamespaceInQtObject();
    void nativeModuleImport();
    void lockedRootObject();
    void crossReferencingSingletonsDeletion();
    void bindingInstallUseAfterFree();

public slots:
    QObject *createAQObjectForOwnershipTest ()
    {
        static QObject *ptr = new QObject();
        return ptr;
    }

private:
    QTemporaryDir m_tempDir;
};

class ObjectCaller : public QObject
{
    Q_OBJECT

signals:
    void doubleReply(const double a);
};

class CppSingleton : public QObject {
    Q_OBJECT
public:
    static uint instantiations;
    uint id = 0;

    CppSingleton() : id(++instantiations) {}

    static QObject *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        return new CppSingleton();
    }
};

uint CppSingleton::instantiations = 0;

class JsSingleton : public QObject {
    Q_OBJECT
public:
    static uint instantiations;
    uint id = 0;

    JsSingleton() : id(++instantiations) {}

    static QJSValue create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        QJSValue value = jsEngine->newQObject(new JsSingleton());
        return value;
    }
};

uint JsSingleton::instantiations = 0;

void tst_qqmlengine::initTestCase()
{
    QVERIFY2(m_tempDir.isValid(), qPrintable(m_tempDir.errorString()));
    QQmlDataTest::initTestCase();
}

void tst_qqmlengine::rootContext()
{
    QQmlEngine engine;

    QVERIFY(engine.rootContext());

    QCOMPARE(engine.rootContext()->engine(), &engine);
    QVERIFY(!engine.rootContext()->parentContext());
}

class NetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    NetworkAccessManagerFactory() : manager(nullptr) {}

    QNetworkAccessManager *create(QObject *parent) override {
        manager = new QNetworkAccessManager(parent);
        return manager;
    }

    QNetworkAccessManager *manager;
};

void tst_qqmlengine::networkAccessManager()
{
    QQmlEngine *engine = new QQmlEngine;

    // Test QQmlEngine created manager
    QPointer<QNetworkAccessManager> manager = engine->networkAccessManager();
    QVERIFY(manager != nullptr);
    delete engine;

    // Test factory created manager
    engine = new QQmlEngine;
    NetworkAccessManagerFactory factory;
    engine->setNetworkAccessManagerFactory(&factory);
    QCOMPARE(engine->networkAccessManagerFactory(), &factory);
    QNetworkAccessManager *engineNam = engine->networkAccessManager(); // calls NetworkAccessManagerFactory::create()
    QCOMPARE(engineNam, factory.manager);
    delete engine;
}

class ImmediateReply : public QNetworkReply {

    Q_OBJECT

public:
    ImmediateReply() {
        setFinished(true);
    }
    qint64 readData(char* , qint64 ) override {
        return 0;
    }
    void abort() override { }
};

class ImmediateManager : public QNetworkAccessManager {

    Q_OBJECT

public:
    ImmediateManager(QObject *parent = nullptr) : QNetworkAccessManager(parent) {
    }

    QNetworkReply *createRequest(Operation, const QNetworkRequest & , QIODevice * outgoingData = nullptr) override
    {
        Q_UNUSED(outgoingData);
        return new ImmediateReply;
    }
};

class ImmediateFactory : public QQmlNetworkAccessManagerFactory {

public:
    QNetworkAccessManager *create(QObject *) override
    { return new ImmediateManager; }
};

void tst_qqmlengine::synchronousNetworkAccessManager()
{
    ImmediateFactory factory;
    QQmlEngine engine;
    engine.setNetworkAccessManagerFactory(&factory);
    QQmlComponent c(&engine, QUrl("myScheme://test.qml"));
    // reply is finished, so should not be in loading state.
    QVERIFY(!c.isLoading());
}


void tst_qqmlengine::baseUrl()
{
    QQmlEngine engine;

    QUrl cwd = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());

    QCOMPARE(engine.baseUrl(), cwd);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd.resolved(QUrl("main.qml")));

    QDir dir = QDir::current();
    dir.cdUp();
    QVERIFY(dir != QDir::current());
    QDir::setCurrent(dir.path());
    QCOMPARE(QDir::current(), dir);

    QUrl cwd2 = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
    QCOMPARE(engine.baseUrl(), cwd2);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd2.resolved(QUrl("main.qml")));

    engine.setBaseUrl(cwd);
    QCOMPARE(engine.baseUrl(), cwd);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd.resolved(QUrl("main.qml")));


    const QString testPath = QDir::currentPath() + QLatin1String("/");
    const QString rootPath = QDir::rootPath();
    engine.setBaseUrl(QUrl());

    // Check that baseUrl returns a url to a localFile
    QCOMPARE(engine.baseUrl().toLocalFile(), testPath);

    QDir::setCurrent(QDir::rootPath());

    // Make sure this also works when in the rootPath
    QCOMPARE(engine.baseUrl().toLocalFile(), rootPath);
}

void tst_qqmlengine::contextForObject()
{
    QQmlEngine *engine = new QQmlEngine;

    // Test null-object
    QVERIFY(!QQmlEngine::contextForObject(nullptr));

    // Test an object with no context
    QObject object;
    QVERIFY(!QQmlEngine::contextForObject(&object));

    // Test setting null-object
    QQmlEngine::setContextForObject(nullptr, engine->rootContext());

    // Test setting null-context
    QQmlEngine::setContextForObject(&object, nullptr);

    // Test setting context
    QQmlEngine::setContextForObject(&object, engine->rootContext());
    QCOMPARE(QQmlEngine::contextForObject(&object), engine->rootContext());

    QQmlContext context(engine->rootContext());

    // Try changing context
    QTest::ignoreMessage(QtWarningMsg, "QQmlEngine::setContextForObject(): Object already has a QQmlContext");
    QQmlEngine::setContextForObject(&object, &context);
    QCOMPARE(QQmlEngine::contextForObject(&object), engine->rootContext());

    // Delete context
    delete engine; engine = nullptr;
    QVERIFY(!QQmlEngine::contextForObject(&object));
}

void tst_qqmlengine::offlineStoragePath()
{
    // Without these set, QDesktopServices::storageLocation returns
    // strings with extra "//" at the end. We set them to ignore this problem.
    qApp->setApplicationName("tst_qqmlengine");
    qApp->setOrganizationName("QtProject");
    qApp->setOrganizationDomain("www.qt-project.org");

    QQmlEngine engine;

    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QCOMPARE(dataLocation.isEmpty(), engine.offlineStoragePath().isEmpty());

    QDir dir(dataLocation);
    dir.mkpath("QML");
    dir.cd("QML");
    dir.mkpath("OfflineStorage");
    dir.cd("OfflineStorage");

    QCOMPARE(QDir::fromNativeSeparators(engine.offlineStoragePath()), dir.path());

    QSignalSpy offlineStoragePathSpy(&engine, &QQmlEngine::offlineStoragePathChanged);
    engine.setOfflineStoragePath(QDir::homePath());
    QCOMPARE(offlineStoragePathSpy.size(), 1);
    QCOMPARE(engine.offlineStoragePath(), QDir::homePath());
}

void tst_qqmlengine::offlineDatabaseStoragePath()
{
    // Without these set, QDesktopServices::storageLocation returns
    // strings with extra "//" at the end. We set them to ignore this problem.
    qApp->setApplicationName("tst_qqmlengine");
    qApp->setOrganizationName("QtProject");
    qApp->setOrganizationDomain("www.qt-project.org");

    QQmlEngine engine;
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString databaseName = QLatin1String("foo");
    QString databaseLocation = engine.offlineStorageDatabaseFilePath(databaseName);
    QCOMPARE(dataLocation.isEmpty(), databaseLocation.isEmpty());

    QDir dir(dataLocation);
    dir.mkpath("QML");
    dir.cd("QML");
    dir.mkpath("OfflineStorage");
    dir.cd("OfflineStorage");
    dir.mkpath("Databases");
    dir.cd("Databases");
    QCOMPARE(QFileInfo(databaseLocation).dir().path(), dir.path());

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(databaseName.toUtf8());
    QCOMPARE(databaseLocation, QDir::toNativeSeparators(dir.filePath(QLatin1String(md5.result().toHex()))));
}

void tst_qqmlengine::clearComponentCache()
{
    QQmlEngine engine;

    const QString fileName = m_tempDir.filePath(QStringLiteral("temp.qml"));
    const QUrl fileUrl = QUrl::fromLocalFile(fileName);

    // Create original qml file
    {
        QFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("import QtQuick 2.0\nQtObject {\nproperty int test: 10\n}\n");
        file.close();
    }

    // Test "test" property
    {
        QQmlComponent component(&engine, fileUrl);
        QObject *obj = component.create();
        QVERIFY(obj != nullptr);
        QCOMPARE(obj->property("test").toInt(), 10);
        delete obj;
    }

    // Modify qml file
    {
        // On macOS with HFS+ the precision of file times is measured in seconds, so to ensure that
        // the newly written file has a modification date newer than an existing cache file, we must
        // wait.
        // Similar effects of lacking precision have been observed on some Linux systems.
        QThread::sleep(1);

        QFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("import QtQuick 2.0\nQtObject {\nproperty int test: 11\n}\n");
        file.close();
    }

    // Test cache hit
    {
        QQmlComponent component(&engine, fileUrl);
        QObject *obj = component.create();
        QVERIFY(obj != nullptr);
        QCOMPARE(obj->property("test").toInt(), 10);
        delete obj;
    }

    // Clear cache
    engine.clearComponentCache();

    // Test cache refresh
    {
        QQmlComponent component(&engine, fileUrl);
        QObject *obj = component.create();
        QVERIFY(obj != nullptr);
        QCOMPARE(obj->property("test").toInt(), 11);
        delete obj;
    }

    // Regular Synchronous loading will leave us with an event posted
    // to the gui thread and an extra refcount that will only be dropped after the
    // event delivery. Call sendPostedEvents() to get rid of it so that
    // the temporary directory can be removed.
    QCoreApplication::sendPostedEvents();
}

struct ComponentCacheFunctions : public QObject, public QQmlIncubationController
{
    Q_OBJECT
public:
    QQmlEngine *engine;

    ComponentCacheFunctions(QQmlEngine &e) : engine(&e) {}

    Q_INVOKABLE void trim()
    {
        // Wait for any pending deletions to occur
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();

        // There might be JS function objects around that hold a last ref to the compilation unit that's
        // keeping the type compilation data (CompilationUnit) around. Let's collect them as well so that
        // trim works well.
        engine->collectGarbage();

        engine->trimComponentCache();
    }

    Q_INVOKABLE bool isTypeLoaded(QString file)
    {
        return QQmlEnginePrivate::get(engine)->isTypeLoaded(tst_qqmlengine::instance()->testFileUrl(file));
    }

    Q_INVOKABLE bool isScriptLoaded(QString file)
    {
        return QQmlEnginePrivate::get(engine)->isScriptLoaded(tst_qqmlengine::instance()->testFileUrl(file));
    }

    Q_INVOKABLE void beginIncubation()
    {
        startTimer(0);
    }

    Q_INVOKABLE void waitForIncubation()
    {
        while (incubatingObjectCount() > 0) {
            QCoreApplication::processEvents();
        }
    }

private:
    void timerEvent(QTimerEvent *) override
    {
        incubateFor(1000);
    }
};

void tst_qqmlengine::trimComponentCache()
{
    QFETCH(QString, file);

    QQmlEngine engine;
    ComponentCacheFunctions componentCache(engine);
    engine.setIncubationController(&componentCache);

    QQmlComponent component(&engine, testFileUrl(file));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {"componentCache", QVariant::fromValue(&componentCache)}
    }));
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("success").toBool(), true);
}

void tst_qqmlengine::trimComponentCache_data()
{
    QTest::addColumn<QString>("file");

    // The various tests here are for two types of components: those that are
    // empty apart from their inherited elements, and those that define new properties.
    // For each there are five types of composition: extension, aggregation,
    // aggregation via component, property and object-created-via-transient-component.
    foreach (const QString &test, (QStringList() << "EmptyComponent"
                                                 << "VMEComponent"
                                                 << "EmptyExtendEmptyComponent"
                                                 << "VMEExtendEmptyComponent"
                                                 << "EmptyExtendVMEComponent"
                                                 << "VMEExtendVMEComponent"
                                                 << "EmptyAggregateEmptyComponent"
                                                 << "VMEAggregateEmptyComponent"
                                                 << "EmptyAggregateVMEComponent"
                                                 << "VMEAggregateVMEComponent"
                                                 << "EmptyPropertyEmptyComponent"
                                                 << "VMEPropertyEmptyComponent"
                                                 << "EmptyPropertyVMEComponent"
                                                 << "VMEPropertyVMEComponent"
                                                 << "VMETransientEmptyComponent"
                                                 << "VMETransientVMEComponent")) {
        // For these cases, we first test that the component instance keeps the components
        // referenced, and then that the instantiated object keeps the components referenced
        for (int i = 1; i <= 2; ++i) {
            QString name(QString("%1-%2").arg(test).arg(i));
            QString file(QString("test%1.%2.qml").arg(test).arg(i));
            QTest::newRow(name.toLatin1().constData()) << file;
        }
    }

    // Test that a transient component is correctly referenced
    QTest::newRow("TransientComponent-1") << "testTransientComponent.1.qml";
    QTest::newRow("TransientComponent-2") << "testTransientComponent.2.qml";

    // Test that components can be reloaded after unloading
    QTest::newRow("ReloadComponent") << "testReloadComponent.qml";

    // Test that components are correctly referenced when dynamically loaded
    QTest::newRow("LoaderComponent") << "testLoaderComponent.qml";

    // Test that components are correctly referenced when incubated
    QTest::newRow("IncubatedComponent") << "testIncubatedComponent.qml";

    // Test that a top-level omponents is correctly referenced
    QTest::newRow("TopLevelComponent") << "testTopLevelComponent.qml";

    // TODO:
    // Test that scripts are unloaded when no longer referenced
    QTest::newRow("ScriptComponent") << "testScriptComponent.qml";
}

static QJSValue createValueSingleton(QQmlEngine *, QJSEngine *) { return 13u; }

void tst_qqmlengine::clearSingletons()
{
    ObjectCaller objectCaller1;
    ObjectCaller objectCaller2;
    const int cppInstance = qmlRegisterSingletonInstance(
                "ClearSingletons", 1, 0, "CppInstance", &objectCaller1);

#if QT_DEPRECATED_SINCE(6, 3)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QQmlPrivate::SingletonFunctor deprecatedSingletonFunctor;
    deprecatedSingletonFunctor.m_object = &objectCaller2;
    const int deprecatedCppInstance = qmlRegisterSingletonType<ObjectCaller>(
                "ClearSingletons", 1, 0, "DeprecatedCppInstance", deprecatedSingletonFunctor);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 3)

    const int cppFactory = qmlRegisterSingletonType<CppSingleton>(
                "ClearSingletons", 1, 0, "CppFactory", &CppSingleton::create);
    const int jsValue = qmlRegisterSingletonType(
                "ClearSingletons", 1, 0, "JsValue", &createValueSingleton);
    const int jsObject = qmlRegisterSingletonType(
                "ClearSingletons", 1, 0, "JsObject", &JsSingleton::create);
    const int qmlObject = qmlRegisterSingletonType(
                testFileUrl("QmlSingleton.qml"), "ClearSingletons", 1, 0, "QmlSingleton");

    QQmlEngine engine;

    QCOMPARE(engine.singletonInstance<ObjectCaller *>(cppInstance), &objectCaller1);
#if QT_DEPRECATED_SINCE(6, 3)
    QCOMPARE(engine.singletonInstance<ObjectCaller *>(deprecatedCppInstance), &objectCaller2);
#endif
    const CppSingleton *oldCppSingleton = engine.singletonInstance<CppSingleton *>(cppFactory);
    QVERIFY(oldCppSingleton != nullptr);
    const uint oldCppSingletonId = oldCppSingleton->id;
    QVERIFY(oldCppSingletonId > 0);
    QCOMPARE(CppSingleton::instantiations, oldCppSingletonId);
    QCOMPARE(engine.singletonInstance<QJSValue>(jsValue).toUInt(), 13u);
    const JsSingleton *oldJsSingleton = engine.singletonInstance<JsSingleton *>(jsObject);
    QVERIFY(oldJsSingleton != nullptr);
    const uint oldJsSingletonId = oldJsSingleton->id;
    const QObject *oldQmlSingleton = engine.singletonInstance<QObject *>(qmlObject);
    QVERIFY(oldQmlSingleton != nullptr);

    QQmlComponent c(&engine);
    c.setData("import ClearSingletons\n"
              "import QtQml\n"
              "QtObject {\n"
              "    property QtObject a: CppInstance\n"
#if QT_DEPRECATED_SINCE(6, 3)
              "    property QtObject f: DeprecatedCppInstance\n"
#endif
              "    property QtObject b: CppFactory\n"
              "    property int c: JsValue\n"
              "    property QtObject d: JsObject\n"
              "    property QtObject e: QmlSingleton\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> singletonUser(c.create());
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("a")), &objectCaller1);
#if QT_DEPRECATED_SINCE(6, 3)
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("f")), &objectCaller2);
#endif
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("b")), oldCppSingleton);
    QCOMPARE(singletonUser->property("c").toUInt(), 13u);
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("d")), oldJsSingleton);
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("e")), oldQmlSingleton);

    engine.clearSingletons();
    QCOMPARE(CppSingleton::instantiations, oldCppSingletonId);

    QCOMPARE(engine.singletonInstance<ObjectCaller *>(cppInstance), &objectCaller1);

#if QT_DEPRECATED_SINCE(6, 3)
    // Singleton instances created with previous versions of qmlRegisterSingletonInstance()
    // are lost and cannot be accessed anymore. We can only call their synthesized factory
    // functions once. This is not a big problem as you have to recompile in order to use
    // clearSingletons() anyway.
    QTest::ignoreMessage(
                QtWarningMsg,
                "<Unknown File>: Singleton registered by registerSingletonInstance "
                "must only be accessed from one engine");
    QTest::ignoreMessage(
                QtCriticalMsg,
                "<Unknown File>: qmlRegisterSingletonType(): \"DeprecatedCppInstance\" is not "
                "available because the callback function returns a null pointer.");
    QCOMPARE(engine.singletonInstance<ObjectCaller *>(deprecatedCppInstance), nullptr);
#endif

    QCOMPARE(CppSingleton::instantiations, oldCppSingletonId);
    const CppSingleton *newCppSingleton = engine.singletonInstance<CppSingleton *>(cppFactory);
    QVERIFY(newCppSingleton != nullptr); // The pointer may be the same as the old one
    QCOMPARE(CppSingleton::instantiations, oldCppSingletonId + 1);
    QCOMPARE(newCppSingleton->id, CppSingleton::instantiations);
    QCOMPARE(engine.singletonInstance<QJSValue>(jsValue).toUInt(), 13u);
    const JsSingleton *newJsSingleton = engine.singletonInstance<JsSingleton *>(jsObject);
    QVERIFY(newJsSingleton != nullptr);
    QVERIFY(newJsSingleton->id != oldJsSingletonId);
    const QObject *newQmlSingleton = engine.singletonInstance<QObject *>(qmlObject);
    QVERIFY(newQmlSingleton != nullptr);
    QVERIFY(newQmlSingleton != oldQmlSingleton);

    // Holding on to an old singleton instance is OK. We don't delete those.
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("a")), &objectCaller1);
#if QT_DEPRECATED_SINCE(6, 3)
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("f")), &objectCaller2);
#endif

    // Deleting the singletons created by factories has cleared their references in QML.
    // We don't renew them as the point of clearing the singletons is not having any
    // singletons left afterwards.
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("b")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("d")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(singletonUser->property("e")), nullptr);

    // Value types are unaffected as they are copied.
    QCOMPARE(singletonUser->property("c").toUInt(), 13u);
}

void tst_qqmlengine::repeatedCompilation()
{
    QQmlEngine engine;

    for (int i = 0; i < 100; ++i) {
        engine.collectGarbage();
        engine.trimComponentCache();

        QQmlComponent component(&engine, testFileUrl("repeatedCompilation.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QCOMPARE(object->property("success").toBool(), true);
    }
}

void tst_qqmlengine::failedCompilation()
{
    QFETCH(QString, file);

    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl(file));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QQmlComponent: Component is not ready");
    QVERIFY(!component.isReady());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object.isNull());

    engine.collectGarbage();
    engine.trimComponentCache();
    engine.clearComponentCache();
}

void tst_qqmlengine::failedCompilation_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("Invalid URL") << "failedCompilation.does.not.exist.qml";
    QTest::newRow("Invalid content") << "failedCompilation.1.qml";
}

void tst_qqmlengine::outputWarningsToStandardError()
{
    QQmlEngine engine;

    QCOMPARE(engine.outputWarningsToStandardError(), true);

    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0; QtObject { property int a: undefined }", QUrl());

    QVERIFY(c.isReady());

    QQmlTestMessageHandler messageHandler;

    QScopedPointer<QObject> o(c.create());

    QVERIFY(o != nullptr);
    o.reset();

    QCOMPARE(messageHandler.messages().size(), 1);
    QCOMPARE(messageHandler.messages().at(0), QLatin1String("<Unknown File>:1:32: Unable to assign [undefined] to int"));
    messageHandler.clear();

    engine.setOutputWarningsToStandardError(false);
    QCOMPARE(engine.outputWarningsToStandardError(), false);

    o.reset(c.create());

    QVERIFY(o != nullptr);
    o.reset();

    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));
}

void tst_qqmlengine::objectOwnership()
{
    {
    QCOMPARE(QQmlEngine::objectOwnership(nullptr), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(nullptr, QQmlEngine::JavaScriptOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(nullptr), QQmlEngine::CppOwnership);
    }

    {
    QObject o;
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::CppOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::JavaScriptOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::JavaScriptOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::CppOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    }

    {
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0; QtObject { property QtObject object: QtObject {} }", QUrl());

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(QQmlEngine::objectOwnership(o.data()), QQmlEngine::CppOwnership);

    QObject *o2 = qvariant_cast<QObject *>(o->property("object"));
    QCOMPARE(QQmlEngine::objectOwnership(o2), QQmlEngine::JavaScriptOwnership);

    o.reset();
    }
    {
        QObject *ptr = createAQObjectForOwnershipTest();
        QSignalSpy spy(ptr, SIGNAL(destroyed()));
        {
            QQmlEngine engine;
            QQmlComponent c(&engine);
            QQmlEngine::setObjectOwnership(ptr, QQmlEngine::JavaScriptOwnership);
            c.setData("import QtQuick 2.0; Item { required property QtObject test; property int data: test.createAQObjectForOwnershipTest() ? 0 : 1 }", QUrl());
            QVERIFY(c.isReady());
            QScopedPointer<QObject> o(
                        c.createWithInitialProperties({{"test", QVariant::fromValue(this)}}));
            QVERIFY(o != nullptr);
        }
        QTRY_VERIFY(spy.size());
    }
    {
        QObject *ptr = new QObject();
        QSignalSpy spy(ptr, SIGNAL(destroyed()));
        {
            QQmlEngine engine;
            QQmlComponent c(&engine);
            QQmlEngine::setObjectOwnership(ptr, QQmlEngine::JavaScriptOwnership);
            c.setData("import QtQuick 2.0; QtObject { required property QtObject test; property var object: { var i = test; test ? 0 : 1 }  }", QUrl());
            QVERIFY(c.isReady());
            QScopedPointer<QObject> o(
                        c.createWithInitialProperties({{"test", QVariant::fromValue(ptr)}}));
            QVERIFY(o != nullptr);
            QQmlProperty testProp(o.data(), "test");
            testProp.write(QVariant::fromValue<QObject*>(nullptr));
        }
        QTRY_VERIFY(spy.size());
    }
}

// Test an object can be accessed by multiple engines
void tst_qqmlengine::multipleEngines()
{
    QObject o;
    o.setObjectName("TestName");

    // Simultaneous engines
    {
        QQmlEngine engine1;
        QQmlEngine engine2;
        engine1.rootContext()->setContextProperty("object", &o);
        engine2.rootContext()->setContextProperty("object", &o);

        QQmlExpression expr1(engine1.rootContext(), nullptr, QString("object.objectName"));
        QQmlExpression expr2(engine2.rootContext(), nullptr, QString("object.objectName"));

        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
        QCOMPARE(expr2.evaluate().toString(), QString("TestName"));
    }

    // Serial engines
    {
        QQmlEngine engine1;
        engine1.rootContext()->setContextProperty("object", &o);
        QQmlExpression expr1(engine1.rootContext(), nullptr, QString("object.objectName"));
        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
    }
    {
        QQmlEngine engine1;
        engine1.rootContext()->setContextProperty("object", &o);
        QQmlExpression expr1(engine1.rootContext(), nullptr, QString("object.objectName"));
        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
    }
}

void tst_qqmlengine::qtqmlModule_data()
{
    QTest::addColumn<QUrl>("testFile");
    QTest::addColumn<QString>("expectedError");
    QTest::addColumn<QStringList>("expectedWarnings");

    QTest::newRow("import QtQml of correct version (2.0)")
            << testFileUrl("qtqmlModule.1.qml")
            << QString()
            << QStringList();

    QTest::newRow("import QtQml of incorrect version (3.0)")
            << testFileUrl("qtqmlModule.2.qml")
            << QString(testFileUrl("qtqmlModule.2.qml").toString() + QLatin1String(":1 module \"QtQml\" version 3.0 is not installed\n"))
            << QStringList();

    QTest::newRow("import QtQml of incorrect version (1.0)")
            << testFileUrl("qtqmlModule.3.qml")
            << QString(testFileUrl("qtqmlModule.3.qml").toString() + QLatin1String(":1 module \"QtQml\" version 1.0 is not installed\n"))
            << QStringList();

    QTest::newRow("import QtQml of old version (2.50)")
            << testFileUrl("qtqmlModule.4.qml")
            << QString()
            << QStringList();

    QTest::newRow("QtQml 2.0 module provides Component, QtObject, Connections, Binding and Timer")
            << testFileUrl("qtqmlModule.5.qml")
            << QString()
            << QStringList();

    QTest::newRow("can import QtQml then QtQuick")
            << testFileUrl("qtqmlModule.6.qml")
            << QString()
            << QStringList();

    QTest::newRow("can import QtQuick then QtQml")
            << testFileUrl("qtqmlModule.7.qml")
            << QString()
            << QStringList();

    QTest::newRow("no import results in no QtObject availability")
            << testFileUrl("qtqmlModule.8.qml")
            << QString(testFileUrl("qtqmlModule.8.qml").toString() + QLatin1String(":4 QtObject is not a type\n"))
            << QStringList();

    QTest::newRow("importing QtQml only results in no Item availability")
            << testFileUrl("qtqmlModule.9.qml")
            << QString(testFileUrl("qtqmlModule.9.qml").toString() + QLatin1String(":4 Item is not a type\n"))
            << QStringList();

    QTest::newRow("import QtQml of incorrect version (6.50)")
            << testFileUrl("qtqmlModule.10.qml")
            << QString(testFileUrl("qtqmlModule.10.qml").toString() + QLatin1String(":1 module \"QtQml\" version 6.50 is not installed\n"))
            << QStringList();
}

// Test that the engine registers the QtQml module
void tst_qqmlengine::qtqmlModule()
{
    QFETCH(QUrl, testFile);
    QFETCH(QString, expectedError);
    QFETCH(QStringList, expectedWarnings);

    foreach (const QString &w, expectedWarnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(w));

    QQmlEngine e;
    QQmlComponent c(&e, testFile);
    if (expectedError.isEmpty()) {
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
    } else {
        QCOMPARE(c.errorString(), expectedError);
    }
}

class CustomSelector : public QQmlAbstractUrlInterceptor
{
public:
    CustomSelector(const QUrl &base):m_base(base){}
    QUrl intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType d) override
    {
        if ((url.scheme() != QStringLiteral("file") && url.scheme() != QStringLiteral("qrc"))
            || url.path().contains("QtQml"))
            return url;
        if (!m_interceptionPoints.contains(d))
            return url;

        if (url.path().endsWith("Test.2/qmldir")) {//Special case
            QUrl url = m_base;
            url.setPath(m_base.path() + "interception/module/intercepted/qmldir");
            return url;
        }
        // Special case: with 5.10 we always add the implicit import, so we need to explicitly handle this case now
        if (url.path().endsWith("intercepted/qmldir"))
            return url;

        QString alteredPath = url.path();
        int a = alteredPath.lastIndexOf('/');
        if (a < 0)
            a = 0;
        alteredPath.insert(a, QStringLiteral("/intercepted"));

        QUrl ret = url;
        ret.setPath(alteredPath);
        return ret;
    }
    QList<QQmlAbstractUrlInterceptor::DataType> m_interceptionPoints;
    QUrl m_base;
};

Q_DECLARE_METATYPE(QList<QQmlAbstractUrlInterceptor::DataType>);

void tst_qqmlengine::urlInterceptor_data()
{
    QTest::addColumn<QUrl>("testFile");
    QTest::addColumn<QList<QQmlAbstractUrlInterceptor::DataType> >("interceptionPoint");
    QTest::addColumn<QString>("expectedChildString");
    QTest::addColumn<QString>("expectedScriptString");
    QTest::addColumn<QString>("expectedResolvedUrl");
    QTest::addColumn<QString>("expectedAbsoluteUrl");

    QTest::newRow("InterceptTypes")
        << testFileUrl("interception/types/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::QmlFile << QQmlAbstractUrlInterceptor::JavaScriptFile << QQmlAbstractUrlInterceptor::UrlString)
        << QStringLiteral("intercepted")
        << QStringLiteral("intercepted")
        << testFileUrl("interception/types/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///intercepted/doesNotExist.file");

    QTest::newRow("InterceptQmlDir")
        << testFileUrl("interception/qmldir/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::QmldirFile << QQmlAbstractUrlInterceptor::UrlString)
        << QStringLiteral("intercepted")
        << QStringLiteral("base file")
        << testFileUrl("interception/qmldir/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///intercepted/doesNotExist.file");

    QTest::newRow("InterceptModule")//just a Test{}, needs to intercept the module import for it to work
        << testFileUrl("interception/module/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::QmldirFile )
        << QStringLiteral("intercepted")
        << QStringLiteral("intercepted")
        << testFileUrl("interception/module/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///doesNotExist.file");

    QTest::newRow("InterceptStrings")
        << testFileUrl("interception/strings/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::UrlString)
        << QStringLiteral("base file")
        << QStringLiteral("base file")
        << testFileUrl("interception/strings/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///intercepted/doesNotExist.file");

    QTest::newRow("InterceptIncludes")
        << testFileUrl("interception/includes/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::JavaScriptFile)
        << QStringLiteral("base file")
        << QStringLiteral("intercepted include file")
        << testFileUrl("interception/includes/doesNotExist.file").toString()
        << QStringLiteral("file:///doesNotExist.file");
}

void tst_qqmlengine::urlInterceptor()
{

    QFETCH(QUrl, testFile);
    QFETCH(QList<QQmlAbstractUrlInterceptor::DataType>, interceptionPoint);
    QFETCH(QString, expectedChildString);
    QFETCH(QString, expectedScriptString);
    QFETCH(QString, expectedResolvedUrl);
    QFETCH(QString, expectedAbsoluteUrl);

    QQmlEngine e;
    e.addImportPath(testFileUrl("interception/imports").url());
    CustomSelector cs(testFileUrl(""));
    cs.m_interceptionPoints = interceptionPoint;
    e.addUrlInterceptor(&cs);
    QQmlComponent c(&e, testFile); //Note that this can get intercepted too
    QScopedPointer<QObject> o(c.create());
    if (!o)
        qDebug() << c.errorString();
    QVERIFY(o);
    //Test a URL as a property initialization
    QCOMPARE(o->property("filePath").toString(), QUrl("doesNotExist.file").toString());
    //Test a URL as a Type location
    QCOMPARE(o->property("childString").toString(), expectedChildString);
    //Test a URL as a Script location
    QCOMPARE(o->property("scriptString").toString(), expectedScriptString);
    //Test a URL as a resolveUrl() call
    QCOMPARE(o->property("resolvedUrl").toString(), expectedResolvedUrl);
    QCOMPARE(o->property("absoluteUrl").toString(), expectedAbsoluteUrl);
}

void tst_qqmlengine::qmlContextProperties()
{
    QQmlEngine e;

    QQmlComponent c(&e, testFileUrl("TypeofQmlProperty.qml"));
    QScopedPointer<QObject> o(c.create());
    if (!o) {
        qDebug() << c.errorString();
    }
    QVERIFY(o);
}

void tst_qqmlengine::testGCCorruption()
{
    QQmlEngine e;

    QQmlComponent c(&e, testFileUrl("testGCCorruption.qml"));
    QScopedPointer<QObject> o(c.create());
    QVERIFY2(o, qPrintable(c.errorString()));
}

void tst_qqmlengine::testGroupedPropertyRevisions()
{
    QQmlEngine e;

    QQmlComponent c(&e, testFileUrl("testGroupedPropertiesRevision.1.qml"));
    QScopedPointer<QObject> object(c.create());
    QVERIFY2(object.data(), qPrintable(c.errorString()));
    QQmlComponent c2(&e, testFileUrl("testGroupedPropertiesRevision.2.qml"));
    QVERIFY(!c2.errorString().isEmpty());
}

void tst_qqmlengine::componentFromEval()
{
    QQmlEngine engine;
    const QUrl testUrl = testFileUrl("EmptyComponent.qml");
    QJSValue result = engine.evaluate("Qt.createComponent(\"" + testUrl.toString() + "\");");
    QPointer<QQmlComponent> component(qobject_cast<QQmlComponent*>(result.toQObject()));
    QVERIFY(!component.isNull());
    QScopedPointer<QObject> item(component->create());
    QVERIFY(!item.isNull());
}

void tst_qqmlengine::qrcUrls()
{
    QQmlEngine engine;
    QQmlEnginePrivate *pEngine = QQmlEnginePrivate::get(&engine);

    {
        QQmlRefPointer<QQmlTypeData> oneQml(pEngine->typeLoader.getType(QUrl("qrc:/qrcurls.qml")));
        QVERIFY(oneQml.data() != nullptr);
        QVERIFY(!oneQml->backupSourceCode().isValid());
        QQmlRefPointer<QQmlTypeData> twoQml(pEngine->typeLoader.getType(QUrl("qrc:///qrcurls.qml")));
        QVERIFY(twoQml.data() != nullptr);
        QCOMPARE(oneQml.data(), twoQml.data());
    }

    {
        QQmlRefPointer<QQmlTypeData> oneJS(pEngine->typeLoader.getType(QUrl("qrc:/qrcurls.js")));
        QVERIFY(oneJS.data() != nullptr);
        QVERIFY(!oneJS->backupSourceCode().isValid());
        QQmlRefPointer<QQmlTypeData> twoJS(pEngine->typeLoader.getType(QUrl("qrc:///qrcurls.js")));
        QVERIFY(twoJS.data() != nullptr);
        QCOMPARE(oneJS.data(), twoJS.data());
    }
}

void tst_qqmlengine::cppSignalAndEval()
{
    ObjectCaller objectCaller;
    QQmlEngine engine;
    qmlRegisterSingletonInstance("Test", 1, 0, "CallerCpp", &objectCaller);
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.9\n"
              "import Test 1.0\n"
              "Item {\n"
              "    property var r: 0\n"
              "    Connections {\n"
              "        target: CallerCpp;\n"
              "        function onDoubleReply() {\n"
              "            eval('var z = 1');\n"
              "            r = a;\n"
              "        }\n"
              "    }\n"
              "}",
              QUrl(QStringLiteral("qrc:/main.qml")));
    QScopedPointer<QObject> object(c.create());
    QVERIFY(!object.isNull());
    emit objectCaller.doubleReply(1.1234);
    QCOMPARE(object->property("r"), 1.1234);
}

class SomeQObjectClass : public QObject {
    Q_OBJECT
public:
    SomeQObjectClass() : QObject(nullptr){}
};

class Dayfly : public QObject
{
    Q_OBJECT
};

void tst_qqmlengine::singletonInstance()
{
    QQmlEngine engine;

    int cppSingletonTypeId = qmlRegisterSingletonType<CppSingleton>("Test", 1, 0, "CppSingleton", &CppSingleton::create);
    int jsValueSingletonTypeId = qmlRegisterSingletonType("Test", 1, 0, "JsSingleton", &JsSingleton::create);

    {
        // Cpp QObject singleton type
        QJSValue value = engine.singletonInstance<QJSValue>(cppSingletonTypeId);
        QVERIFY(!value.isUndefined());
        QVERIFY(value.isQObject());
        QObject *instance = value.toQObject();
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "CppSingleton");

        QCOMPARE(engine.singletonInstance<CppSingleton *>("Test", "CppSingleton"), instance);
    }

    {
        // QJSValue QObject singleton type
        QJSValue value = engine.singletonInstance<QJSValue>(jsValueSingletonTypeId);
        QVERIFY(!value.isUndefined());
        QVERIFY(value.isQObject());
        QObject *instance = value.toQObject();
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "JsSingleton");
    }

    {
        int data = 30;
        auto id = qmlRegisterSingletonType<CppSingleton>("Qt.test",1,0,"CapturingLambda",[data](QQmlEngine*, QJSEngine*){ // register qobject singleton with capturing lambda
                auto o = new CppSingleton;
                o->setProperty("data", data);
                return o;
        });
        QJSValue value = engine.singletonInstance<QJSValue>(id);
        QVERIFY(!value.isUndefined());
        QVERIFY(value.isQObject());
        QObject *instance = value.toQObject();
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "CppSingleton");
        QCOMPARE(instance->property("data"), data);
    }
    {
        qmlRegisterSingletonType<CppSingleton>("Qt.test",1,0,"NotAmbiguous", [](QQmlEngine* qeng, QJSEngine* jeng) -> QObject* {return CppSingleton::create(qeng, jeng);}); // test that overloads for qmlRegisterSingleton are not ambiguous
    }
    {
        // Register QObject* directly
        CppSingleton single;
        int id = qmlRegisterSingletonInstance("Qt.test", 1, 0, "CppOwned",
                                                                &single);
        QQmlEngine engine2;
        CppSingleton *singlePtr = engine2.singletonInstance<CppSingleton *>(id);
        QVERIFY(singlePtr);
        QCOMPARE(&single, singlePtr);
        QVERIFY(engine2.objectOwnership(singlePtr) == QQmlEngine::CppOwnership);
    }

    {
        CppSingleton single;
        QQmlEngine engineA;
        QQmlEngine engineB;
        int id = qmlRegisterSingletonInstance("Qt.test", 1, 0, "CppOwned", &single);
        auto singlePtr = engineA.singletonInstance<CppSingleton *>(id);
        QVERIFY(singlePtr);
        singlePtr = engineA.singletonInstance<CppSingleton *>(id); // accessing the singleton multiple times from the same engine is fine
        QVERIFY(singlePtr);
        QTest::ignoreMessage(QtMsgType::QtCriticalMsg, "<Unknown File>: qmlRegisterSingletonType(): \"CppOwned\" is not available because the callback function returns a null pointer.");
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: Singleton registered by registerSingletonInstance must only be accessed from one engine");
        QCOMPARE(&single, singlePtr);
        auto noSinglePtr = engineB.singletonInstance<CppSingleton *>(id);
        QVERIFY(!noSinglePtr);
    }

    {
        CppSingleton single;
        QThread newThread {};
        single.moveToThread(&newThread);
        QCOMPARE(single.thread(), &newThread);
        QQmlEngine engineB;
        int id = qmlRegisterSingletonInstance("Qt.test", 1, 0, "CppOwned", &single);
        QTest::ignoreMessage(QtMsgType::QtCriticalMsg, "<Unknown File>: qmlRegisterSingletonType(): \"CppOwned\" is not available because the callback function returns a null pointer.");
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: Registered object must live in the same thread as the engine it was registered with");
        auto noSinglePtr = engineB.singletonInstance<CppSingleton *>(id);
        QVERIFY(!noSinglePtr);
    }

    // test the case where we haven't loaded the module yet
    {
        auto singleton = engine.singletonInstance<PurelyDeclarativeSingleton *>("OnlyDeclarative", "PurelyDeclarativeSingleton");
        QVERIFY(singleton);
        // requesting the singleton twice yields the same result
        auto again = engine.singletonInstance<PurelyDeclarativeSingleton *>("OnlyDeclarative", "PurelyDeclarativeSingleton");
        QCOMPARE(again, singleton);

        // different engines -> different singletons
        QQmlEngine engine2;
        auto differentEngine = engine2.singletonInstance<PurelyDeclarativeSingleton *>("OnlyDeclarative", "PurelyDeclarativeSingleton");
        QCOMPARE_NE(differentEngine, singleton);
    }

    {
        // Invalid types
        QJSValue value;
        value = engine.singletonInstance<QJSValue>(-4711);
        QVERIFY(value.isUndefined());
        value = engine.singletonInstance<QJSValue>(1701);
        QVERIFY(value.isUndefined());
    }

    {
        // Valid, but non-singleton type
        int typeId = qmlRegisterType<CppSingleton>("Test", 1, 0, "NotASingleton");
        QJSValue value = engine.singletonInstance<QJSValue>(typeId);
        QVERIFY(value.isUndefined());
    }

    {
        // Cpp QObject singleton type
        CppSingleton *instance = engine.singletonInstance<CppSingleton*>(cppSingletonTypeId);
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "CppSingleton");
        QCOMPARE(instance, engine.singletonInstance<QJSValue>(cppSingletonTypeId).toQObject());
    }

    {
        // Wrong destination type
        SomeQObjectClass * instance = engine.singletonInstance<SomeQObjectClass*>(cppSingletonTypeId);
        QVERIFY(!instance);
    }

    {
        // deleted object
        auto dayfly = new Dayfly{};
        auto id = qmlRegisterSingletonInstance("Vanity", 1, 0, "Dayfly", dayfly);
        delete dayfly;
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: The registered singleton has already been deleted. Ensure that it outlives the engine.");
        QObject *instance = engine.singletonInstance<QObject*>(id);
        QVERIFY(!instance);
    }
}

void tst_qqmlengine::aggressiveGc()
{
    const QByteArray origAggressiveGc = qgetenv("QV4_MM_AGGRESSIVE_GC");
    qputenv("QV4_MM_AGGRESSIVE_GC", "true");
    {
        QQmlEngine engine; // freezing should not run into infinite recursion
        QJSValue obj = engine.newObject();
        QVERIFY(obj.isObject());
    }
    qputenv("QV4_MM_AGGRESSIVE_GC", origAggressiveGc);
}

void tst_qqmlengine::cachedGetterLookup_qtbug_75335()
{
    QQmlEngine engine;
    const QUrl testUrl = testFileUrl("CachedGetterLookup.qml");
    QQmlComponent component(&engine, testUrl);
    QVERIFY(component.isReady());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

class EvilSingleton : public QObject
{
    Q_OBJECT
public:
    QPointer<QQmlEngine> m_engine;
    EvilSingleton(QQmlEngine *engine) : m_engine(engine) {
        connect(this, &QObject::destroyed, this, [this]() {
            QQmlComponent component(m_engine);
            component.setData("import QtQml 2.0\nQtObject {}", QUrl("file://Stuff.qml"));
            QVERIFY(component.isReady());
            QScopedPointer<QObject> obj(component.create());
            QVERIFY(obj);
        });
    }
};

void tst_qqmlengine::createComponentOnSingletonDestruction()
{
    qmlRegisterSingletonType<EvilSingleton>("foo.foo", 1, 0, "Singleton",
                                            [](QQmlEngine *engine, QJSEngine *) {
        return new EvilSingleton(engine);
    });

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("evilSingletonInstantiation.qml"));
    QVERIFY(component.isReady());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);
}

void tst_qqmlengine::uiLanguage()
{
    {
        QQmlEngine engine;

        QObject::connect(&engine, &QJSEngine::uiLanguageChanged, [&engine]() {
            engine.retranslate();
        });

        QSignalSpy uiLanguageChangeSpy(&engine, SIGNAL(uiLanguageChanged()));

        QQmlComponent component(&engine, testFileUrl("uiLanguage.qml"));

        QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        QVERIFY(engine.uiLanguage().isEmpty());
        QCOMPARE(object->property("numberOfTranslationBindingEvaluations").toInt(), 1);

        QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
        engine.setUiLanguage("TestLanguage");
        QCOMPARE(object->property("numberOfTranslationBindingEvaluations").toInt(), 2);
        QCOMPARE(object->property("chosenLanguage").toString(), "TestLanguage");

        QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
        engine.evaluate("Qt.uiLanguage = \"anotherLanguage\"");
        QCOMPARE(engine.uiLanguage(), QString("anotherLanguage"));
        QCOMPARE(object->property("numberOfTranslationBindingEvaluations").toInt(), 3);
        QCOMPARE(object->property("chosenLanguage").toString(), "anotherLanguage");
    }

    {
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("uiLanguage.qml"));

        QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());

        engine.setUiLanguage("TestLanguage");
        QCOMPARE(object->property("chosenLanguage").toString(), "TestLanguage");
    }
}

class I18nAwareClass : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(I18nAware)

    Q_PROPERTY(QString text READ text NOTIFY textChanged)
signals:
    void textChanged();
public:
    int counter = 0;

    QString text()
    {
        if (auto engine = qmlEngine(this))
            engine->markCurrentFunctionAsTranslationBinding();
        return QLatin1String("Hello, %1").arg(QString::number(counter++));
    }
};

void tst_qqmlengine::markCurrentFunctionAsTranslationBinding()
{
    QQmlEngine engine;
    qmlRegisterTypesAndRevisions<I18nAwareClass>("i18ntest", 1);
    QQmlComponent comp(&engine, testFileUrl("markCurrentFunctionAsTranslationBinding.qml"));
    std::unique_ptr<QObject> root { comp.create() };
    QCOMPARE(root->property("result"), "Hello, 0");
    engine.retranslate();
    QCOMPARE(root->property("result"), "Hello, 1");
}

void tst_qqmlengine::executeRuntimeFunction()
{
    QQmlEngine engine;
    QQmlEnginePrivate *priv = QQmlEnginePrivate::get(std::addressof(engine));

    const QUrl url = testFileUrl("runtimeFunctions.qml");
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> dummy(component.create());
    QVERIFY(dummy);

    // getConstantValue():
    int constant = 0;
    void *a0[] = { const_cast<void *>(reinterpret_cast<const void *>(std::addressof(constant))) };
    QMetaType t0[] = { QMetaType::fromType<int>() };
    priv->executeRuntimeFunction(url, /* index = */ 0, dummy.get(), /* argc = */ 0, a0, t0);
    QCOMPARE(constant, 42);

    // squareValue():
    int squared = 0;
    int x = 5;
    void *a1[] = { const_cast<void *>(reinterpret_cast<const void *>(std::addressof(squared))),
                   const_cast<void *>(reinterpret_cast<const void *>(std::addressof(x))) };
    QMetaType t1[] = { QMetaType::fromType<int>(), QMetaType::fromType<int>() };
    priv->executeRuntimeFunction(url, /* index = */ 1, dummy.get(), /* argc = */ 1, a1, t1);
    QCOMPARE(squared, x * x);

    // concatenate():
    QString concatenated;
    QString str1 = QStringLiteral("Hello"); // uses "raw data" storage
    QString str2 = QLatin1String(", Qml"); // uses own QString storage
    void *a2[] = { const_cast<void *>(reinterpret_cast<const void *>(std::addressof(concatenated))),
                   const_cast<void *>(reinterpret_cast<const void *>(std::addressof(str1))),
                   const_cast<void *>(reinterpret_cast<const void *>(std::addressof(str2))) };
    QMetaType t2[] = { QMetaType::fromType<QString>(), QMetaType::fromType<QString>(),
                       QMetaType::fromType<QString>() };
    priv->executeRuntimeFunction(url, /* index = */ 2, dummy.get(), /* argc = */ 2, a2, t2);
    QCOMPARE(concatenated, str1 + str2);

    // capture `this`:
    QCOMPARE(dummy->property("foo").toInt(), 42);
    QCOMPARE(dummy->property("bar").toInt(), 0);
    priv->executeRuntimeFunction(url, /* index = */ 4, dummy.get());
    QCOMPARE(dummy->property("bar").toInt(), 1 + 42 + 1);

    QCOMPARE(dummy->property("baz").toInt(), -100);
    int y = 1;
    void *a3[] = { nullptr, const_cast<void *>(reinterpret_cast<const void *>(&y)) };
    QMetaType t3[] = { QMetaType::fromType<void>(), QMetaType::fromType<int>() };
    priv->executeRuntimeFunction(url, /* index = */ 6, dummy.get(), 1, a3, t3);
    QCOMPARE(dummy->property("bar").toInt(), -98);
    QCOMPARE(dummy->property("baz").toInt(), -100);
}

class WithQProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo BINDABLE fooBindable)

public:
    WithQProperty(QObject *parent = nullptr) : QObject(parent) { m_foo.setValue(12); }

    int foo() const { return m_foo.value(); }
    void setFoo(int foo) { m_foo.setValue(foo); }
    QBindable<int> fooBindable() { return QBindable<int>(&m_foo); }

    int getFooWithCapture()
    {
        const QMetaObject *m = metaObject();
        currentEngine->captureProperty(this, m->property(m->indexOfProperty("foo")));
        return m_foo.value();
    }

    static QQmlEngine *currentEngine;

private:
    QProperty<int> m_foo;
};

class WithoutQProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo NOTIFY fooChanged)
public:
    WithoutQProperty(QObject *parent = nullptr) : QObject(parent), m_foo(new WithQProperty(this)) {}

    int foo() const { return m_foo->getFooWithCapture(); }

    void setFoo(int foo) {
        if (foo != m_foo->foo()) {
            m_foo->setFoo(foo);
            emit fooChanged();
        }
    }

    void triggerBinding(int val)
    {
        m_foo->setFoo(val);
    }

signals:
    void fooChanged();

private:
    WithQProperty *m_foo;
};

QQmlEngine *WithQProperty::currentEngine = nullptr;

void tst_qqmlengine::captureQProperty()
{
    qmlRegisterType<WithoutQProperty>("Foo", 1, 0, "WithoutQProperty");
    QQmlEngine engine;
    WithQProperty::currentEngine = &engine;
    QQmlComponent c(&engine);
    c.setData("import Foo\n"
              "WithoutQProperty {\n"
              "    property int x: foo\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), c.errorString().toUtf8());
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("x").toInt(), 12);
    static_cast<WithoutQProperty *>(o.data())->triggerBinding(13);
    QCOMPARE(o->property("x").toInt(), 13);
}

void tst_qqmlengine::listWrapperAsListReference()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQml\nQtObject {\nproperty list<QtObject> c: [ QtObject {} ]\n}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QJSManagedValue m = engine.toManagedValue(o.data());
    QJSValue prop = m.property("c");
    const QQmlListReference ref = qjsvalue_cast<QQmlListReference>(prop);
    QCOMPARE(ref.size(), 1);
}

void tst_qqmlengine::attachedObjectAsObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQml\nQtObject { property var a: Component }", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QJSManagedValue m = engine.toManagedValue(o.data());
    QJSValue prop = m.property("a");
    const QQmlComponentAttached *attached = qjsvalue_cast<QQmlComponentAttached *>(prop);
    QCOMPARE(attached, qmlAttachedPropertiesObject<QQmlComponent>(o.data()));
}

class WithListProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQmlComponent> components READ components CONSTANT)
    QML_ELEMENT
public:

    QQmlListProperty<QQmlComponent> components()
    {
        return QQmlListProperty<QQmlComponent>(this, &m_components);
    }

private:
    QList<QQmlComponent *> m_components;
};

void tst_qqmlengine::listPropertyAsQJSValue()
{
    qmlRegisterTypesAndRevisions<WithListProperty>("Foo", 1);
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import Foo\nWithListProperty {}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    WithListProperty *parent = qobject_cast<WithListProperty *>(o.data());
    QVERIFY(parent);
    QQmlListProperty<QQmlComponent> prop = parent->components();
    QJSValue val = engine.toScriptValue(prop);
    QQmlListReference ref = engine.fromScriptValue<QQmlListReference>(val);
    ref.append(&c);
    QCOMPARE(prop.count(&prop), 1);
    QCOMPARE(prop.at(&prop, 0), &c);
}

void tst_qqmlengine::stringToColor()
{
    QQmlEngine engine;

    // Make it import QtQuick, so that color becomes available.
    QQmlComponent c(&engine);
    c.setData("import QtQuick\nItem {}", QUrl());
    QVERIFY(c.isReady());
    QScopedPointer<QObject> o(c.create());

    const QMetaType metaType(QMetaType::QColor);
    QVariant color(metaType);
    QVERIFY(QV4::ExecutionEngine::metaTypeFromJS(
                engine.handle()->newString(QStringLiteral("#abcdef"))->asReturnedValue(),
                metaType, color.data()));
    QVERIFY(color.isValid());
    QCOMPARE(color.metaType(), metaType);

    QVariant variant(QStringLiteral("#abcdef"));
    QVERIFY(variant.convert(metaType));
    QCOMPARE(variant.metaType(), metaType);

    QCOMPARE(color, variant);
}

class WithToString : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Q_INVOKABLE QString toString() const { return QStringLiteral("things"); }
};

class WithToNumber : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Q_INVOKABLE int toString() const { return 4; }
};

void tst_qqmlengine::qobjectToString()
{
    qmlRegisterTypesAndRevisions<WithToString>("WithToString", 1);
    qmlRegisterTypesAndRevisions<WithToNumber>("WithToString", 1);
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(
        import WithToString
        import QtQml

        WithToString {
            id: self
            property QtObject weird: WithToNumber {}
            objectName: toString() + ' ' + self.toString() + ' ' + weird.toString()
        }
    )", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->objectName(), QStringLiteral("things things 4"));
}

void tst_qqmlengine::qtNamespaceInQtObject()
{
    QQmlEngine engine;
    QJSValue qtObject = engine.globalObject().property(QStringLiteral("Qt"));

    // Qt namespace enums are there.
    QCOMPARE(qtObject.property(QStringLiteral("Checked")).toInt(), 2);

    // QtObject methods are also there.
    QVERIFY(qtObject.property(QStringLiteral("rect")).isCallable());

    // QObject is also there.
    QVERIFY(qtObject.hasProperty(QStringLiteral("objectName")));
}

void tst_qqmlengine::nativeModuleImport()
{
    QQmlEngine engine;

    QJSValue name("TheName");
    QJSValue obj = engine.newObject();
    obj.setProperty("name", name);
    engine.registerModule("info.mjs", obj);

    QQmlComponent c(&engine, testFileUrl("nativeModuleImport.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("a").toString(), QStringLiteral("Hello World"));
    QCOMPARE(o->property("b").toString(), QStringLiteral("TheName"));
    QCOMPARE(o->property("c").toString(), QStringLiteral("TheName"));
    QCOMPARE(o->property("d").toString(), QStringLiteral("TheName"));
    QCOMPARE(o->property("e").toString(), QStringLiteral("TheName"));
}

void tst_qqmlengine::lockedRootObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("lockedRootObject.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QTest::ignoreMessage(
                QtWarningMsg, "You cannot shadow the locked property 'hasOwnProperty' in QML.");
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("myErrorName").toString(), QStringLiteral("MyError1"));
    QCOMPARE(o->property("errorName").toString(), QStringLiteral("MyError2"));
    QCOMPARE(o->property("mathMax").toInt(), 4);
    QCOMPARE(o->property("extendGlobal").toInt(), 32);
    QCOMPARE(o->property("prototypeTrick").toString(), QStringLiteral("SyntaxError"));
    QCOMPARE(o->property("shadowMethod1").toString(), QStringLiteral("not a TypeError"));
    QCOMPARE(o->property("shadowMethod2").toBool(), false);
    QCOMPARE(o->property("changeObjectProto1").toString(), QStringLiteral("not an Object"));
    QCOMPARE(o->property("changeObjectProto2").toBool(), false);
    QCOMPARE(o->property("defineProperty1").toString(), QStringLiteral("not a URIError"));
    QCOMPARE(o->property("defineProperty2").toBool(), false);
}

void tst_qqmlengine::crossReferencingSingletonsDeletion()
{
    QQmlEngine engine;
    engine.addImportPath(testFileUrl("crossReferencingSingletonsDeletion").url());
    QQmlComponent c(&engine, testFileUrl("crossReferencingSingletonsDeletion/Module/Main.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    std::unique_ptr<QObject> o{ c.create() };
    QVERIFY(o);
    QCOMPARE(o->property("s").toString(), "SingletonA");
}

void tst_qqmlengine::bindingInstallUseAfterFree()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindingInstallUseAfterFree.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    std::unique_ptr<QObject> o{ c.create() };
    QVERIFY(o);
}

QTEST_MAIN(tst_qqmlengine)

#include "tst_qqmlengine.moc"
