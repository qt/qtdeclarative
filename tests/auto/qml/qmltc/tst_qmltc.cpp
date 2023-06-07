// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmltc.h"

// Generated headers:
#include "ResolvedNameConflict.h"
#include "helloworld.h"
#include "simpleqtquicktypes.h"
#include "typewithenums.h"
#include "methods.h"
#include "properties.h"
#include "objectwithid.h"
#include "documentwithids.h"
#include "importnamespace.h"
#include "deferredproperties.h"
#include "deferredproperties_group.h"
#include "deferredproperties_attached.h"
#include "deferredproperties_complex.h"
#include "gradients.h"
#include "qjsvalueassignments.h"
#include "extensiontypebindings.h"
#include "qtbug103956_main.h"
#include "nonstandardinclude.h"
#include "specialproperties.h"
#include "regexpbindings.h"
#include "aliasassignments.h"
#include "connections.h"

#include "signalhandlers.h"
#include "javascriptfunctions.h"
#include "changingbindings.h"
#include "propertyalias.h"
#include "propertyalias_external.h"
#include "propertyaliasattributes.h"
#include "complexaliases.h"
#include "propertychangehandler.h"
#include "nestedhelloworld.h"
#include "componenthelloworld.h"
#include "propertyreturningfunction.h"
#include "listproperty.h"
#include "listpropertysamename.h"
#include "defaultproperty.h"
#include "defaultpropertycorrectselection.h"
#include "attachedproperty.h"
#include "attachedpropertyderived.h"
#include "groupedproperty.h"
#include "groupedproperty_qquicktext.h"
#include "localimport.h"
#include "localimport_explicit.h"
#include "newpropertyboundtoold.h"
#include "oldpropertyboundtonew.h"
#include "nonlocalqmlpropertyboundtoany.h"
#include "localderived.h"
#include "justanimation.h"
#include "justanimationonalias.h"
#include "behaviorandanimation.h"
#include "behaviorandanimationonalias.h"
#include "bindingsthroughids.h"
#include "localimport_context.h"
#include "neighbors_context.h"
#include "delegate_context.h"
#include "nontrivial_context.h"
#include "javascriptcaller.h"
#include "listview.h"
#include "bindingonvaluetype.h"
#include "keyevents.h"
#include "privatepropertysubclass.h"
#include "calqlatrbits.h"
#include "propertychangeandsignalhandlers.h"
#include "valuetypelistproperty.h"
#include "translations.h"
#include "translationsbyid.h"
#include "defaultalias.h"
#include "generalizedgroupedproperty.h"
#include "appendtoqqmllistproperty.h"
#include "inlinecomponents.h"
#include "repeatercrash.h"
#include "aliases.h"
#include "inlinecomponentsfromdifferentfiles.h"
#include "helloexportedworld.h"

#include "testprivateproperty.h"
#include "singletons.h"
#include "mysignals.h"
#include "namespacedtypes.h"
#include "type.h"

// Qt:
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qurl.h>
#include <QtCore/qmetatype.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmllist.h>
#include <private/qqmltimer_p.h>

#include <QtTest/qsignalspy.h>

#include <QtCore/private/qobject_p.h>
#include <QtTest/private/qemulationdetector_p.h>
#include <QtCore/qtranslator.h>

#ifndef QMLTC_TESTS_DISABLE_CACHE
#    error "QMLTC_TESTS_DISABLE_CACHE is supposed to be defined and be equal to either 0 or 1"
#endif

#define PREPEND_NAMESPACE(name) QmltcTests::name // silent contract that the namespace is QmltcTest

using namespace Qt::StringLiterals;

tst_qmltc::tst_qmltc()
{
#if defined(QMLTC_TESTS_DISABLE_CACHE) && QMLTC_TESTS_DISABLE_CACHE
    qputenv("QML_DISABLE_DISK_CACHE", "1");
#else
    qputenv("QML_DISABLE_DISK_CACHE", "0");
#endif
}

void tst_qmltc::initTestCase()
{
    const auto status = isCacheDisabled() ? u"DISABLED" : u"ENABLED";
    qInfo() << u"Disk cache is" << status;

    // Note: just check whether the QML code is valid. QQmlComponent is good for
    // it. also, we can use qrc to make sure the file is in the resource system.
    QUrl urls[] = {
        QUrl("qrc:/qt/qml/QmltcTests/NameConflict.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/HelloWorld.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/simpleQtQuickTypes.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/typeWithEnums.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/methods.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/properties.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/ObjectWithId.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/documentWithIds.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/importNamespace.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/gradients.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/qjsvalueAssignments.qml"),

        QUrl("qrc:/qt/qml/QmltcTests/deferredProperties.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/deferredProperties_group.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/deferredProperties_attached.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/deferredProperties_complex.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/extensionTypeBindings.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/nonStandardInclude.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/specialProperties.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/regexpBindings.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/AliasBase.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/aliasAssignments.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/Connections.qml"),

        QUrl("qrc:/qt/qml/QmltcTests/qtbug103956/SubComponent.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/qtbug103956/MainComponent.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/qtbug103956/qtbug103956_main.qml"),

        QUrl("qrc:/qt/qml/QmltcTests/signalHandlers.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/javaScriptFunctions.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/changingBindings.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/propertyAlias.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/propertyAlias_external.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/complexAliases.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/propertyChangeHandler.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/NestedHelloWorld.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/ComponentHelloWorld.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/propertyReturningFunction.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/listProperty.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/listPropertySameName.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/defaultProperty.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/defaultPropertyCorrectSelection.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/AttachedProperty.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/attachedPropertyDerived.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/groupedProperty.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/groupedProperty_qquicktext.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/localImport.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/newPropertyBoundToOld.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/oldPropertyBoundToNew.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/nonLocalQmlPropertyBoundToAny.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/justAnimation.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/justAnimationOnAlias.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/behaviorAndAnimation.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/behaviorAndAnimationOnAlias.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/bindingsThroughIds.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/localImport_context.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/neighbors_context.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/delegate_context.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/nontrivial_context.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/javascriptCaller.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/listView.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/bindingOnValueType.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/keyEvents.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/PrivateProperty.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/privatePropertySubclass.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/calqlatrBits.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/valueTypeListProperty.qml"),
        QUrl("qrc:/qt/qml/QmltcTests/appendToQQmlListProperty.qml"),
    };

    QQmlEngine e;
    QQmlComponent component(&e);
    for (const auto &url : urls) {
        component.loadUrl(url);
        QVERIFY2(!component.isError(), qPrintable(u"Bad QML file. "_s + component.errorString()));
    }
}

void tst_qmltc::qmlNameConflictResolution()
{
    // we can include user-renamed files
    QQmlEngine e;
    // Note: the C++ class name is derived from the source qml file path, not
    // the output .h/.cpp, so: NameConflict class name for NameConflict.qml
    PREPEND_NAMESPACE(NameConflict) created(&e); // note: declared in ResolvedNameConflict.h
}

void tst_qmltc::helloWorld()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(HelloWorld) created(&e);

    QCOMPARE(created.hello(), u"Hello, World");
    QCOMPARE(created.property("hello").toString(), QStringLiteral("Hello, World"));
    QCOMPARE(created.greeting(), QStringLiteral("Hello, World!"));
    QCOMPARE(created.property("greeting").toString(), QStringLiteral("Hello, World!"));

    created.setProperty("hello", QStringLiteral("Hello, Qml"));

    QCOMPARE(created.property("hello").toString(), QStringLiteral("Hello, Qml"));
    QCOMPARE(created.property("greeting").toString(), QStringLiteral("Hello, Qml!"));
    QCOMPARE(created.greeting(), QStringLiteral("Hello, Qml!"));
}

void tst_qmltc::qtQuickIncludes()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(simpleQtQuickTypes) created(&e); // it should just compile as well
    // since the file name is lower-case, let's also test that it's marked as
    // QML_ANONYMOUS
    const QMetaObject *mo = created.metaObject();
    QVERIFY(mo);
    QCOMPARE(mo->classInfo(mo->indexOfClassInfo("QML.Element")).value(), "anonymous");
}

void tst_qmltc::enumerations()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(typeWithEnums) created(&e);

    // sanity
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::A, 0);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::B, 1);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::C, 2);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::D, 3);

    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::A_, 1);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::B_, 2);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::B2_, 3);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::C_, 41);
    QCOMPARE(PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::D_, 42);

    const QMetaObject *mo = created.metaObject();
    QVERIFY(mo);
    const QMetaEnum enumerator1 = mo->enumerator(mo->indexOfEnumerator("NoValuesSpecified"));
    QCOMPARE(enumerator1.enumName(), "NoValuesSpecified");
    QCOMPARE(enumerator1.keyCount(), 4);
    QCOMPARE(enumerator1.key(2), "C");
    QCOMPARE(enumerator1.value(2), PREPEND_NAMESPACE(typeWithEnums)::NoValuesSpecified::C);

    const QMetaEnum enumerator2 = mo->enumerator(mo->indexOfEnumerator("ValuesSpecified"));
    QCOMPARE(enumerator2.enumName(), "ValuesSpecified");
    QCOMPARE(enumerator2.keyCount(), 5);
    QCOMPARE(enumerator2.key(2), "B2_");
    QCOMPARE(enumerator2.value(2), PREPEND_NAMESPACE(typeWithEnums)::ValuesSpecified::B2_);
}

void tst_qmltc::methods()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(methods) created(&e);

    const QMetaObject *mo = created.metaObject();
    QVERIFY(mo);

    QMetaMethod metaJustSignal = mo->method(mo->indexOfSignal("justSignal()"));
    QMetaMethod metaTypedSignal = mo->method(mo->indexOfSignal(
            QMetaObject::normalizedSignature("typedSignal(QString,QObject *,double)")));
    QMetaMethod metaJustMethod = mo->method(mo->indexOfMethod("justMethod()"));
    QMetaMethod metaUntypedMethod = mo->method(mo->indexOfMethod(
            QMetaObject::normalizedSignature("untypedMethod(QVariant,QVariant)")));
    QMetaMethod metaTypedMethod = mo->method(
            mo->indexOfMethod(QMetaObject::normalizedSignature("typedMethod(double,int)")));

    QVERIFY(metaJustSignal.isValid());
    QVERIFY(metaTypedSignal.isValid());
    QVERIFY(metaJustMethod.isValid());
    QVERIFY(metaUntypedMethod.isValid());
    QVERIFY(metaTypedMethod.isValid());

    QCOMPARE(metaJustSignal.methodType(), QMetaMethod::Signal);
    QCOMPARE(metaTypedSignal.methodType(), QMetaMethod::Signal);
    QCOMPARE(metaJustMethod.methodType(), QMetaMethod::Method);
    QCOMPARE(metaUntypedMethod.methodType(), QMetaMethod::Method);
    QCOMPARE(metaTypedMethod.methodType(), QMetaMethod::Method);

    QCOMPARE(metaTypedSignal.parameterMetaType(0), QMetaType::fromType<QString>());
    QCOMPARE(metaTypedSignal.parameterMetaType(1), QMetaType::fromType<QObject *>());
    QCOMPARE(metaTypedSignal.parameterMetaType(2), QMetaType::fromType<double>());
    QCOMPARE(metaTypedSignal.parameterNames(), QList<QByteArray>({ "a", "b", "c" }));

    QCOMPARE(metaUntypedMethod.parameterMetaType(0), QMetaType::fromType<QVariant>());
    QCOMPARE(metaUntypedMethod.parameterMetaType(1), QMetaType::fromType<QVariant>());
    QCOMPARE(metaUntypedMethod.parameterNames(), QList<QByteArray>({ "d", "c" }));

    QCOMPARE(metaTypedMethod.parameterMetaType(0), QMetaType::fromType<double>());
    QCOMPARE(metaTypedMethod.parameterMetaType(1), QMetaType::fromType<int>());
    QCOMPARE(metaTypedMethod.returnMetaType(), QMetaType::fromType<QString>());
    QCOMPARE(metaTypedMethod.parameterNames(), QList<QByteArray>({ "a", "b" }));
}

void tst_qmltc::properties()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(properties) created(&e); // check that it is creatable

    // what we can do is compare the types via QMetaType
    const QMetaObject *mo = created.metaObject();
    QVERIFY(mo);

    const auto propertyMetaType = [&](const char *propertyName) {
        return mo->property(mo->indexOfProperty(propertyName)).metaType();
    };

    QCOMPARE(propertyMetaType("boolP"), QMetaType::fromType<bool>());
    QCOMPARE(propertyMetaType("doubleP"), QMetaType::fromType<double>());
    QCOMPARE(propertyMetaType("intP"), QMetaType::fromType<int>());
    QCOMPARE(propertyMetaType("listQtObjP"), QMetaType::fromType<QQmlListProperty<QObject>>());
    QCOMPARE(propertyMetaType("realP"), QMetaType::fromType<double>());
    QCOMPARE(propertyMetaType("stringP"), QMetaType::fromType<QString>());
    QCOMPARE(propertyMetaType("urlP"), QMetaType::fromType<QUrl>());
    QCOMPARE(propertyMetaType("varP"), QMetaType::fromType<QVariant>());

    QCOMPARE(propertyMetaType("nullObjP"), QMetaType::fromType<QObject *>());
    QCOMPARE(propertyMetaType("nullVarP"), QMetaType::fromType<QVariant>());

    QCOMPARE(propertyMetaType("varP"), QMetaType::fromType<QVariant>());
    QCOMPARE(propertyMetaType("colorP"), QMetaType::fromType<QColor>());
    QCOMPARE(propertyMetaType("dateP"), QMetaType::fromType<QDateTime>());
    QCOMPARE(propertyMetaType("fontP"), QMetaType::fromType<QFont>());
    QCOMPARE(propertyMetaType("matrix4x4P"), QMetaType::fromType<QMatrix4x4>());
    QCOMPARE(propertyMetaType("pointP"), QMetaType::fromType<QPointF>());
    QCOMPARE(propertyMetaType("quatP"), QMetaType::fromType<QQuaternion>());
    QCOMPARE(propertyMetaType("rectP"), QMetaType::fromType<QRectF>());
    QCOMPARE(propertyMetaType("sizeP"), QMetaType::fromType<QSizeF>());
    QCOMPARE(propertyMetaType("vec2dP"), QMetaType::fromType<QVector2D>());
    QCOMPARE(propertyMetaType("vec3dP"), QMetaType::fromType<QVector3D>());
    QCOMPARE(propertyMetaType("vec4dP"), QMetaType::fromType<QVector4D>());

    QCOMPARE(propertyMetaType("defaultObjP"), QMetaType::fromType<QObject *>());

    // attributes:
    QCOMPARE(mo->classInfo(mo->indexOfClassInfo("DefaultProperty")).value(), "defaultObjP");
    QVERIFY(!mo->property(mo->indexOfProperty("readonlyStringP")).isWritable());
    QVERIFY(mo->property(mo->indexOfProperty("requiredRealP")).isRequired());

    // extra:
    QCOMPARE(propertyMetaType("timerP"), QMetaType::fromType<QQmlTimer *>());
    QCOMPARE(propertyMetaType("listCompP"), QMetaType::fromType<QQmlListProperty<QQmlComponent>>());

    QCOMPARE(propertyMetaType("table"), QMetaType::fromType<QObject *>());
    QCOMPARE(propertyMetaType("explicitCompP"), QMetaType::fromType<QObject *>());
    QCOMPARE(propertyMetaType("sentinelForComponent"), QMetaType::fromType<QObject *>());

    // now, test property values:
    QCOMPARE(created.boolP(), true);
    QCOMPARE(created.doubleP(), 0.5);
    QCOMPARE(created.intP(), 42);
    QCOMPARE(created.realP(), 2.32);
    QCOMPARE(created.stringP(), u"hello, world"_s);
    QCOMPARE(created.urlP(), u"https://www.qt.io/"_s);
    QCOMPARE(created.varP(), 42.42);

    QCOMPARE(created.boolP(), true);
    QCOMPARE(created.boolP(), true);

    QCOMPARE(created.colorP(), QColor(u"blue"_s));

    QCOMPARE(created.readonlyStringP(), u"foobar"_s);

    // object bindinds:
    const auto objectCtx = e.contextForObject(&created);
    QQmlListReference listQtObj(&created, "listQtObjP");
    QCOMPARE(listQtObj.size(), 3);
    {
        QQuickText *child0 = qobject_cast<QQuickText *>(listQtObj.at(0));
        QVERIFY(child0);
        QCOMPARE(child0->text(), u"child0"_s);
        QCOMPARE(objectCtx->objectForName("listQtObjP_child_0"), child0);

        QObject *child1 = listQtObj.at(1);
        QVERIFY(child1);
        QCOMPARE(child1->property("what").toString(), u"child1"_s);

        QQuickItem *child2 = qobject_cast<QQuickItem *>(listQtObj.at(2));
        QVERIFY(child2);
        QQmlListReference data(child2, "data");
        QCOMPARE(data.size(), 1);
        QQuickRectangle *child2Rect = qobject_cast<QQuickRectangle *>(data.at(0));
        QVERIFY(child2Rect);
        QCOMPARE(objectCtx->objectForName("listQtObjP_child_2_rect"), child2Rect);
    }

    QQmlTimer *timer = created.timerP();
    QVERIFY(timer);
    QCOMPARE(timer->interval(), 42);

    // nulls:
    QCOMPARE(created.nullObjP(), nullptr);
    QCOMPARE(created.nullVarP(), QVariant::fromValue(nullptr));

    QQuickTableView *table = qobject_cast<QQuickTableView *>(created.table());
    QVERIFY(table);
    {
        QQmlComponent *beforeDelegate = qvariant_cast<QQmlComponent *>(table->property("before"));
        QVERIFY(beforeDelegate);
        QQmlComponent *delegate = table->delegate();
        QVERIFY(delegate);
        QQmlComponent *afterDelegate = qvariant_cast<QQmlComponent *>(table->property("after"));
        QVERIFY(afterDelegate);

        QScopedPointer<QObject> beforeDelegateObject(beforeDelegate->create());
        QVERIFY(beforeDelegateObject);
        QVERIFY(qobject_cast<QQuickText *>(beforeDelegateObject.get()));
        QCOMPARE(beforeDelegateObject->property("text").toString(), u"beforeDelegate"_s);

        QScopedPointer<QObject> delegateObject(delegate->create());
        QVERIFY(delegateObject);
        QVERIFY(qobject_cast<QQuickText *>(delegateObject.get()));
        QCOMPARE(delegateObject->property("text").toString(), u"delegate"_s);

        QScopedPointer<QObject> afterDelegateObject(afterDelegate->create());
        QVERIFY(afterDelegateObject);
        QVERIFY(qobject_cast<QQuickText *>(afterDelegateObject.get()));
        QCOMPARE(afterDelegateObject->property("text").toString(), u"afterDelegate"_s);
    }

    QQmlComponent *explicitComp = qobject_cast<QQmlComponent *>(created.explicitCompP());
    QVERIFY(explicitComp);
    QScopedPointer<QObject> explicitCompObject(explicitComp->create());
    QVERIFY(explicitCompObject);
    QVERIFY(qobject_cast<QQuickText *>(explicitCompObject.get()));
    QCOMPARE(explicitCompObject->property("text").toString(), u"not a delegate"_s);

    QObject *sentinelForComponent = created.sentinelForComponent();
    QVERIFY(sentinelForComponent);
    QCOMPARE(sentinelForComponent->property("text").toString(), u"should be correctly created"_s);
}

