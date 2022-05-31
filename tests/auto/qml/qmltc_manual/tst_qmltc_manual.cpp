// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QDebug>

#include "testclasses.h"

#include <QtCore/qscopedpointer.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtCore/qproperty.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobjectdefs.h>
#include <QtCore/qmetaobject.h>

#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlanybinding_p.h>
#include <private/qquickitem_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qquickanchors_p.h>

#include <array>
#include <memory>

class tst_qmltc_manual : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qmltc_manual();

private slots:
    void cppBinding();
    void signalHandlers();
    void signalHandlers_qmlcachegen();
    void jsFunctions();
    void changingBindings();
    void propertyAlias();
    void propertyChangeHandler();
    void locallyImported();
    void localImport();
    void neighbors();
    void anchors();

private:
    void signalHandlers_impl(const QUrl &url);
};

tst_qmltc_manual::tst_qmltc_manual()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qmltc_manual::cppBinding()
{
    QQmlEngine e;
    HelloWorld::url = testFileUrl("HelloWorld.qml");
    HelloWorld created(&e);

    QCOMPARE(created.property("hello").toString(), QStringLiteral("Hello, World"));
    QCOMPARE(created.getGreeting(), QStringLiteral("Hello, World!"));
    QCOMPARE(created.property("greeting").toString(), QStringLiteral("Hello, World!"));

    created.setProperty("hello", QStringLiteral("Hello, Qml"));

    QCOMPARE(created.property("hello").toString(), QStringLiteral("Hello, Qml"));
    QCOMPARE(created.property("greeting").toString(), QStringLiteral("Hello, Qml!"));
    QCOMPARE(created.getGreeting(), QStringLiteral("Hello, Qml!"));
}

void tst_qmltc_manual::signalHandlers_impl(const QUrl &url)
{
    QQmlEngine e;
    ANON_signalHandlers::url = url;
    ANON_signalHandlers created(&e);

    // signal emission works from C++
    emit created.signal1();
    emit created.signal2(QStringLiteral("42"), 42);

    QCOMPARE(created.property("signal1P").toInt(), 1);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("42"));
    QCOMPARE(created.property("signal2P2").toInt(), 42);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("4242"));

    // signal emission works through meta object system
    QMetaObject::invokeMethod(&created, "signal1");
    QMetaObject::invokeMethod(&created, "signal2", Q_ARG(QString, QStringLiteral("foo")),
                              Q_ARG(int, 23));

    QCOMPARE(created.property("signal1P").toInt(), 2);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("foo"));
    QCOMPARE(created.property("signal2P2").toInt(), 23);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("foo23"));

    // signal emission works through QML/JS
    created.qmlEmitSignal1();
    created.qmlEmitSignal2();

    QCOMPARE(created.property("signal1P").toInt(), 3);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("xyz"));
    QCOMPARE(created.property("signal2P2").toInt(), 123);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("xyz123"));

    created.qmlEmitSignal2WithArgs(QStringLiteral("abc"), 0);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("abc"));
    QCOMPARE(created.property("signal2P2").toInt(), 0);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("abc0"));
}

void tst_qmltc_manual::signalHandlers()
{
    // use QQmlTypeCompiler's compilation unit
    signalHandlers_impl(testFileUrl("signalHandlers.qml"));
}

void tst_qmltc_manual::signalHandlers_qmlcachegen()
{
    // use qmlcachegen's compilation unit
    signalHandlers_impl(
                QUrl("qrc:/qt/qml/QmltcManualTests/data/signalHandlers.qml"));
}

void tst_qmltc_manual::jsFunctions()
{
    QQmlEngine e;
    ANON_javaScriptFunctions::url = testFileUrl("javaScriptFunctions.qml");
    ANON_javaScriptFunctions created(&e);

    created.func1();
    created.func2(QStringLiteral("abc"));

    QCOMPARE(created.property("func1P").toInt(), 1);
    QCOMPARE(created.property("func2P").toString(), QStringLiteral("abc"));
    QCOMPARE(created.func3(), false);

    created.setProperty("func3P", true);
    QCOMPARE(created.func3(), true);
}

void tst_qmltc_manual::changingBindings()
{
    QQmlEngine e;
    ANON_changingBindings::url = testFileUrl("changingBindings.qml");
    ANON_changingBindings created(&e);

    // test initial binding
    QCOMPARE(created.initialBindingCallCount, 1); // eager evaluation
    QCOMPARE(created.property("p2").toInt(), 2); // p1 + 1
    QCOMPARE(created.initialBindingCallCount, 1);

    // test JS constant value
    created.resetToConstant();
    QCOMPARE(created.property("p2").toInt(), 42); // p2 = 42
    QCOMPARE(created.initialBindingCallCount, 1);

    // test Qt.binding()
    created.resetToNewBinding();
    created.setProperty("p1", 100);
    QCOMPARE(created.property("p2").toInt(), 200); // p1 * 2
    QCOMPARE(created.initialBindingCallCount, 1);

    // test setting initial (C++) binding
    created.setProperty("p1", 11);
    created.resetToInitialBinding();
    QCOMPARE(created.initialBindingCallCount, 2); // eager evaluation
    QCOMPARE(created.property("p2").toInt(), 12); // p1 + 1 (again)
    QCOMPARE(created.initialBindingCallCount, 2);

    // test resetting value through C++
    created.setP2(0);
    created.setP1(-10);

    QCOMPARE(created.property("p2").toInt(), 0);
    QCOMPARE(created.initialBindingCallCount, 2);

    created.setProperty("p2", 1);
    QCOMPARE(created.property("p2").toInt(), 1);
    QCOMPARE(created.initialBindingCallCount, 2);

    // test binding can be set again even after reset from C++
    created.resetToNewBinding();
    QCOMPARE(created.property("p2").toInt(), -20);
    QCOMPARE(created.initialBindingCallCount, 2);
}

