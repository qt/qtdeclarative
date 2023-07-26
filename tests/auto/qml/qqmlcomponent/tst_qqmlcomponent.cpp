// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QDebug>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlincubator.h>
#include <QtQuick>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/testhttpserver_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlguardedcontextdata_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4executablecompilationunit_p.h>
#include <qcolor.h>
#include <qsignalspy.h>

#include <algorithm>

using namespace Qt::StringLiterals;

class WithQJSValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue v READ v WRITE setV NOTIFY vChanged)

public:
    QJSValue v() const { return m_v; }
    void setV(const QJSValue &newV)
    {
        if (!m_v.strictlyEquals(newV)) {
            m_v = newV;
            emit vChanged();
        }
    }

signals:
    void vChanged();

private:
    QJSValue m_v;
};

class MyIC : public QObject, public QQmlIncubationController
{
    Q_OBJECT
public:
    MyIC() { startTimer(5); }
protected:
    void timerEvent(QTimerEvent*) override {
        incubateFor(5);
    }
};

class ComponentWatcher : public QObject
{
    Q_OBJECT
public:
    ComponentWatcher(QQmlComponent *comp) : loading(0), error(0), ready(0) {
        connect(comp, SIGNAL(statusChanged(QQmlComponent::Status)),
                this, SLOT(statusChanged(QQmlComponent::Status)));
    }

    int loading;
    int error;
    int ready;

public slots:
    void statusChanged(QQmlComponent::Status status) {
        switch (status) {
        case QQmlComponent::Loading:
            ++loading;
            break;
        case QQmlComponent::Error:
            ++error;
            break;
        case QQmlComponent::Ready:
            ++ready;
            break;
        default:
            break;
        }
    }
};

static void gc(QQmlEngine &engine)
{
    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

class tst_qqmlcomponent : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlcomponent() : QQmlDataTest(QT_QMLTEST_DATADIR) { engine.setIncubationController(&ic); }

private slots:
    void null();
    void loadEmptyUrl();
    void qmlCreateWindow();
    void qmlCreateObjectAutoParent_data();
    void qmlCreateObjectAutoParent();
    void qmlCreateObjectWithProperties();
    void qmlCreateObjectClean();
    void qmlCreateObjectDirty();
    void qmlIncubateObject();
    void qmlCreateParentReference();
    void async();
    void asyncHierarchy();
    void asyncForceSync();
    void componentUrlCanonicalization();
    void onDestructionLookup();
    void onDestructionCount();
    void recursion();
    void recursionContinuation();
    void partialComponentCreation();
    void callingContextForInitialProperties();
    void setNonExistentInitialProperty();
    void relativeUrl_data();
    void relativeUrl();
    void setDataNoEngineNoSegfault();
    void testRequiredProperties_data();
    void testRequiredProperties();
    void testRequiredPropertiesFromQml();
    void testSetInitialProperties();
    void createInsideJSModule();
    void qmlErrorIsReported();
    void initJSValueProp();
    void qmlPropertySignalExists();
    void componentTypes();
    void boundComponent();
    void loadFromModule_data();
    void loadFromModule();
    void loadFromModuleThenCreateWithIncubator();
    void loadFromModuleFailures_data();
    void loadFromModuleFailures();
    void loadFromModuleRequired();
    void loadFromQrc();
    void removeBinding();
    void complexObjectArgument();

private:
    QQmlEngine engine;
    MyIC ic;
};

void tst_qqmlcomponent::null()
{
    {
        QQmlComponent c;
        QVERIFY(c.isNull());
    }

    {
        QQmlComponent c(&engine);
        QVERIFY(c.isNull());
    }
}


void tst_qqmlcomponent::loadEmptyUrl()
{
    QQmlComponent c(&engine);
    c.loadUrl(QUrl());

    QVERIFY(c.isError());
    QCOMPARE(c.errors().size(), 1);
    QQmlError error = c.errors().first();
    QCOMPARE(error.url(), QUrl());
    QCOMPARE(error.line(), -1);
    QCOMPARE(error.column(), -1);
    QCOMPARE(error.description(), QLatin1String("Invalid empty URL"));
}