void tst_qmltc::ids()
{
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(ObjectWithId) created(&e); // shouldn't crash here

        auto objectCtx = QQmlContextData::get(e.contextForObject(&created));
        QVERIFY(objectCtx);
        QCOMPARE(objectCtx->parent(), QQmlContextData::get(e.rootContext()));
        QCOMPARE(objectCtx->asQQmlContext()->objectForName("objectWithId"), &created);
        QCOMPARE(objectCtx->contextObject(), &created);
    }

    {
        QQmlEngine e;
        PREPEND_NAMESPACE(documentWithIds) created(&e); // shouldn't crash here

        auto ctx = e.contextForObject(&created);
        QVERIFY(ctx);
        auto objectCtx = QQmlContextData::get(ctx);
        QVERIFY(objectCtx);
        QCOMPARE(objectCtx->parent(), QQmlContextData::get(e.rootContext()));
        QCOMPARE(objectCtx->contextObject(), &created);

        // first check that ids match object names
        const auto objectNameById = [&ctx](const QString &id) {
            auto object = ctx->objectForName(id);
            if (!object)
                return QString();
            return object->objectName();
        };

        QCOMPARE(objectNameById("rectangle"), u"rectangle"_s);
        QCOMPARE(objectNameById("row"), u"row"_s);
        QCOMPARE(objectNameById("textInRectangle"), u"textInRectangle"_s);
        QCOMPARE(objectNameById("itemInList"), u"itemInList"_s);
        QCOMPARE(objectNameById("objectInList"), u"objectInList"_s);
        QCOMPARE(objectNameById("item"), u"item"_s);
        QCOMPARE(objectNameById("gridView"), u"gridView"_s);
        QCOMPARE(objectNameById("tableView"), u"tableView"_s);
        QCOMPARE(objectNameById("sentinel"), u"sentinel"_s);

        const auto verifyComponent = [&](QQmlComponent *component, const QString &componentId,
                                         const QString &objectId) {
            QVERIFY(component);
            if (!componentId.isEmpty()) // empty string for implicit components
                QCOMPARE(ctx->objectForName(componentId), component);
            QCOMPARE(ctx->objectForName(objectId), nullptr);

            QScopedPointer<QObject> root(component->create());
            QCOMPARE(root->objectName(), objectId);
            auto rootCtx = e.contextForObject(root.get());
            QVERIFY(rootCtx);
            QCOMPARE(rootCtx->objectForName(objectId), root.get());
        };

        auto explicitComponent = qobject_cast<QQmlComponent *>(created.explicitCompProperty());
        verifyComponent(explicitComponent, u"explicitComponent"_s, u"explicitText"_s);

        QQmlListReference children(&created, "data");
        QCOMPARE(children.size(), 2);
        QQuickTableView *table = qobject_cast<QQuickTableView *>(children.at(1));
        QVERIFY(table);
        QCOMPARE(ctx->objectForName(u"tableView"_s), table);
        QCOMPARE(table->objectName(), u"tableView"_s);

        auto before = qvariant_cast<QQmlComponent *>(table->property("before"));
        verifyComponent(before, u"beforeDelegate"_s, u"beforeDelegateText"_s);
        auto after = qvariant_cast<QQmlComponent *>(table->property("after"));
        verifyComponent(after, u"afterDelegate"_s, u"afterDelegateText"_s);

        auto delegate = table->delegate();
        verifyComponent(delegate, /* implicit component */ QString(), u"delegateRect"_s);

        // TableView is really special when you add Component to a default
        // property. see QQuickFlickablePrivate::data_append
        QQmlComponent *beforeChild = nullptr;
        QQmlComponent *afterChild = nullptr;
        const auto tableChildren = table->children(); // QObject::children()
        QVERIFY(tableChildren.size() >= 2);
        for (QObject *child : tableChildren) {
            auto comp = qobject_cast<QQmlComponent *>(child);
            if (!comp)
                continue;
            // this is bad, but there doesn't seem to be any better choice
            if (ctx->objectForName(u"beforeDelegateDefaultProperty"_s) == comp)
                beforeChild = comp;
            else if (ctx->objectForName(u"afterDelegateDefaultProperty"_s) == comp)
                afterChild = comp;
        }
        // we just used ctx->objectForName() to find these components, so
        // there's no point in checking the same condition in verifyComponent()
        verifyComponent(beforeChild, QString(), u"beforeDelegateDefaultPropertyText"_s);
        verifyComponent(afterChild, QString(), u"afterDelegateDefaultPropertyText"_s);
    }
}

void tst_qmltc::importNamespace()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(importNamespace) created(&e); // compilation of this type shouldn't crash
    QCOMPARE(created.text(), u"hello, world"_s);
}

void tst_qmltc::deferredProperties()
{
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(deferredProperties) created(&e);
        QVERIFY(created.deferredProperty()
                == nullptr); // binding is not applied since it is deferred

        qmlExecuteDeferred(&created);

        QQuickRectangle *rect = qobject_cast<QQuickRectangle *>(created.deferredProperty());
        QVERIFY(rect);
        QCOMPARE(rect->width(), created.width() * 2);
        QCOMPARE(rect->implicitHeight(), 4);
        QCOMPARE(rect->height(), rect->implicitHeight());

        QQmlListReference children(rect, "data");
        QCOMPARE(children.size(), 1);
        QQuickRectangle *subRect = qobject_cast<QQuickRectangle *>(children.at(0));
        QVERIFY(subRect);
        QCOMPARE(subRect->width(), created.width());
        QCOMPARE(subRect->height(), rect->height());
    }
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(deferredProperties_group) created(&e);
        QCOMPARE(created.getGroup()->getStr(), u"foobar"_s);
        QCOMPARE(created.getGroup()->getDeferred(), 0);
        // Note: we can't easily evaluate a deferred binding for a
        // `group.deferred` here, so just accept the fact the the value is not
        // set at all as a successful test
    }
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(deferredProperties_attached) created(&e);
        TestTypeAttachedWithDeferred *attached = qobject_cast<TestTypeAttachedWithDeferred *>(
                qmlAttachedPropertiesObject<DeferredAttached>(&created, false));
        QVERIFY(attached);

        QCOMPARE(attached->getAttachedFormula(), 43);
        QCOMPARE(attached->getDeferred(), 0);
        // Note: we can't easily evaluate a deferred binding for a
        // `group.deferred` here, so just accept the fact the the value is not
        // set at all as a successful test
    }
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(deferredProperties_complex) created(&e);

        // `group` binding is not deferred as per current behavior outside of
        // PropertyChanges and friends. we defer `group.deferred` binding though
        QCOMPARE(created.getGroup()->getStr(), u"still immediate"_s);
        QCOMPARE(created.getGroup()->getDeferred(), 0);

        QVERIFY(!qmlAttachedPropertiesObject<DeferredAttached>(&created, false));

        qmlExecuteDeferred(&created);

        TestTypeAttachedWithDeferred *attached = qobject_cast<TestTypeAttachedWithDeferred *>(
                qmlAttachedPropertiesObject<DeferredAttached>(&created, false));
        QVERIFY(attached);

        QCOMPARE(attached->getAttachedFormula(), 20);
        QCOMPARE(attached->getDeferred(), 100);
    }
}

// QTBUG-102560
void tst_qmltc::gradients()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(gradients) created(&e);
    QQmlListReference children(&created, "data");
    QCOMPARE(children.size(), 1);
    QQuickRectangle *rect = qobject_cast<QQuickRectangle *>(children.at(0));
    QVERIFY(rect);
    QVERIFY(rect->gradient().isQObject());

    QQuickGradient *gradient = qobject_cast<QQuickGradient *>(rect->gradient().toQObject());
    QVERIFY(gradient);
    QQmlListProperty<QQuickGradientStop> stops = gradient->stops();
    QCOMPARE(stops.count(&stops), 2);
    QQuickGradientStop *stop0 = stops.at(&stops, 0);
    QVERIFY(stop0);
    QQuickGradientStop *stop1 = stops.at(&stops, 1);
    QVERIFY(stop1);

    QCOMPARE(stop0->position(), 0.0);
    QCOMPARE(stop1->position(), 1.0);
    QCOMPARE(stop0->color(), QColor::fromString("black"));
    QCOMPARE(stop1->color(), QColor::fromString("yellow"));
}

void tst_qmltc::jsvalueAssignments()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(qjsvalueAssignments) created(&e);
    QVERIFY(created.jsvalue().isBool());
    QVERIFY(created.jsvalue().toBool());
}

void tst_qmltc::extensionTypeBindings()
{

    const auto verifyExtensionType = [](QObject *root) {
        QQmlListReference data(root, "data");
        QCOMPARE(data.count(), 9);

        // NB: Text object is not at index 0 due to non-QQuickItem-derived types
        // added along with it. This has something to do with QQuickItem's
        // internals that we just accept here
        auto text = qobject_cast<QQuickText *>(data.at(8));
        QVERIFY(text);
        auto withExtension = qobject_cast<TypeWithExtension *>(data.at(0));
        QVERIFY(withExtension);
        auto withExtensionDerived = qobject_cast<TypeWithExtensionDerived *>(data.at(1));
        QVERIFY(withExtensionDerived);
        auto withExtensionNamespace = qobject_cast<TypeWithExtensionNamespace *>(data.at(2));
        QVERIFY(withExtensionNamespace);

        // extra:
        auto withBaseTypeExtension = qobject_cast<TypeWithBaseTypeExtension *>(data.at(3));
        QVERIFY(withBaseTypeExtension);

        // qml:
        auto qmlWithExtension = qobject_cast<TypeWithExtension *>(data.at(4));
        QVERIFY(qmlWithExtension);
        auto qmlWithBaseTypeExtension = qobject_cast<TypeWithBaseTypeExtension *>(data.at(5));
        QVERIFY(qmlWithBaseTypeExtension);

        // script bindings:
        auto withExtensionDerivedScript = qobject_cast<TypeWithExtensionDerived *>(data.at(6));
        QVERIFY(withExtensionDerivedScript);
        auto withExtensionNamespaceScript = qobject_cast<TypeWithExtensionNamespace *>(data.at(7));
        QVERIFY(withExtensionNamespaceScript);

        QFont font = text->font();
        QCOMPARE(font.letterSpacing(), 13);

        QCOMPARE(withExtension->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(root->property("extCountAlias").toInt(), -10);

        root->setProperty("extCountAlias", 42);
        QCOMPARE(withExtension->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(root->property("extCountAlias").toInt(), 42);
        QVERIFY(withExtension->property("shouldBeVisible").toBool());
        QCOMPARE(withExtension->property("foo").toDouble(), 0);

        QCOMPARE(withExtensionDerived->getStr(), TypeWithExtensionDerived::unsetStr);
        QCOMPARE(withExtensionDerived->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(root->property("extDerivedStrAlias").toString(), u"hooray"_s);
        QCOMPARE(root->property("extDerivedCountAlias").toInt(), -10);
        // taken from extension
        QCOMPARE(withExtensionDerived->property("str").toString(), u"hooray"_s);
        QCOMPARE(withExtensionDerived->property("count").toInt(), -10);

        root->setProperty("extDerivedStrAlias", u"foo"_s);
        root->setProperty("extDerivedCountAlias", 42);
        QCOMPARE(withExtensionDerived->getStr(), TypeWithExtensionDerived::unsetStr);
        QCOMPARE(withExtensionDerived->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(root->property("extDerivedStrAlias").toString(), u"foo"_s);
        QCOMPARE(root->property("extDerivedCountAlias").toInt(), 42);
        QCOMPARE(withExtensionDerived->property("str").toString(), u"foo"_s);
        QCOMPARE(withExtensionDerived->property("count").toInt(), 42);
        QVERIFY(withExtensionDerived->property("shouldBeVisible").toBool());

        // namespace properties are ignored
        QCOMPARE(withExtensionNamespace->getCount(), -10);
        QCOMPARE(root->property("extNamespaceCountAlias").toInt(), -10);
        root->setProperty("extNamespaceCountAlias", 42);
        QCOMPARE(withExtensionNamespace->getCount(), 42);
        QCOMPARE(root->property("extNamespaceCountAlias").toInt(), 42);
        QVERIFY(withExtensionNamespace->property("shouldBeVisible").toBool());

        // extra:
        QCOMPARE(withBaseTypeExtension->getStr(), TypeWithExtensionDerived::unsetStr);
        QCOMPARE(withBaseTypeExtension->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(withBaseTypeExtension->property("str").toString(), u"hooray"_s);
        QCOMPARE(withBaseTypeExtension->property("count").toInt(), -10);
        QVERIFY(withBaseTypeExtension->property("shouldBeVisible").toBool());

        // qml:
        QCOMPARE(qmlWithExtension->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(qmlWithExtension->property("count"), -10);
        QVERIFY(qmlWithExtension->property("shouldBeVisibleFromBase").toBool());
        QVERIFY(qmlWithExtension->property("shouldBeVisible").toBool());

        QCOMPARE(qmlWithBaseTypeExtension->getStr(), TypeWithExtensionDerived::unsetStr);
        QCOMPARE(qmlWithBaseTypeExtension->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(qmlWithBaseTypeExtension->property("str").toString(), u"hooray"_s);
        QCOMPARE(qmlWithBaseTypeExtension->property("count").toInt(), -10);
        QVERIFY(qmlWithBaseTypeExtension->property("shouldBeVisibleFromBase").toBool());
        QVERIFY(qmlWithBaseTypeExtension->property("shouldBeVisible").toBool());

        // script bindings:
        QCOMPARE(withExtensionDerivedScript->getStr(), TypeWithExtensionDerived::unsetStr);
        QCOMPARE(withExtensionDerivedScript->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(withExtensionDerivedScript->property("str").toString(), u"hooray"_s);
        QCOMPARE(withExtensionDerivedScript->property("count").toInt(), -10);

        QCOMPARE(withExtensionNamespaceScript->getCount(), -10);
        QCOMPARE(withExtensionNamespaceScript->property("count").toInt(), -10);
    };

    {
        QQmlEngine e;
        QQmlComponent component(&e);
        component.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/extensionTypeBindings.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));

        verifyExtensionType(root.get());
    }

    if (QTest::currentTestFailed()) {
        qDebug() << "QQmlComponent test failed";
        return;
    }

    {
        QQmlEngine e;
        PREPEND_NAMESPACE(extensionTypeBindings) created(&e);

        verifyExtensionType(&created);

        // additionally, check that setting aliases directly works fine
        QQmlListReference data(&created, "data");
        auto withExtension = qobject_cast<TypeWithExtension *>(data.at(0));
        auto withExtensionDerived = qobject_cast<TypeWithExtensionDerived *>(data.at(1));
        auto withExtensionNamespace = qobject_cast<TypeWithExtensionNamespace *>(data.at(2));

        created.setExtCountAlias(-77);
        QCOMPARE(withExtension->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(withExtension->property("count").toInt(), -77); // via extension
        QCOMPARE(created.extCountAlias(), -77);

        created.setExtDerivedCountAlias(-77);
        created.setExtDerivedStrAlias(u"bar"_s);
        QCOMPARE(withExtensionDerived->getCount(), TypeWithExtension::unsetCount);
        QCOMPARE(withExtensionDerived->getStr(), TypeWithExtensionDerived::unsetStr);
        QCOMPARE(withExtensionDerived->property("count").toInt(), -77); // via extension
        QCOMPARE(withExtensionDerived->property("str").toString(), u"bar"_s); // via extension
        QCOMPARE(created.extDerivedCountAlias(), -77);
        QCOMPARE(created.extDerivedStrAlias(), u"bar"_s);

        created.setExtNamespaceCountAlias(-77);
        QCOMPARE(withExtensionNamespace->getCount(), -77);
        QCOMPARE(withExtensionNamespace->property("count").toInt(), -77);
    }
}

// QTBUG-103956
void tst_qmltc::visibleAliasMethods()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(qtbug103956_main) created(&e);
    QVERIFY(created.firstComponent());
    QCOMPARE(created.firstComponent()->setMe(), true);
}

// QTBUG-104094
void tst_qmltc::nonStandardIncludesInsideModule()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(nonStandardInclude) created(&e);
    QVERIFY(created.good());
}

void tst_qmltc::specialProperties()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(specialProperties) created(&e);
    QCOMPARE(created.property("x"), 42);
    QCOMPARE(created.m_y, u"fourty two"_s);
    QCOMPARE(created.bindableZ().value(), 3.2);
    QCOMPARE(created.xAlias(), 42);
    QCOMPARE(created.yAlias(), u"fourty two"_s);
    QCOMPARE(created.zAlias(), 3.2);

    created.setXAlias(43);
    QCOMPARE(created.property("x"), 43);
    created.setYAlias(u"foo"_s);
    QCOMPARE(created.m_y, u"foo"_s);
    created.setZAlias(4.2);
    QCOMPARE(created.bindableZ().value(), 4.2);

    // alias attributes:
    const QMetaObject *mo = created.metaObject();
    QMetaProperty xxAlias = mo->property(mo->indexOfProperty("xxAlias"));

    QQmlComponent c(&e);
    c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/specialProperties.qml"));
    QScopedPointer<QObject> _fromEngine(c.create());
    QVERIFY2(_fromEngine, qPrintable(c.errorString()));
    QObject &fromEngine = *_fromEngine;
    const QMetaObject *fromEngineMetaObject = fromEngine.metaObject();

    QMetaProperty xxAliasFromEngine =
            fromEngineMetaObject->property(mo->indexOfProperty("xxAlias"));
    QVERIFY(xxAlias.isValid());
    QVERIFY(xxAliasFromEngine.isValid());
    QCOMPARE(xxAlias.isConstant(), xxAliasFromEngine.isConstant());
    QCOMPARE(created.xyAlias(), u"reset");
}

void tst_qmltc::regexpBindings()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(regexpBindings) created(&e);
    QCOMPARE(created.regularExpression().pattern(), u"ab*c");
    QVERIFY(created.regularExpression().match(u"abbbc"_s).hasMatch());
}

void tst_qmltc::aliasAssignments()
{
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(AliasBase) created(&e);
        QCOMPARE(created.alias1(), 2);
    }

    {
        QQmlEngine e;
        PREPEND_NAMESPACE(aliasAssignments) created(&e);
        QCOMPARE(created.alias1(), 4);
        QCOMPARE(created.alias2(), 4);
    }
}

void tst_qmltc::connections()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(Connections) created(&e);
}

void tst_qmltc::signalHandlers()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(signalHandlers) created(&e);

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

void tst_qmltc::jsFunctions()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(javaScriptFunctions) created(&e);

    created.func1();
    created.func2(QStringLiteral("abc"));

    QCOMPARE(created.property("func1P").toInt(), 1);
    QCOMPARE(created.property("func2P").toString(), QStringLiteral("abc"));
    QCOMPARE(created.func3(), false);

    created.setProperty("func3P", true);
    QCOMPARE(created.func3(), true);
}