void tst_qmltc_manual::propertyAlias()
{
    QQmlEngine e;
    ANON_propertyAlias::url = testFileUrl("propertyAlias.qml");
    ANON_propertyAlias created(&e);

    // test initial binding
    QCOMPARE(created.property("origin").toInt(), 6); // dummy / 2
    QCOMPARE(created.property("aliasToOrigin").toInt(), 6);

    QCOMPARE(created.getAliasValue(), 6);
    QCOMPARE(created.getAliasToOrigin(), 6);
    created.setDummy(10);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 5);
    QCOMPARE(created.getAliasValue(), 5);
    QCOMPARE(created.getAliasToOrigin(), 5);

    // test the C++ setter
    created.setOrigin(7);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 7);
    QCOMPARE(created.getAliasValue(), 7);
    QCOMPARE(created.getAliasToOrigin(), 7);

    // test meta-object setter
    created.setProperty("origin", 1);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 1);
    QCOMPARE(created.getAliasValue(), 1);
    QCOMPARE(created.getAliasToOrigin(), 1);

    // test QML/JS setter
    created.resetOriginToConstant();
    QCOMPARE(created.property("aliasToOrigin").toInt(), 189);
    QCOMPARE(created.getAliasValue(), 189);
    QCOMPARE(created.getAliasToOrigin(), 189);

    // test QML/JS alias setter
    created.resetAliasToConstant();
    QCOMPARE(created.property("origin").toInt(), 42);
    QCOMPARE(created.getOrigin(), 42);
    // check the alias just to make sure it also works
    QCOMPARE(created.property("aliasToOrigin").toInt(), 42);
    QCOMPARE(created.getAliasValue(), 42);
    QCOMPARE(created.getAliasToOrigin(), 42);

    // test QML/JS binding reset
    created.resetOriginToNewBinding(); // dummy
    created.setDummy(99);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 99);
    QCOMPARE(created.getAliasValue(), 99);
    QCOMPARE(created.getAliasToOrigin(), 99);

    // test QML/JS binding reset through alias
    created.resetAliasToNewBinding(); // dummy * 3
    created.setDummy(-8);
    QCOMPARE(created.property("origin").toInt(), -24);
    QCOMPARE(created.getOrigin(), -24);
    QCOMPARE(created.property("aliasToOrigin").toInt(), -24);
    QCOMPARE(created.getAliasValue(), -24);
    QCOMPARE(created.getAliasToOrigin(), -24);
}

void tst_qmltc_manual::propertyChangeHandler()
{
    QQmlEngine e;
    ANON_propertyChangeHandler::url = testFileUrl("propertyChangeHandler.qml");
    ANON_propertyChangeHandler created(&e);

    // test that fetching "dirty" property value doesn't trigger property change
    // handler
    QCOMPARE(created.getWatcher(), 0);
    QCOMPARE(created.getP(), 42); // due to binding
    QCOMPARE(created.getWatcher(), 0);
    QCOMPARE(created.property("watcher").toInt(), 0);

    // test that binding triggers property change handler
    created.setDummy(20);
    QCOMPARE(created.getWatcher(), 20);
    QCOMPARE(created.property("watcher").toInt(), 20);

    // test that property setting (through C++) triggers property change handler
    created.setWatcher(-100);
    created.setProperty("p", 18);
    QCOMPARE(created.getWatcher(), 18);

    // test that property setting triggers property change handler
    created.setWatcher(-47);
    created.setP(96);
    QCOMPARE(created.property("watcher").toInt(), 96);
}

void tst_qmltc_manual::locallyImported()
{
    QQmlEngine e;
    LocallyImported::url = testFileUrl("LocallyImported.qml");

    LocallyImported created(&e);
    QCOMPARE(created.getCount(), 1);

    QQmlContext *ctx = e.contextForObject(&created);
    QVERIFY(ctx);
    QCOMPARE(qvariant_cast<QObject *>(ctx->contextProperty("foo")), &created);
}