void tst_qqmlcomponent::qmlIncubateObject()
{
    QQmlComponent component(&engine, testFileUrl("incubateObject.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), false);

    QTRY_VERIFY(object->property("test2").toBool());

    delete object;
}

void tst_qqmlcomponent::qmlCreateWindow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("createWindow.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
}

void tst_qqmlcomponent::qmlCreateObjectAutoParent_data()
{
    QTest::addColumn<QString>("testFile");

    QTest::newRow("createObject") << QStringLiteral("createObject.qml");
    QTest::newRow("createQmlObject") <<  QStringLiteral("createQmlObject.qml");
}


void tst_qqmlcomponent::qmlCreateObjectAutoParent()
{
    QFETCH(QString, testFile);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl(testFile));
    QScopedPointer<QObject> root(qobject_cast<QQuickItem *>(component.create()));
    QVERIFY(!root.isNull());
    QObject *qtobjectParent = root->property("qtobjectParent").value<QObject*>();
    QQuickItem *itemParent = qobject_cast<QQuickItem *>(root->property("itemParent").value<QObject*>());
    QQuickWindow *windowParent = qobject_cast<QQuickWindow *>(root->property("windowParent").value<QObject*>());
    QVERIFY(qtobjectParent);
    QVERIFY(itemParent);
    QVERIFY(windowParent);

    QObject *qtobject_qtobject = root->property("qtobject_qtobject").value<QObject*>();
    QObject *qtobject_item = root->property("qtobject_item").value<QObject*>();
    QObject *qtobject_window = root->property("qtobject_window").value<QObject*>();
    QObject *item_qtobject = root->property("item_qtobject").value<QObject*>();
    QObject *item_item = root->property("item_item").value<QObject*>();
    QObject *item_window = root->property("item_window").value<QObject*>();
    QObject *window_qtobject = root->property("window_qtobject").value<QObject*>();
    QObject *window_item = root->property("window_item").value<QObject*>();
    QObject *window_window = root->property("window_window").value<QObject*>();

    QVERIFY(qtobject_qtobject);
    QVERIFY(qtobject_item);
    QVERIFY(qtobject_window);
    QVERIFY(item_qtobject);
    QVERIFY(item_item);
    QVERIFY(item_window);
    QVERIFY(window_qtobject);
    QVERIFY(window_item);
    QVERIFY(window_window);

    QVERIFY(QByteArray(qtobject_item->metaObject()->className()).startsWith("QQuickItem"));
    QVERIFY(QByteArray(qtobject_window->metaObject()->className()).startsWith("QQuickWindow"));
    QVERIFY(QByteArray(item_item->metaObject()->className()).startsWith("QQuickItem"));
    QVERIFY(QByteArray(item_window->metaObject()->className()).startsWith("QQuickWindow"));
    QVERIFY(QByteArray(window_item->metaObject()->className()).startsWith("QQuickItem"));
    QVERIFY(QByteArray(window_window->metaObject()->className()).startsWith("QQuickWindow"));

    QCOMPARE(qtobject_qtobject->parent(), qtobjectParent);
    QCOMPARE(qtobject_item->parent(), qtobjectParent);
    QCOMPARE(qtobject_window->parent(), qtobjectParent);
    QCOMPARE(item_qtobject->parent(), itemParent);
    QCOMPARE(item_item->parent(), itemParent);
    QCOMPARE(item_window->parent(), itemParent);
    QCOMPARE(window_qtobject->parent(), windowParent);
    QCOMPARE(window_item->parent(), windowParent);
    QCOMPARE(window_window->parent(), windowParent);

    QCOMPARE(qobject_cast<QQuickItem *>(qtobject_item)->parentItem(), (QQuickItem *)nullptr);
    QCOMPARE(qobject_cast<QQuickWindow *>(qtobject_window)->transientParent(), (QQuickWindow *)nullptr);
    QCOMPARE(qobject_cast<QQuickItem *>(item_item)->parentItem(), itemParent);
    QCOMPARE(qobject_cast<QQuickWindow *>(item_window)->transientParent(), itemParent->window());
    QCOMPARE(qobject_cast<QQuickItem *>(window_item)->parentItem(), windowParent->contentItem());
    QCOMPARE(qobject_cast<QQuickWindow *>(window_window)->transientParent(), windowParent);
}

void tst_qqmlcomponent::qmlCreateObjectWithProperties()
{
    QQmlEngine engine;

    QTest::ignoreMessage(
            QtMsgType::QtWarningMsg,
            QRegularExpression(".*createObjectWithScript.qml: Setting initial properties failed: "
                               "Item does not have a property called not_i"));
    QTest::ignoreMessage(
            QtMsgType::QtWarningMsg,
            QRegularExpression(
                    ".*createObjectWithScript.qml:45:13: Required property i was not initialized"));

    QQmlComponent component(&engine, testFileUrl("createObjectWithScript.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    {
        QScopedPointer<QObject> testObject1(object->property("declarativerectangle")
                                                    .value<QObject*>());
        QVERIFY(testObject1);
        QCOMPARE(testObject1->parent(), object.data());
        QCOMPARE(testObject1->property("x").value<int>(), 17);
        QCOMPARE(testObject1->property("y").value<int>(), 17);
        QCOMPARE(testObject1->property("color").value<QColor>(), QColor(255,255,255));
        QCOMPARE(QQmlProperty::read(testObject1.data(),"border.width").toInt(), 3);
        QCOMPARE(QQmlProperty::read(testObject1.data(),"innerRect.border.width").toInt(), 20);
    }

    {
        QScopedPointer<QObject> testObject2(object->property("declarativeitem").value<QObject*>());
        QVERIFY(testObject2);
        QCOMPARE(testObject2->parent(), object.data());
        //QCOMPARE(testObject2->metaObject()->className(), "QDeclarativeItem_QML_2");
        QCOMPARE(testObject2->property("x").value<int>(), 17);
        QCOMPARE(testObject2->property("y").value<int>(), 17);
        QCOMPARE(testObject2->property("testBool").value<bool>(), true);
        QCOMPARE(testObject2->property("testInt").value<int>(), 17);
        QCOMPARE(testObject2->property("testObject").value<QObject*>(), object.data());
    }

    {
        QScopedPointer<QObject> testBindingObj(object->property("bindingTestObject")
                                                       .value<QObject*>());
        QVERIFY(testBindingObj);
        QCOMPARE(testBindingObj->parent(), object.data());
        QCOMPARE(testBindingObj->property("testValue").value<int>(), 300);
        object->setProperty("width", 150);
        QCOMPARE(testBindingObj->property("testValue").value<int>(), 150 * 3);
    }

    {
        QScopedPointer<QObject> testBindingThisObj(object->property("bindingThisTestObject")
                                                           .value<QObject*>());
        QVERIFY(testBindingThisObj);
        QCOMPARE(testBindingThisObj->parent(), object.data());
        QCOMPARE(testBindingThisObj->property("testValue").value<int>(), 900);
        testBindingThisObj->setProperty("width", 200);
        QCOMPARE(testBindingThisObj->property("testValue").value<int>(), 200 * 3);
    }

    {
        QScopedPointer<QObject> badRequired(object->property("badRequired").value<QObject *>());
        QVERIFY(!badRequired);

        QScopedPointer<QObject> goodRequired(object->property("goodRequired").value<QObject *>());
        QVERIFY(goodRequired);
        QCOMPARE(goodRequired->parent(), object.data());
        QCOMPARE(goodRequired->property("i").value<int>(), 42);
    }

    {
        QScopedPointer<QObject> bindingAsInitial(object->property("bindingAsInitial").value<QObject *>());
        QVERIFY(bindingAsInitial);
        QVERIFY(object->property("bindingUsed").toBool());
    }
}

void tst_qqmlcomponent::qmlCreateObjectClean()
{
    QQmlEngine engine;
    QVERIFY(engine.outputWarningsToStandardError());
    QObject::connect(&engine, &QQmlEngine::warnings, [](const QList<QQmlError> &) {
        QFAIL("Calls with suitable parameters should not generate any warnings.");
    });
    QQmlComponent component(&engine, testFileUrl("createObjectClean.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QVERIFY(qvariant_cast<QObject *>(object->property("a")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(object->property("b")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(object->property("c")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(object->property("d")) != nullptr);
}

void tst_qqmlcomponent::qmlCreateObjectDirty()
{
    QQmlEngine engine;
    engine.setOutputWarningsToStandardError(false);
    QObject::connect(&engine, &QQmlEngine::warnings, [](const QList<QQmlError> &warnings) {
        QCOMPARE(warnings.size(), 1);
        QCOMPARE(warnings[0].description(),
                "QML Component: Unsuitable arguments passed to createObject(). The first argument "
                "should be a QObject* or null, and the second argument should be a JavaScript "
                "object or a QVariantMap");
    });
    QQmlComponent component(&engine, testFileUrl("createObjectDirty.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QVERIFY(qvariant_cast<QObject *>(object->property("a")) != nullptr);
}

void tst_qqmlcomponent::qmlCreateParentReference()
{
    QQmlEngine engine;

    QCOMPARE(engine.outputWarningsToStandardError(), true);

    QQmlTestMessageHandler messageHandler;

    QQmlComponent component(&engine, testFileUrl("createParentReference.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QObject *object = component.create();
    QVERIFY(object != nullptr);

    QVERIFY(QMetaObject::invokeMethod(object, "createChild"));
    delete object;

    engine.setOutputWarningsToStandardError(false);
    QCOMPARE(engine.outputWarningsToStandardError(), false);

    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));
}

void tst_qqmlcomponent::async()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
    server.serveDirectory(dataDirectory());

    QQmlComponent component(&engine);
    ComponentWatcher watcher(&component);
    component.loadUrl(server.url("/TestComponent.qml"), QQmlComponent::Asynchronous);
    QCOMPARE(watcher.loading, 1);
    QTRY_VERIFY(component.isReady());
    QCOMPARE(watcher.ready, 1);
    QCOMPARE(watcher.error, 0);

    QObject *object = component.create();
    QVERIFY(object != nullptr);

    delete object;
}

void tst_qqmlcomponent::asyncHierarchy()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
    server.serveDirectory(dataDirectory());

    // ensure that the item hierarchy is compiled correctly.
    QQmlComponent component(&engine);
    ComponentWatcher watcher(&component);
    component.loadUrl(server.url("/TestComponent.2.qml"), QQmlComponent::Asynchronous);
    QCOMPARE(watcher.loading, 1);
    QTRY_VERIFY(component.isReady());
    QCOMPARE(watcher.ready, 1);
    QCOMPARE(watcher.error, 0);

    QObject *root = component.create();
    QVERIFY(root != nullptr);

    // ensure that the parent-child relationship hierarchy is correct
    // (use QQuickItem* for all children rather than types which are not publicly exported)
    QQuickItem *c1 = root->findChild<QQuickItem*>("c1", Qt::FindDirectChildrenOnly);
    QVERIFY(c1);
    QQuickItem *c1c1 = c1->findChild<QQuickItem*>("c1c1", Qt::FindDirectChildrenOnly);
    QVERIFY(c1c1);
    QQuickItem *c1c2 = c1->findChild<QQuickItem*>("c1c2", Qt::FindDirectChildrenOnly);
    QVERIFY(c1c2);
    QQuickItem *c1c2c3 = c1c2->findChild<QQuickItem*>("c1c2c3", Qt::FindDirectChildrenOnly);
    QVERIFY(c1c2c3);
    QQuickItem *c2 = root->findChild<QQuickItem*>("c2", Qt::FindDirectChildrenOnly);
    QVERIFY(c2);
    QQuickItem *c2c1 = c2->findChild<QQuickItem*>("c2c1", Qt::FindDirectChildrenOnly);
    QVERIFY(c2c1);
    QQuickItem *c2c1c1 = c2c1->findChild<QQuickItem*>("c2c1c1", Qt::FindDirectChildrenOnly);
    QVERIFY(c2c1c1);
    QQuickItem *c2c1c2 = c2c1->findChild<QQuickItem*>("c2c1c2", Qt::FindDirectChildrenOnly);
    QVERIFY(c2c1c2);

    // ensure that values and bindings are assigned correctly
    QVERIFY(root->property("success").toBool());

    delete root;
}

void tst_qqmlcomponent::asyncForceSync()
{
    {
        // 1) make sure that HTTP URLs cannot be completed synchronously
        TestHTTPServer server;
        QVERIFY2(server.listen(), qPrintable(server.errorString()));
        server.serveDirectory(dataDirectory());

        // ensure that the item hierarchy is compiled correctly.
        QQmlComponent component(&engine);
        component.loadUrl(server.url("/TestComponent.2.qml"), QQmlComponent::Asynchronous);
        QCOMPARE(component.status(), QQmlComponent::Loading);
        QQmlComponent component2(&engine, server.url("/TestComponent.2.qml"), QQmlComponent::PreferSynchronous);
        QCOMPARE(component2.status(), QQmlComponent::Loading);
    }
    {
        // 2) make sure that file:// URL can be completed synchronously

        // ensure that the item hierarchy is compiled correctly.
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("/TestComponent.2.qml"), QQmlComponent::Asynchronous);
        QCOMPARE(component.status(), QQmlComponent::Loading);
        QQmlComponent component2(&engine, testFileUrl("/TestComponent.2.qml"), QQmlComponent::PreferSynchronous);
        QCOMPARE(component2.status(), QQmlComponent::Ready);
        QCOMPARE(component.status(), QQmlComponent::Loading);
        QTRY_COMPARE_WITH_TIMEOUT(component.status(), QQmlComponent::Ready, 0);
    }
}

void tst_qqmlcomponent::componentUrlCanonicalization()
{
    // ensure that url canonicalization succeeds so that type information
    // is not generated multiple times for the same component.
    {
        // load components via import
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("componentUrlCanonicalization.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QVERIFY(object->property("success").toBool());
    }

    {
        // load one of the components dynamically, which would trigger
        // import of the other if it were not already loaded.
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("componentUrlCanonicalization.2.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QVERIFY(object->property("success").toBool());
    }

    {
        // load components with more deeply nested imports
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("componentUrlCanonicalization.3.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QVERIFY(object->property("success").toBool());
    }

    {
        // load components with unusually specified import paths
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("componentUrlCanonicalization.4.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QVERIFY(object->property("success").toBool());
    }

    {
        // Do not crash with various nonsense import paths
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("componentUrlCanonicalization.5.qml"));
        QTest::ignoreMessage(QtWarningMsg, QLatin1String("QQmlComponent: Component is not ready").data());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object.isNull());
    }
}

void tst_qqmlcomponent::onDestructionLookup()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("onDestructionLookup.qml"));
    QScopedPointer<QObject> object(component.create());
    gc(engine);
    QVERIFY(object != nullptr);
    QVERIFY(object->property("success").toBool());
}

void tst_qqmlcomponent::onDestructionCount()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("onDestructionCount.qml"));

    QLatin1String warning("Component.onDestruction");

    {
        // Warning should be emitted during create()
        QTest::ignoreMessage(QtWarningMsg, warning.data());

        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
    }

    // Warning should not be emitted any further
    QCOMPARE(engine.outputWarningsToStandardError(), true);

    QStringList warnings;
    {
        QQmlTestMessageHandler messageHandler;

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        warnings = messageHandler.messages();
    }

    engine.setOutputWarningsToStandardError(false);
    QCOMPARE(engine.outputWarningsToStandardError(), false);

    QCOMPARE(warnings.size(), 0);
}

void tst_qqmlcomponent::recursion()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("recursion.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QTest::ignoreMessage(QtWarningMsg, QLatin1String("QQmlComponent: Component creation is recursing - aborting").data());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    // Sub-object creation does not succeed
    QCOMPARE(object->property("success").toBool(), false);
}

void tst_qqmlcomponent::recursionContinuation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("recursionContinuation.qml"));

    for (int i = 0; i < 10; ++i)
        QTest::ignoreMessage(QtWarningMsg, QLatin1String("QQmlComponent: Component creation is recursing - aborting").data());

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    // Eventual sub-object creation succeeds
    QVERIFY(object->property("success").toBool());
}