void tst_qmltc::changingBindings()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(changingBindings) created(&e);

    // test initial binding
    QCOMPARE(created.property("p2").toInt(), 2); // p1 + 1

    // test JS constant value
    created.resetToConstant();
    QCOMPARE(created.property("p2").toInt(), 42); // 42

    // test Qt.binding()
    created.resetToNewBinding();
    created.setProperty("p1", 100);
    QCOMPARE(created.property("p2").toInt(), 200); // p1 * 2

    // test resetting value through C++
    created.setP2(0);
    created.setP1(-10);
    QCOMPARE(created.property("p2").toInt(), 0);

    created.setProperty("p2", 1);
    QCOMPARE(created.property("p2").toInt(), 1);

    // test binding can be set again even after reset from C++
    created.resetToNewBinding();
    QCOMPARE(created.property("p2").toInt(), -20);
}

void tst_qmltc::propertyAlias()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(propertyAlias) created(&e);

    // test initial binding
    QCOMPARE(created.property("origin").toInt(), 6); // dummy / 2
    QCOMPARE(created.property("aliasToOrigin").toInt(), 6);

    QCOMPARE(created.getAliasValue().toInt(), 6);
    QCOMPARE(created.aliasToOrigin(), 6);
    created.setDummy(10);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 5);
    QCOMPARE(created.getAliasValue().toInt(), 5);
    QCOMPARE(created.aliasToOrigin(), 5);

    // test the C++ setter
    created.setOrigin(7);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 7);
    QCOMPARE(created.getAliasValue().toInt(), 7);
    QCOMPARE(created.aliasToOrigin(), 7);

    // test meta-object setter
    created.setProperty("origin", 1);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 1);
    QCOMPARE(created.getAliasValue().toInt(), 1);
    QCOMPARE(created.aliasToOrigin(), 1);

    // test QML/JS setter
    created.resetOriginToConstant();
    QCOMPARE(created.property("aliasToOrigin").toInt(), 189);
    QCOMPARE(created.getAliasValue().toInt(), 189);
    QCOMPARE(created.aliasToOrigin(), 189);

    // test QML/JS alias setter
    created.resetAliasToConstant();
    QCOMPARE(created.property("origin").toInt(), 42);
    QCOMPARE(created.origin(), 42);
    // check the alias just to make sure it also works
    QCOMPARE(created.property("aliasToOrigin").toInt(), 42);
    QCOMPARE(created.getAliasValue().toInt(), 42);
    QCOMPARE(created.aliasToOrigin(), 42);

    // test QML/JS binding reset
    created.resetOriginToNewBinding(); // dummy
    created.setDummy(99);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 99);
    QCOMPARE(created.getAliasValue().toInt(), 99);
    QCOMPARE(created.aliasToOrigin(), 99);

    // test QML/JS binding reset through alias
    created.resetAliasToNewBinding(); // dummy * 3
    created.setDummy(-8);
    QCOMPARE(created.property("origin").toInt(), -24);
    QCOMPARE(created.origin(), -24);
    QCOMPARE(created.property("aliasToOrigin").toInt(), -24);
    QCOMPARE(created.getAliasValue().toInt(), -24);
    QCOMPARE(created.aliasToOrigin(), -24);
}

void tst_qmltc::propertyAlias_external()
{
    QQmlEngine e;

    PREPEND_NAMESPACE(propertyAlias_external) created(&e);
    QCOMPARE(created.height(), 1);
    QCOMPARE(created.height(), created.heightAlias());

    created.setHeightAlias(42);
    QCOMPARE(created.height(), 42);
    QCOMPARE(created.height(), created.heightAlias());

    QCOMPARE(created.parentItem(), nullptr);
    QCOMPARE(created.parentAlias(), created.parentItem());

    QSignalSpy heightChangedSpy(&created, &QQuickItem::heightChanged);
    QSignalSpy heightAliasChangedSpy(&created, &PREPEND_NAMESPACE(propertyAlias_external)::heightAliasChanged);
    created.setHeight(10);
    QCOMPARE(created.heightAlias(), 10);
    QCOMPARE(heightChangedSpy.size(), 1);
    QCOMPARE(heightAliasChangedSpy.size(), 1);
}

void tst_qmltc::propertyAliasAttribute()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(propertyAliasAttributes) fromQmltc(&e);

    QQmlComponent c(&e);
    c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/propertyAliasAttributes.qml"));
    QScopedPointer<QObject> _fromEngine(c.create());
    QVERIFY2(_fromEngine, qPrintable(c.errorString()));
    QObject &fromEngine = *_fromEngine;
    const QMetaObject *fromEngineMetaObject = fromEngine.metaObject();

    const QString stringA = u"The quick brown fox"_s;
    const QString stringB = u"jumps over the lazy dog."_s;

    QCOMPARE(fromQmltc.readOnlyAlias(), u"Hello World!"_s);
    QCOMPARE(fromEngine.property("readOnlyAlias"), u"Hello World!"_s);

    QVERIFY(!fromQmltc.setProperty("readOnlyAlias", u"Some string"_s));
    QVERIFY(!fromEngine.setProperty("readOnlyAlias", u"Some string"_s));

    // reading and writing from alias is already covered in the alias test
    // check if it works on properties with the MEMBERS attribute
    fromQmltc.setReadAndWriteMemberAlias(stringA);
    fromEngine.setProperty("readAndWriteMemberAlias", stringA);

    QCOMPARE(fromQmltc.property("readAndWriteMember"), stringA);
    QCOMPARE(fromEngine.property("readAndWriteMember"), stringA);

    fromQmltc.setReadAndWriteMemberAlias(stringB);
    fromEngine.setProperty("readAndWriteMemberAlias", stringB);

    QCOMPARE(fromQmltc.readAndWriteMemberAlias(), stringB);
    QCOMPARE(fromQmltc.property("readAndWriteMember"), stringB);

    QCOMPARE(fromEngine.property("readAndWriteMemberAlias"), stringB);
    QCOMPARE(fromEngine.property("readAndWriteMember"), stringB);

    // check if alias can be reset through property
    fromQmltc.setResettableAlias(stringA);
    fromEngine.setProperty("resettableAlias", stringB);
    fromQmltc.resetResettable();
    const int resettableIdx = fromEngineMetaObject->indexOfProperty("resettable");
    QVERIFY(fromEngineMetaObject->property(resettableIdx).reset(&fromEngine));
    QCOMPARE(fromQmltc.resettable(), u"Reset!"_s);
    QCOMPARE(fromQmltc.resettableAlias(), u"Reset!"_s);
    QCOMPARE(fromEngine.property("resettable"), u"Reset!"_s);
    QCOMPARE(fromEngine.property("resettableAlias"), u"Reset!"_s);

    // check if property can be reset through alias
    fromQmltc.setResettableAlias(stringA);
    fromEngine.setProperty("resettableAlias", stringA);
    fromQmltc.resetResettableAlias();
    QMetaMethod resetResettableAlias = fromEngineMetaObject->method(
            fromEngineMetaObject->indexOfMethod("resetResettableAlias"));
    resetResettableAlias.invoke(&fromEngine);
    QCOMPARE(fromQmltc.resettable(), u"Reset!"_s);
    QCOMPARE(fromQmltc.resettableAlias(), u"Reset!"_s);
    QCOMPARE(fromEngine.property("resettable"), u"Reset!"_s);
    QCOMPARE(fromEngine.property("resettableAlias"), u"Reset!"_s);

    // check if property can be reset by assigning undefined to alias
    fromQmltc.setResettableAlias(stringA);
    fromEngine.setProperty("resettableAlias", stringA);
    fromQmltc.assignUndefinedToResettableAlias();
    QMetaMethod assignUndefinedToResettableAlias = fromEngineMetaObject->method(
            fromEngineMetaObject->indexOfMethod("assignUndefinedToResettableAlias"));
    assignUndefinedToResettableAlias.invoke(&fromEngine);
    QCOMPARE(fromQmltc.resettableAlias(), u"Reset!"_s);
    QCOMPARE(fromQmltc.resettable(), u"Reset!"_s);
    QCOMPARE(fromEngine.property("resettableAlias"), u"Reset!"_s);
    QCOMPARE(fromEngine.property("resettable"), u"Reset!"_s);

    // check if property can be reset by assigning undefined to alias of
    // non-resettable prop which should not happen: instead, nothing should happen
    fromQmltc.setUnresettableAlias(stringA);
    fromEngine.setProperty("unresettableAlias", stringA);
    fromQmltc.assignUndefinedToUnresettableAlias();
    QMetaMethod assignUndefinedToUnresettableAlias = fromEngineMetaObject->method(
            fromEngineMetaObject->indexOfMethod("assignUndefinedToUnresettableAlias"));
    assignUndefinedToUnresettableAlias.invoke(&fromEngine);
    QCOMPARE(fromQmltc.unresettableAlias(), stringA);
    QCOMPARE(fromQmltc.property("unresettable"), stringA);
    QCOMPARE(fromEngine.property("unresettableAlias"), stringA);
    QCOMPARE(fromEngine.property("unresettable"), stringA);

    // check if notify arrives!
    fromQmltc.setReadAndWrite(stringB);
    fromEngine.setProperty("readAndWrite", stringB);
    qsizetype calls = 0;
    QSignalSpy spyQmltc(&fromQmltc, SIGNAL(notifiableChanged(QString)));
    QSignalSpy spyEngine(&fromEngine, SIGNAL(notifiableChanged(QString)));
    // write through alias
    fromQmltc.setNotifiableAlias(stringA);
    QVERIFY(fromEngine.setProperty("notifiableAlias", stringA));
    QCOMPARE(spyQmltc.size(), ++calls);
    QCOMPARE(spyEngine.size(), calls);
    // write through property
    fromQmltc.setReadAndWriteAndNotify(stringB);
    QVERIFY(fromEngine.setProperty("notifiable", stringB));
    QCOMPARE(spyQmltc.size(), ++calls);
    QCOMPARE(spyEngine.size(), calls);

    fromQmltc.setNotifiableMemberAlias(stringA);
    QVERIFY(fromEngine.setProperty("notifiableMemberAlias", stringA));
    QCOMPARE(spyQmltc.size(), ++calls);
    QCOMPARE(spyEngine.size(), calls);
    fromQmltc.setProperty("notifiableMember", stringB);
    QVERIFY(fromEngine.setProperty("notifiableMember", stringB));
    QCOMPARE(spyQmltc.size(), ++calls);
    QCOMPARE(spyEngine.size(), calls);

    // check that the alias to a revisioned property works
    fromQmltc.setLatestReadAndWriteAlias(stringA);
    QVERIFY(fromEngine.setProperty("latestReadAndWriteAlias", stringA));
    QCOMPARE(fromQmltc.latestReadAndWriteAlias(), stringA);
    QCOMPARE(fromQmltc.property("latestReadAndWrite"), stringA);
    QCOMPARE(fromEngine.property("latestReadAndWriteAlias"), stringA);
    QCOMPARE(fromEngine.property("latestReadAndWrite"), stringA);

    QVERIFY(fromQmltc.setProperty("latestReadAndWrite", stringB));
    QVERIFY(fromEngine.setProperty("latestReadAndWrite", stringB));
    QCOMPARE(fromQmltc.latestReadAndWriteAlias(), stringB);
    QCOMPARE(fromQmltc.property("latestReadAndWrite"), stringB);
    QCOMPARE(fromEngine.property("latestReadAndWriteAlias"), stringB);
    QCOMPARE(fromEngine.property("latestReadAndWrite"), stringB);

    // check if metaobject of alias is correct
    const QVector<const QMetaObject *> metaObjects = {
        fromQmltc.metaObject(),
        fromEngine.metaObject(),
    };

    QVERIFY(metaObjects[0]);
    QVERIFY(metaObjects[1]);

    QVector<QHash<QString, QMetaProperty>> metaProperties(2);
    for (int j = 0; j < metaObjects.size(); j++) {
        const QMetaObject *metaObject = metaObjects[j];
        for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
            metaProperties[j][QString::fromLatin1(metaObject->property(i).name())] =
                    metaObject->property(i);
        }
    }

    {
        QVERIFY(metaProperties[0].contains("hasAllAttributesAlias"));
        QVERIFY(metaProperties[1].contains("hasAllAttributesAlias"));
        QMetaProperty mpQmltc = metaProperties[0].value("hasAllAttributesAlias");
        QMetaProperty mpEngine = metaProperties[1].value("hasAllAttributesAlias");
        QCOMPARE(mpQmltc.isReadable(), mpEngine.isReadable());
        QCOMPARE(mpQmltc.isWritable(), mpEngine.isWritable());
        QCOMPARE(mpQmltc.isResettable(), mpEngine.isResettable());
        QCOMPARE(mpQmltc.hasNotifySignal(), mpEngine.hasNotifySignal());
        QCOMPARE(mpQmltc.revision(), mpEngine.revision());
        QCOMPARE(mpQmltc.isDesignable(), mpEngine.isDesignable());
        QCOMPARE(mpQmltc.isScriptable(), mpEngine.isScriptable());
        QCOMPARE(mpQmltc.isStored(), mpEngine.isStored());
        QCOMPARE(mpQmltc.isUser(), mpEngine.isUser());
        QCOMPARE(mpQmltc.isBindable(), mpEngine.isBindable());
        QCOMPARE(mpQmltc.isConstant(), mpEngine.isConstant());
        QCOMPARE(mpQmltc.isFinal(), mpEngine.isFinal());
        QCOMPARE(mpQmltc.isRequired(), mpEngine.isRequired());
    }

    {
        QVERIFY(metaProperties[0].contains("hasAllAttributes2Alias"));
        QVERIFY(metaProperties[1].contains("hasAllAttributes2Alias"));
        QMetaProperty mpQmltc = metaProperties[0].value("hasAllAttributes2Alias");
        QMetaProperty mpEngine = metaProperties[1].value("hasAllAttributes2Alias");
        QCOMPARE(mpQmltc.isReadable(), mpEngine.isReadable());
        QCOMPARE(mpQmltc.isWritable(), mpEngine.isWritable());
        QCOMPARE(mpQmltc.isResettable(), mpEngine.isResettable());
        QCOMPARE(mpQmltc.hasNotifySignal(), mpEngine.hasNotifySignal());
        QCOMPARE(mpQmltc.revision(), mpEngine.revision());
        QCOMPARE(mpQmltc.isDesignable(), mpEngine.isDesignable());
        QCOMPARE(mpQmltc.isScriptable(), mpEngine.isScriptable());
        QCOMPARE(mpQmltc.isStored(), mpEngine.isStored());
        QCOMPARE(mpQmltc.isUser(), mpEngine.isUser());
        QCOMPARE(mpQmltc.isBindable(), mpEngine.isBindable());
        QCOMPARE(mpQmltc.isConstant(), mpEngine.isConstant());
        QCOMPARE(mpQmltc.isFinal(), mpEngine.isFinal());
        QCOMPARE(mpQmltc.isRequired(), mpEngine.isRequired());
    }
}