void tst_qmltc_manual::localImport()
{
    // NB: compare object creation compiler against QQmlComponent
    {
        QQmlEngine e;
        QQmlComponent comp(&e);
        comp.loadUrl(testFileUrl("localImport.qml"));
        QScopedPointer<QObject> root(comp.create());
        QVERIFY2(root, qPrintable(comp.errorString()));

        QQmlContext *ctx = e.contextForObject(root.get());
        QVERIFY(ctx);
        QVERIFY(ctx->parentContext());
        QCOMPARE(ctx->contextProperty("foo"), QVariant());
        QCOMPARE(qvariant_cast<QObject *>(ctx->contextProperty("bar")), root.get());
        QCOMPARE(ctx->objectForName("foo"), nullptr);
        QCOMPARE(ctx->objectForName("bar"), root.get());
        QCOMPARE(QQmlContextData::get(ctx)->parent(), QQmlContextData::get(e.rootContext()));

        int count = root->property("count").toInt();
        QVariant magicValue {};
        QMetaObject::invokeMethod(root.get(), "getMagicValue", Q_RETURN_ARG(QVariant, magicValue));
        QCOMPARE(magicValue.toInt(), (count * 3 + 1));

        count = root->property("count").toInt();
        magicValue = QVariant();
        QMetaObject::invokeMethod(root.get(), "localGetMagicValue",
                                  Q_RETURN_ARG(QVariant, magicValue));
        QCOMPARE(magicValue.toInt(), (count * 3 + 1));
    }

    // In this case, the context hierarchy is the following:
    // * LocallyImported: rootContext -> locallyImportedContext
    // * ANON_localImport: ... -> locallyImportedContext -> localImportContext
    //
    // this resembles the object hierarchy where LocallyImported is a base class
    // of ANON_localImport. having an explicit parent context (from
    // LocallyImported) guarantees that e.g. parent id / base class methods are
    // found during JavaScript (property) lookups
    {
        QQmlEngine e;
        LocallyImported::url = testFileUrl("LocallyImported.qml");
        ANON_localImport::url = testFileUrl("localImport.qml");

        ANON_localImport created(&e);
        QCOMPARE(created.getP1(), 41);
        QCOMPARE(created.getCount(), 42);

        QQmlContext *ctx = e.contextForObject(&created);
        QVERIFY(ctx);
        QVERIFY(ctx->parentContext());
        QEXPECT_FAIL("",
                     "Inconsistent with QQmlComponent: 'foo' could actually be found in generated "
                     "C++ base class context",
                     Continue);
        QCOMPARE(ctx->contextProperty("foo"), QVariant());
        QCOMPARE(qvariant_cast<QObject *>(ctx->contextProperty("bar")), &created);
        // NB: even though ctx->contextProperty("foo") finds the object,
        // objectForName("foo") still returns nullptr as "foo" exists in the
        // ctx->parent()
        QCOMPARE(ctx->objectForName("foo"), nullptr);
        QCOMPARE(ctx->objectForName("bar"), &created);
        QEXPECT_FAIL("",
                     "Inconsistent with QQmlComponent: LocallyImported is a _visible_ parent of "
                     "ANON_localImport, same stays true for context",
                     Continue);
        QCOMPARE(QQmlContextData::get(ctx)->parent(), QQmlContextData::get(e.rootContext()));
        QCOMPARE(QQmlContextData::get(ctx)->parent()->parent(),
                 QQmlContextData::get(e.rootContext()));

        int count = created.getCount();
        QCOMPARE(created.getMagicValue().toInt(), (count * 3 + 1));
        count = created.getCount();
        QCOMPARE(created.localGetMagicValue().toInt(), (count * 3 + 1));
    }
}

void tst_qmltc_manual::neighbors()
{
    {
        QQmlEngine e;
        QQmlComponent comp(&e);
        comp.loadUrl(testFileUrl("neighbors.qml"));
        QScopedPointer<QObject> root(comp.create());
        QVERIFY2(root, qPrintable(comp.errorString()));

        auto rootCtx = QQmlContextData::get(e.contextForObject(root.get()));
        QQmlListReference children(root.get(), "data");
        QCOMPARE(children.size(), 2);
        auto child1Ctx = QQmlContextData::get(e.contextForObject(children.at(0)));
        auto child2Ctx = QQmlContextData::get(e.contextForObject(children.at(1)));

        QCOMPARE(rootCtx->parent(), QQmlContextData::get(e.rootContext()));
        QCOMPARE(child1Ctx, rootCtx);
        QCOMPARE(child2Ctx, rootCtx);
        QCOMPARE(child2Ctx->parent(), QQmlContextData::get(e.rootContext()));

        QQmlContext *rootQmlCtx = rootCtx->asQQmlContext();
        QCOMPARE(rootQmlCtx->objectForName("root"), root.get());
        QCOMPARE(rootQmlCtx->objectForName("child1"), children.at(0));
        QCOMPARE(rootQmlCtx->objectForName("child2"), children.at(1));
    }

    // this case is different from tst_qmltc_manual::localImport() as
    // LocallyImported is not a parent of a document root. Thus, the context
    // hierarchy:
    // * ANON_neighbors: rootContext -> neighborsContext
    // * ANON_neighbors_QtObject: rootContext -> neighborsContext
    // * LocallyImported: ... -> neighborsContext -> locallyImportedContext
    // * ANON_neighbors_LocallyImported: ... -> locallyImportedContext
    //
    // this should resemble the context hierarchy that QQmlObjectCreator
    // assembles, but here the outer context of ANON_neighbors_LocallyImported
    // remains to be the one from LocallyImported base class, which guarantees
    // that we can lookup stuff that originates from LocallyImported.
    {
        QQmlEngine e;
        LocallyImported::url = testFileUrl("LocallyImported.qml");
        ANON_neighbors::url = testFileUrl("neighbors.qml");
        ANON_neighbors_QtObject::url = testFileUrl("neighbors.qml");
        ANON_neighbors_LocallyImported::url2 = testFileUrl("neighbors.qml");

        ANON_neighbors created(&e);
        QQmlListReference children(&created, "data");
        QCOMPARE(children.size(), 2);
        ANON_neighbors_QtObject *child1 = qobject_cast<ANON_neighbors_QtObject *>(children.at(0));
        ANON_neighbors_LocallyImported *child2 =
                qobject_cast<ANON_neighbors_LocallyImported *>(children.at(1));
        QVERIFY(child1 && child2);

        auto rootCtx = QQmlContextData::get(e.contextForObject(&created));
        auto child1Ctx = QQmlContextData::get(e.contextForObject(child1));
        auto child2Ctx = QQmlContextData::get(e.contextForObject(child2));

        QCOMPARE(rootCtx->parent(), QQmlContextData::get(e.rootContext()));
        QCOMPARE(child1Ctx, rootCtx);
        QEXPECT_FAIL("",
                     "Inconsistent with QQmlComponent: non-root object with generated C++ base has "
                     "the context of that base",
                     Continue);
        QCOMPARE(child2Ctx, rootCtx);
        QEXPECT_FAIL("",
                     "Inconsistent with QQmlComponent: non-root object with generated C++ base has "
                     "the context of that base",
                     Continue);
        QCOMPARE(child2Ctx->parent(), QQmlContextData::get(e.rootContext()));
        // the rootCtx is actually a parent in this case
        QCOMPARE(child2Ctx->parent(), rootCtx);

        QQmlContext *rootQmlCtx = rootCtx->asQQmlContext();
        QCOMPARE(rootQmlCtx->objectForName("root"), &created);
        QCOMPARE(rootQmlCtx->objectForName("child1"), child1);
        QCOMPARE(rootQmlCtx->objectForName("child2"), child2);

        QCOMPARE(child1->getP(), 41);
        QCOMPARE(child1->getP2(), child2->getCount() * 2);
        QCOMPARE(child2->getP(), child1->getP() + 1);

        child1->setP(44);
        QCOMPARE(child2->getP(), 45);

        child2->setCount(4);
        QCOMPARE(child1->getP2(), 8);

        int count = child2->getCount();
        QVariant magicValue {};
        QMetaObject::invokeMethod(child2, "getMagicValue", Q_RETURN_ARG(QVariant, magicValue));
        QCOMPARE(magicValue.toInt(), (count * 3 + 1));
    }
}