void tst_qqmlcomponent::partialComponentCreation()
{
    const int maxCount = 17;
    QQmlEngine engine;
    QScopedPointer<QQmlComponent> components[maxCount];
    QScopedPointer<QObject> objects[maxCount];
    QQmlTestMessageHandler messageHandler;

    QCOMPARE(engine.outputWarningsToStandardError(), true);

    for (int i = 0; i < maxCount; i++) {
        components[i].reset(new QQmlComponent(&engine, testFileUrl("QtObjectComponent.qml")));
        objects[i].reset(components[i]->beginCreate(engine.rootContext()));
        QVERIFY(objects[i].isNull() == false);
    }
    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));

    for (int i = 0; i < maxCount; i++) {
        components[i]->completeCreate();
    }
    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));
}

class CallingContextCheckingClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
public:
    CallingContextCheckingClass()
        : m_value(0)
    {}

    int value() const { return m_value; }
    void setValue(int v) {
        scopeObject.clear();
        callingContextData.setContextData(nullptr);

        m_value = v;
        QJSEngine *jsEngine = qjsEngine(this);
        if (!jsEngine)
            return;
        QV4::ExecutionEngine *v4 = jsEngine->handle();
        if (!v4)
            return;
        QV4::Scope scope(v4);
        QV4::Scoped<QV4::QmlContext> qmlContext(scope, v4->qmlContext());
        if (!qmlContext)
            return;
        callingContextData = qmlContext->qmlContext();
        scopeObject = qmlContext->qmlScope();
    }

    int m_value;
    QQmlGuardedContextData callingContextData;
    QPointer<QObject> scopeObject;
};