// TODO: we need to support RESET in aliases as well? (does it make sense?)
void tst_qmltc::complexAliases()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(complexAliases) created(&e);

    // setup:
    QQuickItem *theRect = nullptr;
    QQuickItem *theText = nullptr;
    QObject *accessibleBehavior = nullptr;
    PREPEND_NAMESPACE(complexAliases_LocallyImported) *localImport = nullptr;
    PREPEND_NAMESPACE(complexAliases_LocallyImported_Rectangle) *theRectInsideImported = nullptr;
    {
        QQmlListReference children(&created, "data");
        QCOMPARE(children.size(), 2);

        theRect = qobject_cast<QQuickItem *>(children.at(0));
        QVERIFY(theRect);
        {
            QQmlListReference children(theRect, "data");
            QCOMPARE(children.size(), 1);
            theText = qobject_cast<QQuickItem *>(children.at(0));
            QVERIFY(theText);
        }

        localImport = qobject_cast<PREPEND_NAMESPACE(complexAliases_LocallyImported) *>(children.at(1));
        QVERIFY(localImport);
        {
            QQmlListReference children(localImport, "baseDefaultList");
            QCOMPARE(children.size(), 1);
            theRectInsideImported =
                    qobject_cast<PREPEND_NAMESPACE(complexAliases_LocallyImported_Rectangle) *>(children.at(0));
            QVERIFY(theRectInsideImported);
        }
        accessibleBehavior = qvariant_cast<QObject *>(
                e.contextForObject(&created)->contextProperty("accessibleBehavior"));
        QVERIFY(accessibleBehavior);
    }

    // aText:
    QCOMPARE(created.aText(), theRect->property("text").toString());
    created.updateTextThroughAlias(QStringLiteral("42"));
    QCOMPARE(created.aText(), QStringLiteral("42"));
    QCOMPARE(created.aText(), theRect->property("text").toString());
    created.updateText(QStringLiteral("fourty-two"));
    QCOMPARE(theRect->property("text").toString(), QStringLiteral("fourty-two"));
    QCOMPARE(created.aText(), theRect->property("text").toString());

    // aColor:
    QCOMPARE(created.aColor(), QColor(QStringLiteral("red")));
    QCOMPARE(created.aColor(), theText->property("color").value<QColor>());
    created.setAColor(QColor("black"));
    QCOMPARE(created.aColor(), QColor(QStringLiteral("black")));
    QCOMPARE(created.aColor(), theText->property("color").value<QColor>());
    theText->setProperty("color", QColor(QStringLiteral("green")));
    QCOMPARE(theText->property("color").value<QColor>(), QColor(QStringLiteral("green")));
    QCOMPARE(created.aColor(), theText->property("color").value<QColor>());

    // aliases to ids:
    QCOMPARE(created.aRectObject(), theRect);
    QCOMPARE(created.aTextObject(), theText);

    // aLetterSpacing:
    QCOMPARE(created.aLetterSpacing(), theText->property("font").value<QFont>().letterSpacing());
    created.setALetterSpacing(5);
    QCOMPARE(created.aLetterSpacing(), 5);
    QCOMPARE(created.aLetterSpacing(), theText->property("font").value<QFont>().letterSpacing());

    // aWordSpacing:
    QCOMPARE(created.aWordSpacing(), theText->property("font").value<QFont>().wordSpacing());
    created.setAWordSpacing(42);
    QCOMPARE(created.aWordSpacing(), 42);
    QCOMPARE(created.aWordSpacing(), theText->property("font").value<QFont>().wordSpacing());

    // aFont:
    QCOMPARE(created.aFont(), theText->property("font").value<QFont>());
    QSignalSpy aFontSpy(&created, &PREPEND_NAMESPACE(complexAliases)::aFontChanged);
    QFont newFont = created.aFont();
    newFont.setWeight(QFont::DemiBold);
    created.setAFont(newFont);
    QCOMPARE(created.aFont(), newFont);
    QCOMPARE(created.aFont(), theText->property("font").value<QFont>());
    QCOMPARE(aFontSpy.size(), 1);
    newFont.setStyle(QFont::StyleOblique);
    theText->setProperty("font", newFont);
    QCOMPARE(theText->property("font").value<QFont>(), newFont);
    QCOMPARE(created.aFont(), theText->property("font").value<QFont>());
    QCOMPARE(aFontSpy.size(), 2);

    created.setAWordSpacing(1);
    QCOMPARE(aFontSpy.size(), 3);

    // aliasToObjectAlias:
    QCOMPARE(created.aliasToObjectAlias(), created.aRectObject());

    // aliasToPropertyAlias:
    QCOMPARE(created.aliasToPropertyAlias(), created.aText());
    created.setAliasToPropertyAlias(QStringLiteral("milky way"));
    QCOMPARE(created.aliasToPropertyAlias(), QStringLiteral("milky way"));
    QCOMPARE(created.aliasToPropertyAlias(), created.aText());
    QCOMPARE(created.aliasToPropertyAlias(), theRect->property("text").toString());
    QProperty<QString> source;
    created.bindableAliasToPropertyAlias().setBinding(Qt::makePropertyBinding(source));
    source.setValue(QStringLiteral("bound value"));
    QCOMPARE(created.aliasToPropertyAlias(), QStringLiteral("bound value"));
    QCOMPARE(created.aliasToPropertyAlias(), created.aText());
    QCOMPARE(created.aliasToPropertyAlias(), theRect->property("text").toString());
    source.setValue(QStringLiteral("foobar"));
    QCOMPARE(created.aliasToPropertyAlias(), QStringLiteral("foobar"));
    QCOMPARE(created.aliasToPropertyAlias(), created.aText());
    QCOMPARE(created.aliasToPropertyAlias(), theRect->property("text").toString());
    created.setAliasToPropertyAlias(QStringLiteral("resetting back"));
    source.setValue(QStringLiteral("unused"));
    QCOMPARE(created.aliasToPropertyAlias(), QStringLiteral("resetting back"));
    QCOMPARE(created.aliasToPropertyAlias(), created.aText());
    QCOMPARE(created.aliasToPropertyAlias(), theRect->property("text").toString());

    // aliasToValueTypeAlias:
    QCOMPARE(created.aliasToValueTypeAlias(), created.aFont());
    QSignalSpy aliasToValueTypeAliasSpy(&created, &PREPEND_NAMESPACE(complexAliases)::aliasToValueTypeAliasChanged);
    newFont.setPixelSize(389);
    created.setAliasToValueTypeAlias(newFont);
    QCOMPARE(created.aliasToValueTypeAlias(), newFont);
    QCOMPARE(created.aliasToValueTypeAlias(), created.aFont());
    QCOMPARE(created.aliasToValueTypeAlias(), theText->property("font").value<QFont>());
    QCOMPARE(aFontSpy.size(), 4);
    QCOMPARE(aliasToValueTypeAliasSpy.size(), 1);
    newFont.setPixelSize(12);
    created.setAFont(newFont);
    QCOMPARE(aFontSpy.size(), 5);
    QCOMPARE(aliasToValueTypeAliasSpy.size(), 2);
    QCOMPARE(created.aliasToValueTypeAlias(), created.aFont());
    QCOMPARE(created.aliasToValueTypeAlias(), theText->property("font").value<QFont>());

    // aliasToPropertyOfValueTypeAlias:
    QCOMPARE(created.aliasToPropertyOfValueTypeAlias(), newFont.pixelSize());
    created.setAliasToPropertyOfValueTypeAlias(3);
    QCOMPARE(created.aliasToPropertyOfValueTypeAlias(), created.aFont().pixelSize());
    QCOMPARE(created.aFont(), theText->property("font").value<QFont>());
    QCOMPARE(aFontSpy.size(), 6);
    QCOMPARE(aliasToValueTypeAliasSpy.size(), 3);

    // aliasToImportedMessage:
    QCOMPARE(created.aliasToImportedMessage(), localImport->property("message").toString());
    created.setAliasToImportedMessage(QStringLiteral("new value"));
    QCOMPARE(created.aliasToImportedMessage(), localImport->property("message").toString());
    localImport->setProperty("message", QStringLiteral("old value"));
    QCOMPARE(created.aliasToImportedMessage(), localImport->property("message").toString());
    // don't test bindability, since it's tested by another property

    // aliasToOnAssignmentProperty
    QCOMPARE(created.aliasToOnAssignmentProperty(), QStringLiteral("width"));

    // localImport.importedAliasToText:
    QCOMPARE(localImport->importedAliasToText(), theRect->property("text").toString());
    localImport->setImportedAliasToText(QStringLiteral("text never seen before"));
    QCOMPARE(localImport->importedAliasToText(), theRect->property("text").toString());
    theRect->setProperty("text", QStringLiteral("text, maybe, already seen before"));
    QCOMPARE(localImport->importedAliasToText(), theRect->property("text").toString());
    QCOMPARE(localImport->importedAliasToText(), created.aText());

    // theRectInsideImported.internallyImportedAliasToText:
    QCOMPARE(theRectInsideImported->internallyImportedAliasToText(),
             theText->property("text").toString());
    theRectInsideImported->setInternallyImportedAliasToText(QStringLiteral("set unset reset"));
    QCOMPARE(theRectInsideImported->internallyImportedAliasToText(),
             theText->property("text").toString());
    theText->setProperty("text", QStringLiteral("zzz"));
    QCOMPARE(theRectInsideImported->internallyImportedAliasToText(),
             theText->property("text").toString());

    // aliasToPrivatePalette:
    QCOMPARE(created.aliasToPrivatePalette(), QQuickItemPrivate::get(theRect)->palette());
    QSignalSpy paletteChangedSpy(&created, &PREPEND_NAMESPACE(complexAliases)::aliasToPrivatePaletteChanged);
    QQuickPalette *newPalette = new QQuickPalette(&created);
    newPalette->fromQPalette(QPalette(QColor(u"cyan"_s)));
    QCOMPARE(newPalette->button(), QColor(u"cyan"_s));
    created.setAliasToPrivatePalette(newPalette);
    QCOMPARE(paletteChangedSpy.size(), 1);
    QCOMPARE(QQuickItemPrivate::get(theRect)->palette()->button(), QColor(u"cyan"_s));
    QCOMPARE(created.aliasToPrivatePalette(), QQuickItemPrivate::get(theRect)->palette());

    // aliasToAnchors:
    QCOMPARE(created.aliasToAnchors(), QQuickItemPrivate::get(theRect)->anchors());
    QQuickItemPrivate::get(theRect)->anchors()->setTopMargin(5);
    QCOMPARE(created.aliasToAnchors()->topMargin(), 5);

    // aliasToPrivateData:
    QCOMPARE(created.aliasToPrivateData(), QQuickItemPrivate::get(&created)->data());
}

void tst_qmltc::propertyChangeHandler()
{
    {
        QQmlEngine e;
        QQmlComponent c(&e);
        c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/propertyChangeHandler.qml"));
        QScopedPointer<QObject> root(c.create());
        QVERIFY2(root, qPrintable(c.errorString()));
        QCOMPARE(root->property("watcher").toInt(), 42);
    }

    QQmlEngine e;
    PREPEND_NAMESPACE(propertyChangeHandler) created(&e);

    // NB: watcher is set to 42 - aligned with QQmlComponent
    QCOMPARE(created.watcher(), 42);
    QCOMPARE(created.p(), 42); // due to binding
    QCOMPARE(created.watcher(), 42);
    QCOMPARE(created.property("watcher").toInt(), 42);

    // test that binding triggers property change handler
    created.setDummy(20);
    QCOMPARE(created.watcher(), 20);
    QCOMPARE(created.property("watcher").toInt(), 20);

    // test that property setting (through C++) triggers property change handler
    created.setWatcher(-100);
    created.setProperty("p", 18);
    QCOMPARE(created.watcher(), 18);

    // test that property setting triggers property change handler
    created.setWatcher(-47);
    created.setP(96);
    QCOMPARE(created.property("watcher").toInt(), 96);
}

void tst_qmltc::nestedHelloWorld()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(NestedHelloWorld) created(&e);

    QCOMPARE(created.hello(), QStringLiteral("Hello"));
    QCOMPARE(created.jsGetGreeting().toString(), QStringLiteral("Hello, World!"));
    QCOMPARE(created.greeting(), QStringLiteral("Hello, World!"));

    QPointer<PREPEND_NAMESPACE(NestedHelloWorld_QtObject)> firstLevelChild(
            qobject_cast<PREPEND_NAMESPACE(NestedHelloWorld_QtObject) *>(created.child()));
    QVERIFY(firstLevelChild);
    QCOMPARE(firstLevelChild->hello(), QStringLiteral("Hello"));
    QCOMPARE(firstLevelChild->jsGetGreeting().toString(), QStringLiteral("Hello, Qt!"));
    QCOMPARE(firstLevelChild->greeting(), QStringLiteral("Hello, Qt!"));

    QPointer<PREPEND_NAMESPACE(NestedHelloWorld_QtObject_QtObject)> secondLevelChild(
            qobject_cast<PREPEND_NAMESPACE(NestedHelloWorld_QtObject_QtObject) *>(firstLevelChild->child()));
    QVERIFY(secondLevelChild);
    QCOMPARE(secondLevelChild->hello(), QStringLiteral("Hello"));
    QCOMPARE(secondLevelChild->jsGetGreeting().toString(), QStringLiteral("Hello, Qml!"));
    QCOMPARE(secondLevelChild->greeting(), QStringLiteral("Hello, Qml!"));
}

void tst_qmltc::componentHelloWorld()
{
    QQmlEngine e;
    QScopedPointer<PREPEND_NAMESPACE(ComponentHelloWorld)> created(new PREPEND_NAMESPACE(ComponentHelloWorld)(&e));
    QVERIFY(QQmlData::get(created.get())->context->componentAttacheds());

    QCOMPARE(created->hello(), QStringLiteral("Hello, World!"));

    QSignalSpy onDestroySpy(created.get(), &PREPEND_NAMESPACE(ComponentHelloWorld)::sDestroying);
    QCOMPARE(onDestroySpy.size(), 0);
    created.reset();
    QCOMPARE(onDestroySpy.size(), 1);
}

void tst_qmltc::propertyReturningFunction()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(propertyReturningFunction) created(&e);

    QCOMPARE(created.counter(), 0);
    QVariant f = created.f();
    QCOMPARE(created.counter(), 0);

    created.property("f");
    QCOMPARE(created.counter(), 0);

    QJSValue function = qvariant_cast<QJSValue>(f);
    QVERIFY(function.isCallable());
    function.call();
    QCOMPARE(created.counter(), 1);
    function.call();
    QCOMPARE(created.counter(), 2);
}

void tst_qmltc::listProperty()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(listProperty) created(&e);

    QQmlComponent c(&e);
    c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/listProperty.qml"));
    QScopedPointer<QObject> fromEngine(c.create());
    QVERIFY2(fromEngine, qPrintable(c.errorString()));

    QCOMPARE(created.hello(), QStringLiteral("Hello from parent"));

    QQmlListReference ref(&created, "children");
    QCOMPARE(ref.count(), 2);
    QList<QObject *> children = { ref.at(0), ref.at(1) };
    for (auto &child : children) {
        QVERIFY(child);
    }
    QCOMPARE(children.at(0)->property("hello").toString(),
             QStringLiteral("Hello from parent.children[0]"));
    QCOMPARE(children.at(1)->property("hello").toString(),
             QStringLiteral("Hello from parent.children[1]"));

    QQmlListReference refIds(&created, "ids");
    QCOMPARE(refIds.count(), 3);
    QCOMPARE(refIds.at(0), &created);
    QCOMPARE(refIds.at(1), ref.at(0));
    QCOMPARE(refIds.at(2), ref.at(1));

    QCOMPARE(fromEngine->property("firstCount"), 4);
    QCOMPARE(fromEngine->property("secondCount"), 5);

    QCOMPARE(created.firstCount(), 4);
    QCOMPARE(created.secondCount(), 5);

    QCOMPARE(created.childrenCount(), 2);
    QCOMPARE(created.childrenAt(0)->property("hello"), u"Hello from parent.children[0]"_s);
    QCOMPARE(created.childrenAt(1)->property("hello"), u"Hello from parent.children[1]"_s);

    created.childrenAppend(created.appendMe());
    QCOMPARE(created.childrenCount(), 3);
    QCOMPARE(created.childrenAt(0)->property("hello"), u"Hello from parent.children[0]"_s);
    QCOMPARE(created.childrenAt(1)->property("hello"), u"Hello from parent.children[1]"_s);
    QCOMPARE(created.childrenAt(2)->property("hello"), u"Hello from parent.children[2]"_s);

    created.childrenReplace(0, created.appendMe());
    QCOMPARE(created.childrenCount(), 3);
    QCOMPARE(created.childrenAt(0)->property("hello"), u"Hello from parent.children[2]"_s);
    QCOMPARE(created.childrenAt(1)->property("hello"), u"Hello from parent.children[1]"_s);
    QCOMPARE(created.childrenAt(2)->property("hello"), u"Hello from parent.children[2]"_s);

    created.childrenRemoveLast();
    QCOMPARE(created.childrenCount(), 2);
    QCOMPARE(created.childrenAt(0)->property("hello"), u"Hello from parent.children[2]"_s);
    QCOMPARE(created.childrenAt(1)->property("hello"), u"Hello from parent.children[1]"_s);

    created.childrenClear();
    QCOMPARE(created.childrenCount(), 0);
}