void tst_qmltc_manual::anchors()
{
    QQmlEngine e;
    ANON_anchors::url = testFileUrl("anchors.qml");
    ANON_anchors created(&e);

    QQuickAnchors *anchors =
            static_cast<QQuickItemPrivate *>(QObjectPrivate::get(&created))->anchors();
    QCOMPARE(anchors->topMargin(), 42);
    created.setValue(7);
    QCOMPARE(anchors->topMargin(), 7);
}

// test workaround: hardcode runtime function indices. because they could be
// rather unexpected and passing wrong ones leads to UB and flakiness.
//
// NB: if you update the QML files that are used by the QQmlEngine runtime
// function execution, make sure that the hardcoded values are in sync with
// those changes! An example of when things could go wrong: adding new, removing
// old or changing the order of e.g. bindings on properties, signal handlers, JS
// functions
namespace FunctionIndices {
static constexpr int HELLO_WORLD_GREETING_BINDING = 0;

static constexpr int SIGNAL_HANDLERS_ON_SIGNAL1 = 1;
static constexpr int SIGNAL_HANDLERS_ON_SIGNAL2 = 3;
static constexpr int SIGNAL_HANDLERS_QML_EMIT_SIGNAL1 = 4;
static constexpr int SIGNAL_HANDLERS_QML_EMIT_SIGNAL2 = 5;
static constexpr int SIGNAL_HANDLERS_QML_EMIT_SIGNAL2_WITH_ARGS = 6;

static constexpr int JS_FUNCTIONS_FUNC1 = 0;
static constexpr int JS_FUNCTIONS_FUNC2 = 1;
static constexpr int JS_FUNCTIONS_FUNC3 = 2;

static constexpr int CHANGING_BINDINGS_P2_BINDING = 0;
static constexpr int CHANGING_BINDINGS_RESET_TO_CONSTANT = 1;
static constexpr int CHANGING_BINDINGS_RESET_TO_NEW_BINDING = 2;

static constexpr int PROPERTY_ALIAS_ORIGIN_BINDING = 0;
static constexpr int PROPERTY_ALIAS_RESET_ALIAS_TO_CONSTANT = 1;
static constexpr int PROPERTY_ALIAS_RESET_ORIGIN_TO_CONSTANT = 2;
static constexpr int PROPERTY_ALIAS_RESET_ALIAS_TO_NEW_BINDING = 3;
static constexpr int PROPERTY_ALIAS_RESET_ORIGIN_TO_NEW_BINDING = 5;
static constexpr int PROPERTY_ALIAS_GET_ALIAS_VALUE = 7;

static constexpr int PROPERTY_CHANGE_HANDLER_P_BINDING = 0;
static constexpr int PROPERTY_CHANGE_HANDLER_ON_P_CHANGED = 1;

static constexpr int LOCALLY_IMPORTED_GET_MAGIC_VALUE = 0;
static constexpr int LOCALLY_IMPORTED_ON_COMPLETED = 1;

static constexpr int LOCAL_IMPORT_COUNT_BINDING = 0;
static constexpr int LOCAL_IMPORT_LOCAL_GET_MAGIC_VALUE = 1;

static constexpr int NEIGHBOUR_IDS_CHILD1_P2_BINDING = 0;
static constexpr int NEIGHBOUR_IDS_CHILD2_P_BINDING = 1;

static constexpr int ANCHORS_ANCHORS_TOP_MARGIN_BINDING = 0;
};

// test utility function for type erasure. the "real" code would be
// auto-generated by the compiler
template<typename... IOArgs, size_t Size = sizeof...(IOArgs) + 1>
static void typeEraseArguments(std::array<void *, Size> &a, std::array<QMetaType, Size> &t,
                               std::nullptr_t, IOArgs &&... args)
{
    a = { /* return value */ nullptr, /* rest */
          const_cast<void *>(
                  reinterpret_cast<const void *>(std::addressof(std::forward<IOArgs>(args))))... };
    t = { /* return type */ QMetaType::fromType<void>(),
          /* types */ QMetaType::fromType<std::decay_t<IOArgs>>()... };
}