void tst_qqmlcomponent::callingContextForInitialProperties()
{
    qmlRegisterType<CallingContextCheckingClass>("qqmlcomponenttest", 1, 0, "CallingContextCheckingClass");

    QQmlComponent testFactory(&engine, testFileUrl("callingQmlContextComponent.qml"));

    QQmlComponent component(&engine, testFileUrl("callingQmlContext.qml"));
    QScopedPointer<QObject> root(component.beginCreate(engine.rootContext()));
    QVERIFY(!root.isNull());
    root->setProperty("factory", QVariant::fromValue(&testFactory));
    component.completeCreate();
    QTRY_VERIFY(qvariant_cast<QObject *>(root->property("incubatedObject")));
    QObject *o = qvariant_cast<QObject *>(root->property("incubatedObject"));
    CallingContextCheckingClass *checker = qobject_cast<CallingContextCheckingClass*>(o);
    QVERIFY(checker);

    QVERIFY(!checker->callingContextData.isNull());
    QVERIFY(checker->callingContextData->urlString().endsWith(QStringLiteral("callingQmlContext.qml")));

    QVERIFY(!checker->scopeObject.isNull());
    QVERIFY(checker->scopeObject->metaObject()->indexOfProperty("incubatedObject") != -1);
}

void tst_qqmlcomponent::setNonExistentInitialProperty()
{
    QQmlIncubationController controller;
    QQmlEngine engine;
    engine.setIncubationController(&controller);
    QQmlComponent component(&engine, testFileUrl("nonExistentInitialProperty.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QMetaObject::invokeMethod(obj.data(), "startIncubation");
    QJSValue incubatorStatus = obj->property("incubator").value<QJSValue>();
    incubatorStatus.property("forceCompletion").callWithInstance(incubatorStatus);
    QJSValue objectWrapper = incubatorStatus.property("object");
    QVERIFY(objectWrapper.isQObject());
    QPointer<QObject> object(objectWrapper.toQObject());
    QVERIFY(object->property("ok").toBool());
}

void tst_qqmlcomponent::relativeUrl_data()
{
    QTest::addColumn<QUrl>("url");

#if !defined(Q_OS_ANDROID)
    QTest::addRow("fromLocalFile") << QUrl::fromLocalFile("data/QtObjectComponent.qml");
    QTest::addRow("fromLocalFileHash") << QUrl::fromLocalFile("data/QtObjectComponent#2.qml");
    QTest::addRow("constructor") << QUrl("data/QtObjectComponent.qml");
#endif
    QTest::addRow("absolute") << QUrl::fromLocalFile(QFINDTESTDATA("data/QtObjectComponent.qml"));
    QTest::addRow("qrc") << QUrl("qrc:/data/QtObjectComponent.qml");
}

void tst_qqmlcomponent::relativeUrl()
{
    QFETCH(QUrl, url);

    QQmlComponent component(&engine);
    // Shouldn't assert in QQmlTypeLoader; we want QQmlComponent to assume that
    // data/QtObjectComponent.qml refers to the data/QtObjectComponent.qml in the current working directory.
    component.loadUrl(url);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
}

void tst_qqmlcomponent::setDataNoEngineNoSegfault()
{
    QQmlEngine eng;
    QQmlComponent comp;
    QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Must provide an engine before calling setData");
    comp.setData("import QtQuick 1.0; QtObject { }", QUrl(""));
    QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Must provide an engine before calling create");
    auto c = comp.create();
    QVERIFY(!c);
}

class RequiredDefaultCpp : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QQuickItem *defaultProperty MEMBER m_defaultProperty NOTIFY defaultPropertyChanged REQUIRED)
    Q_SIGNAL void defaultPropertyChanged();
    Q_CLASSINFO("DefaultProperty", "defaultProperty")
private:
    QQuickItem *m_defaultProperty = nullptr;
};

class TwoRequiredProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged REQUIRED)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged REQUIRED)

    int m_index = 0;
    QString m_name;

public:
    TwoRequiredProperties(QObject *parent = nullptr) : QObject(parent) { }

    int index() const { return m_index; }
    QString name() const { return m_name; }

    void setIndex(int x)
    {
        if (m_index == x)
            return;
        m_index = x;
        Q_EMIT indexChanged();
    }
    void setName(const QString &x)
    {
        if (m_name == x)
            return;
        m_name = x;
        Q_EMIT nameChanged();
    }

Q_SIGNALS:
    void indexChanged();
    void nameChanged();
};

class ShadowedRequiredProperty : public TwoRequiredProperties
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged /* REQUIRED */)
    // Note: should be able to take methods from the base class
};

class AttachedRequiredProperties : public QObject
{
    Q_OBJECT
    QML_ATTACHED(TwoRequiredProperties)
public:
    AttachedRequiredProperties(QObject *parent = nullptr) : QObject(parent) { }
    static TwoRequiredProperties *qmlAttachedProperties(QObject *parent)
    {
        return new TwoRequiredProperties(parent);
    }
};

class GroupedRequiredProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TwoRequiredProperties *group READ group)
    TwoRequiredProperties m_group;