void tst_qmltc::listPropertiesWithTheSameName()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(listPropertySameName) created(&e);
    QQmlListReference rootData(&created, "data");
    QCOMPARE(rootData.count(), 1);

    QObject *child = rootData.at(0);
    QVERIFY(child);
    QCOMPARE(child->property("what").toString(), QStringLiteral("child"));
    QQmlListReference childData(child, "data");
    QCOMPARE(childData.count(), 1);

    QObject *childChild = childData.at(0);
    QVERIFY(childChild);
    QCOMPARE(childChild->property("what").toString(), QStringLiteral("child.child"));
}

void tst_qmltc::defaultProperty()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(defaultProperty) created(&e);

    QCOMPARE(created.hello(), QStringLiteral("Hello from parent"));

    QObject *firstLevelChild = created.child();
    QVERIFY(firstLevelChild);
    QCOMPARE(firstLevelChild->property("hello").toString(),
             QStringLiteral("Hello from parent.child"));
    QVERIFY(qobject_cast<PREPEND_NAMESPACE(defaultProperty_DefaultPropertyManyChildren) *>(firstLevelChild));

    QQmlListReference ref(firstLevelChild, "children");
    QCOMPARE(ref.count(), 2);
    QList<QObject *> secondLevelChildren = { ref.at(0), ref.at(1) };
    for (auto &child : secondLevelChildren) {
        QVERIFY(child);
    }
    QCOMPARE(secondLevelChildren.at(0)->property("hello").toString(),
             QStringLiteral("Hello from parent.child.children[0]"));
    QCOMPARE(secondLevelChildren.at(1)->property("hello").toString(),
             QStringLiteral("Hello from parent.child.children[1]"));
}

void tst_qmltc::defaultPropertyCorrectSelection()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(defaultPropertyCorrectSelection) created(&e);
    QQmlListReference children(&created, "data");
    QCOMPARE(children.count(), 1);
    QCOMPARE(created.unused(), nullptr);
}

void tst_qmltc::defaultAlias()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(defaultAlias) created(&e);

    QQmlComponent c(&e);
    c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/defaultAlias.qml"));
    QScopedPointer<QObject> fromEngine(c.create());
    QVERIFY2(fromEngine, qPrintable(c.errorString()));

    auto *child = static_cast<PREPEND_NAMESPACE(defaultAlias_QtObject) *>(created.child());
    QVERIFY(fromEngine->property("child").canConvert<QObject *>());
    QObject *childFromEngine = fromEngine->property("child").value<QObject *>();
    QVERIFY(childFromEngine);
    QCOMPARE(child->hello(), childFromEngine->property("hello"));
}

void tst_qmltc::attachedProperty()
{
    QScopeGuard exitTest([oldCount = TestTypeAttached::creationCount]() {
        QCOMPARE(TestTypeAttached::creationCount, oldCount + 1);
    });
    Q_UNUSED(exitTest);

    QQmlEngine e;
    PREPEND_NAMESPACE(AttachedProperty) created(&e);

    TestTypeAttached *attached = qobject_cast<TestTypeAttached *>(
            qmlAttachedPropertiesObject<TestType>(&created, false));
    QVERIFY(attached);

    QCOMPARE(attached->getAttachedCount(), 42);
    QCOMPARE(attached->getAttachedFormula(), 42);
    QVERIFY(attached->getAttachedObject());
    QCOMPARE(attached->getAttachedObject()->property("name").toString(),
             QStringLiteral(u"root.TestType.attachedObject"));

    // sanity
    QCOMPARE(created.myCount(), attached->getAttachedCount());
    QCOMPARE(created.myTriggerFired(), false);

    emit created.myTriggered();
    QCOMPARE(created.myTriggerFired(), true);
    QCOMPARE(attached->getAttachedCount(), 43);
    QCOMPARE(created.myCount(), attached->getAttachedCount());
    QCOMPARE(attached->getAttachedFormula(), 42 * 2);

    created.setMyTriggerFired(false);
    emit attached->triggered();
    QCOMPARE(created.myTriggerFired(), true);
    QCOMPARE(attached->getAttachedCount(), 44);
    QCOMPARE(created.myCount(), attached->getAttachedCount());
    QCOMPARE(attached->getAttachedFormula(), 42 * 2 * 2);

    emit created.updateAttachedCount();
    QCOMPARE(attached->getAttachedCount(), 45);
    QCOMPARE(created.myCount(), attached->getAttachedCount());
    QCOMPARE(attached->getAttachedFormula(), 42 * 2 * 2 * 2);
}

void tst_qmltc::attachedPropertyObjectCreatedOnce()
{
    QScopeGuard exitTest([oldCount = TestTypeAttached::creationCount]() {
        QCOMPARE(TestTypeAttached::creationCount, oldCount + 1);
    });
    Q_UNUSED(exitTest);

    QQmlEngine e;
    PREPEND_NAMESPACE(attachedPropertyDerived) created(&e);

    TestTypeAttached *attached = qobject_cast<TestTypeAttached *>(
            qmlAttachedPropertiesObject<TestType>(&created, false));
    QVERIFY(attached);

    QCOMPARE(attached->getAttachedCount(), -314);
}

void tst_qmltc::groupedProperty()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(groupedProperty) created(&e);

    TestTypeGrouped *grouped = created.getGroup();
    QVERIFY(grouped);

    int unspecifiedValue = 42; // could be 42 or 84

    QCOMPARE(grouped->getCount(), 42);
    QCOMPARE(grouped->getFormula(), unspecifiedValue); // TODO: this is unspecified
    QVERIFY(grouped->getObject());
    QCOMPARE(grouped->getObject()->property("name").toString(),
             QStringLiteral(u"root.group.object"));

    // sanity
    QCOMPARE(created.myCount(), grouped->getCount());
    QCOMPARE(created.myTriggerFired(), false);

    const int initialFormulaValue = grouped->getFormula();

    emit created.myTriggered();
    QCOMPARE(created.myTriggerFired(), true);
    QCOMPARE(grouped->getCount(), 43);
    QCOMPARE(created.myCount(), grouped->getCount());
    QCOMPARE(grouped->getFormula(), initialFormulaValue * 2);

    created.setMyTriggerFired(false);
    emit grouped->triggered();
    QCOMPARE(created.myTriggerFired(), true);
    QCOMPARE(grouped->getCount(), 44);
    QCOMPARE(created.myCount(), grouped->getCount());
    QCOMPARE(grouped->getFormula(), initialFormulaValue * 2 * 2);

    emit created.updateCount();
    QCOMPARE(grouped->getCount(), 45);
    QCOMPARE(created.myCount(), grouped->getCount());
    QCOMPARE(grouped->getFormula(), initialFormulaValue * 2 * 2 * 2);
}

// TODO: add a test for private property which is a value type
// TODO: what about signals of private properties?
void tst_qmltc::groupedProperty_qquicktext()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(groupedProperty_qquicktext) created(&e);

    QQuickAnchors *anchors = QQuickItemPrivate::get(&created)->anchors();
    QVERIFY(anchors);

    QCOMPARE(anchors->alignWhenCentered(), false);
    QCOMPARE(anchors->topMargin(), qreal(1));
    QCOMPARE(anchors->bottomMargin(), qreal(42));

    QFont font = created.font();
    QCOMPARE(font.family(), u"Helvetica"_s);
    QCOMPARE(font.pointSize(), 4);
    QCOMPARE(font.letterSpacing(), 3);

    QQmlListReference ref(&created, "data");
    QCOMPARE(ref.count(), 1);
    QQuickItem *child = qobject_cast<QQuickItem *>(ref.at(0));
    QVERIFY(child);
    QQuickAnchors *childAnchors = QQuickItemPrivate::get(child)->anchors();
    QVERIFY(childAnchors);
    QCOMPARE(childAnchors->topMargin(), qreal(1));
    QCOMPARE(childAnchors->bottomMargin(), qreal(42));

    anchors->setTopMargin(2);
    anchors->setBottomMargin(11);
    QCOMPARE(childAnchors->topMargin(), qreal(2));
    QCOMPARE(childAnchors->bottomMargin(), qreal(11));
}

template<typename T>
void localImport_impl(T &created)
{
    QCOMPARE(created.baseMessage(), QStringLiteral(u"base"));
    QCOMPARE(created.count(), 1);
    QCOMPARE(created.derivedMessage(), QStringLiteral(u"derived"));
    QCOMPARE(created.message(), QStringLiteral(u"derived.message"));

    QVERIFY(created.baseObject());
    QQuickText *baseObject = qobject_cast<QQuickText *>(created.baseObject());
    QVERIFY(baseObject);
    QCOMPARE(baseObject->text(), QStringLiteral(u"derived.baseObject"));

    QQmlListReference ref(&created, "baseDefaultList");
    QCOMPARE(ref.count(), 3);
    QList<QObject *> children = { ref.at(0), ref.at(1), ref.at(2) };
    for (auto &child : children) {
        QVERIFY(child);
    }

    QVERIFY(qobject_cast<QQuickText *>(children.at(0)));
    QVERIFY(qobject_cast<QQuickRectangle *>(children.at(1)));
    QVERIFY(qobject_cast<PREPEND_NAMESPACE(HelloWorld) *>(children.at(2)));

    QCOMPARE(children.at(0)->property("text").toString(), QStringLiteral("derived.child[0]"));
    QCOMPARE(children.at(1)->property("text").toString(), QStringLiteral("derived.child[1]"));
    QCOMPARE(children.at(2)->property("text").toString(), QStringLiteral("derived.child[2]"));
    QCOMPARE(children.at(2)->property("hello").toString(), QStringLiteral("Hello, World"));
}

void tst_qmltc::localImport()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(localImport) created(&e);
    localImport_impl(created);
}

void tst_qmltc::explicitLocalImport()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(localImport_explicit) created(&e);
    localImport_impl(created);
}

void tst_qmltc::newPropertyBoundToOld()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(newPropertyBoundToOld) created(&e);

    // sanity
    QCOMPARE(created.width(), 10);
    QCOMPARE(created.widthComponent(), 10);
    QCOMPARE(created.newWidth(), created.width() + created.widthComponent());

    // update new style property first to see if this works
    created.setWidthComponent(11);
    QCOMPARE(created.widthComponent(), 11);
    QCOMPARE(created.width(), 10);
    QCOMPARE(created.newWidth(), 21);

    // update old style property afterwards to see if this also works
    created.setWidth(89);
    QCOMPARE(created.width(), 89);
    QCOMPARE(created.widthComponent(), 11);
    QCOMPARE(created.newWidth(), 100);
}

void tst_qmltc::oldPropertyBoundToNew()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(oldPropertyBoundToNew) created(&e);

    // sanity
    QCOMPARE(created.width(), 10);
    QCOMPARE(created.heightComponent(), 10);
    QCOMPARE(created.height(), created.width() + created.heightComponent());

    // update old style property first to see if this works
    created.setWidth(11);
    QCOMPARE(created.width(), 11);
    QCOMPARE(created.heightComponent(), 10);
    QCOMPARE(created.height(), 21);

    // update new style property afterwards to see if this also works
    created.setHeightComponent(89);
    QCOMPARE(created.heightComponent(), 89);
    QCOMPARE(created.width(), 11);
    QCOMPARE(created.height(), 100);
}

void tst_qmltc::nonLocalQmlPropertyBoundToAny()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(nonLocalQmlPropertyBoundToAny) created(&e);

    // sanity
    QCOMPARE(created.width(), 10);
    QCOMPARE(created.newWidth(), 10);
    QCOMPARE(created.message(), QStringLiteral("width=20"));

    // update old style property
    created.setWidth(11);
    QCOMPARE(created.width(), 11);
    QCOMPARE(created.newWidth(), 10);
    QCOMPARE(created.message(), QStringLiteral("width=21"));

    // update new style property
    created.setNewWidth(89);
    QCOMPARE(created.newWidth(), 89);
    QCOMPARE(created.width(), 11);
    QCOMPARE(created.message(), QStringLiteral("width=100"));
}

void tst_qmltc::localImportWithOnCompleted()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(localDerived) created(&e);

    // sanity
    QCOMPARE(created.count(), 1); // from base type
    QCOMPARE(created.p1(), 323);
    QCOMPARE(created.p2(), created.p1() + 10);

    created.setP1(32);
    QCOMPARE(created.p1(), 32);
    QCOMPARE(created.p2(), created.p1() + 10);
}

void tst_qmltc::justAnimation()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(justAnimation) created(&e);

    // sanity
    QCOMPARE(created.width(), 0);

    created.setWidth(500);
    QTRY_VERIFY(created.width() != 0);
    QTRY_VERIFY(created.width() == 500);
}

void tst_qmltc::justAnimationOnAlias()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(justAnimationOnAlias) created(&e);

    // sanity
    QCOMPARE(created.width(), 100);
    QCOMPARE(created.widthAlias(), 100);

    // through "real" property
    created.setWidth(500);
    QTRY_VERIFY(created.widthAlias() != 100);
    QTRY_VERIFY(created.widthAlias() == 500);
    QCOMPARE(created.widthAlias(), 500);

    // through alias
    created.setWidthAlias(10);
    QTRY_VERIFY(created.widthAlias() != 500);
    QTRY_VERIFY(created.widthAlias() == 10);
}

void tst_qmltc::behaviorAndAnimation()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(behaviorAndAnimation) created(&e);

    // sanity
    QCOMPARE(created.width(), 100);

    created.setWidth(500);
    QTRY_VERIFY(created.width() != 100);
    QTRY_VERIFY(created.width() == 500);
}

void tst_qmltc::behaviorAndAnimationOnAlias()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(behaviorAndAnimationOnAlias) created(&e);

    // sanity
    QCOMPARE(created.width(), 100);
    QCOMPARE(created.widthAlias(), 100);

    // through "real" property
    created.setWidth(500);
    QTRY_VERIFY(created.widthAlias() != 100);
    QTRY_VERIFY(created.widthAlias() == 500);
    QCOMPARE(created.widthAlias(), 500);

    // through alias
    created.setWidthAlias(10);
    QTRY_VERIFY(created.widthAlias() != 500);
    QTRY_VERIFY(created.widthAlias() == 10);
}

void tst_qmltc::bindingsThroughIds()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(bindingsThroughIds) created(&e);

    QCOMPARE(created.text(), QStringLiteral("theText.text"));
    QCOMPARE(created.width(), 42);

    QQmlContext *mainContext = e.contextForObject(&created);
    QVERIFY(mainContext);
    QCOMPARE(mainContext->objectForName(QStringLiteral("theItem")), &created);

    QList<QObject *> children = created.children();
    QCOMPARE(children.size(), 2);
    QQmlContext *theTextContext = e.contextForObject(children.at(0));
    QCOMPARE(mainContext, theTextContext);
    QCOMPARE(theTextContext->objectForName(QStringLiteral("theText")), children.at(0));

    QQmlContext *theRectContext = e.contextForObject(children.at(1));
    QCOMPARE(mainContext, theRectContext);
    QCOMPARE(theRectContext->objectForName(QStringLiteral("theRect")), children.at(1));

    QCOMPARE(children.at(0)->property("color"), children.at(1)->property("color"));
}

void tst_qmltc::contextHierarchy_rootBaseIsQml()
{
    {
        QQmlEngine e;
        QQmlComponent c(&e);
        c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/localImport_context.qml"));
        QScopedPointer<QObject> root(c.create());
        QVERIFY2(root, qPrintable(c.errorString()));
        // sanity
        QCOMPARE(root->property("count").toInt(), 43);
    }

    QQmlEngine e;
    PREPEND_NAMESPACE(localImport_context) created(&e);
    QCOMPARE(created.p1(), 41);
    // NB: this is super weird, but count would indeed be 43 and not (p1 + 1)
    // due to parent's Component.onCompleted being called after the binding
    // setup in the child class - same happens in QQmlComponent case
    QCOMPARE(created.count(), 43);

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
                 "localImport, same stays true for context",
                 Continue);
    QCOMPARE(QQmlContextData::get(ctx)->parent(), QQmlContextData::get(e.rootContext()));
    QCOMPARE(QQmlContextData::get(ctx)->parent()->parent(), QQmlContextData::get(e.rootContext()));

    int count = created.count();
    QCOMPARE(created.getMagicValue().toInt(), (count * 3 + 1));
    count = created.count();
    QCOMPARE(created.localGetMagicValue().toInt(), (count * 3 + 1));
}