template<typename... IOArgs, size_t Size = sizeof...(IOArgs)>
static void typeEraseArguments(std::array<void *, Size> &a, std::array<QMetaType, Size> &t,
                               IOArgs &&... args)
{
    a = { /* all values, including return value */ const_cast<void *>(
            reinterpret_cast<const void *>(std::addressof(std::forward<IOArgs>(args))))... };
    t = { /* types */ QMetaType::fromType<std::decay_t<IOArgs>>()... };
}

// test utility that fetches a QV4::Function from the engine
inline QV4::Function *getRuntimeFunction(QQmlEngine *engine, const QUrl &url, qsizetype index)
{
    QQmlEnginePrivate *priv = QQmlEnginePrivate::get(engine);
    Q_ASSERT(priv);
    const auto unit = priv->compilationUnitFromUrl(url);
    return unit->runtimeFunctions.value(index, nullptr);
}

// test utility that sets up the binding call arguments
template<typename CreateBinding>
inline decltype(auto) createBindingInScope(QQmlEngine *qmlengine, QObject *thisObject,
                                           CreateBinding create)
{
    QV4::ExecutionEngine *v4 = qmlengine->handle();
    Q_ASSERT(v4);

    QQmlContext *ctx = qmlengine->contextForObject(thisObject);
    if (!ctx)
        ctx = qmlengine->rootContext();
    QV4::Scope scope(v4);
    QV4::ExecutionContext *executionCtx = v4->scriptContext();
    QQmlRefPointer<QQmlContextData> ctxtdata = QQmlContextData::get(ctx);
    QV4::Scoped<QV4::QmlContext> qmlContext(
            scope, QV4::QmlContext::create(executionCtx, ctxtdata, thisObject));

    return create(ctxtdata, qmlContext);
}

ContextRegistrator::ContextRegistrator(QQmlEngine *engine, QObject *This)
{
    Q_ASSERT(engine && This);
    if (engine->contextForObject(This)) // already set
        return;

    // use simple form of the logic done in create() and set(). this code
    // shouldn't actually be used in real generated classes. it just exists
    // here for convenience
    Q_ASSERT(!This->parent() || engine->contextForObject(This->parent()) || engine->rootContext());
    QQmlContext *parentContext = engine->contextForObject(This->parent());
    QQmlContext *context =
            parentContext ? parentContext : new QQmlContext(engine->rootContext(), This);
    Q_ASSERT(context);
    // NB: not only sets the context, but also seeds engine into This, so
    // that qmlEngine(This) works
    engine->setContextForObject(This, context);
    Q_ASSERT(qmlEngine(This));
}

HelloWorld::HelloWorld(QQmlEngine *e, QObject *parent)
    : QObject(parent), ContextRegistrator(e, this)
{
    hello = QStringLiteral("Hello, World");
    QPropertyBinding<QString> HelloWorldCpp_greeting_binding(
            [&]() {
                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::HELLO_WORLD_GREETING_BINDING;
                constexpr int argc = 0;
                QString ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableGreeting().setBinding(HelloWorldCpp_greeting_binding);
}

QUrl HelloWorld::url = QUrl(); // workaround

ANON_signalHandlers::ANON_signalHandlers(QQmlEngine *e, QObject *parent)
    : QObject(parent), ContextRegistrator(e, this)
{
    signal1P = 0;
    signal2P1 = QStringLiteral("");
    signal2P2 = 0;
    signal2P3 = QStringLiteral("");

    QObject::connect(this, &ANON_signalHandlers::signal1, this, &ANON_signalHandlers::onSignal1);
    QObject::connect(this, &ANON_signalHandlers::signal2, this, &ANON_signalHandlers::onSignal2);
}

void ANON_signalHandlers::onSignal1()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::SIGNAL_HANDLERS_ON_SIGNAL1;
    e->executeRuntimeFunction(url, index, this);
}

void ANON_signalHandlers::onSignal2(QString x, int y)
{
    constexpr int argc = 2;
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, nullptr, x, y);

    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const qsizetype index = FunctionIndices::SIGNAL_HANDLERS_ON_SIGNAL2;
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
}

void ANON_signalHandlers::qmlEmitSignal1()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::SIGNAL_HANDLERS_QML_EMIT_SIGNAL1;
    e->executeRuntimeFunction(url, index, this);
}

void ANON_signalHandlers::qmlEmitSignal2()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::SIGNAL_HANDLERS_QML_EMIT_SIGNAL2;
    e->executeRuntimeFunction(url, index, this);
}

void ANON_signalHandlers::qmlEmitSignal2WithArgs(QString x, int y)
{
    constexpr int argc = 2;
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, nullptr, x, y);

    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::SIGNAL_HANDLERS_QML_EMIT_SIGNAL2_WITH_ARGS;
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
}

QUrl ANON_signalHandlers::url = QUrl(); // workaround

ANON_javaScriptFunctions::ANON_javaScriptFunctions(QQmlEngine *e, QObject *parent)
    : QObject(parent), ContextRegistrator(e, this)
{
    func1P = 0;
    func2P = QStringLiteral("");
    func3P = false;
}

void ANON_javaScriptFunctions::func1()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::JS_FUNCTIONS_FUNC1;
    e->executeRuntimeFunction(url, index, this);
}

void ANON_javaScriptFunctions::func2(QString x)
{
    constexpr int argc = 1;
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, nullptr, x);

    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::JS_FUNCTIONS_FUNC2;
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
}

bool ANON_javaScriptFunctions::func3()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::JS_FUNCTIONS_FUNC3;
    constexpr int argc = 0;
    bool ret {};
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, ret);
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    return ret;
}