public:
    GroupedRequiredProperties(QObject *parent = nullptr) : QObject(parent) { }
    TwoRequiredProperties *group() { return &m_group; }
};

class RequiredGroup : public QObject
{
    Q_OBJECT
    // use QQuickPalette not to create an extra test-only
    Q_PROPERTY(QQuickPalette *group READ group REQUIRED)
    QQuickPalette m_group;

public:
    RequiredGroup(QObject *parent = nullptr) : QObject(parent) { }
    QQuickPalette *group() { return &m_group; }
};

void tst_qqmlcomponent::testRequiredProperties_data()
{
    qmlRegisterType<RequiredDefaultCpp>("qt.test", 1, 0, "RequiredDefaultCpp");
    qmlRegisterType<TwoRequiredProperties>("qt.test", 1, 0, "TwoRequiredProperties");
    qmlRegisterType<ShadowedRequiredProperty>("qt.test", 1, 0, "ShadowedRequiredProperty");
    qmlRegisterUncreatableType<AttachedRequiredProperties>(
            "qt.test", 1, 0, "AttachedRequiredProperty",
            tr("AttachedRequiredProperties is an attached property"));
    qmlRegisterType<GroupedRequiredProperties>("qt.test", 1, 0, "GroupedRequiredProperty");
    qmlRegisterType<RequiredGroup>("qt.test", 1, 0, "RequiredGroup");

    QTest::addColumn<QUrl>("testFile");
    QTest::addColumn<bool>("shouldSucceed");
    QTest::addColumn<QString>("errorMsg");

    QTest::addRow("requiredSetViaChainedAlias") << testFileUrl("requiredSetViaChainedAlias.qml") << true << "";
    QTest::addRow("requiredNotSet") << testFileUrl("requiredNotSet.qml") << false << "Required property i was not initialized";
    QTest::addRow("requiredSetInSameFile") << testFileUrl("RequiredSetInSameFile.qml") << true << "";
    QTest::addRow("requiredSetViaAlias1") << testFileUrl("requiredSetViaAliasBeforeSameFile.qml") << true << "";
    QTest::addRow("requiredSetViaAlias2") << testFileUrl("requiredSetViaAliasAfterSameFile.qml") << true << "";
    QTest::addRow("requiredSetViaAlias3") << testFileUrl("requiredSetViaAliasParentFile.qml") << true << "";
    QTest::addRow("requiredSetInBase") << testFileUrl("requiredChildOfGoodBase.qml") << true << "";
    QTest::addRow("requiredNotSetInBase") << testFileUrl("requiredChildOfBadBase.qml") << false << "Required property i was not initialized";
    QTest::addRow("rerequiredSet") << testFileUrl("rerequiredSet.qml") << true << "";
    QTest::addRow("rerequiredNotSet") << testFileUrl("RerequiredNotSet.qml") << false
                                      << "Required property i was not initialized";
    QTest::addRow("requiredSetLater(rerequired)")
            << testFileUrl("requiredSetLater.rerequired.qml") << true << "";
    QTest::addRow("rerequiredSetLater") << testFileUrl("rerequiredSetLater.qml") << true << "";
    QTest::addRow("shadowing") << testFileUrl("shadowing.qml") << false <<  "Required property i was not initialized";
    QTest::addRow("shadowing (C++)") << testFileUrl("shadowingFromCpp.qml") << false
                                     << "Required property name was not initialized";
    QTest::addRow("shadowing (C++ indirect)") << testFileUrl("shadowingFromQmlChild.qml") << false
                                              << "Required property name was not initialized";
    QTest::addRow("setLater") << testFileUrl("requiredSetLater.qml") << true << "";
    QTest::addRow("setViaAliasToSubcomponent") << testFileUrl("setViaAliasToSubcomponent.qml") << true << "";
    QTest::addRow("aliasToSubcomponentNotSet") << testFileUrl("aliasToSubcomponentNotSet.qml") << false << "It can be set via the alias property i_alias";
    QTest::addRow("required default set") << testFileUrl("requiredDefault.1.qml") << true << "";
    QTest::addRow("required default not set") << testFileUrl("requiredDefault.2.qml") << false << "Required property requiredDefault was not initialized";
    QTest::addRow("required default set (C++)") << testFileUrl("requiredDefault.3.qml") << true << "";
    QTest::addRow("required default not set (C++)") << testFileUrl("requiredDefault.4.qml") << false << "Required property defaultProperty was not initialized";
    // QTBUG-96200:
    QTest::addRow("required two set one (C++)") << testFileUrl("requiredTwoProperties.qml") << false
                                                << "Required property name was not initialized";
    QTest::addRow("required two set two (C++)")
            << testFileUrl("RequiredTwoPropertiesSet.qml") << true << "";
    QTest::addRow("required two set two (C++ indirect)")
            << testFileUrl("requiredTwoPropertiesDummy.qml") << true << "";

    QTest::addRow("required set (inline component)")
            << testFileUrl("requiredPropertyInlineComponent.good.qml") << true << "";
    QTest::addRow("required not set (inline component)")
            << testFileUrl("requiredPropertyInlineComponent.bad.qml") << false
            << "Required property i was not initialized";
    QTest::addRow("required set (inline component, C++)")
            << testFileUrl("requiredPropertyInlineComponentWithCppBase.good.qml") << true << "";
    QTest::addRow("required not set (inline component, C++)")
            << testFileUrl("requiredPropertyInlineComponentWithCppBase.bad.qml") << false
            << "Required property name was not initialized";
    QTest::addRow("required set (inline component, C++ indirect)")
            << testFileUrl("requiredPropertyInlineComponentWithIndirectCppBase.qml") << true << "";
    QTest::addRow("required not set (attached)")
            << testFileUrl("requiredPropertiesInAttached.bad.qml") << false
            << "Attached property has required properties. This is not supported";
    QTest::addRow("required two set one (attached)")
            << testFileUrl("requiredPropertiesInAttached.bad2.qml") << false
            << "Attached property has required properties. This is not supported";
    QTest::addRow("required two set two (attached)")
            << testFileUrl("RequiredPropertiesInAttached.qml") << false
            << "Attached property has required properties. This is not supported";
    QTest::addRow("required two set two (attached indirect)")
            << testFileUrl("requiredPropertiesInAttachedIndirect.qml") << false
            << "Attached property has required properties. This is not supported";
    QTest::addRow("required itself not set (group)")
            << testFileUrl("requiredGroup.bad.qml") << false
            << "Required property group was not initialized";
    QTest::addRow("required itself set (group)")
            << testFileUrl("requiredGroup.good.qml") << true << "";
    QTest::addRow("required not set (group)")
            << testFileUrl("requiredPropertiesInGroup.bad.qml") << false
            << "Required property index was not initialized";
    QTest::addRow("required two set one (group)")
            << testFileUrl("requiredPropertiesInGroup.bad2.qml") << false
            << "Required property name was not initialized";
    QTest::addRow("required two set two (group)")
            << testFileUrl("RequiredPropertiesInGroup.qml") << true << "";
    QTest::addRow("required two set two (group indirect)")
            << testFileUrl("requiredPropertiesInGroupIndirect.qml") << true << "";
}