void tst_qmltc::contextHierarchy_childBaseIsQml()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(neighbors_context) created(&e);
    QQmlListReference children(&created, "data");
    QCOMPARE(children.size(), 2);
    auto *child1 = qobject_cast<PREPEND_NAMESPACE(neighbors_context_QtObject) *>(children.at(0));
    auto child2 = qobject_cast<PREPEND_NAMESPACE(neighbors_context_LocallyImported_context) *>(children.at(1));
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

    QCOMPARE(child1->p(), 41);
    QCOMPARE(child1->p2(), child2->count() * 2);
    QCOMPARE(child2->p(), child1->p() + 1);

    child1->setP(44);
    QCOMPARE(child2->p(), 45);

    child2->setCount(4);
    QCOMPARE(child1->p2(), 8);

    int count = child2->count();
    QVariant magicValue {};
    QMetaObject::invokeMethod(child2, "getMagicValue", Q_RETURN_ARG(QVariant, magicValue));
    QCOMPARE(magicValue.toInt(), (count * 3 + 1));
}

void tst_qmltc::contextHierarchy_delegate()
{
    // TODO: not everything works (if the QML file is changed, so QQmlComponent
    // here is useful)
    {
        QQmlEngine e;
        QQmlComponent c(&e);
        c.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/delegate_context.qml"));
        QScopedPointer<QObject> root(c.create());
        QVERIFY2(root, qPrintable(c.errorString()));
        QQmlListReference data(root.get(), "data");
        QCOMPARE(data.count(), 1);
        auto listView = qobject_cast<QQuickListView *>(data.at(0));
        QVERIFY(listView);

        QPointer<QObject> delegate = listView->currentItem();
        QVERIFY(delegate);
        QCOMPARE(delegate->property("text").toString(), QStringLiteral("hello delegate"));
        QQmlListReference dataOfDelegate(delegate.get(), "data");
        QCOMPARE(dataOfDelegate.count(), 1);
        QVERIFY(dataOfDelegate.at(0));
        QCOMPARE(dataOfDelegate.at(0)->property("text").toString(),
                 QStringLiteral("hello delegate text"));
    }

    QQmlEngine e;
    PREPEND_NAMESPACE(delegate_context) created(&e);
    QQmlListReference data(&created, "data");
    QCOMPARE(data.count(), 1);
    auto listView = qobject_cast<PREPEND_NAMESPACE(delegate_context_ListView) *>(data.at(0));
    QVERIFY(listView);
    QCOMPARE(created.text(), QStringLiteral("hello"));
    QVERIFY(listView->delegate());

    QPointer<QObject> delegate = listView->currentItem();
    QVERIFY(delegate);
    QCOMPARE(delegate->property("text").toString(), QStringLiteral("hello delegate"));
    QQmlListReference dataOfDelegate(delegate.get(), "data");
    QCOMPARE(dataOfDelegate.count(), 1);
    QVERIFY(dataOfDelegate.at(0));
    QCOMPARE(dataOfDelegate.at(0)->property("text").toString(),
             QStringLiteral("hello delegate text"));
}

void tst_qmltc::contextHierarchy_nontrivial()
{
    // NB: test that we don't crash
    QQmlEngine e;
    PREPEND_NAMESPACE(nontrivial_context) created(&e);
    Q_UNUSED(created);
    QQmlListReference children(&created, "data");
    QCOMPARE(children.count(), 3);

    auto imported = qobject_cast<PREPEND_NAMESPACE(nontrivial_context_LocallyImported) *>(children.at(2));
    QVERIFY(imported);
    QQmlListReference childrenOfImported(imported, "baseDefaultList");
    QCOMPARE(childrenOfImported.count(), 2);
    QVERIFY(childrenOfImported.at(0));
    QCOMPARE(childrenOfImported.at(0)->property("text").toString(), QStringLiteral("hello, world"));

    auto mouseArea = qobject_cast<PREPEND_NAMESPACE(nontrivial_context_MouseArea) *>(children.at(1));
    QVERIFY(mouseArea);
    emit mouseArea->clicked(nullptr);
    QCOMPARE(childrenOfImported.at(0)->property("text").toString(), QStringLiteral("clicked"));

    auto helloWorld = qobject_cast<PREPEND_NAMESPACE(HelloWorld) *>(childrenOfImported.at(1));
    QVERIFY(helloWorld);
    QCOMPARE(helloWorld->greeting(), QStringLiteral("Hello, World!"));
}

void tst_qmltc::javascriptImport()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(javascriptCaller) created(&e);
    QVERIFY(!created.valueIsBad());
    QVERIFY(created.valueIsGood());
}

void tst_qmltc::listView()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Flaky on QEMU. Sometimes can't correctly run JavaScript code, QTBUG-99355");

    QQmlEngine e;
    PREPEND_NAMESPACE(listView) created(&e);
    QQmlListReference children(&created, "data");
    QCOMPARE(children.count(), 1);
    auto *view = qobject_cast<PREPEND_NAMESPACE(listView_ListView) *>(children.at(0));
    QVERIFY(view);
    QQmlListModel *model = qvariant_cast<QQmlListModel *>(view->model());
    QVERIFY(model);
    QCOMPARE(model->count(), 0);

    created.appendDigit("5");
    QCOMPARE(model->count(), 1);

    created.appendOperator("+");
    QCOMPARE(model->count(), 2);

    // TODO: add more testing (e.g. check that values are actually recorded)
}

void tst_qmltc::bindingOnValueType()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(bindingOnValueType) created(&e);
    QQmlListReference children(&created, "data");
    QCOMPARE(children.count(), 1);

    PREPEND_NAMESPACE(bindingOnValueType_QtObject) *subItem =
            qobject_cast<PREPEND_NAMESPACE(bindingOnValueType_QtObject) *>(children.at(0));
    QVERIFY(subItem);
    QCOMPARE(subItem->value(), 1);

    QCOMPARE(created.font().pixelSize(), 2 * subItem->value());
    subItem->setValue(3);
    QCOMPARE(created.font().pixelSize(), 2 * subItem->value());
}

void tst_qmltc::keyEvents()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(keyEvents) created(&e);

    QCOMPARE(created.k(), QStringLiteral(""));

    QKeyEvent keyPlusEvent(QEvent::KeyPress, Qt::Key_Plus, Qt::NoModifier);
    QCoreApplication::sendEvent(&created, &keyPlusEvent);
    QCOMPARE(created.k(), QStringLiteral("+"));

    QKeyEvent keySlashEvent(QEvent::KeyPress, Qt::Key_Slash, Qt::NoModifier);
    QCoreApplication::sendEvent(&created, &keySlashEvent);
    QCOMPARE(created.k(), QStringLiteral("/"));

    QKeyEvent keyUnknownhEvent(QEvent::KeyPress, Qt::Key_Comma, Qt::NoModifier);
    QCoreApplication::sendEvent(&created, &keyUnknownhEvent);
    QCOMPARE(created.k(), QStringLiteral("unknown"));

    // repeat for postEvents
    QCoreApplication::postEvent(&created,
                                new QKeyEvent(QEvent::KeyPress, Qt::Key_Plus, Qt::NoModifier));
    QCoreApplication::sendPostedEvents(&created);
    QCOMPARE(created.k(), QStringLiteral("+"));

    QCoreApplication::postEvent(&created,
                                new QKeyEvent(QEvent::KeyPress, Qt::Key_F33, Qt::NoModifier));
    QCoreApplication::sendPostedEvents(&created);
    QCOMPARE(created.k(), QStringLiteral("unknown"));
}

void tst_qmltc::privateProperties()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(privatePropertySubclass) created(&e);
    QCOMPARE(created.dummy(), u"bar"_s);
    QCOMPARE(created.strAlias(), u"foobar"_s);
    QCOMPARE(created.smthAlias(), 42);

    auto privateCreated = static_cast<PrivatePropertyTypePrivate *>(QObjectPrivate::get(&created));
    QVERIFY(privateCreated);
    QCOMPARE(privateCreated->foo(), u"Smth is: 42"_s);

    ValueTypeGroup vt = privateCreated->vt();
    QCOMPARE(vt.count(), 11);

    TestTypeGrouped *group = privateCreated->getGroup();
    QCOMPARE(group->getCount(), 43);
    QCOMPARE(group->getStr(), created.strAlias());
}

void tst_qmltc::calqlatrBits()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(calqlatrBits) created(&e);

    QQmlListReference children(&created, "data");
    QCOMPARE(children.count(), 2);
    auto context = e.contextForObject(&created);
    QQuickText *textItem = qobject_cast<QQuickText *>(context->objectForName("textItem"));
    QVERIFY(textItem);
    // NB: not at index 0 due to AnimationController and strange logic of
    // appending items (when these items are not QQuickItem based?)
    QCOMPARE(children.at(1), textItem);

    QCOMPARE(textItem->wrapMode(), QQuickText::WrapAnywhere);

    QQuickAnimationController *controller =
            qobject_cast<QQuickAnimationController *>(context->objectForName("controller"));
    QVERIFY(controller);
    QCOMPARE(children.at(0), controller);
    QQuickNumberAnimation *anim = qobject_cast<QQuickNumberAnimation *>(controller->animation());
    // sanity
    QVERIFY(anim);
    QCOMPARE(anim->target(), textItem);
    QCOMPARE(anim->property(), u"scale"_s);
    QCOMPARE(anim->duration(), 50);
    QCOMPARE(anim->from(), 1);
    QCOMPARE(anim->to(), 0.5);
    QVERIFY(!anim->isRunning());
    QCOMPARE(anim->easing().type(), QEasingCurve::InOutQuad);

    controller->completeToEnd();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

    QSignalSpy scaleChangedSpy(textItem, &QQuickItem::scaleChanged);
    controller->completeToBeginning();
    QTRY_VERIFY(scaleChangedSpy.size() > 0);
}

void tst_qmltc::trickyPropertyChangeAndSignalHandlers()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(propertyChangeAndSignalHandlers) created(&e);

    // sanity
    QCOMPARE(created.aChangedCount1(), 0);
    QCOMPARE(created.bChangedCount1(), 0);
    QCOMPARE(created.cChangedCount1(), 0);
    QCOMPARE(created.dChangedCount1(), 0);
    QCOMPARE(created.cChangedCount2(), 0);
    QCOMPARE(created.dChangedCount2(), 0);
    QCOMPARE(created.cChangedCount3(), 0);
    QCOMPARE(created.dChangedCount3(), 0);
    QCOMPARE(created.dChangedStr3(), QString());
    QCOMPARE(created.cChangedCount4(), 0);
    QCOMPARE(created.dChangedCount4(), 0);
    QCOMPARE(created.dChangedStr4(), QString());

    QQmlContext *ctx = e.contextForObject(&created);
    QVERIFY(ctx);
    TypeWithProperties *one = qobject_cast<TypeWithProperties *>(ctx->objectForName("one"));
    QVERIFY(one);
    TypeWithProperties *two = qobject_cast<TypeWithProperties *>(ctx->objectForName("two"));
    QVERIFY(two);
    TypeWithProperties *three = qobject_cast<TypeWithProperties *>(ctx->objectForName("three"));
    QVERIFY(three);
    TypeWithProperties *four = qobject_cast<TypeWithProperties *>(ctx->objectForName("four"));
    QVERIFY(four);

    one->setA(10);
    QCOMPARE(created.aChangedCount1(), 1);
    one->setB("1");
    QCOMPARE(created.bChangedCount1(), 1);
    one->setC(2.5);
    QCOMPARE(created.cChangedCount1(), 1);
    one->setD(-10);
    QCOMPARE(created.dChangedCount1(), 1);
    two->setC(44.5);
    QCOMPARE(created.cChangedCount2(), 1);
    three->setC(42.0);
    QCOMPARE(created.cChangedCount3(), 42);
    three->setD(10);
    QCOMPARE(created.dChangedCount3(), 10);
    QCOMPARE(created.dChangedStr3(), u"d changed"_s);
    four->setC(1.5);
    QCOMPARE(created.cChangedCount4(), 2); // cChangedCount4 is int, so 0.5 is truncated
    four->setD(84);
    // Note: due to signal-over-property-change-handler preference, we bind to
    // signal in the case when the property is both bindable and notifiable. in
    // this test, it would mean that we get proper dChanged*4 values intead of
    // `undefined` junk
    QCOMPARE(created.dChangedCount4(), 42);
    QCOMPARE(created.dChangedStr4(), u"d changed!"_s);

    created.changeProperties1();
    QCOMPARE(created.aChangedCount1(), 2);
    QCOMPARE(created.bChangedCount1(), 2);
    QCOMPARE(created.cChangedCount1(), 2);

    created.changeProperties2();
    QCOMPARE(created.cChangedCount2(), 2);

    created.changeProperties3(22);
    QCOMPARE(created.cChangedCount3(), 22);

    TypeWithProperties *five = qobject_cast<TypeWithProperties *>(ctx->objectForName("five"));
    QVERIFY(five);
    const Qt::MouseButtons a = Qt::RightButton | Qt::MiddleButton;
    const Qt::MouseButton b = Qt::LeftButton;
    emit five->signalWithEnum(a, b);

    QCOMPARE(Qt::MouseButton(five->property("mouseButtonA").toInt()), a);
    QCOMPARE(Qt::MouseButtons(five->property("mouseButtonB").toInt()), b);
}

void tst_qmltc::valueTypeListProperty()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(valueTypeListProperty) created(&e);
    QList<int> intsRef = created.arrayOfInts();
    QCOMPARE(intsRef.size(), 4);
    QList<int> intsGroundTruth = { 1, 0, 42, -5 };

    QCOMPARE(intsRef, intsGroundTruth);

    QCOMPARE(created.arrayOfInts().at(2), 42);
    created.incrementPlease();
    QCOMPARE(created.arrayOfInts().at(2), 43);

    QList<QFont> arrayOfFonts = created.arrayOfFonts();
    QCOMPARE(arrayOfFonts.size(), 2);
    QCOMPARE(arrayOfFonts[0].family(), "Arial");
    QCOMPARE(arrayOfFonts[1].family(), "Comic Sans");

    QList<QColor> arrayOfColors = created.arrayOfColors();

    QCOMPARE(arrayOfColors.size(), 3);

    QVERIFY(arrayOfColors[0].red() > 0);
    QCOMPARE(arrayOfColors[0].green(), 0);
    QCOMPARE(arrayOfColors[0].blue(), 0);

    QCOMPARE(arrayOfColors[1].red(), 0);
    QVERIFY(arrayOfColors[1].green() > 0);
    QCOMPARE(arrayOfColors[1].blue(), 0);

    QCOMPARE(arrayOfColors[2].red(), 0);
    QCOMPARE(arrayOfColors[2].green(), 0);
    QVERIFY(arrayOfColors[2].blue() > 0);

    QQmlListReference arrayOfInts2(&created, "arrayOfInts");
    QVERIFY(!arrayOfInts2.isValid());

    QQmlListReference arrayOfFonts2(&created, "arrayOfFonts");
    QVERIFY(!arrayOfFonts2.isValid());

    QQmlListReference arrayOfColors2(&created, "arrayOfColors");
    QVERIFY(!arrayOfColors2.isValid());
}

void tst_qmltc::translations()
{
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(translations) created(&e);

        QCOMPARE(created.alsoTranslated(), "Bye bye!");
        QCOMPARE(created.text(), "Bye bye!");

        QCOMPARE(created.hardcodedContext(), "Bye bye!");
        QCOMPARE(created.anotherContext(), "Bye bye!");
        QCOMPARE(created.toBeTranslatedLater(), "Bye bye!");
        QCOMPARE(created.toBeTranslatedLaterWithHardcodedContext(), "Bye bye!");

        QCOMPARE(created.translatedN(), "The solution is 42");
        QCOMPARE(created.translatedNWithContextAndAmbiguation(), "The solution has 42 degrees");
        QCOMPARE(created.translatedNWithContextAndAmbiguation2(), "The solution has 43 degrees");

        QCOMPARE(created.combination(), "Bye bye!");

        QTranslator translator;
        QVERIFY(translator.load("translations_ge.qm", ":/i18n"));
        QVERIFY(qApp->installTranslator(&translator));
        e.retranslate();

        QCOMPARE(created.text(), u"Tschüssi!"_s);
        QCOMPARE(created.alsoTranslated(), u"Tschüssi!"_s);

        QCOMPARE(created.hardcodedContext(), u"Tschüssi!"_s);
        QCOMPARE(created.anotherContext(), u"Bis später!"_s);
        QCOMPARE(created.toBeTranslatedLater(), "Bye bye!");
        QCOMPARE(created.toBeTranslatedLaterWithHardcodedContext(), "Bye bye!");

        QCOMPARE(created.translatedN(), u"Die Lösung ist 42"_s);
        QCOMPARE(created.translatedNWithContextAndAmbiguation(),
                 u"Die Lösung des Problems ist 42 grad."_s);
        QCOMPARE(created.translatedNWithContextAndAmbiguation2(), u"Die Lösung ist 43 grad warm"_s);

        QCOMPARE(created.combination(), u"Tschüssi!"_s);
    }
    {
        QQmlEngine e;
        PREPEND_NAMESPACE(translationsById) created(&e);

        QCOMPARE(created.alsoTranslated(), "ID1");
        QCOMPARE(created.text(), "ID1");

        QCOMPARE(created.toBeTranslatedLater(), "ID1");
        QCOMPARE(created.translatedN(), "ID2");

        QTranslator translator;
        QVERIFY(translator.load("translationsById_ge.qm", ":/i18n"));
        QVERIFY(qApp->installTranslator(&translator));
        e.retranslate();

        QCOMPARE(created.text(), u"Tschüssi!"_s);
        QCOMPARE(created.alsoTranslated(), u"Tschüssi!"_s);

        QCOMPARE(created.toBeTranslatedLater(), "ID1");
        QCOMPARE(created.translatedN(), u"Ich sehe 5 Äpfeln"_s);
    }
}