QUrl ANON_javaScriptFunctions::url = QUrl(); // workaround

void ANON_changingBindings::resetToInitialBinding()
{
    QPropertyBinding<int> ANON_changingBindings_p2_binding(
            [&]() {
                initialBindingCallCount++;

                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::CHANGING_BINDINGS_P2_BINDING;
                constexpr int argc = 0;
                int ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableP2().setBinding(ANON_changingBindings_p2_binding);
}

ANON_changingBindings::ANON_changingBindings(QQmlEngine *e, QObject *parent)
    : QObject(parent), ContextRegistrator(e, this)
{
    p1 = 1;
    resetToInitialBinding();
}

void ANON_changingBindings::resetToConstant()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::CHANGING_BINDINGS_RESET_TO_CONSTANT;
    e->executeRuntimeFunction(url, index, this);
}

void ANON_changingBindings::resetToNewBinding()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::CHANGING_BINDINGS_RESET_TO_NEW_BINDING;
    e->executeRuntimeFunction(url, index, this);
}

QUrl ANON_changingBindings::url = QUrl(); // workaround

void ANON_propertyAlias::resetToInitialBinding()
{
    QPropertyBinding<int> ANON_propertyAlias_origin_binding(
            [&]() {
                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::PROPERTY_ALIAS_ORIGIN_BINDING;
                constexpr int argc = 0;
                int ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableOrigin().setBinding(ANON_propertyAlias_origin_binding);
}

ANON_propertyAlias::ANON_propertyAlias(QQmlEngine *e, QObject *parent)
    : QObject(parent), ContextRegistrator(e, this)
{
    dummy = 12;
    resetToInitialBinding();
}

void ANON_propertyAlias::resetAliasToConstant()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ALIAS_TO_CONSTANT;
    e->executeRuntimeFunction(url, index, this);
}
void ANON_propertyAlias::resetOriginToConstant()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ORIGIN_TO_CONSTANT;
    e->executeRuntimeFunction(url, index, this);
}
void ANON_propertyAlias::resetAliasToNewBinding()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ALIAS_TO_NEW_BINDING;
    e->executeRuntimeFunction(url, index, this);
}
void ANON_propertyAlias::resetOriginToNewBinding()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ORIGIN_TO_NEW_BINDING;
    e->executeRuntimeFunction(url, index, this);
}

int ANON_propertyAlias::getAliasValue()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::PROPERTY_ALIAS_GET_ALIAS_VALUE;
    constexpr int argc = 0;
    int ret {};
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, ret);
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    return ret;
}

QUrl ANON_propertyAlias::url = QUrl(); // workaround

ANON_propertyChangeHandler::ANON_propertyChangeHandler(QQmlEngine *e, QObject *parent)
    : QObject(parent), ContextRegistrator(e, this)
{
    dummy = 42;
    QPropertyBinding<int> ANON_propertyChangeHandler_p_binding(
            [&]() {
                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::PROPERTY_CHANGE_HANDLER_P_BINDING;
                constexpr int argc = 0;
                int ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableP().setBinding(ANON_propertyChangeHandler_p_binding);
    watcher = 0;

    // NB: make sure property change handler appears after setBinding().
    // this prevents preliminary binding evaluation (which would fail as
    // this object doesn't yet know about qmlEngine(this))
    pChangeHandler.reset(new QPropertyChangeHandler<ANON_propertyChangeHandler_p_changeHandler>(
            bindableP().onValueChanged(ANON_propertyChangeHandler_p_changeHandler(this))));
}

ANON_propertyChangeHandler::ANON_propertyChangeHandler_p_changeHandler::
        ANON_propertyChangeHandler_p_changeHandler(ANON_propertyChangeHandler *obj)
    : This(obj)
{
}

void ANON_propertyChangeHandler::ANON_propertyChangeHandler_p_changeHandler::operator()()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(This));
    const auto index = FunctionIndices::PROPERTY_CHANGE_HANDLER_ON_P_CHANGED;
    e->executeRuntimeFunction(This->url, index, This);
}

QUrl ANON_propertyChangeHandler::url = QUrl(); // workaround

LocallyImported::LocallyImported(QObject *parent) : QObject(parent) { }

LocallyImported::LocallyImported(QQmlEngine *e, QObject *parent) : LocallyImported(parent)
{
    init(e, QQmlContextData::get(e->rootContext()));
}

QQmlRefPointer<QQmlContextData>
LocallyImported::init(QQmlEngine *e, const QQmlRefPointer<QQmlContextData> &parentContext)
{
    // NB: this object is the root object of LocallyImported.qml
    constexpr int subComponentIndex = 0;
    auto context = parentContext;
    context = ContextRegistrator::create(e, url, context, subComponentIndex);
    ContextRegistrator::set(this, context, QQmlContextData::DocumentRoot);
    context->setIdValue(0, this);
    count = 0;
    finalize(e); // call here because it's document root
    return context;
}

Q_INVOKABLE QVariant LocallyImported::getMagicValue()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::LOCALLY_IMPORTED_GET_MAGIC_VALUE;
    constexpr int argc = 0;
    QVariant ret {};
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, ret);
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    return ret;
}

void LocallyImported::completedSlot()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::LOCALLY_IMPORTED_ON_COMPLETED;
    constexpr int argc = 0;
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, nullptr);
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
}

void LocallyImported::finalize(QQmlEngine *e)
{
    Q_UNUSED(e);
    // 1. finalize children - no children here, so do nothing

    // 2. finalize self
    completedSlot();
}