void tst_qqmlcomponent::testRequiredProperties()
{
    QQmlEngine eng;
    using QScopedObjPointer = QScopedPointer<QObject>;
    QFETCH(QUrl, testFile);
    QFETCH(bool, shouldSucceed);
    QQmlComponent comp(&eng);
    comp.loadUrl(testFile);
    QScopedObjPointer obj {comp.create()};
    QEXPECT_FAIL("required not set (group)",
                 "We fail to recognize required sub-properties inside a group property when that "
                 "group property is unused (QTBUG-96544)",
                 Abort);
    QEXPECT_FAIL("required two set one (group)",
                 "We fail to recognized required sub-properties inside a group property, even when "
                 "that group property is used (QTBUG-96544)",
                 Abort);
    if (shouldSucceed) {
        QVERIFY2(comp.isReady(), qPrintable(comp.errorString()));
        QVERIFY(obj);
    } else {
        QVERIFY2(!obj, "The object is valid when it shouldn't be");
        QFETCH(QString, errorMsg);
        QVERIFY2(comp.errorString().contains(errorMsg), qPrintable(comp.errorString()));
    }
}

void tst_qqmlcomponent::testRequiredPropertiesFromQml()
{
    QQmlEngine eng;
    {
        QQmlComponent comp(&eng);
        comp.loadUrl(testFileUrl("createdFromQml.qml"));
        QScopedPointer<QObject> obj { comp.create() };
        QVERIFY(obj);
        auto root = qvariant_cast<QQuickItem*>(obj->property("it"));
        QVERIFY(root);
        QCOMPARE(root->property("i").toInt(), 42);
    }
    {
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, QRegularExpression(".*requiredNotSet.qml:4:5: Required property i was not initialized"));
        QQmlComponent comp(&eng);
        comp.loadUrl(testFileUrl("createdFromQmlFail.qml"));
        QScopedPointer<QObject> obj { comp.create() };
        QVERIFY(obj);
        QCOMPARE(qvariant_cast<QQuickItem *>(obj->property("it")), nullptr);
    }
}

struct ComponentWithPublicSetInitial : QQmlComponent
{
    using QQmlComponent::QQmlComponent;
    void setInitialProperties(QObject *o, QVariantMap map)
    {
        QQmlComponent::setInitialProperties(o, map);
    }
};

void tst_qqmlcomponent::testSetInitialProperties()
{
    QQmlEngine eng;
    {
        //  QVariant
        ComponentWithPublicSetInitial comp(&eng);
        comp.loadUrl(testFileUrl("variantBasedInitialization.qml"));
        QScopedPointer<QObject> obj { comp.beginCreate(eng.rootContext()) };
        QVERIFY(obj);
        QUrl myurl = comp.url();
        QFont myfont;
        QDateTime mydate = QDateTime::currentDateTime();
        QPoint mypoint {1,2};
        QSizeF mysize {0.5, 0.3};
        QMatrix4x4 matrix {};
        QQuaternion quat {5.0f, 0.3f, 0.2f, 0.1f};
        QVector2D vec2 {2.0f, 3.1f};
        QVector3D vec3 {1.0f, 2.0, 3.0f};
        QVector4D vec4 {1.0f, 2.0f, 3.0f, 4.0f};
#define ASJSON(NAME) {QLatin1String(#NAME), NAME}
        comp.setInitialProperties(obj.get(), QVariantMap {
                                                     {QLatin1String("i"), 42},
                                                     {QLatin1String("b"), true},
                                                     {QLatin1String("d"), 3.1416},
                                                     {QLatin1String("s"), QLatin1String("hello world")},
                                                     {QLatin1String("nothing"), QVariant::fromValue( nullptr)},
                                                     ASJSON(myurl),
                                                     ASJSON(myfont),
                                                     ASJSON(mydate),
                                                     ASJSON(mypoint),
                                                     ASJSON(mysize),
                                                     ASJSON(matrix),
                                                     ASJSON(quat),
                                                     ASJSON(vec2), ASJSON(vec3), ASJSON(vec4)
                                             });
#undef ASJSON
        comp.completeCreate();
        QVERIFY(comp.errors().empty());
        QCOMPARE(obj->property("i"), 42);
        QCOMPARE(obj->property("b"), true);
        QCOMPARE(obj->property("d"), 3.1416);
        QCOMPARE(obj->property("s"), QLatin1String("hello world"));
        QCOMPARE(obj->property("nothing"), QVariant::fromValue(nullptr));
#define COMPARE(NAME) QCOMPARE(obj->property(#NAME), NAME)
        COMPARE(myurl);
        COMPARE(myfont);
        COMPARE(mydate);
        QCOMPARE(obj->property("mypoint"), QPointF(mypoint));
        COMPARE(mysize);
        COMPARE(matrix);
        COMPARE(quat);
        COMPARE(vec2);
        COMPARE(vec3);
        COMPARE(vec4);
#undef COMPARE

    }
    {
        // createWithInitialProperties convenience function
        QQmlComponent comp(&eng);
        comp.loadUrl(testFileUrl("requiredNotSet.qml"));
        QScopedPointer<QObject> obj {comp.createWithInitialProperties( QVariantMap { {QLatin1String("i"), QJsonValue{42}}  })};
        QVERIFY(obj);
        QCOMPARE(obj->property("i"), 42);
    }
    {
        // createWithInitialProperties: setting a nonexistent property
        QQmlComponent comp(&eng);
        comp.loadUrl(testFileUrl("allJSONTypes.qml"));
        const QRegularExpression errorMessage { QStringLiteral(
                ".*allJSONTypes.qml: Setting initial properties failed: Item does not have a "
                "property called notThePropertiesYoureLookingFor") };
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, errorMessage);
        QScopedPointer<QObject> obj {
            comp.createWithInitialProperties(QVariantMap { {"notThePropertiesYoureLookingFor", 42} })
        };
        QVERIFY(obj);
        QVERIFY(comp.isReady()); // despite the error, the component is still ready

        // QTBUG-101439: repeated creation succeeds as well
        QScopedPointer<QObject> objEmpty { comp.create() };
        QVERIFY(objEmpty);
    }

    {
        QQmlComponent comp(&eng);
        comp.loadUrl(testFileUrl("requiredNotSet.qml"));
        QTest::ignoreMessage(
                QtMsgType::QtWarningMsg,
                QRegularExpression(".*requiredNotSet.qml: Setting initial properties failed: Item "
                                   "does not have a property called not_i"));
        QScopedPointer<QObject> obj { comp.createWithInitialProperties(
                QVariantMap { { QLatin1String("not_i"), QJsonValue { 42 } } }) };
        QVERIFY(!obj);
        QVERIFY(comp.isError());
        QVERIFY(comp.errorString().contains("Required property i was not initialized"));

        QScopedPointer<QObject> objGood { comp.createWithInitialProperties(
                QVariantMap { { QLatin1String("i"), QJsonValue { 42 } } }) };
        QVERIFY2(objGood, qPrintable(comp.errorString()));
        QCOMPARE(objGood->property("i"), 42);
    }

    // QJSValue unpacking - QTBUG-101440
    {
        QQmlComponent comp(&eng);
        comp.setData(R"(
            import QtQml
            QtObject {
                property int x
                property int y: func ? func() : -1
                property var func // special
            }
        )", QUrl());

        QJSValue data = eng.evaluate("({ \"x\": 42, \"func\": (function() { return 42; }) })");
        QVERIFY(data.isObject());
        QVariant var = data.toVariant();
        QCOMPARE(var.typeId(), QMetaType::QVariantMap);
        QVariantMap properties = var.toMap();
        QScopedPointer<QObject> object { comp.createWithInitialProperties(properties) };
        QVERIFY(object);
        QCOMPARE(object->property("x"), 42);
        QCOMPARE(object->property("y"), 42);
        QJSValue func = eng.toScriptValue(object->property("func"));
        QVERIFY(func.isCallable());
        QCOMPARE(func.call().toInt(), 42);
    }
}