void tst_qmltc::repeaterCrash()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(repeaterCrash) fromQmltc(&e);

    QQmlComponent component(&e, "qrc:/qt/qml/QmltcTests/repeaterCrash.qml");
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QQuickItem> fromEngine(qobject_cast<QQuickItem *>(component.create()));
    QVERIFY(fromEngine);

    const int size = 7;

    const auto listFromEngine = fromEngine->childItems();
    const auto listFromQmltc = fromQmltc.childItems();

    QCOMPARE(listFromEngine.size(), size);
    QCOMPARE(listFromQmltc.size(), size);

    for (int i = 0; i < size; i++) {
        // the repeater itself has no objName property
        if (i == 5)
            continue;

        const QVariant nameFromEngine = listFromEngine.at(i)->property("objName");
        const QVariant nameFromQmltc = listFromQmltc.at(i)->property("objName");

        QVERIFY(nameFromEngine.isValid());
        QVERIFY(nameFromQmltc.isValid());
        QCOMPARE(nameFromQmltc.toString(), nameFromEngine.toString());
    }
}

void tst_qmltc::generalizedGroupedProperty()
{
    QQmlEngine e;
    {
        PREPEND_NAMESPACE(generalizedGroupedProperty) fromQmltc(&e);

        QCOMPARE(fromQmltc.getGroup()->getCount(), 5);
        fromQmltc.setMyInt(42);
        QCOMPARE(fromQmltc.getGroup()->getCount(), 42);
        fromQmltc.getGroup()->setCount(55);
        QCOMPARE(fromQmltc.getGroup()->getCount(), 55);
        QCOMPARE(fromQmltc.myInt(), 42);

        QCOMPARE(fromQmltc.getGroup()->getFormula(), 8);
        QCOMPARE(fromQmltc.getGroup()->getStr(), "Hello World!");

        qmlExecuteDeferred(&fromQmltc);

        QCOMPARE(fromQmltc.getGroup()->getCount(), 55);
        QCOMPARE(fromQmltc.getGroup()->getFormula(), 8);
        QCOMPARE(fromQmltc.getGroup()->getStr(), "Hello World!");
    }
    {
        QQmlComponent component(&e, "qrc:/qt/qml/QmltcTests/generalizedGroupedProperty.qml");
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> fromEngine(component.create());
        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("count"), 5);
        fromEngine->setProperty("myInt", 43);
        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("count"), 43);
        fromEngine->property("group").value<QObject *>()->setProperty("count", 56);
        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("count"), 56);
        QCOMPARE(fromEngine->property("myInt").value<int>(), 43);

        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("formula").value<int>(),
                 8);
        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("str").toString(),
                 "Hello World!");

        qmlExecuteDeferred(fromEngine.data());

        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("count"), 56);
        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("formula").value<int>(),
                 8);
        QCOMPARE(fromEngine->property("group").value<QObject *>()->property("str").toString(),
                 "Hello World!");
    }
}

void tst_qmltc::appendToQQmlListProperty()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(appendToQQmlListProperty) fromQmltc(&e);
    QQmlComponent component(&e, "qrc:/qt/qml/QmltcTests/appendToQQmlListProperty.qml");
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> fromEngine(component.create());

    auto itemFromEngine = fromEngine->property("myItem").value<QQuickItem *>();
    auto itemFromQmltc = fromQmltc.myItem();

    QCOMPARE(itemFromEngine->children().size(), 3);
    QCOMPARE(itemFromQmltc->children().size(), 3);

    QCOMPARE(itemFromEngine->children().at(0)->property("hello"), u"hello1"_s);
    QCOMPARE(itemFromEngine->children().at(1)->property("hello"), u"hello2"_s);
    QCOMPARE(itemFromEngine->children().at(2)->property("hello"), u"I am a Rectangle."_s);

    QCOMPARE(itemFromQmltc->children().at(0)->property("hello"), u"hello1"_s);
    QCOMPARE(itemFromQmltc->children().at(1)->property("hello"), u"hello2"_s);
    QCOMPARE(itemFromQmltc->children().at(2)->property("hello"), u"I am a Rectangle."_s);

    QVariantList referenceComponentList = { QVariant(u"Hello"_s), QVariant(42), QVariant(4.0) };
    QCOMPARE(fromQmltc.myComponentList().toList(), referenceComponentList);
    QCOMPARE(fromEngine->property("myComponentList").toList(), referenceComponentList);

    QList<int> referenceValueTypeList = { 12489, 10, 42 };
    QVariantList referenceValueTypeList2 = { 12489, 10, 42 };
    QCOMPARE(fromQmltc.myValueTypeList().toList(), referenceValueTypeList);
    QCOMPARE(fromEngine->property("myValueTypeList").toList(), referenceValueTypeList2);

    QQmlListReference qtObjectsFromQmltc(&fromQmltc, "myQtObjectList");
    QVERIFY(qtObjectsFromQmltc.isValid());
    QQmlListReference qtObjectsFromEngine(fromEngine.data(), "myQtObjectList");
    QVERIFY(qtObjectsFromEngine.isValid());

    QCOMPARE(qtObjectsFromQmltc.size(), 3);
    QCOMPARE(qtObjectsFromEngine.size(), 3);

    QCOMPARE(qtObjectsFromQmltc.at(0)->property("hello"), u"Guten Morgen!"_s);
    QCOMPARE(qtObjectsFromQmltc.at(1)->property("hello"), u"I am a Rectangle."_s);
    QCOMPARE(qtObjectsFromQmltc.at(2)->property("hello"), u"Moin!"_s);

    QCOMPARE(qtObjectsFromEngine.at(0)->property("hello"), u"Guten Morgen!"_s);
    QCOMPARE(qtObjectsFromEngine.at(1)->property("hello"), u"I am a Rectangle."_s);
    QCOMPARE(qtObjectsFromEngine.at(2)->property("hello"), u"Moin!"_s);

    QQmlListReference qtHWFromQmltc(&fromQmltc, "myHelloWorldList");
    QQmlListReference qtHWFromEngine(fromEngine.data(), "myHelloWorldList");

    QCOMPARE(qtHWFromQmltc.size(), 3);
    QCOMPARE(qtHWFromEngine.size(), 3);

    QCOMPARE(qtHWFromQmltc.at(0)->property("hello"), u"Good morning1"_s);
    QCOMPARE(qtHWFromQmltc.at(1)->property("hello"), u"Good morning2"_s);
    QCOMPARE(qtHWFromQmltc.at(2)->property("hello"), u"Good morning3"_s);

    QCOMPARE(qtHWFromEngine.at(0)->property("hello"), u"Good morning1"_s);
    QCOMPARE(qtHWFromEngine.at(1)->property("hello"), u"Good morning2"_s);
    QCOMPARE(qtHWFromEngine.at(2)->property("hello"), u"Good morning3"_s);

    // make sure that extensions are handled correctly, as they require a slightly different code
    // path to be generated
    QQmlListReference extendedFromQmltc(fromQmltc.extended().value<QObject *>(), "myList");
    QVERIFY(extendedFromQmltc.isValid());
    QQmlListReference extendedFromEngine(fromEngine->property("extended").value<QObject *>(),
                                         "myList");
    QVERIFY(extendedFromEngine.isValid());
    QCOMPARE(extendedFromQmltc.size(), 3);
    QCOMPARE(extendedFromEngine.size(), 3);
}

// test classes to access protected member typecount
class myInlineComponentA : public PREPEND_NAMESPACE(inlineComponents_A)
{
    friend class tst_qmltc;
};
class myInlineComponentB : public PREPEND_NAMESPACE(inlineComponents_B)
{
    friend class tst_qmltc;
};
class myInlineComponentMyComponent : public PREPEND_NAMESPACE(inlineComponents_MyComponent)
{
    friend class tst_qmltc;
};
class myInlineComponentAPlus : public PREPEND_NAMESPACE(inlineComponents_APlus)
{
    friend class tst_qmltc;
};
class myInlineComponentAPlusPlus : public PREPEND_NAMESPACE(inlineComponents_APlusPlus)
{
    friend class tst_qmltc;
};

void tst_qmltc::inlineComponents()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(inlineComponents) createdByQmltc(&e);

    QQmlComponent component(&e);
    component.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/inlineComponents.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> createdByComponent(component.create());

    const QString testData = u"Major"_s;
    const QString testData2 = u"Tom"_s;

    // check typecounts of inline components
    QCOMPARE(myInlineComponentA::q_qmltc_typeCount(), 1u);
    QCOMPARE(myInlineComponentB::q_qmltc_typeCount(), 1u);
    QCOMPARE(myInlineComponentAPlus::q_qmltc_typeCount(), 2u);
    QCOMPARE(myInlineComponentAPlusPlus::q_qmltc_typeCount(), 3u);
    QCOMPARE(myInlineComponentMyComponent::q_qmltc_typeCount(), 2u);

    // test properties with component types, see if they are generated
    // and if they contain one of the properties of the component
    {
        auto firstFromQmltc = createdByQmltc.myMyComponent();
        auto secondFromQmltc = createdByQmltc.myMyComponentAsQtObject();

        QVERIFY(firstFromQmltc);
        QVERIFY(secondFromQmltc);
        QCOMPARE(firstFromQmltc->myX(), u"SharedX"_s);
        QCOMPARE(secondFromQmltc->property("myX"), u"SharedX"_s);
        QCOMPARE(createdByQmltc.signalTriggered(), 0);
        emit createdByQmltc.myMyComponentSignal(firstFromQmltc);
        QCOMPARE(createdByQmltc.signalTriggered(), 123);

        auto firstFromComponent = createdByComponent->property("myMyComponent").value<QObject *>();
        auto secondFromComponent =
                createdByComponent->property("myMyComponentAsQtObject").value<QObject *>();

        QVERIFY(firstFromComponent);
        QVERIFY(secondFromComponent);
        QCOMPARE(firstFromComponent->property("myX"), u"SharedX"_s);
        QCOMPARE(secondFromComponent->property("myX"), u"SharedX"_s);
        QCOMPARE(createdByComponent->property("signalTriggered"), 0);
        const QMetaObject *createdByComponentMetaObject = createdByComponent->metaObject();
        QVERIFY(createdByComponentMetaObject);
        const int indexOfMethod =
                createdByComponentMetaObject->indexOfMethod("myMyComponentSignal(QObject*)");
        QVERIFY(indexOfMethod != -1);
        auto method = createdByComponentMetaObject->method(indexOfMethod);
        QVERIFY(method.isValid());
        QVERIFY(method.invoke(createdByComponent.get(), Q_ARG(QObject *, firstFromComponent)));
        QCOMPARE(createdByComponent->property("signalTriggered"), 123);
    }

    // test if nonrecursive components behave well
    {
        auto *myMyComponentFromQmltc = (PREPEND_NAMESPACE(
                inlineComponents_MyComponent_3) *)createdByQmltc.myMyComponentComponent();
        auto *myMyComponentFromQmltc2 = (PREPEND_NAMESPACE(
                inlineComponents_MyComponent_4) *)createdByQmltc.myMyComponentComponent2();
        QVERIFY(myMyComponentFromQmltc);
        QVERIFY(myMyComponentFromQmltc2);
        auto *myMyComponentFromComponent =
                createdByComponent->property("myMyComponentComponent").value<QObject *>();
        // second MyComponent
        auto *myMyComponentFromComponent2 =
                createdByComponent->property("myMyComponentComponent2").value<QObject *>();
        QVERIFY(myMyComponentFromComponent);
        QVERIFY(myMyComponentFromComponent2);

        // test writing and reading own properties on myMyComponentComponent
        QCOMPARE(myMyComponentFromQmltc->myY(), u"NotSharedY1"_s);
        myMyComponentFromQmltc->setMyY(testData);
        QCOMPARE(myMyComponentFromQmltc->myY(), testData);

        QCOMPARE(myMyComponentFromComponent->property("myY"), u"NotSharedY1"_s);
        QVERIFY(myMyComponentFromComponent->setProperty("myY", testData));
        QCOMPARE(myMyComponentFromComponent->property("myY"), testData);

        // test writing and reading own properties on myMyComponentComponent2
        QCOMPARE(myMyComponentFromQmltc2->myY2(), u"NotSharedY2"_s);
        myMyComponentFromQmltc2->setMyY2(testData2);
        QCOMPARE(myMyComponentFromQmltc2->myY2(), testData2);

        QCOMPARE(myMyComponentFromComponent2->property("myY2"), u"NotSharedY2"_s);
        QVERIFY(myMyComponentFromComponent2->setProperty("myY2", testData2));
        QCOMPARE(myMyComponentFromComponent2->property("myY2"), testData2);

        // test writing and reading inline component properties on myMyComponentComponent
        QCOMPARE(myMyComponentFromQmltc->myX(), u"MyComponent1"_s);
        myMyComponentFromQmltc->setMyX(testData);
        QCOMPARE(myMyComponentFromQmltc->myX(), testData);

        QCOMPARE(myMyComponentFromComponent->property("myX"), u"MyComponent1"_s);
        QVERIFY(myMyComponentFromComponent->setProperty("myX", testData));
        QCOMPARE(myMyComponentFromComponent->property("myX"), testData);

        // test writing and reading inline component properties on myMyComponentComponent2
        QCOMPARE(myMyComponentFromQmltc2->myX(), u"MyComponent2"_s);
        myMyComponentFromQmltc2->setMyX(testData2);
        QCOMPARE(myMyComponentFromQmltc2->myX(), testData2);
        // check that the other x property was not overridden
        QCOMPARE(myMyComponentFromQmltc->myX(), testData);

        QCOMPARE(myMyComponentFromComponent2->property("myX"), u"MyComponent2"_s);
        QVERIFY(myMyComponentFromComponent2->setProperty("myX", testData2));
        QCOMPARE(myMyComponentFromComponent2->property("myX"), testData2);
        // check that the other x property was not overridden
        QCOMPARE(myMyComponentFromComponent->property("myX"), testData);

        // check if the literal binding is working on myProperty
        QCOMPARE(myMyComponentFromQmltc->myProperty(), u"check literal binding"_s);
        QCOMPARE(myMyComponentFromQmltc2->myProperty(), u"check literal binding"_s);

        QCOMPARE(myMyComponentFromComponent->property("myProperty"), u"check literal binding"_s);
        QCOMPARE(myMyComponentFromComponent2->property("myProperty"), u"check literal binding"_s);

        // check the Item in the MyComponent component
        QCOMPARE(myMyComponentFromQmltc->children().size(), 1);
        QCOMPARE(myMyComponentFromQmltc2->children().size(), 1);
        auto *childFromQmltc =
                (PREPEND_NAMESPACE(inlineComponents_MyComponent_Item) *)myMyComponentFromQmltc
                        ->children()
                        .front();
        auto *childFromQmltc2 =
                (PREPEND_NAMESPACE(inlineComponents_MyComponent_Item) *)myMyComponentFromQmltc2
                        ->children()
                        .front();

        QVERIFY(childFromQmltc);
        QVERIFY(childFromQmltc2);

        QCOMPARE(myMyComponentFromComponent->children().size(), 1);
        QCOMPARE(myMyComponentFromComponent2->children().size(), 1);
        auto *childFromComponent = myMyComponentFromComponent->children().front();
        auto *childFromComponent2 = myMyComponentFromComponent2->children().front();

        QVERIFY(childFromComponent);
        QVERIFY(childFromComponent2);

        // test writing and reading inline components childrens
        // .. also tests at the same time if the literal bindings inside inline components are
        // set correctly
        QCOMPARE(childFromQmltc->myZ(), "SharedZ");
        QCOMPARE(childFromQmltc2->myZ(), "SharedZ");
        childFromQmltc->setMyZ(testData);
        childFromQmltc2->setMyZ(testData2);
        QCOMPARE(childFromQmltc->myZ(), testData);
        QCOMPARE(childFromQmltc2->myZ(), testData2);

        QCOMPARE(childFromComponent->property("myZ"), "SharedZ");
        QCOMPARE(childFromComponent2->property("myZ"), "SharedZ");
        QVERIFY(childFromComponent->setProperty("myZ", testData));
        QVERIFY(childFromComponent2->setProperty("myZ", testData2));
        QCOMPARE(childFromComponent->property("myZ"), testData);
        QCOMPARE(childFromComponent2->property("myZ"), testData2);

        // verify aliases inside of inline components:
        childFromQmltc->setAliasToMyX(testData);
        childFromQmltc2->setAliasToMyX(testData2);
        QCOMPARE(myMyComponentFromQmltc->myX(), testData);
        QCOMPARE(childFromQmltc->aliasToMyX(), testData);

        QCOMPARE(myMyComponentFromQmltc2->myX(), testData2);
        QCOMPARE(childFromQmltc2->aliasToMyX(), testData2);
        childFromQmltc2->setAliasToMyX(testData2);
        QCOMPARE(myMyComponentFromQmltc2->myX(), testData2);
        QCOMPARE(childFromQmltc2->aliasToMyX(), testData2);

        QVERIFY(childFromComponent->setProperty("aliasToMyX", testData));
        QVERIFY(childFromComponent2->setProperty("aliasToMyX", testData2));
        QCOMPARE(myMyComponentFromComponent->property("myX"), testData);
        QCOMPARE(childFromComponent->property("aliasToMyX"), testData);

        QCOMPARE(myMyComponentFromComponent2->property("myX"), testData2);
        QCOMPARE(childFromComponent2->property("aliasToMyX"), testData2);
        QVERIFY(childFromComponent2->setProperty("aliasToMyX", testData2));
        QCOMPARE(myMyComponentFromComponent2->property("myX"), testData2);
        QCOMPARE(childFromComponent2->property("aliasToMyX"), testData2);
    }

    // test if recursive components are behaving well
    {
        auto *myAFromQmltc =
                (PREPEND_NAMESPACE(inlineComponents_A_1) *)createdByQmltc.myAComponent();
        auto *innerBFromQmltc = (PREPEND_NAMESPACE(inlineComponents_A_B) *)myAFromQmltc->b();
        QVERIFY(innerBFromQmltc);
        auto *innerAFromQmltc = (PREPEND_NAMESPACE(inlineComponents_A_B_A) *)innerBFromQmltc->a();
        QVERIFY(innerAFromQmltc);
        constexpr bool typeNotCompiledAsQQmlComponent =
                std::is_same_v<decltype(myAFromQmltc->b()),
                               PREPEND_NAMESPACE(inlineComponents_B) *>;
        constexpr bool typeNotCompiledAsQQmlComponent2 =
                std::is_same_v<decltype(innerBFromQmltc->a()),
                               PREPEND_NAMESPACE(inlineComponents_A) *>;
        QVERIFY(typeNotCompiledAsQQmlComponent);
        QVERIFY(typeNotCompiledAsQQmlComponent2);

        auto *myAFromComponent = createdByComponent->property("myAComponent").value<QObject *>();
        auto *innerBFromComponent = myAFromComponent->property("b").value<QObject *>();
        QVERIFY(innerBFromComponent);
        auto *innerAFromComponent = innerBFromComponent->property("a").value<QObject *>();
        QVERIFY(innerAFromComponent);

        QCOMPARE(myAFromComponent->property("data"), u"Hello From Outside!"_s);
        QCOMPARE(innerAFromComponent->property("data"), u"Hello From Inside!"_s);
    }

    // test if ids in inlineComponents are not getting mixed up with those from the root component
    {
        auto *conflictingComponentTomFromQmltc =
                createdByQmltc.tom()
                        .value<PREPEND_NAMESPACE(inlineComponents_ConflictingComponent_1) *>();
        auto *conflictingComponentJerryFromQmltc =
                createdByQmltc.jerry()
                        .value<PREPEND_NAMESPACE(inlineComponents_ConflictingComponent_2) *>();

        auto *conflictingComponentTomFromComponent =
                createdByComponent->property("tom").value<QObject *>();
        auto *conflictingComponentJerryFromComponent =
                createdByComponent->property("jerry").value<QObject *>();

        QCOMPARE(conflictingComponentTomFromQmltc->output(), "Tom: outer");
        QCOMPARE(conflictingComponentJerryFromQmltc->output(), "Jerry: outer");
        QCOMPARE(createdByQmltc.output(), "inner");

        QCOMPARE(conflictingComponentTomFromComponent->property("output"), "Tom: outer");
        QCOMPARE(conflictingComponentJerryFromComponent->property("output"), "Jerry: outer");
        QCOMPARE(createdByComponent->property("output"), "inner");

        QVERIFY(conflictingComponentTomFromQmltc->children()[0]->setProperty("conflicting",
                                                                             u"Tomtom"_s));
        QCOMPARE(conflictingComponentTomFromQmltc->output(), "Tom: Tomtom");
        QCOMPARE(conflictingComponentJerryFromQmltc->output(), "Jerry: outer");
        QCOMPARE(createdByQmltc.output(), "inner");

        QVERIFY(conflictingComponentTomFromComponent->children()[0]->setProperty("conflicting",
                                                                                 u"Tomtom"_s));
        QCOMPARE(conflictingComponentTomFromComponent->property("output"), "Tom: Tomtom");
        QCOMPARE(conflictingComponentJerryFromComponent->property("output"), "Jerry: outer");
        QCOMPARE(createdByComponent->property("output"), "inner");

        QVERIFY(createdByQmltc.setProperty("conflicting", u"Rootroot"_s));
        QCOMPARE(conflictingComponentTomFromQmltc->output(), "Tom: Tomtom");
        QCOMPARE(conflictingComponentJerryFromQmltc->output(), "Jerry: outer");
        QCOMPARE(createdByQmltc.output(), "Rootroot");

        QVERIFY(createdByComponent->setProperty("conflicting", u"Rootroot"_s));
        QCOMPARE(conflictingComponentTomFromComponent->property("output"), "Tom: Tomtom");
        QCOMPARE(conflictingComponentJerryFromComponent->property("output"), "Jerry: outer");
        QCOMPARE(createdByComponent->property("output"), "Rootroot");
    }

    // check 'empty' components that just get stuff through inheritance
    QCOMPARE(createdByQmltc.property("empty").value<QObject *>()->property("objectName"),
             u"EmptyComponentObject"_s);
    QCOMPARE(createdByComponent->property("empty").value<QObject *>()->property("objectName"),
             u"EmptyComponentObject"_s);

    // check that inline components can "hide" other imports
    auto myRectangleFromQmltc =
            createdByQmltc.property("inlineComponentFoundBeforeOtherImports").value<QObject *>();
    QVERIFY(myRectangleFromQmltc);
    QCOMPARE(myRectangleFromQmltc->property("myData"), u"Not from QtQuick.Rectangle"_s);

    auto myRectangleFromComponent =
            createdByComponent->property("inlineComponentFoundBeforeOtherImports")
                    .value<QObject *>();
    QVERIFY(myRectangleFromComponent);
    QCOMPARE(myRectangleFromComponent->property("myData"), u"Not from QtQuick.Rectangle"_s);

    // check that inline components are resolved in the correct order
    {
        auto componentFromQmltc = createdByQmltc.inlineComponentOrder()
                                          .value<PREPEND_NAMESPACE(inlineComponents_IC2_1) *>();
        auto componentFromComponent =
                createdByComponent->property("inlineComponentOrder").value<QObject *>();

        QCOMPARE(componentFromQmltc->color(), QColorConstants::Blue);
        QCOMPARE(componentFromComponent->property("color"), QColorConstants::Blue);
    }

    // check that inline components can be used in lists
    {
        QQmlListReference childrenFromQmltc(&createdByQmltc, "componentList");
        QQmlListReference childrenFromComponent(createdByComponent.data(), "componentList");
        QQmlListReference testList(&createdByQmltc, "testList");

        QCOMPARE(testList.size(), 3);
        QCOMPARE(childrenFromComponent.size(), 2);
        QCOMPARE(childrenFromQmltc.size(), 2);

        QCOMPARE(childrenFromQmltc.at(0)->property("age"), 65);
        QCOMPARE(childrenFromComponent.at(0)->property("age"), 65);

        QCOMPARE(childrenFromQmltc.at(1)->property("age"), 62);
        QCOMPARE(childrenFromComponent.at(1)->property("age"), 62);
    }
}