QUrl LocallyImported::url = QUrl(); // workaround

ANON_localImport::ANON_localImport(QObject *parent) : LocallyImported(parent) { }

ANON_localImport::ANON_localImport(QQmlEngine *e, QObject *parent) : ANON_localImport(parent)
{
    // NB: always use e->rootContext() as parent context in the public ctor
    init(e, QQmlContextData::get(e->rootContext()));
}

QQmlRefPointer<QQmlContextData>
ANON_localImport::init(QQmlEngine *e, const QQmlRefPointer<QQmlContextData> &parentContext)
{
    // init function is a multi-step procedure:
    //
    // 0. [optional] call base class' init() method (when base class is also
    //    generated from QML), passing parentContext and getting back
    //    another context
    // 1. create child QQmlContextData from the context
    // 2. * EITHER: patch the context to be the parent context - when this
    //      type is not root and it has generated C++ base class
    //    * OR: set the context for this object
    // 3. [optional] set id by QmlIR::Object::id
    // 4. do the simple initialization bits (e.g. set property values, etc.)
    //    - might actually slip to finalize()?
    // 5. [document root only] call finalize() which finalizes all types in
    //    this document - after context and instances are set up correctly

    constexpr int componentIndex = 0; // root index
    auto context = parentContext;
    // 0.
    context = LocallyImported::init(e, context);
    // 1.
    context = ContextRegistrator::create(e, url, context, componentIndex);
    // 2.
    // if not root and parent is also generated C++ object, patch context
    // context = parentContext;
    // else:
    ContextRegistrator::set(this, context, QQmlContextData::DocumentRoot);
    // 3.
    context->setIdValue(0, this);
    // 4.
    p1 = 41;
    // 5.
    finalize(e); // call here because it's document root
    return context;
}

