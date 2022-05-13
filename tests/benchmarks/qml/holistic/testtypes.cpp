// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "testtypes.h"
#include <QQmlEngine>

static QJSValue script_api(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);

    static int testProperty = 13;
    QJSValue v = scriptEngine->newObject();
    v.setProperty("scriptTestProperty", testProperty++);
    return v;
}

static QObject *qobject_api(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);

    testQObjectApi *o = new testQObjectApi;
    o->setQObjectTestProperty(20);
    return o;
}

static QObject *qobject_api_engine_parent(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine);

    static int testProperty = 26;
    testQObjectApi *o = new testQObjectApi(engine);
    o->setQObjectTestProperty(testProperty++);
    return o;
}

void registerTypes()
{
    qmlRegisterType<MyQmlObject>("Qt.test", 1,0, "MyQmlObjectAlias");
    qmlRegisterType<MyQmlObject>("Qt.test", 1,0, "MyQmlObject");

    qRegisterMetaType<MyQmlObject::MyType>("MyQmlObject::MyType");

    qmlRegisterType<ScarceResourceProvider>("Qt.test", 1,0, "MyScarceResourceProvider");
    qmlRegisterType<ArbitraryVariantProvider>("Qt.test", 1,0, "MyArbitraryVariantProvider");

    qmlRegisterSingletonType("Qt.test",1,0,"Script",script_api);             // register (script) singleton Type for an existing uri which contains elements
    qmlRegisterSingletonType<testQObjectApi>("Qt.test",1,0,"QObject",qobject_api);            // register (qobject) for an existing uri for which another singleton Type was previously regd.  Should replace!
    qmlRegisterSingletonType("Qt.test.scriptApi",1,0,"Script",script_api);   // register (script) singleton Type for a uri which doesn't contain elements
    qmlRegisterSingletonType<testQObjectApi>("Qt.test.qobjectApi",1,0,"QObject",qobject_api); // register (qobject) singleton Type for a uri which doesn't contain elements
    qmlRegisterSingletonType<testQObjectApi>("Qt.test.qobjectApi",1,3,"QObject",qobject_api); // register (qobject) singleton Type for a uri which doesn't contain elements, minor version set
    qmlRegisterSingletonType<testQObjectApi>("Qt.test.qobjectApi",2,0,"QObject",qobject_api); // register (qobject) singleton Type for a uri which doesn't contain elements, major version set
    qmlRegisterSingletonType<testQObjectApi>("Qt.test.qobjectApiParented",1,0,"QObject",qobject_api_engine_parent); // register (parented qobject) singleton Type for a uri which doesn't contain elements
}

//#include "testtypes.moc"