void tst_qqmlcomponent::createInsideJSModule()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("jsmodule/test.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->property("ok").toBool());
}

void tst_qqmlcomponent::qmlErrorIsReported()
{
    struct LogControl
    {
        LogControl() { QLoggingCategory::setFilterRules("qt.qml.diskcache.debug=true"); }
        ~LogControl() { QLoggingCategory::setFilterRules(QString()); }
    };
    LogControl lc;
    Q_UNUSED(lc);

    QRegularExpression errorMessage(
            R"(.*Cannot assign to non-existent property.*onSomePropertyChanged.*)");
    QTest::ignoreMessage(QtDebugMsg, errorMessage);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("qmlWithError.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root == nullptr);
    QVERIFY(component.isError());
    const auto componentErrors = component.errors();
    QVERIFY(std::any_of(componentErrors.begin(), componentErrors.end(), [&](const QQmlError &e) {
        return errorMessage.match(e.toString()).hasMatch();
    }));
}

void tst_qqmlcomponent::initJSValueProp()
{
    qmlRegisterType<WithQJSValue>("ComponentTest", 1, 0, "WithQJSValue");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import ComponentTest\nWithQJSValue {}", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.createWithInitialProperties({{ u"v"_s, 5}}));
    QVERIFY(!o.isNull());
    WithQJSValue *withQJSValue = qobject_cast<WithQJSValue *>(o.data());
    QVERIFY(withQJSValue);
    const QJSValue jsValue = withQJSValue->v();
    QVERIFY(jsValue.isNumber());
    QCOMPARE(jsValue.toInt(), 5);
}

void tst_qqmlcomponent::qmlPropertySignalExists()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml; QtObject { property int p: 41; function doStuff() { p++; } }",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QSignalSpy changeSignalSpy(o.get(), SIGNAL(pChanged()));
    QVERIFY(QMetaObject::invokeMethod(o.get(), "doStuff"));
    QCOMPARE(changeSignalSpy.size(), 1);
    QCOMPARE(o->property("p").toInt(), 42);
}

void tst_qqmlcomponent::componentTypes()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        // not allowed: "Cannot create empty component specification"
        component.setData("import QtQml; Component { }", QUrl());
        QVERIFY(!component.isReady());
    }

    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("ComponentType.qml"));
        QScopedPointer<QObject> o(component.create());
        QVERIFY2(!o.isNull(), qPrintable(component.errorString()));
        QQmlComponent *oComponent = qobject_cast<QQmlComponent *>(o.get());
        QVERIFY(oComponent);
        QScopedPointer<QObject> enclosed(oComponent->create());
        QVERIFY(!enclosed.isNull());
        QCOMPARE(enclosed->objectName(), u"enclosed"_s);
    }

    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("componentTypes.qml"));
        QScopedPointer<QObject> o(component.create());
        QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

        QQmlContext *ctx = engine.contextForObject(o.get());

        QObject *normal = ctx->objectForName(u"normal"_s);
        QVERIFY(normal);
        QCOMPARE(normal->property("text").toString(), u"indirect component"_s);

        // check (and thus "document" in code) various ways of how ids work
        QVERIFY(ctx->objectForName(u"accessibleNormal"_s));
        QVERIFY(!ctx->objectForName(u"inaccessibleNormal"_s));
        QVERIFY(ctx->objectForName(u"accessible"_s));
        QVERIFY(!ctx->objectForName(u"inaccessible"_s));
        QVERIFY(ctx->objectForName(u"accessibleDelegate"_s));
        QVERIFY(!ctx->objectForName(u"inaccessibleDelegate"_s));

        QCOMPARE(qvariant_cast<QObject *>(o->property("p2"))->property("text").toString(),
                 u"foo"_s);
        auto p3Object = qvariant_cast<QObject *>(o->property("p3"));
        QVERIFY(p3Object);
        QVERIFY(p3Object->property("text").toString().isEmpty());

        QQmlComponent *normalComponent = qobject_cast<QQmlComponent *>(normal);
        QVERIFY(normalComponent);
        QScopedPointer<QObject> enclosed(normalComponent->create());
        QVERIFY(enclosed);
        QCOMPARE(enclosed->objectName(), u"enclosed"_s);
    }
}