void ANON_localImport::finalize(QQmlEngine *e)
{
    Q_UNUSED(e);
    // 1. finalize children - no children here, so do nothing

    // 2. finalize self
    QPropertyBinding<int> ANON_localImport_count_binding(
            [&]() {
                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::LOCAL_IMPORT_COUNT_BINDING;
                constexpr int argc = 0;
                int ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableCount().setBinding(ANON_localImport_count_binding);
}

Q_INVOKABLE QVariant ANON_localImport::localGetMagicValue()
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
    const auto index = FunctionIndices::LOCAL_IMPORT_LOCAL_GET_MAGIC_VALUE;
    constexpr int argc = 0;
    QVariant ret {};
    std::array<void *, argc + 1> a {};
    std::array<QMetaType, argc + 1> t {};
    typeEraseArguments(a, t, ret);
    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    return ret;
}

QUrl ANON_localImport::url = QUrl(); // workaround

ANON_neighbors_QtObject::ANON_neighbors_QtObject(QObject *parent) : QObject(parent) { }

ANON_neighbors_QtObject::ANON_neighbors_QtObject(QQmlEngine *e, QObject *parent)
    : ANON_neighbors_QtObject(parent)
{
    // NB: non-root of the document
    init(e, QQmlContextData::get(e->contextForObject(parent)));
}

QQmlRefPointer<QQmlContextData>
ANON_neighbors_QtObject::init(QQmlEngine *e, const QQmlRefPointer<QQmlContextData> &parentContext)
{
    constexpr int componentIndex = 1;
    auto context = ContextRegistrator::create(e, url, parentContext, componentIndex);
    ContextRegistrator::set(this, context, QQmlContextData::OrdinaryObject);
    context->setIdValue(componentIndex, this);

    p = 41;
    return context;
}

void ANON_neighbors_QtObject::finalize(QQmlEngine *e) // called by the document root
{
    Q_UNUSED(e);
    // 1. finalize children - empty as we don't have children here

    // 2. finalize self - call all "dynamic" code - e.g. script bindings,
    //    Component.onCompleted and so on - everything that may reference
    //    some random part of the document and thus needs to be delayed
    //    until all objects in the document are initialized
    QPropertyBinding<int> ANON_neighbors_QtObject_p2_binding(
            [&]() {
                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::NEIGHBOUR_IDS_CHILD1_P2_BINDING;
                constexpr int argc = 0;
                int ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableP2().setBinding(ANON_neighbors_QtObject_p2_binding);
}

QUrl ANON_neighbors_QtObject::url = QUrl(); // workaround

ANON_neighbors_LocallyImported::ANON_neighbors_LocallyImported(QObject *parent)
    : LocallyImported(parent)
{
}

ANON_neighbors_LocallyImported::ANON_neighbors_LocallyImported(QQmlEngine *e, QObject *parent)
    : ANON_neighbors_LocallyImported(parent)
{
    // NB: non-root of the document
    init(e, QQmlContextData::get(e->contextForObject(parent)));
}

QQmlRefPointer<QQmlContextData>
ANON_neighbors_LocallyImported::init(QQmlEngine *e,
                                     const QQmlRefPointer<QQmlContextData> &parentContext)
{
    constexpr int componentIndex = 2;
    auto context = LocallyImported::init(e, parentContext);
    context = ContextRegistrator::create(e, url2, context, componentIndex);
    // if not root and parent is also generated C++ object, patch context
    context = parentContext;
    // else: ContextRegistrator::set(this, context, QQmlContextData::OrdinaryObject);
    context->setIdValue(componentIndex, this);
    return context;
}

void ANON_neighbors_LocallyImported::finalize(QQmlEngine *e) // called by the document root
{
    Q_UNUSED(e);
    // 1. finalize children - empty as we don't have children here

    // 2. finalize self - call all "dynamic" code - e.g. script bindings,
    //    Component.onCompleted and so on - everything that may reference
    //    some random part of the document and thus needs to be delayed
    //    until all objects in the document are initialized
    QPropertyBinding<int> ANON_neighbors_LocallyImported_p_binding(
            [&]() {
                QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                const auto index = FunctionIndices::NEIGHBOUR_IDS_CHILD2_P_BINDING;
                constexpr int argc = 0;
                int ret {};
                std::array<void *, argc + 1> a {};
                std::array<QMetaType, argc + 1> t {};
                typeEraseArguments(a, t, ret);
                e->executeRuntimeFunction(url2, index, this, argc, a.data(), t.data());
                return ret;
            },
            QT_PROPERTY_DEFAULT_BINDING_LOCATION);
    bindableP().setBinding(ANON_neighbors_LocallyImported_p_binding);
}
QUrl ANON_neighbors_LocallyImported::url2 = QUrl(); // workaround

// NB: only root subclasses the helper - as it initiates the finalization
ANON_neighbors::ANON_neighbors(QObject *parent) : QQuickItem()
{
    setParent(parent);
}

ANON_neighbors::ANON_neighbors(QQmlEngine *e, QObject *parent) : ANON_neighbors(parent)
{
    // NB: use e->rootContext() as this object is document root
    init(e, QQmlContextData::get(e->rootContext()));
}

QQmlRefPointer<QQmlContextData>
ANON_neighbors::init(QQmlEngine *e, const QQmlRefPointer<QQmlContextData> &parentContext)
{
    constexpr int componentIndex = 0; // root index
    auto context = ContextRegistrator::create(e, url, parentContext, componentIndex);
    ContextRegistrator::set(this, context, QQmlContextData::DocumentRoot);
    context->setIdValue(componentIndex, this);

    finalize(e); // call here because it's document root
    return context;
}

void ANON_neighbors::finalize(QQmlEngine *e)
{
    Q_UNUSED(e);
    // 0. set up object bindings and record all new objects for further
    //    finalization
    QList<QObject *> objectsToFinalize;
    objectsToFinalize.reserve(2); // we know it's 2 at compile time
    QQmlListReference listrefData(this, "data");
    {
        auto o = new ANON_neighbors_QtObject(e, this);
        listrefData.append(o);
        objectsToFinalize.append(o);
    }
    {
        auto o = new ANON_neighbors_LocallyImported(e, this);
        listrefData.append(o);
        objectsToFinalize.append(o);
    }

    // 1. finalize children
    // use static_cast instead of polymorphism as we know the types at
    // compile-time
    static_cast<ANON_neighbors_QtObject *>(objectsToFinalize.at(0))->finalize(e);
    static_cast<ANON_neighbors_LocallyImported *>(objectsToFinalize.at(1))->finalize(e);

    // 2. finalize self - empty as we don't have any bindings here
}

QUrl ANON_neighbors::url = QUrl(); // workaround

ANON_anchors::ANON_anchors(QObject *parent) : QQuickItem()
{
    setParent(parent);
}

ANON_anchors::ANON_anchors(QQmlEngine *e, QObject *parent) : ANON_anchors(parent)
{
    // NB: use e->rootContext() as this object is document root
    init(e, QQmlContextData::get(e->rootContext()));
}

QQmlRefPointer<QQmlContextData>
ANON_anchors::init(QQmlEngine *e, const QQmlRefPointer<QQmlContextData> &parentContext)
{
    constexpr int componentIndex = 0; // root index
    auto context = ContextRegistrator::create(e, url, parentContext, componentIndex);
    ContextRegistrator::set(this, context, QQmlContextData::DocumentRoot);
    finalize(e); // call here because it's document root
    return context;
}

void ANON_anchors::finalize(QQmlEngine *e)
{
    Q_UNUSED(e);

    this->value = 42;

    // create binding (in a proper way) on Item's anchors through
    // QQmlAnyBinding. if this is achievable through C++, then compiler could do
    // it as well (by generating roughly the same code).
    const QMetaObject *mo = this->metaObject();
    QVERIFY(mo);
    // fetching QMetaProperty is possible through the compiler
    QMetaProperty anchorsProperty = mo->property(mo->indexOfProperty("anchors"));
    QQuickAnchors *anchors = qvariant_cast<QQuickAnchors *>(anchorsProperty.read(this));
    QVERIFY(anchors);

    // below is binding-specific code that is part of a special qmltc library
    auto v4Func = getRuntimeFunction(qmlEngine(this), url,
                                     FunctionIndices::ANCHORS_ANCHORS_TOP_MARGIN_BINDING);
    QVERIFY(v4Func);
    const QMetaObject *anchorsMo = anchors->metaObject();
    QVERIFY(anchorsMo);
    QMetaProperty topMarginProperty = anchorsMo->property(anchorsMo->indexOfProperty("topMargin"));
    QVERIFY(QByteArray(topMarginProperty.name()) == "topMargin");

    createBindingInScope(
            qmlEngine(this), this,
            [&](const QQmlRefPointer<QQmlContextData> &ctxt, QV4::ExecutionContext *scope) {
                QQmlBinding *binding = QQmlBinding::create(topMarginProperty.metaType(), v4Func,
                                                           this, ctxt, scope);
                binding->setTarget(anchors, topMarginProperty.propertyIndex(), false, -1);
                QQmlPropertyPrivate::setBinding(binding);
            });
}

QUrl ANON_anchors::url = QUrl(); // workaround

QTEST_MAIN(tst_qmltc_manual)

#include "tst_qmltc_manual.moc"