void tst_qmltc::aliases()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(aliases) fromQmltc(&e);

    QQmlComponent component(&e);
    component.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/aliases.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> fromComponent(component.create());
    const QString testString = u"myTestString"_s;

    QCOMPARE(fromQmltc.aliasToAlias(), u"Hello World!"_s);
    QCOMPARE(fromComponent->property("aliasToAlias"), u"Hello World!"_s);

    fromQmltc.setAliasToAlias(testString);
    QVERIFY(fromComponent->setProperty("aliasToAlias", testString));

    QCOMPARE(fromQmltc.aliasToAlias(), testString);
    QCOMPARE(fromComponent->property("aliasToAlias"), testString);

    QCOMPARE(fromQmltc.aliasToOtherFile(), u"Set me!"_s);
    QCOMPARE(fromComponent->property("aliasToOtherFile"), u"Set me!"_s);

    fromQmltc.setAliasToOtherFile(testString);
    QVERIFY(fromComponent->setProperty("aliasToOtherFile", testString));

    QCOMPARE(fromQmltc.aliasToOtherFile(), testString);
    QCOMPARE(fromComponent->property("aliasToOtherFile"), testString);
}

void tst_qmltc::inlineComponentsFromDifferentFiles()
{
    // check that inline components can be imported from different files
    QQmlEngine e;
    PREPEND_NAMESPACE(inlineComponentsFromDifferentFiles) createdByQmltc(&e);

    QQmlComponent component(&e);
    component.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/inlineComponentsFromDifferentFiles.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> createdByComponent(component.create());

    QCOMPARE(createdByQmltc.fromModule1()->objName(), u"IC1"_s);
    QCOMPARE(createdByComponent->property("fromModule1")
                     .value<QObject *>()
                     ->property("objName")
                     .toString(),
             u"IC1"_s);

    QCOMPARE(createdByQmltc.fromModule2().value<QObject *>()->property("objName").toString(),
             u"IC1"_s);
    QCOMPARE(createdByComponent->property("fromModule2")
                     .value<QObject *>()
                     ->property("objName")
                     .toString(),
             u"IC1"_s);

    QCOMPARE(createdByQmltc.fromOtherFile1()->objName(), u"IC1"_s);
    QCOMPARE(createdByComponent->property("fromOtherFile1")
                     .value<QObject *>()
                     ->property("objName")
                     .toString(),
             u"IC1"_s);

    QCOMPARE(createdByQmltc.fromOtherFile2()->objName(), u"IC2"_s);
    QCOMPARE(createdByComponent->property("fromOtherFile2")
                     .value<QObject *>()
                     ->property("objName")
                     .toString(),
             u"IC2"_s);

    QCOMPARE(createdByQmltc.fromOtherFile3()->objName(), u"IC3"_s);
    QCOMPARE(createdByComponent->property("fromOtherFile3")
                     .value<QObject *>()
                     ->property("objName")
                     .toString(),
             u"IC3"_s);

    QCOMPARE(createdByQmltc.reExported()->objName(), u"IC100"_s);
    QCOMPARE(createdByComponent->property("reExported")
                     .value<QObject *>()
                     ->property("objName")
                     .toString(),
             u"IC100"_s);

    // test how good/bad inline components mix up with enums (they have very similar syntax)
    // hard code enum values for better test readability
    const int dog = 4;

    QCOMPARE(createdByQmltc.looksLikeEnumIsEnum(), dog);
    QCOMPARE(
            createdByQmltc.looksLikeEnumIsInlineComponent().value<QObject *>()->property("objName"),
            u"IC1"_s);

    QCOMPARE(createdByComponent->property("looksLikeEnumIsEnum"), dog);
    QCOMPARE(createdByComponent->property("looksLikeEnumIsInlineComponent")
                     .value<QObject *>()
                     ->property("objName"),
             u"IC1"_s);
}

void tst_qmltc::singletons()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(singletons) createdByQmltc(&e);
    // make sure also that singletons are not shared between engines
    QQmlEngine e2;
    PREPEND_NAMESPACE(singletons) createdByQmltcSecondEngine(&e2);

    QQmlComponent component(&e);
    component.loadUrl(QUrl("qrc:/qt/qml/QmltcTests/singletons.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> createdByComponent(component.create());

    // read default value, write one of the singletons and check that all singletons were written!
    {
        QCOMPARE(createdByQmltc.cppSingleton1(), 42);
        QCOMPARE(createdByQmltcSecondEngine.cppSingleton1(), 42);
        QCOMPARE(createdByComponent->property("cppSingleton1"), 42);

        QCOMPARE(createdByQmltc.cppSingleton2(), 42);
        QCOMPARE(createdByQmltcSecondEngine.cppSingleton2(), 42);
        QCOMPARE(createdByComponent->property("cppSingleton2"), 42);

        // change the singletonvalue
        createdByQmltc.writeSingletonType();

        QCOMPARE(createdByComponent->property("cppSingleton1"), 100);
        QCOMPARE(createdByQmltcSecondEngine.cppSingleton1(), 42);
        QCOMPARE(createdByQmltc.cppSingleton1(), 100);

        QCOMPARE(createdByComponent->property("cppSingleton2"), 100);
        QCOMPARE(createdByQmltcSecondEngine.cppSingleton2(), 42);
        QCOMPARE(createdByQmltc.cppSingleton2(), 100);
    }

    // same schema for singletons defined in qml
    {
        QCOMPARE(createdByQmltc.qmlSingleton1(), 42);
        QCOMPARE(createdByQmltcSecondEngine.qmlSingleton1(), 42);
        QCOMPARE(createdByComponent->property("qmlSingleton1"), 42);

        QCOMPARE(createdByQmltc.qmlSingleton2(), 42);
        QCOMPARE(createdByQmltcSecondEngine.qmlSingleton2(), 42);
        QCOMPARE(createdByComponent->property("qmlSingleton2"), 42);

        // change the singletonvalue
        createdByQmltc.writeSingletonThing();

        // read the others to see if value can be found in all instances
        QCOMPARE(createdByQmltc.qmlSingleton1(), 100);
        QCOMPARE(createdByQmltcSecondEngine.qmlSingleton1(), 42);
        QCOMPARE(createdByComponent->property("qmlSingleton1"), 100);

        QCOMPARE(createdByQmltc.qmlSingleton2(), 100);
        QCOMPARE(createdByQmltcSecondEngine.qmlSingleton2(), 42);
        QCOMPARE(createdByComponent->property("qmlSingleton2"), 100);
    }
}

void tst_qmltc::constSignalParameters()
{
    QQmlEngine e;
    PREPEND_NAMESPACE(mySignals) fromQmltc(&e);

    int primitive = 123;
    QFont defaultGadget;
    QFont gadget;
    gadget.setBold(true);
    QQuickItem myItem;
    myItem.setObjectName("New Name");

    // by value
    fromQmltc.setPrimitive(123);
    emit fromQmltc.signalWithPrimitive(primitive);
    QCOMPARE(fromQmltc.primitive(), primitive);

    fromQmltc.setGadget(defaultGadget);
    emit fromQmltc.signalWithGadget(gadget);
    QCOMPARE(fromQmltc.gadget(), gadget);

    // by const ref
    fromQmltc.setPrimitive(123);
    emit fromQmltc.signalWithConstReferenceToPrimitive(primitive);
    QCOMPARE(fromQmltc.primitive(), primitive);

    fromQmltc.setGadget(defaultGadget);
    emit fromQmltc.signalWithConstReferenceToGadget(gadget);
    QCOMPARE(fromQmltc.gadget(), gadget);

    // by pointer
    fromQmltc.setObject(nullptr);
    emit fromQmltc.signalWithPointer(&myItem);
    QCOMPARE(fromQmltc.object(), &myItem);

    fromQmltc.setObject(nullptr);
    emit fromQmltc.signalWithPointerToConst(&myItem);
    QCOMPARE(fromQmltc.object(), &myItem);

    fromQmltc.setObject(nullptr);
    emit fromQmltc.signalWithPointerToConst2(&myItem);
    QCOMPARE(fromQmltc.object(), &myItem);

    fromQmltc.setObject(nullptr);
    emit fromQmltc.signalWithConstPointer(&myItem);
    QCOMPARE(fromQmltc.object(), &myItem);

    fromQmltc.setObject(nullptr);
    emit fromQmltc.signalWithConstPointerToConst(&myItem);
    QCOMPARE(fromQmltc.object(), &myItem);

    fromQmltc.setObject(nullptr);
    emit fromQmltc.signalWithConstPointerToConst2(&myItem);
    QCOMPARE(fromQmltc.object(), &myItem);
}

void tst_qmltc::cppNamespaces()
{
    // see if qmltc works correctly with c++ namespaced types
    QQmlEngine e;
    PREPEND_NAMESPACE(NamespacedTypes) createdByQmltc(&e);
    QCOMPARE(createdByQmltc.myObject()->property("value"), 123);
    createdByQmltc.f();
    QCOMPARE(createdByQmltc.myObject()->property("value"), 55);
}

void tst_qmltc::namespacedName()
{
    // cmake script should be able to auto-fill the namespace of the generated modules, and to
    // replace . with ::
    QQmlEngine e;
    NamespaceTest::Subfolder::Type t(&e);
    QCOMPARE(t.data(), u"Hello from namespace"_s);
}

void tst_qmltc::checkExportsAreCompiling()
{
    QQmlEngine e;
    QmltcExportedTests::HelloExportedWorld w(&e);
    QCOMPARE(w.myString(), u"Hello! I should be exported by qmltc"_s);
}

QTEST_MAIN(tst_qmltc)