void tst_qqmlcomponent::boundComponent()
{
    QQmlEngine engine;
    {
        QQmlComponent component(&engine, testFileUrl("nestedBoundComponent.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QVERIFY(component.isBound());

        QScopedPointer<QObject> o(component.create());
        QVERIFY(!o.isNull());

        QQmlComponent *nestedComponent = o->property("c").value<QQmlComponent *>();
        QVERIFY(nestedComponent != nullptr);
        QVERIFY(nestedComponent->isBound());

        QObject *nestedObject = o->property("o").value<QObject *>();
        QVERIFY(nestedObject != nullptr);
        QCOMPARE(nestedObject->objectName(), QLatin1String("bound"));

        QScopedPointer<QObject> contextedObject(nestedComponent->create(qmlContext(o.data())));
        QVERIFY(!contextedObject.isNull());

        QScopedPointer<QObject> uncontextedObject(nestedComponent->create());
        QVERIFY(uncontextedObject.isNull());

        QVERIFY(nestedComponent->errorString().contains(
                QLatin1String("Cannot instantiate bound component outside its creation context")));
    }

    {
        QQmlComponent component(&engine, testFileUrl("BoundInlineComponent.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QVERIFY(component.isBound());

        QScopedPointer<QObject> o(component.create());
        QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

        QObject *nestedObject = o->property("o").value<QObject *>();
        QVERIFY(nestedObject != nullptr);

        QCOMPARE(nestedObject->objectName(), QLatin1String("inline"));
    }

    {
        QQmlComponent component(&engine, testFileUrl("boundInlineComponentUser.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QVERIFY(!component.isBound());
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o.isNull());

        QVERIFY(component.errorString().contains(
                QLatin1String("Cannot instantiate bound inline component in different file")));

    }

    {
        QQmlComponent component(&engine);
        QVERIFY(!component.isBound());

        component.setData("pragma ComponentBehavior: Bound\nsyntax error", QUrl());
        QCOMPARE(component.errorString(), ":2 Syntax error\n"_L1);
        QVERIFY(!component.isBound());
    }
}

void tst_qqmlcomponent::loadFromModule_data()
{
    using namespace Qt::StringLiterals;

    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("typeName");
    QTest::addColumn<QString>("classNameRe");

    QTest::addRow("Item") << u"QtQuick"_s << u"Item"_s << u"QQuickItem"_s;
#if defined(HAS_CONTROLS)
    QTest::addRow("Button") << u"QtQuick.Controls"_s << u"Button"_s << u"Button_QMLTYPE_\\d+"_s;
    QTest::addRow("Basic.Button") << u"QtQuick.Controls.Basic"_s << u"Button"_s << u"Button_QMLTYPE_\\d+"_s;
#endif

    QTest::addRow("IC") << u"test"_s << u"TestComponentWithIC"_s << u"TestComponentWithIC"_s; // sanity check for next test
    QTest::addRow("IC") << u"test"_s << u"TestComponentWithIC.InnerIC"_s << u"InnerIC"_s;

    QTest::addRow("plainQML") << u"plainqml"_s << u"Plain"_s << u"Plain"_s;
}

void tst_qqmlcomponent::loadFromModule()
{
    QFETCH(QString, uri);
    QFETCH(QString, typeName);
    QFETCH(QString, classNameRe);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    QCOMPARE(component.progress(), 0);
    QSignalSpy progressSpy(&component, &QQmlComponent::progressChanged);
    component.loadFromModule(uri, typeName);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    // verify that we changed the progress correctly to 1
    QVERIFY(!progressSpy.isEmpty() || progressSpy.wait());
    QCOMPARE(progressSpy.last().at(0).toDouble(), 1.0);

    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    QRegularExpression classNameMatcher(classNameRe);
    const char *name = object->metaObject()->className();
    QVERIFY2(classNameMatcher.match(name).hasMatch(),
             name);
}

struct CallVerifyingIncubtor : QQmlIncubator
{
    void setInitialState(QObject *) override { setInitialStateCalled = true; }
    void statusChanged(QQmlIncubator::Status status) override { lastStatus = status; }

    QQmlIncubator::Status lastStatus = QQmlIncubator::Null;
    bool setInitialStateCalled = false;
};

void tst_qqmlcomponent::loadFromModuleThenCreateWithIncubator()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine);
    comp.loadFromModule("QtQuick", "Rectangle");
    CallVerifyingIncubtor incubator;
    comp.create(incubator);
    std::unique_ptr<QObject> object { incubator.object() };
    QVERIFY(incubator.setInitialStateCalled);
    QVERIFY(incubator.isReady());
    QCOMPARE(incubator.lastStatus, QQmlIncubator::Ready);
    QCOMPARE(object->metaObject()->className(), "QQuickRectangle");
}

void tst_qqmlcomponent::loadFromModuleFailures_data()
{

    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("typeName");
    QTest::addColumn<QString>("errorMsg");

    QTest::addRow("noSuchModule") << "Does.Not.Exist"
                                  << "Type"
                                  << "No module named \"Does.Not.Exist\" found";
    QTest::addRow("noSuchType") << "QtQml"
                                << "NoSuchType"
                                << "Module \"QtQml\" contains no type named \"NoSuchType\"";
    QTest::addRow("CppSingleton") << u"QtQuick"_s
                                  << u"Application"_s
                                  << u"Application is a singleton, and cannot be loaded"_s;
}

void tst_qqmlcomponent::loadFromModuleFailures()
{
    QFETCH(QString, uri);
    QFETCH(QString, typeName);
    QFETCH(QString, errorMsg);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    QSignalSpy errorSpy(&component, &QQmlComponent::statusChanged);
    component.loadFromModule(uri, typeName);
    QVERIFY(!errorSpy.isEmpty());
    QCOMPARE(errorSpy.first().first().value<QQmlComponent::Status>(),
             QQmlComponent::Error);
    QVERIFY(!component.errors().isEmpty());
    QCOMPARE(component.errors().constFirst().description(),
             errorMsg);
}

struct SingleRequiredProperty : QObject
{
    Q_OBJECT
    Q_PROPERTY(int i MEMBER i REQUIRED)

    int i = 42;
};

void tst_qqmlcomponent::loadFromModuleRequired()
{

    QQmlEngine engine;
    qmlRegisterType<SingleRequiredProperty>("qqmlcomponenttest", 1, 0, "SingleRequiredProperty");
    QQmlComponent component(&engine, "qqmlcomponenttest", "SingleRequiredProperty");
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root);
}

void tst_qqmlcomponent::loadFromQrc()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QStringLiteral(":/qt/qml/test/data/withAot.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQmlComponentPrivate *p = QQmlComponentPrivate::get(&component);
    QVERIFY(p);
    QVERIFY(p->compilationUnit);
    QVERIFY(p->compilationUnit->aotCompiledFunctions);
}

void tst_qqmlcomponent::removeBinding()
{
    QQmlEngine e;
    const QUrl url = testFileUrl("removeBinding.qml");
    QQmlComponent c(&e, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
        QtWarningMsg,
        qPrintable(url.toString() + QStringLiteral(":7:27: QML Component: Unsuitable arguments "
                                                   "passed to createObject(). The first argument "
                                                   "should be a QObject* or null, and the second "
                                                   "argument should be a JavaScript object or a "
                                                   "QVariantMap")));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("result"), QStringLiteral("42"));
    QCOMPARE(o->property("result2"), QStringLiteral("43"));
}

void tst_qqmlcomponent::complexObjectArgument()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("complexObjectArgument.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), QStringLiteral("26 - 25"));
}

QTEST_MAIN(tst_qqmlcomponent)

#include "tst_qqmlcomponent.moc"
