/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdebug.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>
#include <QtCore/qdir.h>
#include <QtCore/qnumeric.h>
#include <private/qdeclarativeengine_p.h>
#include "testtypes.h"
#include "testhttpserver.h"
#include "../../../shared/util.h"

/*
This test covers evaluation of ECMAScript expressions and bindings from within
QML.  This does not include static QML language issues.

Static QML language issues are covered in qmllanguage
*/
inline QUrl TEST_FILE(const QString &filename)
{
    QFileInfo fileInfo(__FILE__);
    return QUrl::fromLocalFile(fileInfo.absoluteDir().filePath("data/" + filename));
}

inline QUrl TEST_FILE(const char *filename)
{
    return TEST_FILE(QLatin1String(filename));
}

class tst_qdeclarativeecmascript : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativeecmascript() {}

private slots:
    void initTestCase();
    void assignBasicTypes();
    void idShortcutInvalidates();
    void boolPropertiesEvaluateAsBool();
    void methods();
    void signalAssignment();
    void bindingLoop();
    void basicExpressions();
    void basicExpressions_data();
    void arrayExpressions();
    void contextPropertiesTriggerReeval();
    void objectPropertiesTriggerReeval();
    void deferredProperties();
    void deferredPropertiesErrors();
    void extensionObjects();
    void overrideExtensionProperties();
    void attachedProperties();
    void enums();
    void valueTypeFunctions();
    void constantsOverrideBindings();
    void outerBindingOverridesInnerBinding();
    void aliasPropertyAndBinding();
    void aliasPropertyReset();
    void nonExistentAttachedObject();
    void scope();
    void importScope();
    void signalParameterTypes();
    void objectsCompareAsEqual();
    void dynamicCreation_data();
    void dynamicCreation();
    void dynamicDestruction();
    void objectToString();
    void objectHasOwnProperty();
    void selfDeletingBinding();
    void extendedObjectPropertyLookup();
    void scriptErrors();
    void functionErrors();
    void propertyAssignmentErrors();
    void signalTriggeredBindings();
    void listProperties();
    void exceptionClearsOnReeval();
    void exceptionSlotProducesWarning();
    void exceptionBindingProducesWarning();
    void transientErrors();
    void shutdownErrors();
    void compositePropertyType();
    void jsObject();
    void undefinedResetsProperty();
    void listToVariant();
    void listAssignment();
    void multiEngineObject();
    void deletedObject();
    void attachedPropertyScope();
    void scriptConnect();
    void scriptDisconnect();
    void ownership();
    void cppOwnershipReturnValue();
    void ownershipCustomReturnValue();
    void qlistqobjectMethods();
    void strictlyEquals();
    void compiled();
    void numberAssignment();
    void propertySplicing();
    void signalWithUnknownTypes();
    void signalWithJSValueInVariant_data();
    void signalWithJSValueInVariant();
    void signalWithJSValueInVariant_twoEngines_data();
    void signalWithJSValueInVariant_twoEngines();
    void moduleApi_data();
    void moduleApi();
    void importScripts();
    void scarceResources();
    void propertyChangeSlots();
    void elementAssign();
    void objectPassThroughSignals();
    void objectConversion();
    void booleanConversion();
    void handleReferenceManagement();
    void stringArg();

    void bug1();
    void bug2();
    void dynamicCreationCrash();
    void dynamicCreationOwnership();
    void regExpBug();
    void nullObjectBinding();
    void deletedEngine();
    void libraryScriptAssert();
    void variantsAssignedUndefined();
    void qtbug_9792();
    void qtcreatorbug_1289();
    void noSpuriousWarningsAtShutdown();
    void canAssignNullToQObject();
    void functionAssignment_fromBinding();
    void functionAssignment_fromJS();
    void functionAssignment_fromJS_data();
    void functionAssignmentfromJS_invalid();
    void eval();
    void function();
    void qtbug_10696();
    void qtbug_11606();
    void qtbug_11600();
    void nonscriptable();
    void deleteLater();
    void in();
    void sharedAttachedObject();
    void objectName();
    void writeRemovesBinding();
    void aliasBindingsAssignCorrectly();
    void aliasBindingsOverrideTarget();
    void aliasWritesOverrideBindings();
    void aliasToCompositeElement();
    void realToInt();
    void dynamicString();
    void include();
    void signalHandlers();

    void callQtInvokables();
    void invokableObjectArg();
    void invokableObjectRet();

    void revisionErrors();
    void revision();

    void automaticSemicolon();

private:
    QDeclarativeEngine engine;
};

void tst_qdeclarativeecmascript::initTestCase() { registerTypes(); }

void tst_qdeclarativeecmascript::assignBasicTypes()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("assignBasicTypes.qml"));
    MyTypeObject *object = qobject_cast<MyTypeObject *>(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->flagProperty(), MyTypeObject::FlagVal1 | MyTypeObject::FlagVal3);
    QCOMPARE(object->enumProperty(), MyTypeObject::EnumVal2);
    QCOMPARE(object->stringProperty(), QString("Hello World!"));
    QCOMPARE(object->uintProperty(), uint(10));
    QCOMPARE(object->intProperty(), -19);
    QCOMPARE((float)object->realProperty(), float(23.2));
    QCOMPARE((float)object->doubleProperty(), float(-19.75));
    QCOMPARE((float)object->floatProperty(), float(8.5));
    QCOMPARE(object->colorProperty(), QColor("red"));
    QCOMPARE(object->dateProperty(), QDate(1982, 11, 25));
    QCOMPARE(object->timeProperty(), QTime(11, 11, 32));
    QCOMPARE(object->dateTimeProperty(), QDateTime(QDate(2009, 5, 12), QTime(13, 22, 1)));
    QCOMPARE(object->pointProperty(), QPoint(99,13));
    QCOMPARE(object->pointFProperty(), QPointF(-10.1, 12.3));
    QCOMPARE(object->sizeProperty(), QSize(99, 13));
    QCOMPARE(object->sizeFProperty(), QSizeF(0.1, 0.2));
    QCOMPARE(object->rectProperty(), QRect(9, 7, 100, 200));
    QCOMPARE(object->rectFProperty(), QRectF(1000.1, -10.9, 400, 90.99));
    QCOMPARE(object->boolProperty(), true);
    QCOMPARE(object->variantProperty(), QVariant("Hello World!"));
    QCOMPARE(object->vectorProperty(), QVector3D(10, 1, 2.2));
    QCOMPARE(object->urlProperty(), component.url().resolved(QUrl("main.qml")));
    delete object;
    }
    {
    QDeclarativeComponent component(&engine, TEST_FILE("assignBasicTypes.2.qml"));
    MyTypeObject *object = qobject_cast<MyTypeObject *>(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->flagProperty(), MyTypeObject::FlagVal1 | MyTypeObject::FlagVal3);
    QCOMPARE(object->enumProperty(), MyTypeObject::EnumVal2);
    QCOMPARE(object->stringProperty(), QString("Hello World!"));
    QCOMPARE(object->uintProperty(), uint(10));
    QCOMPARE(object->intProperty(), -19);
    QCOMPARE((float)object->realProperty(), float(23.2));
    QCOMPARE((float)object->doubleProperty(), float(-19.75));
    QCOMPARE((float)object->floatProperty(), float(8.5));
    QCOMPARE(object->colorProperty(), QColor("red"));
    QCOMPARE(object->dateProperty(), QDate(1982, 11, 25));
    QCOMPARE(object->timeProperty(), QTime(11, 11, 32));
    QCOMPARE(object->dateTimeProperty(), QDateTime(QDate(2009, 5, 12), QTime(13, 22, 1)));
    QCOMPARE(object->pointProperty(), QPoint(99,13));
    QCOMPARE(object->pointFProperty(), QPointF(-10.1, 12.3));
    QCOMPARE(object->sizeProperty(), QSize(99, 13));
    QCOMPARE(object->sizeFProperty(), QSizeF(0.1, 0.2));
    QCOMPARE(object->rectProperty(), QRect(9, 7, 100, 200));
    QCOMPARE(object->rectFProperty(), QRectF(1000.1, -10.9, 400, 90.99));
    QCOMPARE(object->boolProperty(), true);
    QCOMPARE(object->variantProperty(), QVariant("Hello World!"));
    QCOMPARE(object->vectorProperty(), QVector3D(10, 1, 2.2));
    QCOMPARE(object->urlProperty(), component.url().resolved(QUrl("main.qml")));
    delete object;
    }
}

void tst_qdeclarativeecmascript::idShortcutInvalidates()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("idShortcutInvalidates.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QVERIFY(object->objectProperty() != 0);
        delete object->objectProperty();
        QVERIFY(object->objectProperty() == 0);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("idShortcutInvalidates.1.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QVERIFY(object->objectProperty() != 0);
        delete object->objectProperty();
        QVERIFY(object->objectProperty() == 0);
        delete object;
    }
}

void tst_qdeclarativeecmascript::boolPropertiesEvaluateAsBool()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("boolPropertiesEvaluateAsBool.1.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QCOMPARE(object->stringProperty(), QLatin1String("pass"));
        delete object;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("boolPropertiesEvaluateAsBool.2.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QCOMPARE(object->stringProperty(), QLatin1String("pass"));
        delete object;
    }
}

void tst_qdeclarativeecmascript::signalAssignment()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("signalAssignment.1.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QCOMPARE(object->string(), QString());
        emit object->basicSignal();
        QCOMPARE(object->string(), QString("pass"));
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("signalAssignment.2.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QCOMPARE(object->string(), QString());
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->string(), QString("pass 19 Hello world! 10.25 3 2"));
        delete object;
    }
}

void tst_qdeclarativeecmascript::methods()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("methods.1.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QCOMPARE(object->methodCalled(), false);
        QCOMPARE(object->methodIntCalled(), false);
        emit object->basicSignal();
        QCOMPARE(object->methodCalled(), true);
        QCOMPARE(object->methodIntCalled(), false);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("methods.2.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);
        QCOMPARE(object->methodCalled(), false);
        QCOMPARE(object->methodIntCalled(), false);
        emit object->basicSignal();
        QCOMPARE(object->methodCalled(), false);
        QCOMPARE(object->methodIntCalled(), true);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("methods.3.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("test").toInt(), 19);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("methods.4.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("test").toInt(), 19);
        QCOMPARE(object->property("test2").toInt(), 17);
        QCOMPARE(object->property("test3").toInt(), 16);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("methods.5.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("test").toInt(), 9);
        delete object;
    }
}

void tst_qdeclarativeecmascript::bindingLoop()
{
    QDeclarativeComponent component(&engine, TEST_FILE("bindingLoop.qml"));
    QString warning = component.url().toString() + ":5:9: QML MyQmlObject: Binding loop detected for property \"stringProperty\"";
    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeecmascript::basicExpressions_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QVariant>("result");
    QTest::addColumn<bool>("nest");

    QTest::newRow("Syntax error (self test)") << "{console.log({'a':1'}.a)}" << QVariant() << false;
    QTest::newRow("Context property") << "a" << QVariant(1944) << false;
    QTest::newRow("Context property") << "a" << QVariant(1944) << true;
    QTest::newRow("Context property expression") << "a * 2" << QVariant(3888) << false;
    QTest::newRow("Context property expression") << "a * 2" << QVariant(3888) << true;
    QTest::newRow("Overridden context property") << "b" << QVariant("Milk") << false;
    QTest::newRow("Overridden context property") << "b" << QVariant("Cow") << true;
    QTest::newRow("Object property") << "object.stringProperty" << QVariant("Object1") << false;
    QTest::newRow("Object property") << "object.stringProperty" << QVariant("Object1") << true;
    QTest::newRow("Overridden object property") << "objectOverride.stringProperty" << QVariant("Object2") << false;
    QTest::newRow("Overridden object property") << "objectOverride.stringProperty" << QVariant("Object3") << true;
    QTest::newRow("Default object property") << "horseLegs" << QVariant(4) << false;
    QTest::newRow("Default object property") << "antLegs" << QVariant(6) << false;
    QTest::newRow("Default object property") << "emuLegs" << QVariant(2) << false;
    QTest::newRow("Nested default object property") << "horseLegs" << QVariant(4) << true;
    QTest::newRow("Nested default object property") << "antLegs" << QVariant(7) << true;
    QTest::newRow("Nested default object property") << "emuLegs" << QVariant(2) << true;
    QTest::newRow("Nested default object property") << "humanLegs" << QVariant(2) << true;
    QTest::newRow("Context property override default object property") << "millipedeLegs" << QVariant(100) << true;
}

void tst_qdeclarativeecmascript::basicExpressions()
{
    QFETCH(QString, expression);
    QFETCH(QVariant, result);
    QFETCH(bool, nest);

    MyQmlObject object1;
    MyQmlObject object2;
    MyQmlObject object3;
    MyDefaultObject1 default1;
    MyDefaultObject3 default3;
    object1.setStringProperty("Object1");
    object2.setStringProperty("Object2");
    object3.setStringProperty("Object3");

    QDeclarativeContext context(engine.rootContext());
    QDeclarativeContext nestedContext(&context);

    context.setContextObject(&default1);
    context.setContextProperty("a", QVariant(1944));
    context.setContextProperty("b", QVariant("Milk"));
    context.setContextProperty("object", &object1);
    context.setContextProperty("objectOverride", &object2);
    nestedContext.setContextObject(&default3);
    nestedContext.setContextProperty("b", QVariant("Cow"));
    nestedContext.setContextProperty("objectOverride", &object3);
    nestedContext.setContextProperty("millipedeLegs", QVariant(100));

    MyExpression expr(nest?&nestedContext:&context, expression);
    QCOMPARE(expr.evaluate(), result);
}

void tst_qdeclarativeecmascript::arrayExpressions()
{
    QObject obj1;
    QObject obj2;
    QObject obj3;

    QDeclarativeContext context(engine.rootContext());
    context.setContextProperty("a", &obj1);
    context.setContextProperty("b", &obj2);
    context.setContextProperty("c", &obj3);

    MyExpression expr(&context, "[a, b, c, 10]");
    QVariant result = expr.evaluate();
    QCOMPARE(result.userType(), qMetaTypeId<QList<QObject *> >());
    QList<QObject *> list = qvariant_cast<QList<QObject *> >(result);
    QCOMPARE(list.count(), 4);
    QCOMPARE(list.at(0), &obj1);
    QCOMPARE(list.at(1), &obj2);
    QCOMPARE(list.at(2), &obj3);
    QCOMPARE(list.at(3), (QObject *)0);
}

// Tests that modifying a context property will reevaluate expressions
void tst_qdeclarativeecmascript::contextPropertiesTriggerReeval()
{
    QDeclarativeContext context(engine.rootContext());
    MyQmlObject object1;
    MyQmlObject object2;
    MyQmlObject *object3 = new MyQmlObject;

    object1.setStringProperty("Hello");
    object2.setStringProperty("World");

    context.setContextProperty("testProp", QVariant(1));
    context.setContextProperty("testObj", &object1);
    context.setContextProperty("testObj2", object3);

    { 
        MyExpression expr(&context, "testProp + 1");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant(2));

        context.setContextProperty("testProp", QVariant(2));
        QCOMPARE(expr.changed, true);
        QCOMPARE(expr.evaluate(), QVariant(3));
    }

    { 
        MyExpression expr(&context, "testProp + testProp + testProp");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant(6));

        context.setContextProperty("testProp", QVariant(4));
        QCOMPARE(expr.changed, true);
        QCOMPARE(expr.evaluate(), QVariant(12));
    }

    { 
        MyExpression expr(&context, "testObj.stringProperty");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant("Hello"));

        context.setContextProperty("testObj", &object2);
        QCOMPARE(expr.changed, true);
        QCOMPARE(expr.evaluate(), QVariant("World"));
    }

    { 
        MyExpression expr(&context, "testObj.stringProperty /**/");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant("World"));

        context.setContextProperty("testObj", &object1);
        QCOMPARE(expr.changed, true);
        QCOMPARE(expr.evaluate(), QVariant("Hello"));
    }

    { 
        MyExpression expr(&context, "testObj2");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant::fromValue((QObject *)object3));
    }

    delete object3;
}

void tst_qdeclarativeecmascript::objectPropertiesTriggerReeval()
{
    QDeclarativeContext context(engine.rootContext());
    MyQmlObject object1;
    MyQmlObject object2;
    MyQmlObject object3;
    context.setContextProperty("testObj", &object1);

    object1.setStringProperty(QLatin1String("Hello"));
    object2.setStringProperty(QLatin1String("Dog"));
    object3.setStringProperty(QLatin1String("Cat"));

    { 
        MyExpression expr(&context, "testObj.stringProperty");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant("Hello"));

        object1.setStringProperty(QLatin1String("World"));
        QCOMPARE(expr.changed, true);
        QCOMPARE(expr.evaluate(), QVariant("World"));
    }

    { 
        MyExpression expr(&context, "testObj.objectProperty.stringProperty");
        QCOMPARE(expr.changed, false);
        QCOMPARE(expr.evaluate(), QVariant());

        object1.setObjectProperty(&object2);
        QCOMPARE(expr.changed, true);
        expr.changed = false;
        QCOMPARE(expr.evaluate(), QVariant("Dog"));

        object1.setObjectProperty(&object3);
        QCOMPARE(expr.changed, true);
        expr.changed = false;
        QCOMPARE(expr.evaluate(), QVariant("Cat"));

        object1.setObjectProperty(0);
        QCOMPARE(expr.changed, true);
        expr.changed = false;
        QCOMPARE(expr.evaluate(), QVariant());

        object1.setObjectProperty(&object3);
        QCOMPARE(expr.changed, true);
        expr.changed = false;
        QCOMPARE(expr.evaluate(), QVariant("Cat"));

        object3.setStringProperty("Donkey");
        QCOMPARE(expr.changed, true);
        expr.changed = false;
        QCOMPARE(expr.evaluate(), QVariant("Donkey"));
    }
}

void tst_qdeclarativeecmascript::deferredProperties()
{
    QDeclarativeComponent component(&engine, TEST_FILE("deferredProperties.qml"));
    MyDeferredObject *object = 
        qobject_cast<MyDeferredObject *>(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->value(), 0);
    QVERIFY(object->objectProperty() == 0);
    QVERIFY(object->objectProperty2() != 0);
    qmlExecuteDeferred(object);
    QCOMPARE(object->value(), 10);
    QVERIFY(object->objectProperty() != 0);
    MyQmlObject *qmlObject = 
        qobject_cast<MyQmlObject *>(object->objectProperty());
    QVERIFY(qmlObject != 0);
    QCOMPARE(qmlObject->value(), 10);
    object->setValue(19);
    QCOMPARE(qmlObject->value(), 19);

    delete object;
}

// Check errors on deferred properties are correctly emitted
void tst_qdeclarativeecmascript::deferredPropertiesErrors()
{
    QDeclarativeComponent component(&engine, TEST_FILE("deferredPropertiesErrors.qml"));
    MyDeferredObject *object = 
        qobject_cast<MyDeferredObject *>(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->value(), 0);
    QVERIFY(object->objectProperty() == 0);
    QVERIFY(object->objectProperty2() == 0);

    QString warning = component.url().toString() + ":6: Unable to assign [undefined] to QObject* objectProperty";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    qmlExecuteDeferred(object);

    delete object;
}

void tst_qdeclarativeecmascript::extensionObjects()
{
    QDeclarativeComponent component(&engine, TEST_FILE("extensionObjects.qml"));
    MyExtendedObject *object = 
        qobject_cast<MyExtendedObject *>(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->baseProperty(), 13);
    QCOMPARE(object->coreProperty(), 9);
    object->setProperty("extendedProperty", QVariant(11));
    object->setProperty("baseExtendedProperty", QVariant(92));
    QCOMPARE(object->coreProperty(), 11);
    QCOMPARE(object->baseProperty(), 92);

    MyExtendedObject *nested = qobject_cast<MyExtendedObject*>(qvariant_cast<QObject *>(object->property("nested")));
    QVERIFY(nested);
    QCOMPARE(nested->baseProperty(), 13);
    QCOMPARE(nested->coreProperty(), 9);
    nested->setProperty("extendedProperty", QVariant(11));
    nested->setProperty("baseExtendedProperty", QVariant(92));
    QCOMPARE(nested->coreProperty(), 11);
    QCOMPARE(nested->baseProperty(), 92);

    delete object;
}

void tst_qdeclarativeecmascript::overrideExtensionProperties()
{
    QDeclarativeComponent component(&engine, TEST_FILE("extensionObjectsPropertyOverride.qml"));
    OverrideDefaultPropertyObject *object =
        qobject_cast<OverrideDefaultPropertyObject *>(component.create());
    QVERIFY(object != 0);
    QVERIFY(object->secondProperty() != 0);
    QVERIFY(object->firstProperty() == 0);

    delete object;
}

void tst_qdeclarativeecmascript::attachedProperties()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("attachedProperty.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("a").toInt(), 19);
        QCOMPARE(object->property("b").toInt(), 19);
        QCOMPARE(object->property("c").toInt(), 19);
        QCOMPARE(object->property("d").toInt(), 19);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("attachedProperty.2.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("a").toInt(), 26);
        QCOMPARE(object->property("b").toInt(), 26);
        QCOMPARE(object->property("c").toInt(), 26);
        QCOMPARE(object->property("d").toInt(), 26);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("writeAttachedProperty.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);

        QMetaObject::invokeMethod(object, "writeValue2");

        MyQmlAttachedObject *attached =
            qobject_cast<MyQmlAttachedObject *>(qmlAttachedPropertiesObject<MyQmlObject>(object));
        QVERIFY(attached != 0);

        QCOMPARE(attached->value2(), 9);
        delete object;
    }
}

void tst_qdeclarativeecmascript::enums()
{
    // Existent enums
    {
    QDeclarativeComponent component(&engine, TEST_FILE("enums.1.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("a").toInt(), 0);
    QCOMPARE(object->property("b").toInt(), 1);
    QCOMPARE(object->property("c").toInt(), 2);
    QCOMPARE(object->property("d").toInt(), 3);
    QCOMPARE(object->property("e").toInt(), 0);
    QCOMPARE(object->property("f").toInt(), 1);
    QCOMPARE(object->property("g").toInt(), 2);
    QCOMPARE(object->property("h").toInt(), 3);
    QCOMPARE(object->property("i").toInt(), 19);
    QCOMPARE(object->property("j").toInt(), 19);

    delete object;
    }
    // Non-existent enums
    {
    QDeclarativeComponent component(&engine, TEST_FILE("enums.2.qml"));

    QString warning1 = component.url().toString() + ":5: Unable to assign [undefined] to int a";
    QString warning2 = component.url().toString() + ":6: Unable to assign [undefined] to int b";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("a").toInt(), 0);
    QCOMPARE(object->property("b").toInt(), 0);

    delete object;
    }
}

void tst_qdeclarativeecmascript::valueTypeFunctions()
{
    QDeclarativeComponent component(&engine, TEST_FILE("valueTypeFunctions.qml"));
    MyTypeObject *obj = qobject_cast<MyTypeObject*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->rectProperty(), QRect(0,0,100,100));
    QCOMPARE(obj->rectFProperty(), QRectF(0,0.5,100,99.5));

    delete obj;
}

/* 
Tests that writing a constant to a property with a binding on it disables the
binding.
*/
void tst_qdeclarativeecmascript::constantsOverrideBindings()
{
    // From ECMAScript
    {
        QDeclarativeComponent component(&engine, TEST_FILE("constantsOverrideBindings.1.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("c2").toInt(), 0);
        object->setProperty("c1", QVariant(9));
        QCOMPARE(object->property("c2").toInt(), 9);

        emit object->basicSignal();

        QCOMPARE(object->property("c2").toInt(), 13);
        object->setProperty("c1", QVariant(8));
        QCOMPARE(object->property("c2").toInt(), 13);

        delete object;
    }

    // During construction
    {
        QDeclarativeComponent component(&engine, TEST_FILE("constantsOverrideBindings.2.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("c1").toInt(), 0);
        QCOMPARE(object->property("c2").toInt(), 10);
        object->setProperty("c1", QVariant(9));
        QCOMPARE(object->property("c1").toInt(), 9);
        QCOMPARE(object->property("c2").toInt(), 10);

        delete object;
    }

#if 0
    // From C++
    {
        QDeclarativeComponent component(&engine, TEST_FILE("constantsOverrideBindings.3.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("c2").toInt(), 0);
        object->setProperty("c1", QVariant(9));
        QCOMPARE(object->property("c2").toInt(), 9);

        object->setProperty("c2", QVariant(13));
        QCOMPARE(object->property("c2").toInt(), 13);
        object->setProperty("c1", QVariant(7));
        QCOMPARE(object->property("c1").toInt(), 7);
        QCOMPARE(object->property("c2").toInt(), 13);

        delete object;
    }
#endif

    // Using an alias
    {
        QDeclarativeComponent component(&engine, TEST_FILE("constantsOverrideBindings.4.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("c1").toInt(), 0);
        QCOMPARE(object->property("c3").toInt(), 10);
        object->setProperty("c1", QVariant(9));
        QCOMPARE(object->property("c1").toInt(), 9);
        QCOMPARE(object->property("c3").toInt(), 10);

        delete object;
    }
}

/*
Tests that assigning a binding to a property that already has a binding causes
the original binding to be disabled.
*/
void tst_qdeclarativeecmascript::outerBindingOverridesInnerBinding()
{
    QDeclarativeComponent component(&engine, 
                           TEST_FILE("outerBindingOverridesInnerBinding.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(object != 0);

    QCOMPARE(object->property("c1").toInt(), 0);
    QCOMPARE(object->property("c2").toInt(), 0);
    QCOMPARE(object->property("c3").toInt(), 0);

    object->setProperty("c1", QVariant(9));
    QCOMPARE(object->property("c1").toInt(), 9);
    QCOMPARE(object->property("c2").toInt(), 0);
    QCOMPARE(object->property("c3").toInt(), 0);

    object->setProperty("c3", QVariant(8));
    QCOMPARE(object->property("c1").toInt(), 9);
    QCOMPARE(object->property("c2").toInt(), 8);
    QCOMPARE(object->property("c3").toInt(), 8);

    delete object;
}

/*
Access a non-existent attached object.  

Tests for a regression where this used to crash.
*/
void tst_qdeclarativeecmascript::nonExistentAttachedObject()
{
    QDeclarativeComponent component(&engine, TEST_FILE("nonExistentAttachedObject.qml"));

    QString warning = component.url().toString() + ":4: Unable to assign [undefined] to QString stringProperty";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QObject *object = component.create();
    QVERIFY(object != 0);

    delete object;
}

void tst_qdeclarativeecmascript::scope()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("scope.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);

        QCOMPARE(object->property("test1").toInt(), 1);
        QCOMPARE(object->property("test2").toInt(), 2);
        QCOMPARE(object->property("test3").toString(), QString("1Test"));
        QCOMPARE(object->property("test4").toString(), QString("2Test"));
        QCOMPARE(object->property("test5").toInt(), 1);
        QCOMPARE(object->property("test6").toInt(), 1);
        QCOMPARE(object->property("test7").toInt(), 2);
        QCOMPARE(object->property("test8").toInt(), 2);
        QCOMPARE(object->property("test9").toInt(), 1);
        QCOMPARE(object->property("test10").toInt(), 3);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scope.2.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);

        QCOMPARE(object->property("test1").toInt(), 19);
        QCOMPARE(object->property("test2").toInt(), 19);
        QCOMPARE(object->property("test3").toInt(), 14);
        QCOMPARE(object->property("test4").toInt(), 14);
        QCOMPARE(object->property("test5").toInt(), 24);
        QCOMPARE(object->property("test6").toInt(), 24);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scope.3.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);

        QCOMPARE(object->property("test1").toBool(), true);
        QCOMPARE(object->property("test2").toBool(), true);
        QCOMPARE(object->property("test3").toBool(), true);

        delete object;
    }

    // Signal argument scope
    {
        QDeclarativeComponent component(&engine, TEST_FILE("scope.4.qml"));
        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toInt(), 0);
        QCOMPARE(object->property("test2").toString(), QString());

        emit object->argumentSignal(13, "Argument Scope", 9, MyQmlObject::EnumValue4, Qt::RightButton);

        QCOMPARE(object->property("test").toInt(), 13);
        QCOMPARE(object->property("test2").toString(), QString("Argument Scope"));

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scope.5.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);

        QCOMPARE(object->property("test1").toBool(), true);
        QCOMPARE(object->property("test2").toBool(), true);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scope.6.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toBool(), true);

        delete object;
    }
}

// In 4.7, non-library javascript files that had no imports shared the imports of their
// importing context
void tst_qdeclarativeecmascript::importScope()
{
    QDeclarativeComponent component(&engine, TEST_FILE("importScope.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toInt(), 240);

    delete o;
}

/*
Tests that "any" type passes through a synthesized signal parameter.  This
is essentially a test of QDeclarativeMetaType::copy()
*/
void tst_qdeclarativeecmascript::signalParameterTypes()
{
    QDeclarativeComponent component(&engine, TEST_FILE("signalParameterTypes.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(object != 0);

    emit object->basicSignal();

    QCOMPARE(object->property("intProperty").toInt(), 10);
    QCOMPARE(object->property("realProperty").toReal(), 19.2);
    QVERIFY(object->property("colorProperty").value<QColor>() == QColor(255, 255, 0, 255));
    QVERIFY(object->property("variantProperty") == QVariant::fromValue(QColor(255, 0, 255, 255)));
    QVERIFY(object->property("enumProperty") == MyQmlObject::EnumValue3);
    QVERIFY(object->property("qtEnumProperty") == Qt::LeftButton);

    delete object;
}

/*
Test that two JS objects for the same QObject compare as equal.
*/
void tst_qdeclarativeecmascript::objectsCompareAsEqual()
{
    QDeclarativeComponent component(&engine, TEST_FILE("objectsCompareAsEqual.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), true);
    QCOMPARE(object->property("test3").toBool(), true);
    QCOMPARE(object->property("test4").toBool(), true);
    QCOMPARE(object->property("test5").toBool(), true);

    delete object;
}

/*
Confirm bindings and alias properties can coexist.

Tests for a regression where the binding would not reevaluate.
*/
void tst_qdeclarativeecmascript::aliasPropertyAndBinding()
{
    QDeclarativeComponent component(&engine, TEST_FILE("aliasPropertyAndBinding.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("c2").toInt(), 3);
    QCOMPARE(object->property("c3").toInt(), 3);

    object->setProperty("c2", QVariant(19));

    QCOMPARE(object->property("c2").toInt(), 19);
    QCOMPARE(object->property("c3").toInt(), 19);

    delete object;
}

/*
Ensure that we can write undefined value to an alias property,
and that the aliased property is reset correctly if possible.
*/
void tst_qdeclarativeecmascript::aliasPropertyReset()
{
    QObject *object = 0;

    // test that a manual write (of undefined) to a resettable aliased property succeeds
    QDeclarativeComponent c1(&engine, TEST_FILE("aliasreset/aliasPropertyReset.1.qml"));
    object = c1.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() != 0);
    QCOMPARE(object->property("aliasIsUndefined"), QVariant(false));
    QMetaObject::invokeMethod(object, "resetAliased");
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0);
    QCOMPARE(object->property("aliasIsUndefined"), QVariant(true));
    delete object;

    // test that a manual write (of undefined) to a resettable alias property succeeds
    QDeclarativeComponent c2(&engine, TEST_FILE("aliasreset/aliasPropertyReset.2.qml"));
    object = c2.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() != 0);
    QCOMPARE(object->property("loaderSourceComponentIsUndefined"), QVariant(false));
    QMetaObject::invokeMethod(object, "resetAlias");
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0);
    QCOMPARE(object->property("loaderSourceComponentIsUndefined"), QVariant(true));
    delete object;

    // test that an alias to a bound property works correctly
    QDeclarativeComponent c3(&engine, TEST_FILE("aliasreset/aliasPropertyReset.3.qml"));
    object = c3.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() != 0);
    QCOMPARE(object->property("loaderOneSourceComponentIsUndefined"), QVariant(false));
    QCOMPARE(object->property("loaderTwoSourceComponentIsUndefined"), QVariant(false));
    QMetaObject::invokeMethod(object, "resetAlias");
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0);
    QCOMPARE(object->property("loaderOneSourceComponentIsUndefined"), QVariant(true));
    QCOMPARE(object->property("loaderTwoSourceComponentIsUndefined"), QVariant(false));
    delete object;

    // test that a manual write (of undefined) to a resettable alias property
    // whose aliased property's object has been deleted, does not crash.
    QDeclarativeComponent c4(&engine, TEST_FILE("aliasreset/aliasPropertyReset.4.qml"));
    object = c4.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() != 0);
    QObject *loader = object->findChild<QObject*>("loader");
    QVERIFY(loader != 0);
    delete loader;
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0); // deletion should have caused value unset.
    QMetaObject::invokeMethod(object, "resetAlias"); // shouldn't crash.
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0);
    QMetaObject::invokeMethod(object, "setAlias");   // shouldn't crash, and shouldn't change value (since it's no longer referencing anything).
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0);
    delete object;

    // test that binding an alias property to an undefined value works correctly
    QDeclarativeComponent c5(&engine, TEST_FILE("aliasreset/aliasPropertyReset.5.qml"));
    object = c5.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("sourceComponentAlias").value<QDeclarativeComponent*>() == 0); // bound to undefined value.
    delete object;

    // test that a manual write (of undefined) to a non-resettable property fails properly
    QUrl url = TEST_FILE("aliasreset/aliasPropertyReset.error.1.qml");
    QString warning1 = url.toString() + QLatin1String(":15: Error: Cannot assign [undefined] to int");
    QDeclarativeComponent e1(&engine, url);
    object = e1.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("intAlias").value<int>(), 12);
    QCOMPARE(object->property("aliasedIntIsUndefined"), QVariant(false));
    QTest::ignoreMessage(QtWarningMsg, warning1.toLatin1().constData());
    QMetaObject::invokeMethod(object, "resetAlias");
    QCOMPARE(object->property("intAlias").value<int>(), 12);
    QCOMPARE(object->property("aliasedIntIsUndefined"), QVariant(false));
    delete object;
}

void tst_qdeclarativeecmascript::dynamicCreation_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QString>("createdName");

    QTest::newRow("One") << "createOne" << "objectOne";
    QTest::newRow("Two") << "createTwo" << "objectTwo";
    QTest::newRow("Three") << "createThree" << "objectThree";
}

/*
Test using createQmlObject to dynamically generate an item
Also using createComponent is tested.
*/
void tst_qdeclarativeecmascript::dynamicCreation()
{
    QFETCH(QString, method);
    QFETCH(QString, createdName);

    QDeclarativeComponent component(&engine, TEST_FILE("dynamicCreation.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);

    QMetaObject::invokeMethod(object, method.toUtf8());
    QObject *created = object->objectProperty();
    QVERIFY(created);
    QCOMPARE(created->objectName(), createdName);

    delete object;
}

/*
   Tests the destroy function
*/
void tst_qdeclarativeecmascript::dynamicDestruction()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("dynamicDeletion.qml"));
    QDeclarativeGuard<MyQmlObject> object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);
    QDeclarativeGuard<QObject> createdQmlObject = 0;

    QMetaObject::invokeMethod(object, "create");
    createdQmlObject = object->objectProperty();
    QVERIFY(createdQmlObject);
    QCOMPARE(createdQmlObject->objectName(), QString("emptyObject"));

    QMetaObject::invokeMethod(object, "killOther");
    QVERIFY(createdQmlObject);
    QCoreApplication::instance()->processEvents(QEventLoop::DeferredDeletion);
    QVERIFY(createdQmlObject);
    for (int ii = 0; createdQmlObject && ii < 50; ++ii) { // After 5 seconds we should give up
        if (createdQmlObject) {
            QTest::qWait(100);
            QCoreApplication::instance()->processEvents(QEventLoop::DeferredDeletion);
        }
    }
    QVERIFY(!createdQmlObject);

    QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::JavaScriptOwnership);
    QMetaObject::invokeMethod(object, "killMe");
    QVERIFY(object);
    QTest::qWait(0);
    QCoreApplication::instance()->processEvents(QEventLoop::DeferredDeletion);
    QVERIFY(!object);
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("dynamicDeletion.2.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QVERIFY(qvariant_cast<QObject*>(o->property("objectProperty")) == 0);

    QMetaObject::invokeMethod(o, "create");

    QVERIFY(qvariant_cast<QObject*>(o->property("objectProperty")) != 0);

    QMetaObject::invokeMethod(o, "destroy");

    QCoreApplication::instance()->processEvents(QEventLoop::DeferredDeletion);

    QVERIFY(qvariant_cast<QObject*>(o->property("objectProperty")) == 0);

    delete o;
    }
}

/*
   tests that id.toString() works
*/
void tst_qdeclarativeecmascript::objectToString()
{
    QDeclarativeComponent component(&engine, TEST_FILE("declarativeToString.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);
    QMetaObject::invokeMethod(object, "testToString");
    QVERIFY(object->stringProperty().startsWith("MyQmlObject_QML_"));
    QVERIFY(object->stringProperty().endsWith(", \"objName\")"));

    delete object;
}

/*
  tests that id.hasOwnProperty() works
*/
void tst_qdeclarativeecmascript::objectHasOwnProperty()
{
    QUrl url = TEST_FILE("declarativeHasOwnProperty.qml");
    QString warning1 = url.toString() + ":59: TypeError: Cannot call method 'hasOwnProperty' of undefined";
    QString warning2 = url.toString() + ":64: TypeError: Cannot call method 'hasOwnProperty' of undefined";
    QString warning3 = url.toString() + ":69: TypeError: Cannot call method 'hasOwnProperty' of undefined";

    QDeclarativeComponent component(&engine, url);
    QObject *object = component.create();
    QVERIFY(object != 0);

    // test QObjects in QML
    QMetaObject::invokeMethod(object, "testHasOwnPropertySuccess");
    QVERIFY(object->property("result").value<bool>() == true);
    QMetaObject::invokeMethod(object, "testHasOwnPropertyFailure");
    QVERIFY(object->property("result").value<bool>() == false);

    // now test other types in QML
    QObject *child = object->findChild<QObject*>("typeObj");
    QVERIFY(child != 0);
    QMetaObject::invokeMethod(child, "testHasOwnPropertySuccess");
    QCOMPARE(child->property("valueTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("valueTypeHasOwnProperty2").toBool(), true);
    QCOMPARE(child->property("variantTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("stringTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("listTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("emptyListTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("enumTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("typenameHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("typenameHasOwnProperty2").toBool(), true);
    QCOMPARE(child->property("moduleApiTypeHasOwnProperty").toBool(), true);
    QCOMPARE(child->property("moduleApiPropertyTypeHasOwnProperty").toBool(), true);

    QTest::ignoreMessage(QtWarningMsg, warning1.toLatin1().constData());
    QMetaObject::invokeMethod(child, "testHasOwnPropertyFailureOne");
    QCOMPARE(child->property("enumNonValueHasOwnProperty").toBool(), false);
    QTest::ignoreMessage(QtWarningMsg, warning2.toLatin1().constData());
    QMetaObject::invokeMethod(child, "testHasOwnPropertyFailureTwo");
    QCOMPARE(child->property("moduleApiNonPropertyHasOwnProperty").toBool(), false);
    QTest::ignoreMessage(QtWarningMsg, warning3.toLatin1().constData());
    QMetaObject::invokeMethod(child, "testHasOwnPropertyFailureThree");
    QCOMPARE(child->property("listAtInvalidHasOwnProperty").toBool(), false);

    delete object;
}

/*
Tests bindings that indirectly cause their own deletion work.

This test is best run under valgrind to ensure no invalid memory access occur.
*/
void tst_qdeclarativeecmascript::selfDeletingBinding()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("selfDeletingBinding.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        object->setProperty("triggerDelete", true);
        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("selfDeletingBinding.2.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        object->setProperty("triggerDelete", true);
        delete object;
    }
}

/*
Test that extended object properties can be accessed.

This test a regression where this used to crash.  The issue was specificially
for extended objects that did not include a synthesized meta object (so non-root
and no synthesiszed properties).
*/
void tst_qdeclarativeecmascript::extendedObjectPropertyLookup()
{
    QDeclarativeComponent component(&engine, TEST_FILE("extendedObjectPropertyLookup.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

/*
Test file/lineNumbers for binding/Script errors.
*/
void tst_qdeclarativeecmascript::scriptErrors()
{
    QDeclarativeComponent component(&engine, TEST_FILE("scriptErrors.qml"));
    QString url = component.url().toString();

    QString warning1 = url.left(url.length() - 3) + "js:2: Error: Invalid write to global property \"a\"";
    QString warning2 = url + ":5: ReferenceError: Can't find variable: a";
    QString warning3 = url.left(url.length() - 3) + "js:4: Error: Invalid write to global property \"a\"";
    QString warning4 = url + ":10: ReferenceError: Can't find variable: a";
    QString warning5 = url + ":8: ReferenceError: Can't find variable: a";
    QString warning6 = url + ":7: Unable to assign [undefined] to int x";
    QString warning7 = url + ":12: Error: Cannot assign to read-only property \"trueProperty\"";
    QString warning8 = url + ":13: Error: Cannot assign to non-existent property \"fakeProperty\"";

    QTest::ignoreMessage(QtWarningMsg, warning1.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, warning2.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, warning3.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, warning5.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, warning6.toLatin1().constData());
    MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(object != 0);

    QTest::ignoreMessage(QtWarningMsg, warning4.toLatin1().constData());
    emit object->basicSignal();

    QTest::ignoreMessage(QtWarningMsg, warning7.toLatin1().constData());
    emit object->anotherBasicSignal();

    QTest::ignoreMessage(QtWarningMsg, warning8.toLatin1().constData());
    emit object->thirdBasicSignal();

    delete object;
}

/*
Test file/lineNumbers for inline functions.
*/
void tst_qdeclarativeecmascript::functionErrors()
{
    QDeclarativeComponent component(&engine, TEST_FILE("functionErrors.qml"));
    QString url = component.url().toString();

    QString warning = url + ":5: Error: Invalid write to global property \"a\"";

    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());

    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;

    // test that if an exception occurs while invoking js function from cpp, it is reported as expected.
    QDeclarativeComponent componentTwo(&engine, TEST_FILE("scarceresources/scarceResourceFunctionFail.qml"));
    url = componentTwo.url().toString();
    object = componentTwo.create();
    QVERIFY(object != 0);

    QString srpname = object->property("srp_name").toString();
    
    warning = url + QLatin1String(":17: TypeError: Property 'scarceResource' of object ") + srpname + 
              QLatin1String(" is not a function");
    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData()); // we expect a meaningful warning to be printed.
    QMetaObject::invokeMethod(object, "retrieveScarceResource");
    delete object;
}

/*
Test various errors that can occur when assigning a property from script
*/
void tst_qdeclarativeecmascript::propertyAssignmentErrors()
{
    QDeclarativeComponent component(&engine, TEST_FILE("propertyAssignmentErrors.qml"));

    QString url = component.url().toString();

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), true);

    delete object;
}
    
/*
Test bindings still work when the reeval is triggered from within
a signal script.
*/
void tst_qdeclarativeecmascript::signalTriggeredBindings()
{
    QDeclarativeComponent component(&engine, TEST_FILE("signalTriggeredBindings.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);

    QCOMPARE(object->property("base").toReal(), 50.);
    QCOMPARE(object->property("test1").toReal(), 50.);
    QCOMPARE(object->property("test2").toReal(), 50.);

    object->basicSignal();

    QCOMPARE(object->property("base").toReal(), 200.);
    QCOMPARE(object->property("test1").toReal(), 200.);
    QCOMPARE(object->property("test2").toReal(), 200.);

    object->argumentSignal(10, QString(), 10, MyQmlObject::EnumValue4, Qt::RightButton);

    QCOMPARE(object->property("base").toReal(), 400.);
    QCOMPARE(object->property("test1").toReal(), 400.);
    QCOMPARE(object->property("test2").toReal(), 400.);

    delete object;
}

/*
Test that list properties can be iterated from ECMAScript
*/
void tst_qdeclarativeecmascript::listProperties()
{
    QDeclarativeComponent component(&engine, TEST_FILE("listProperties.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toInt(), 21);
    QCOMPARE(object->property("test2").toInt(), 2);
    QCOMPARE(object->property("test3").toBool(), true);
    QCOMPARE(object->property("test4").toBool(), true);

    delete object;
}

void tst_qdeclarativeecmascript::exceptionClearsOnReeval()
{
    QDeclarativeComponent component(&engine, TEST_FILE("exceptionClearsOnReeval.qml"));
    QString url = component.url().toString();

    QString warning = url + ":4: TypeError: Cannot read property 'objectProperty' of null";

    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);

    QCOMPARE(object->property("test").toBool(), false);

    MyQmlObject object2;
    MyQmlObject object3;
    object2.setObjectProperty(&object3);
    object->setObjectProperty(&object2);

    QCOMPARE(object->property("test").toBool(), true);

    delete object;
}

void tst_qdeclarativeecmascript::exceptionSlotProducesWarning()
{
    QDeclarativeComponent component(&engine, TEST_FILE("exceptionProducesWarning.qml"));
    QString url = component.url().toString();

    QString warning = component.url().toString() + ":6: Error: JS exception";

    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeecmascript::exceptionBindingProducesWarning()
{
    QDeclarativeComponent component(&engine, TEST_FILE("exceptionProducesWarning2.qml"));
    QString url = component.url().toString();

    QString warning = component.url().toString() + ":5: Error: JS exception";

    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);
    delete object;
}

static int transientErrorsMsgCount = 0;
static void transientErrorsMsgHandler(QtMsgType, const char *)
{
    ++transientErrorsMsgCount;
}

// Check that transient binding errors are not displayed
void tst_qdeclarativeecmascript::transientErrors()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("transientErrors.qml"));

    transientErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(transientErrorsMsgHandler);

    QObject *object = component.create();
    QVERIFY(object != 0);

    qInstallMsgHandler(old);

    QCOMPARE(transientErrorsMsgCount, 0);

    delete object;
    }

    // One binding erroring multiple times, but then resolving
    {
    QDeclarativeComponent component(&engine, TEST_FILE("transientErrors.2.qml"));

    transientErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(transientErrorsMsgHandler);

    QObject *object = component.create();
    QVERIFY(object != 0);

    qInstallMsgHandler(old);

    QCOMPARE(transientErrorsMsgCount, 0);

    delete object;
    }
}

// Check that errors during shutdown are minimized
void tst_qdeclarativeecmascript::shutdownErrors()
{
    QDeclarativeComponent component(&engine, TEST_FILE("shutdownErrors.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    transientErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(transientErrorsMsgHandler);

    delete object;

    qInstallMsgHandler(old);
    QCOMPARE(transientErrorsMsgCount, 0);
}

void tst_qdeclarativeecmascript::compositePropertyType()
{
    QDeclarativeComponent component(&engine, TEST_FILE("compositePropertyType.qml"));
    QTest::ignoreMessage(QtDebugMsg, "hello world");
    QObject *object = qobject_cast<QObject *>(component.create());
    delete object;
}

// QTBUG-5759
void tst_qdeclarativeecmascript::jsObject()
{
    QDeclarativeComponent component(&engine, TEST_FILE("jsObject.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test").toInt(), 92);

    delete object;
}

void tst_qdeclarativeecmascript::undefinedResetsProperty()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("undefinedResetsProperty.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("resettableProperty").toInt(), 92);

    object->setProperty("setUndefined", true);

    QCOMPARE(object->property("resettableProperty").toInt(), 13);

    object->setProperty("setUndefined", false);

    QCOMPARE(object->property("resettableProperty").toInt(), 92);

    delete object;
    }
    {
    QDeclarativeComponent component(&engine, TEST_FILE("undefinedResetsProperty.2.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("resettableProperty").toInt(), 19);

    QMetaObject::invokeMethod(object, "doReset");

    QCOMPARE(object->property("resettableProperty").toInt(), 13);

    delete object;
    }
}

// QTBUG-6781
void tst_qdeclarativeecmascript::bug1()
{
    QDeclarativeComponent component(&engine, TEST_FILE("bug.1.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test").toInt(), 14);

    object->setProperty("a", 11);

    QCOMPARE(object->property("test").toInt(), 3);

    object->setProperty("b", true);

    QCOMPARE(object->property("test").toInt(), 9);

    delete object;
}

void tst_qdeclarativeecmascript::bug2()
{
    QDeclarativeComponent component(&engine);
    component.setData("import Qt.test 1.0;\nQPlainTextEdit { width: 100 }", QUrl());

    QObject *object = component.create();
    QVERIFY(object != 0);

    delete object;
}

// Don't crash in createObject when the component has errors.
void tst_qdeclarativeecmascript::dynamicCreationCrash()
{
    QDeclarativeComponent component(&engine, TEST_FILE("dynamicCreation.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);

    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
    QMetaObject::invokeMethod(object, "dontCrash");
    QObject *created = object->objectProperty();
    QVERIFY(created == 0);

    delete object;
}

// ownership transferred to JS, ensure that GC runs the dtor
void tst_qdeclarativeecmascript::dynamicCreationOwnership()
{
    int dtorCount = 0;
    int expectedDtorCount = 1; // start at 1 since we expect mdcdo to dtor too.

    // allow the engine to go out of scope too.
    {
        QDeclarativeEngine dcoEngine;
        QDeclarativeComponent component(&dcoEngine, TEST_FILE("dynamicCreationOwnership.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        MyDynamicCreationDestructionObject *mdcdo = object->findChild<MyDynamicCreationDestructionObject*>("mdcdo");
        QVERIFY(mdcdo != 0);
        mdcdo->setDtorCount(&dtorCount);

        for (int i = 1; i < 105; ++i, ++expectedDtorCount) {
            QMetaObject::invokeMethod(object, "dynamicallyCreateJsOwnedObject");
            if (i % 90 == 0) {
                // we do this once manually, but it should be done automatically
                // when the engine goes out of scope (since it should gc in dtor)
                QMetaObject::invokeMethod(object, "performGc");
            }
            if (i % 10 == 0) {
                QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
            }
        }

        delete object;
    }
    QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
    QCOMPARE(dtorCount, expectedDtorCount);
}

//QTBUG-9367
void tst_qdeclarativeecmascript::regExpBug()
{
    QDeclarativeComponent component(&engine, TEST_FILE("regExp.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->regExp().pattern(), QLatin1String("[a-zA-z]"));
    delete object;
}

static inline bool evaluate_error(QV8Engine *engine, v8::Handle<v8::Object> o, const char *source)
{
    QString functionSource = QLatin1String("(function(object) { return ") + 
                             QLatin1String(source) + QLatin1String(" })");
    v8::TryCatch tc;
    v8::Local<v8::Script> program = v8::Script::Compile(engine->toString(functionSource));
    if (tc.HasCaught())
        return false;
    v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(program->Run());
    if (function.IsEmpty())
        return false;
    v8::Handle<v8::Value> args[] = { o };
    function->Call(engine->global(), 1, args);
    return tc.HasCaught();
}

static inline bool evaluate_value(QV8Engine *engine, v8::Handle<v8::Object> o, 
                                  const char *source, v8::Handle<v8::Value> result)
{
    QString functionSource = QLatin1String("(function(object) { return ") + 
                             QLatin1String(source) + QLatin1String(" })");
    v8::TryCatch tc;
    v8::Local<v8::Script> program = v8::Script::Compile(engine->toString(functionSource));
    if (tc.HasCaught())
        return false;
    v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(program->Run());
    if (function.IsEmpty())
        return false;
    v8::Handle<v8::Value> args[] = { o };

    v8::Handle<v8::Value> value = function->Call(engine->global(), 1, args);

    if (tc.HasCaught())
        return false;

    return value->StrictEquals(result);
}

static inline v8::Handle<v8::Value> evaluate(QV8Engine *engine, v8::Handle<v8::Object> o, 
                                             const char *source)
{
    QString functionSource = QLatin1String("(function(object) { return ") + 
                             QLatin1String(source) + QLatin1String(" })");
    v8::TryCatch tc;
    v8::Local<v8::Script> program = v8::Script::Compile(engine->toString(functionSource));
    if (tc.HasCaught())
        return v8::Handle<v8::Value>();
    v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(program->Run());
    if (function.IsEmpty())
        return v8::Handle<v8::Value>();
    v8::Handle<v8::Value> args[] = { o };

    v8::Handle<v8::Value> value = function->Call(engine->global(), 1, args);

    if (tc.HasCaught())
        return v8::Handle<v8::Value>();
    return value;
}

#define EVALUATE_ERROR(source) evaluate_error(engine, object, source)
#define EVALUATE_VALUE(source, result) evaluate_value(engine, object, source, result)
#define EVALUATE(source) evaluate(engine, object, source)

void tst_qdeclarativeecmascript::callQtInvokables()
{
    MyInvokableObject o;

    QDeclarativeEngine qmlengine;
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(&qmlengine);
    
    QV8Engine *engine = ep->v8engine();

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    v8::Local<v8::Object> object = engine->newQObject(&o)->ToObject();

    // Non-existent methods
    o.reset();
    QVERIFY(EVALUATE_ERROR("object.method_nonexistent()"));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), -1);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    QVERIFY(EVALUATE_ERROR("object.method_nonexistent(10, 11)"));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), -1);
    QCOMPARE(o.actuals().count(), 0);

    // Insufficient arguments
    o.reset();
    QVERIFY(EVALUATE_ERROR("object.method_int()"));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), -1);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    QVERIFY(EVALUATE_ERROR("object.method_intint(10)"));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), -1);
    QCOMPARE(o.actuals().count(), 0);

    // Excessive arguments
    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(10, 11)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(10));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_intint(10, 11, 12)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 9);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(10));
    QCOMPARE(o.actuals().at(1), QVariant(11));

    // Test return types
    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_NoArgs()", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 0);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_NoArgs_int()", v8::Integer::New(6)));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 1);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_NoArgs_real()", v8::Number::New(19.75)));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 2);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    {
    v8::Handle<v8::Value> ret = EVALUATE("object.method_NoArgs_QPointF()");
    QVERIFY(!ret.IsEmpty());
    QCOMPARE(engine->toVariant(ret, -1), QVariant(QPointF(123, 4.5)));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 3);
    QCOMPARE(o.actuals().count(), 0);
    }

    o.reset();
    {
    v8::Handle<v8::Value> ret = EVALUATE("object.method_NoArgs_QObject()");
    QCOMPARE(engine->toQObject(ret), (QObject *)&o);
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 4);
    QCOMPARE(o.actuals().count(), 0);
    }

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_NoArgs_unknown()", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 5);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    {
    v8::Handle<v8::Value> ret = EVALUATE("object.method_NoArgs_QScriptValue()");
    QVERIFY(ret->IsString());
    QCOMPARE(engine->toString(ret), QString("Hello world"));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 6);
    QCOMPARE(o.actuals().count(), 0);
    }

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_NoArgs_QVariant()", engine->toString("QML rocks")));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 7);
    QCOMPARE(o.actuals().count(), 0);

    // Test arg types
    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(94)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(94));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(\"94\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(94));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(\"not a number\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_int(object)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 8);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_intint(122, 9)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 9);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(122));
    QCOMPARE(o.actuals().at(1), QVariant(9));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_real(94.3)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 10);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(94.3));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_real(\"94.3\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 10);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(94.3));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_real(\"not a number\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 10);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qIsNaN(o.actuals().at(0).toDouble()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_real(null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 10);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_real(undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 10);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qIsNaN(o.actuals().at(0).toDouble()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_real(object)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 10);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qIsNaN(o.actuals().at(0).toDouble()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QString(\"Hello world\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 11);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant("Hello world"));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QString(19)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 11);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant("19"));

    o.reset();
    {
    QString expected = "MyInvokableObject(0x" + QString::number((quintptr)&o, 16) + ")";
    QVERIFY(EVALUATE_VALUE("object.method_QString(object)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 11);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(expected));
    }

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QString(null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 11);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QString()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QString(undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 11);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QString()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QPointF(0)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 12);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QPointF()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QPointF(null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 12);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QPointF()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QPointF(undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 12);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QPointF()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QPointF(object)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 12);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QPointF()));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QPointF(object.method_get_QPointF())", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 12);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QPointF(99.3, -10.2)));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QPointF(object.method_get_QPoint())", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 12);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QPointF(9, 12)));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QObject(0)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 13);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), qVariantFromValue((QObject *)0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QObject(\"Hello world\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 13);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), qVariantFromValue((QObject *)0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QObject(null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 13);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), qVariantFromValue((QObject *)0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QObject(undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 13);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), qVariantFromValue((QObject *)0));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QObject(object)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 13);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), qVariantFromValue((QObject *)&o));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QScriptValue(null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 14);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(0)).isNull());

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QScriptValue(undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 14);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(0)).isUndefined());

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QScriptValue(19)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 14);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(0)).strictlyEquals(QJSValue(19)));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QScriptValue([19, 20])", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 14);
    QCOMPARE(o.actuals().count(), 1);
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(0)).isArray());

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_intQScriptValue(4, null)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 15);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(4));
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(1)).isNull());

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_intQScriptValue(8, undefined)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 15);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(8));
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(1)).isUndefined());

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_intQScriptValue(3, 19)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 15);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(3));
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(1)).strictlyEquals(QJSValue(19)));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_intQScriptValue(44, [19, 20])", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 15);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(44));
    QVERIFY(qvariant_cast<QJSValue>(o.actuals().at(1)).isArray());

    o.reset();
    QVERIFY(EVALUATE_ERROR("object.method_overload()"));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), -1);
    QCOMPARE(o.actuals().count(), 0);

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_overload(10)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 16);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(10));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_overload(10, 11)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 17);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(10));
    QCOMPARE(o.actuals().at(1), QVariant(11));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_overload(\"Hello\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 18);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(QString("Hello")));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_with_enum(9)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 19);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(9));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_default(10)", v8::Integer::New(19)));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 20);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(10));
    QCOMPARE(o.actuals().at(1), QVariant(19));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_default(10, 13)", v8::Integer::New(13)));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 20);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(10));
    QCOMPARE(o.actuals().at(1), QVariant(13));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_inherited(9)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), -3);
    QCOMPARE(o.actuals().count(), 1);
    QCOMPARE(o.actuals().at(0), QVariant(9));

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QVariant(9)", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 21);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(9));
    QCOMPARE(o.actuals().at(1), QVariant());

    o.reset();
    QVERIFY(EVALUATE_VALUE("object.method_QVariant(\"Hello\", \"World\")", v8::Undefined()));
    QCOMPARE(o.error(), false);
    QCOMPARE(o.invoked(), 21);
    QCOMPARE(o.actuals().count(), 2);
    QCOMPARE(o.actuals().at(0), QVariant(QString("Hello")));
    QCOMPARE(o.actuals().at(1), QVariant(QString("World")));
}

// QTBUG-13047 (check that you can pass registered object types as args)
void tst_qdeclarativeecmascript::invokableObjectArg()
{
    QDeclarativeComponent component(&engine, TEST_FILE("invokableObjectArg.qml"));

    QObject *o = component.create();
    QVERIFY(o);
    MyQmlObject *qmlobject = qobject_cast<MyQmlObject *>(o);
    QVERIFY(qmlobject);
    QCOMPARE(qmlobject->myinvokableObject, qmlobject);

    delete o;
}

// QTBUG-13047 (check that you can return registered object types from methods)
void tst_qdeclarativeecmascript::invokableObjectRet()
{
    QDeclarativeComponent component(&engine, TEST_FILE("invokableObjectRet.qml"));

    QObject *o = component.create();
    QVERIFY(o);
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
}

// QTBUG-5675
void tst_qdeclarativeecmascript::listToVariant()
{
    QDeclarativeComponent component(&engine, TEST_FILE("listToVariant.qml"));

    MyQmlContainer container;

    QDeclarativeContext context(engine.rootContext());
    context.setContextObject(&container);

    QObject *object = component.create(&context);
    QVERIFY(object != 0);

    QVariant v = object->property("test");
    QCOMPARE(v.userType(), qMetaTypeId<QDeclarativeListReference>());
    QVERIFY(qvariant_cast<QDeclarativeListReference>(v).object() == &container);

    delete object;
}

// QTBUG-16316
Q_DECLARE_METATYPE(QDeclarativeListProperty<MyQmlObject>)
void tst_qdeclarativeecmascript::listAssignment()
{
    QDeclarativeComponent component(&engine, TEST_FILE("listAssignment.qml"));
    QObject *obj = component.create();
    QCOMPARE(obj->property("list1length").toInt(), 2);
    QDeclarativeListProperty<MyQmlObject> list1 = obj->property("list1").value<QDeclarativeListProperty<MyQmlObject> >();
    QDeclarativeListProperty<MyQmlObject> list2 = obj->property("list2").value<QDeclarativeListProperty<MyQmlObject> >();
    QCOMPARE(list1.count(&list1), list2.count(&list2));
    QCOMPARE(list1.at(&list1, 0), list2.at(&list2, 0));
    QCOMPARE(list1.at(&list1, 1), list2.at(&list2, 1));
    delete obj;
}

// QTBUG-7957
void tst_qdeclarativeecmascript::multiEngineObject()
{
    MyQmlObject obj;
    obj.setStringProperty("Howdy planet");

    QDeclarativeEngine e1;
    e1.rootContext()->setContextProperty("thing", &obj);
    QDeclarativeComponent c1(&e1, TEST_FILE("multiEngineObject.qml"));

    QDeclarativeEngine e2;
    e2.rootContext()->setContextProperty("thing", &obj);
    QDeclarativeComponent c2(&e2, TEST_FILE("multiEngineObject.qml"));

    QObject *o1 = c1.create();
    QObject *o2 = c2.create();

    QCOMPARE(o1->property("test").toString(), QString("Howdy planet"));
    QCOMPARE(o2->property("test").toString(), QString("Howdy planet"));

    delete o2;
    delete o1;
}

// Test that references to QObjects are cleanup when the object is destroyed
void tst_qdeclarativeecmascript::deletedObject()
{
    QDeclarativeComponent component(&engine, TEST_FILE("deletedObject.qml"));

    QObject *object = component.create();

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), true);
    QCOMPARE(object->property("test3").toBool(), true);
    QCOMPARE(object->property("test4").toBool(), true);

    delete object;
}

void tst_qdeclarativeecmascript::attachedPropertyScope()
{
    QDeclarativeComponent component(&engine, TEST_FILE("attachedPropertyScope.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    MyQmlAttachedObject *attached = 
        qobject_cast<MyQmlAttachedObject *>(qmlAttachedPropertiesObject<MyQmlObject>(object));
    QVERIFY(attached != 0);

    QCOMPARE(object->property("value2").toInt(), 0);

    attached->emitMySignal();

    QCOMPARE(object->property("value2").toInt(), 9);

    delete object;
}

void tst_qdeclarativeecmascript::scriptConnect()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptConnect.1.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toBool(), false);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toBool(), true);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptConnect.2.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toBool(), false);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toBool(), true);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptConnect.3.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toBool(), false);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toBool(), true);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptConnect.4.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->methodCalled(), false);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->methodCalled(), true);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptConnect.5.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->methodCalled(), false);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->methodCalled(), true);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptConnect.6.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toInt(), 0);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);

        delete object;
    }
}

void tst_qdeclarativeecmascript::scriptDisconnect()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptDisconnect.1.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toInt(), 0);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 1);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->basicSignal();
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptDisconnect.2.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toInt(), 0);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 1);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->basicSignal();
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);

        delete object;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptDisconnect.3.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toInt(), 0);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 1);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->basicSignal();
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 3);

        delete object;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("scriptDisconnect.4.qml"));

        MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
        QVERIFY(object != 0);

        QCOMPARE(object->property("test").toInt(), 0);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 1);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->basicSignal();
        QCOMPARE(object->property("test").toInt(), 2);
        emit object->argumentSignal(19, "Hello world!", 10.25, MyQmlObject::EnumValue4, Qt::RightButton);
        QCOMPARE(object->property("test").toInt(), 3);

        delete object;
    }
}

class OwnershipObject : public QObject
{
    Q_OBJECT
public:
    OwnershipObject() { object = new QObject; }

    QPointer<QObject> object;

public slots:
    QObject *getObject() { return object; }
};

void tst_qdeclarativeecmascript::ownership()
{
    OwnershipObject own;
    QDeclarativeContext *context = new QDeclarativeContext(engine.rootContext());
    context->setContextObject(&own);

    {
        QDeclarativeComponent component(&engine, TEST_FILE("ownership.qml"));

        QVERIFY(own.object != 0);

        QObject *object = component.create(context);

        engine.collectGarbage();

        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);

        QVERIFY(own.object == 0);

        delete object;
    }

    own.object = new QObject(&own);

    {
        QDeclarativeComponent component(&engine, TEST_FILE("ownership.qml"));

        QVERIFY(own.object != 0);

        QObject *object = component.create(context);
        
        engine.collectGarbage();

        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);

        QVERIFY(own.object != 0);

        delete object;
    }

    delete context;
}

class CppOwnershipReturnValue : public QObject
{
    Q_OBJECT
public:
    CppOwnershipReturnValue() : value(0) {}
    ~CppOwnershipReturnValue() { delete value; }

    Q_INVOKABLE QObject *create() {
        value = new QObject;
        QDeclarativeEngine::setObjectOwnership(value, QDeclarativeEngine::CppOwnership);
        return value;
    }

    Q_INVOKABLE MyQmlObject *createQmlObject() {
        MyQmlObject *rv = new MyQmlObject;
        value = rv;
        return rv;
    }

    QPointer<QObject> value;
};

// QTBUG-15695.  
// Test setObjectOwnership(CppOwnership) works even when there is no QDeclarativeData
void tst_qdeclarativeecmascript::cppOwnershipReturnValue()
{
    CppOwnershipReturnValue source;

    {
    QDeclarativeEngine engine;
    engine.rootContext()->setContextProperty("source", &source);

    QVERIFY(source.value == 0);

    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0\nQtObject {\nComponent.onCompleted: { var a = source.create(); }\n}\n", QUrl());

    QObject *object = component.create();

    QVERIFY(object != 0);
    QVERIFY(source.value != 0);

    delete object;
    }

    QCoreApplication::instance()->processEvents(QEventLoop::DeferredDeletion);

    QVERIFY(source.value != 0);
}

// QTBUG-15697
void tst_qdeclarativeecmascript::ownershipCustomReturnValue()
{
    CppOwnershipReturnValue source;

    {
    QDeclarativeEngine engine;
    engine.rootContext()->setContextProperty("source", &source);

    QVERIFY(source.value == 0);

    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0\nQtObject {\nComponent.onCompleted: { var a = source.createQmlObject(); }\n}\n", QUrl());

    QObject *object = component.create();

    QVERIFY(object != 0);
    QVERIFY(source.value != 0);

    delete object;
    }

    engine.collectGarbage();
    QCoreApplication::instance()->processEvents(QEventLoop::DeferredDeletion);

    QVERIFY(source.value == 0);
}

class QListQObjectMethodsObject : public QObject
{
    Q_OBJECT
public:
    QListQObjectMethodsObject() {
        m_objects.append(new MyQmlObject());
        m_objects.append(new MyQmlObject());
    }

    ~QListQObjectMethodsObject() {
        qDeleteAll(m_objects);
    }

public slots:
    QList<QObject *> getObjects() { return m_objects; }

private:
    QList<QObject *> m_objects;
};

// Tests that returning a QList<QObject*> from a method works
void tst_qdeclarativeecmascript::qlistqobjectMethods()
{
    QListQObjectMethodsObject obj;
    QDeclarativeContext *context = new QDeclarativeContext(engine.rootContext());
    context->setContextObject(&obj);

    QDeclarativeComponent component(&engine, TEST_FILE("qlistqobjectMethods.qml"));

    QObject *object = component.create(context);

    QCOMPARE(object->property("test").toInt(), 2);
    QCOMPARE(object->property("test2").toBool(), true);

    delete object;
    delete context;
}

// QTBUG-9205
void tst_qdeclarativeecmascript::strictlyEquals()
{
    QDeclarativeComponent component(&engine, TEST_FILE("strictlyEquals.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), true);
    QCOMPARE(object->property("test3").toBool(), true);
    QCOMPARE(object->property("test4").toBool(), true);
    QCOMPARE(object->property("test5").toBool(), true);
    QCOMPARE(object->property("test6").toBool(), true);
    QCOMPARE(object->property("test7").toBool(), true);
    QCOMPARE(object->property("test8").toBool(), true);

    delete object;
}

void tst_qdeclarativeecmascript::compiled()
{
    QDeclarativeComponent component(&engine, TEST_FILE("compiled.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toReal(), qreal(15.7));
    QCOMPARE(object->property("test2").toReal(), qreal(-6.7));
    QCOMPARE(object->property("test3").toBool(), true);
    QCOMPARE(object->property("test4").toBool(), false);
    QCOMPARE(object->property("test5").toBool(), false);
    QCOMPARE(object->property("test6").toBool(), true);

    QCOMPARE(object->property("test7").toInt(), 185);
    QCOMPARE(object->property("test8").toInt(), 167);
    QCOMPARE(object->property("test9").toBool(), true);
    QCOMPARE(object->property("test10").toBool(), false);
    QCOMPARE(object->property("test11").toBool(), false);
    QCOMPARE(object->property("test12").toBool(), true);

    QCOMPARE(object->property("test13").toString(), QLatin1String("HelloWorld"));
    QCOMPARE(object->property("test14").toString(), QLatin1String("Hello World"));
    QCOMPARE(object->property("test15").toBool(), false);
    QCOMPARE(object->property("test16").toBool(), true);

    QCOMPARE(object->property("test17").toInt(), 5);
    QCOMPARE(object->property("test18").toReal(), qreal(176));
    QCOMPARE(object->property("test19").toInt(), 7);
    QCOMPARE(object->property("test20").toReal(), qreal(6.7));
    QCOMPARE(object->property("test21").toString(), QLatin1String("6.7"));
    QCOMPARE(object->property("test22").toString(), QLatin1String("!"));
    QCOMPARE(object->property("test23").toBool(), true);
    QCOMPARE(qvariant_cast<QColor>(object->property("test24")), QColor(0x11,0x22,0x33));
    QCOMPARE(qvariant_cast<QColor>(object->property("test25")), QColor(0x11,0x22,0x33,0xAA));

    delete object;
}

// Test that numbers assigned in bindings as strings work consistently
void tst_qdeclarativeecmascript::numberAssignment()
{
    QDeclarativeComponent component(&engine, TEST_FILE("numberAssignment.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1"), QVariant((qreal)6.7));
    QCOMPARE(object->property("test2"), QVariant((qreal)6.7));
    QCOMPARE(object->property("test2"), QVariant((qreal)6.7));
    QCOMPARE(object->property("test3"), QVariant((qreal)6));
    QCOMPARE(object->property("test4"), QVariant((qreal)6));

    QCOMPARE(object->property("test5"), QVariant((int)7));
    QCOMPARE(object->property("test6"), QVariant((int)7));
    QCOMPARE(object->property("test7"), QVariant((int)6));
    QCOMPARE(object->property("test8"), QVariant((int)6));

    QCOMPARE(object->property("test9"), QVariant((unsigned int)7));
    QCOMPARE(object->property("test10"), QVariant((unsigned int)7));
    QCOMPARE(object->property("test11"), QVariant((unsigned int)6));
    QCOMPARE(object->property("test12"), QVariant((unsigned int)6));

    delete object;
}

void tst_qdeclarativeecmascript::propertySplicing()
{
    QDeclarativeComponent component(&engine, TEST_FILE("propertySplicing.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test").toBool(), true);

    delete object;
}

// QTBUG-16683
void tst_qdeclarativeecmascript::signalWithUnknownTypes()
{
    QDeclarativeComponent component(&engine, TEST_FILE("signalWithUnknownTypes.qml"));

    MyQmlObject *object = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(object != 0);

    MyQmlObject::MyType type;
    type.value = 0x8971123;
    emit object->signalWithUnknownType(type);

    MyQmlObject::MyType result = qvariant_cast<MyQmlObject::MyType>(object->variant());

    QCOMPARE(result.value, type.value);


    delete object;
}

void tst_qdeclarativeecmascript::signalWithJSValueInVariant_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("compare");

    QString compareStrict("(function(a, b) { return a === b; })");
    QTest::newRow("true") << "true" << compareStrict;
    QTest::newRow("undefined") << "undefined" << compareStrict;
    QTest::newRow("null") << "null" << compareStrict;
    QTest::newRow("123") << "123" << compareStrict;
    QTest::newRow("'ciao'") << "'ciao'" << compareStrict;

    QString comparePropertiesStrict(
        "(function(a, b) {"
        "  if (typeof b != 'object')"
        "    return a === b;"
        "  var props = Object.getOwnPropertyNames(b);"
        "  for (var i = 0; i < props.length; ++i) {"
        "    var p = props[i];"
        "    return arguments.callee(a[p], b[p]);"
        "  }"
        "})");
    QTest::newRow("{ foo: 'bar' }") << "({ foo: 'bar' })"  << comparePropertiesStrict;
    QTest::newRow("[10,20,30]") << "[10,20,30]"  << comparePropertiesStrict;
}

void tst_qdeclarativeecmascript::signalWithJSValueInVariant()
{
    QFETCH(QString, expression);
    QFETCH(QString, compare);

    QDeclarativeComponent component(&engine, TEST_FILE("signalWithJSValueInVariant.qml"));
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != 0);

    QJSValue value = engine.evaluate(expression);
    QVERIFY(!engine.hasUncaughtException());
    object->setProperty("expression", expression);
    object->setProperty("compare", compare);
    object->setProperty("pass", false);

    emit object->signalWithVariant(QVariant::fromValue(value));
    QVERIFY(object->property("pass").toBool());
}

void tst_qdeclarativeecmascript::signalWithJSValueInVariant_twoEngines_data()
{
    signalWithJSValueInVariant_data();
}

void tst_qdeclarativeecmascript::signalWithJSValueInVariant_twoEngines()
{
    QFETCH(QString, expression);
    QFETCH(QString, compare);

    QDeclarativeComponent component(&engine, TEST_FILE("signalWithJSValueInVariant.qml"));
    QScopedPointer<MyQmlObject> object(qobject_cast<MyQmlObject *>(component.create()));
    QVERIFY(object != 0);

    QJSEngine engine2;
    QJSValue value = engine2.evaluate(expression);
    QVERIFY(!engine2.hasUncaughtException());
    object->setProperty("expression", expression);
    object->setProperty("compare", compare);
    object->setProperty("pass", false);

    QTest::ignoreMessage(QtWarningMsg, "JSValue can't be rassigned to an another engine.");
    emit object->signalWithVariant(QVariant::fromValue(value));
    QVERIFY(!object->property("pass").toBool());
}

void tst_qdeclarativeecmascript::moduleApi_data()
{
    QTest::addColumn<QUrl>("testfile");
    QTest::addColumn<QString>("errorMessage");
    QTest::addColumn<QStringList>("warningMessages");
    QTest::addColumn<QStringList>("readProperties");
    QTest::addColumn<QVariantList>("readExpectedValues");
    QTest::addColumn<QStringList>("writeProperties");
    QTest::addColumn<QVariantList>("writeValues");
    QTest::addColumn<QStringList>("readBackProperties");
    QTest::addColumn<QVariantList>("readBackExpectedValues");

    QTest::newRow("qobject, register + read + method")
            << TEST_FILE("moduleapi/qobjectModuleApi.qml")
            << QString()
            << QStringList()
            << (QStringList() << "existingUriTest" << "qobjectTest" << "qobjectMethodTest"
                   << "qobjectMinorVersionTest" << "qobjectMajorVersionTest" << "qobjectParentedTest")
            << (QVariantList() << 20 << 20 << 1 << 20 << 20 << 26)
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();

    QTest::newRow("script, register + read")
            << TEST_FILE("moduleapi/scriptModuleApi.qml")
            << QString()
            << QStringList()
            << (QStringList() << "scriptTest")
            << (QVariantList() << 13)
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();

    QTest::newRow("qobject, caching + read")
            << TEST_FILE("moduleapi/qobjectModuleApiCaching.qml")
            << QString()
            << QStringList()
            << (QStringList() << "existingUriTest" << "qobjectParentedTest")
            << (QVariantList() << 20 << 26) // 26, shouldn't have incremented to 27.
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();

    QTest::newRow("script, caching + read")
            << TEST_FILE("moduleapi/scriptModuleApiCaching.qml")
            << QString()
            << QStringList()
            << (QStringList() << "scriptTest")
            << (QVariantList() << 13) // 13, shouldn't have incremented to 14.
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();

    QTest::newRow("qobject, writing + readonly constraints")
            << TEST_FILE("moduleapi/qobjectModuleApiWriting.qml")
            << QString()
            << (QStringList() << QString(QLatin1String("file://") + TEST_FILE("moduleapi/qobjectModuleApiWriting.qml").toLocalFile() + QLatin1String(":14: Error: Cannot assign to read-only property \"qobjectTestProperty\"")))
            << (QStringList() << "readOnlyProperty" << "writableProperty")
            << (QVariantList() << 20 << 50)
            << (QStringList() << "firstProperty" << "writableProperty")
            << (QVariantList() << 30 << 30)
            << (QStringList() << "readOnlyProperty" << "writableProperty")
            << (QVariantList() << 20 << 30);

    QTest::newRow("script, writing + readonly constraints")
            << TEST_FILE("moduleapi/scriptModuleApiWriting.qml")
            << QString()
            << (QStringList() << QString(QLatin1String("file://") + TEST_FILE("moduleapi/scriptModuleApiWriting.qml").toLocalFile() + QLatin1String(":21: Error: Cannot assign to read-only property \"scriptTestProperty\"")))
            << (QStringList() << "readBack" << "unchanged")
            << (QVariantList() << 13 << 42)
            << (QStringList() << "firstProperty" << "secondProperty")
            << (QVariantList() << 30 << 30)
            << (QStringList() << "readBack" << "unchanged")
            << (QVariantList() << 30 << 42);

    QTest::newRow("qobject module API enum values in JS")
            << TEST_FILE("moduleapi/qobjectModuleApiEnums.qml")
            << QString()
            << QStringList()
            << (QStringList() << "enumValue" << "enumMethod")
            << (QVariantList() << 42 << 30)
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();

    QTest::newRow("qobject, invalid major version fail")
            << TEST_FILE("moduleapi/moduleApiMajorVersionFail.qml")
            << QString("QDeclarativeComponent: Component is not ready")
            << QStringList()
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();

    QTest::newRow("qobject, invalid minor version fail")
            << TEST_FILE("moduleapi/moduleApiMinorVersionFail.qml")
            << QString("QDeclarativeComponent: Component is not ready")
            << QStringList()
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList()
            << QStringList()
            << QVariantList();
}

void tst_qdeclarativeecmascript::moduleApi()
{
    QFETCH(QUrl, testfile);
    QFETCH(QString, errorMessage);
    QFETCH(QStringList, warningMessages);
    QFETCH(QStringList, readProperties);
    QFETCH(QVariantList, readExpectedValues);
    QFETCH(QStringList, writeProperties);
    QFETCH(QVariantList, writeValues);
    QFETCH(QStringList, readBackProperties);
    QFETCH(QVariantList, readBackExpectedValues);

    QDeclarativeComponent component(&engine, testfile);

    if (!errorMessage.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, errorMessage.toAscii().constData());

    if (warningMessages.size())
        foreach (const QString &warning, warningMessages)
            QTest::ignoreMessage(QtWarningMsg, warning.toAscii().constData());

    QObject *object = component.create();
    if (!errorMessage.isEmpty()) {
        QVERIFY(object == 0);
    } else {
        QVERIFY(object != 0);
        for (int i = 0; i < readProperties.size(); ++i)
            QCOMPARE(object->property(readProperties.at(i).toAscii().constData()), readExpectedValues.at(i));
        for (int i = 0; i < writeProperties.size(); ++i)
            QVERIFY(object->setProperty(writeProperties.at(i).toAscii().constData(), writeValues.at(i)));
        for (int i = 0; i < readBackProperties.size(); ++i)
            QCOMPARE(object->property(readBackProperties.at(i).toAscii().constData()), readBackExpectedValues.at(i));
        delete object;
    }
}

void tst_qdeclarativeecmascript::importScripts()
{
    QObject *object = 0;

    // first, ensure that the required behaviour works.
    QDeclarativeComponent component(&engine, TEST_FILE("jsimport/testImport.qml"));
    object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("importedScriptStringValue"), QVariant(QString(QLatin1String("Hello, World!"))));
    QCOMPARE(object->property("importedScriptFunctionValue"), QVariant(20));
    QCOMPARE(object->property("importedModuleAttachedPropertyValue"), QVariant(19));
    QCOMPARE(object->property("importedModuleEnumValue"), QVariant(2));
    delete object;

    QDeclarativeComponent componentTwo(&engine, TEST_FILE("jsimport/testImportScoping.qml"));
    object = componentTwo.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("componentError"), QVariant(5));
    delete object;

    // then, ensure that unintended behaviour does not work.
    QDeclarativeComponent failOneComponent(&engine, TEST_FILE("jsimportfail/failOne.qml"));
    QString expectedWarning = QLatin1String("file://") + TEST_FILE("jsimportfail/failOne.qml").toLocalFile() + QLatin1String(":6: TypeError: Cannot call method 'greetingString' of undefined");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toAscii().constData());
    object = failOneComponent.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("importScriptFunctionValue").toString().isEmpty());
    delete object;
    QDeclarativeComponent failTwoComponent(&engine, TEST_FILE("jsimportfail/failTwo.qml"));
    expectedWarning = QLatin1String("file://") + TEST_FILE("jsimportfail/failTwo.qml").toLocalFile() + QLatin1String(":6: ReferenceError: Can't find variable: ImportOneJs");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toAscii().constData());
    object = failTwoComponent.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("importScriptFunctionValue").toString().isEmpty());
    delete object;
    QDeclarativeComponent failThreeComponent(&engine, TEST_FILE("jsimportfail/failThree.qml"));
    expectedWarning = QLatin1String("file://") + TEST_FILE("jsimportfail/failThree.qml").toLocalFile() + QLatin1String(":7: TypeError: Cannot read property 'JsQtTest' of undefined");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toAscii().constData());
    object = failThreeComponent.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("importedModuleAttachedPropertyValue"), QVariant(false));
    delete object;
    QDeclarativeComponent failFourComponent(&engine, TEST_FILE("jsimportfail/failFour.qml"));
    expectedWarning = QLatin1String("file://") + TEST_FILE("jsimportfail/failFour.qml").toLocalFile() + QLatin1String(":6: ReferenceError: Can't find variable: JsQtTest");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toAscii().constData());
    object = failFourComponent.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("importedModuleEnumValue"), QVariant(0));
    delete object;
    QDeclarativeComponent failFiveComponent(&engine, TEST_FILE("jsimportfail/failFive.qml"));
    expectedWarning = QLatin1String("file://") + TEST_FILE("jsimportfail/importWithImports.js").toLocalFile() + QLatin1String(":8: ReferenceError: Can't find variable: Component");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toAscii().constData());
    expectedWarning = QLatin1String("file://") + TEST_FILE("jsimportfail/importPragmaLibrary.js").toLocalFile() + QLatin1String(":6: ReferenceError: Can't find variable: Component");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toAscii().constData());
    object = failFiveComponent.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("componentError"), QVariant(0));
    delete object;

    // also, test that importing scripts with .pragma library works as required
    QDeclarativeComponent pragmaLibraryComponent(&engine, TEST_FILE("jsimport/testImportPragmaLibrary.qml"));
    object = pragmaLibraryComponent.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("testValue"), QVariant(31));
    delete object;

    // and that .pragma library scripts don't inherit imports from any .qml file
    QDeclarativeComponent pragmaLibraryComponentTwo(&engine, TEST_FILE("jsimportfail/testImportPragmaLibrary.qml"));
    object = pragmaLibraryComponentTwo.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("testValue"), QVariant(0));
    delete object;
}

void tst_qdeclarativeecmascript::scarceResources()
{
    QPixmap origPixmap(100, 100);
    origPixmap.fill(Qt::blue);

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(&engine);
    ScarceResourceObject *eo = 0;
    QObject *object = 0;

    // in the following three cases, the instance created from the component
    // has a property which is a copy of the scarce resource; hence, the
    // resource should NOT be detached prior to deletion of the object instance,
    // unless the resource is destroyed explicitly.
    QDeclarativeComponent component(&engine, TEST_FILE("scarceresources/scarceResourceCopy.qml"));
    object = component.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("scarceResourceCopy").isValid());
    QCOMPARE(object->property("scarceResourceCopy").value<QPixmap>(), origPixmap);
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(!eo->scarceResourceIsDetached()); // there are two copies of it in existence: the property of object, and the property of eo.
    delete object;

    QDeclarativeComponent componentTwo(&engine, TEST_FILE("scarceresources/scarceResourceCopyFromJs.qml"));
    object = componentTwo.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("scarceResourceCopy").isValid());
    QCOMPARE(object->property("scarceResourceCopy").value<QPixmap>(), origPixmap);
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(!eo->scarceResourceIsDetached()); // there are two copies of it in existence: the property of object, and the property of eo.
    delete object;

    QDeclarativeComponent componentThree(&engine, TEST_FILE("scarceresources/scarceResourceDestroyedCopy.qml"));
    object = componentThree.create();
    QVERIFY(object != 0);
    QVERIFY(!(object->property("scarceResourceCopy").isValid())); // was manually released prior to being returned.
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should have explicitly been released during the evaluation of the binding.
    delete object;

    // in the following three cases, no other copy should exist in memory,
    // and so it should be detached (unless explicitly preserved).
    QDeclarativeComponent componentFour(&engine, TEST_FILE("scarceresources/scarceResourceTest.qml"));
    object = componentFour.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("scarceResourceTest").isValid());
    QCOMPARE(object->property("scarceResourceTest").toInt(), 100);
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // the resource should have been released after the binding was evaluated.
    delete object;

    QDeclarativeComponent componentFive(&engine, TEST_FILE("scarceresources/scarceResourceTestPreserve.qml"));
    object = componentFive.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("scarceResourceTest").isValid());
    QCOMPARE(object->property("scarceResourceTest").toInt(), 100);
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(!eo->scarceResourceIsDetached()); // this won't be detached since we explicitly preserved it.
    delete object;

    QDeclarativeComponent componentSix(&engine, TEST_FILE("scarceresources/scarceResourceTestMultiple.qml"));
    object = componentSix.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("scarceResourceTest").isValid());
    QCOMPARE(object->property("scarceResourceTest").toInt(), 100);
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // all resources were released manually or automatically released.
    delete object;

    // test that scarce resources are handled correctly for imports
    QDeclarativeComponent componentSeven(&engine, TEST_FILE("scarceresources/scarceResourceCopyImportNoBinding.qml"));
    object = componentSeven.create();
    QVERIFY(object != 0); // the import should have caused the addition of a resource to the ScarceResources list
    QVERIFY(ep->scarceResources.isEmpty()); // but they should have been released by this point.
    delete object;

    QDeclarativeComponent componentEight(&engine, TEST_FILE("scarceresources/scarceResourceCopyImportFail.qml"));
    object = componentEight.create();
    QVERIFY(object != 0);
    QVERIFY(!object->property("scarceResourceCopy").isValid()); // wasn't preserved, so shouldn't be valid.
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    delete object;

    QDeclarativeComponent componentNine(&engine, TEST_FILE("scarceresources/scarceResourceCopyImport.qml"));
    object = componentNine.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("scarceResourceCopy").isValid()); // preserved, so should be valid.
    QCOMPARE(object->property("scarceResourceCopy").value<QPixmap>(), origPixmap);
    QVERIFY(object->property("scarceResourceAssignedCopyOne").isValid()); // assigned before destroy(), so should be valid.
    QCOMPARE(object->property("scarceResourceAssignedCopyOne").value<QPixmap>(), origPixmap);
    QVERIFY(!object->property("scarceResourceAssignedCopyTwo").isValid()); // assigned after destroy(), so should be invalid.
    QVERIFY(ep->scarceResources.isEmpty()); // this will still be zero, because "preserve()" REMOVES it from this list.
    delete object;

    // test that scarce resources are handled properly in signal invocation
    QDeclarativeComponent componentTen(&engine, TEST_FILE("scarceresources/scarceResourceSignal.qml"));
    object = componentTen.create();
    QVERIFY(object != 0);
    QObject *srsc = object->findChild<QObject*>("srsc");
    QVERIFY(srsc);
    QVERIFY(!srsc->property("scarceResourceCopy").isValid()); // hasn't been instantiated yet.
    QCOMPARE(srsc->property("width"), QVariant(5)); // default value is 5.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should be no other copies of it at this stage.
    QMetaObject::invokeMethod(srsc, "testSignal");
    QVERIFY(!srsc->property("scarceResourceCopy").isValid()); // still hasn't been instantiated
    QCOMPARE(srsc->property("width"), QVariant(10)); // but width was assigned to 10.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should still be no other copies of it at this stage.
    QMetaObject::invokeMethod(srsc, "testSignal2"); // assigns scarceResourceCopy to the scarce pixmap.
    QVERIFY(srsc->property("scarceResourceCopy").isValid());
    QCOMPARE(srsc->property("scarceResourceCopy").value<QPixmap>(), origPixmap);
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(!(eo->scarceResourceIsDetached())); // should be another copy of the resource now.
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    delete object;

    // test that scarce resources are handled properly from js functions in qml files
    QDeclarativeComponent componentEleven(&engine, TEST_FILE("scarceresources/scarceResourceFunction.qml"));
    object = componentEleven.create();
    QVERIFY(object != 0);
    QVERIFY(!object->property("scarceResourceCopy").isValid()); // not yet assigned, so should not be valid
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should be no other copies of it at this stage.
    QMetaObject::invokeMethod(object, "retrieveScarceResource");
    QVERIFY(object->property("scarceResourceCopy").isValid()); // assigned, so should be valid.
    QCOMPARE(object->property("scarceResourceCopy").value<QPixmap>(), origPixmap);
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(!eo->scarceResourceIsDetached()); // should be a copy of the resource at this stage.
    QMetaObject::invokeMethod(object, "releaseScarceResource");
    QVERIFY(!object->property("scarceResourceCopy").isValid()); // just released, so should not be valid
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should be no other copies of it at this stage.
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    delete object;

    // test that if an exception occurs while invoking js function from cpp, that the resources are released.
    QDeclarativeComponent componentTwelve(&engine, TEST_FILE("scarceresources/scarceResourceFunctionFail.qml"));
    object = componentTwelve.create();
    QVERIFY(object != 0);
    QVERIFY(!object->property("scarceResourceCopy").isValid()); // not yet assigned, so should not be valid
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should be no other copies of it at this stage.
    QString srp_name = object->property("srp_name").toString();
    QString expectedWarning = componentTwelve.url().toString() + QLatin1String(":17: TypeError: Property 'scarceResource' of object ") + srp_name + QLatin1String(" is not a function");
    QTest::ignoreMessage(QtWarningMsg, qPrintable(expectedWarning)); // we expect a meaningful warning to be printed.
    QMetaObject::invokeMethod(object, "retrieveScarceResource");
    QVERIFY(!object->property("scarceResourceCopy").isValid()); // due to exception, assignment will NOT have occurred.
    eo = qobject_cast<ScarceResourceObject*>(QDeclarativeProperty::read(object, "a").value<QObject*>());
    QVERIFY(eo->scarceResourceIsDetached()); // should be no other copies of it at this stage.
    QVERIFY(ep->scarceResources.isEmpty()); // should have been released by this point.
    delete object;
}

void tst_qdeclarativeecmascript::propertyChangeSlots()
{
    // ensure that allowable property names are allowed and onPropertyNameChanged slots are generated correctly.
    QDeclarativeComponent component(&engine, TEST_FILE("changeslots/propertyChangeSlots.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;

    // ensure that invalid property names fail properly.
    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
    QDeclarativeComponent e1(&engine, TEST_FILE("changeslots/propertyChangeSlotErrors.1.qml"));
    QString expectedErrorString = e1.url().toString() + QLatin1String(":9:5: Cannot assign to non-existent property \"on_nameWithUnderscoreChanged\"");
    QCOMPARE(e1.errors().at(0).toString(), expectedErrorString);
    object = e1.create();
    QVERIFY(object == 0);
    delete object;

    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
    QDeclarativeComponent e2(&engine, TEST_FILE("changeslots/propertyChangeSlotErrors.2.qml"));
    expectedErrorString = e2.url().toString() + QLatin1String(":9:5: Cannot assign to non-existent property \"on____nameWithUnderscoresChanged\"");
    QCOMPARE(e2.errors().at(0).toString(), expectedErrorString);
    object = e2.create();
    QVERIFY(object == 0);
    delete object;

    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
    QDeclarativeComponent e3(&engine, TEST_FILE("changeslots/propertyChangeSlotErrors.3.qml"));
    expectedErrorString = e3.url().toString() + QLatin1String(":9:5: Cannot assign to non-existent property \"on$NameWithDollarsignChanged\"");
    QCOMPARE(e3.errors().at(0).toString(), expectedErrorString);
    object = e3.create();
    QVERIFY(object == 0);
    delete object;

    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
    QDeclarativeComponent e4(&engine, TEST_FILE("changeslots/propertyChangeSlotErrors.4.qml"));
    expectedErrorString = e4.url().toString() + QLatin1String(":9:5: Cannot assign to non-existent property \"on_6NameWithUnderscoreNumberChanged\"");
    QCOMPARE(e4.errors().at(0).toString(), expectedErrorString);
    object = e4.create();
    QVERIFY(object == 0);
    delete object;
}

// Ensure that QObject type conversion works on binding assignment
void tst_qdeclarativeecmascript::elementAssign()
{
    QDeclarativeComponent component(&engine, TEST_FILE("elementAssign.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test").toBool(), true);

    delete object;
}

// QTBUG-12457
void tst_qdeclarativeecmascript::objectPassThroughSignals()
{
    QDeclarativeComponent component(&engine, TEST_FILE("objectsPassThroughSignals.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test").toBool(), true);

    delete object;
}

// QTBUG-21626
void tst_qdeclarativeecmascript::objectConversion()
{
    QDeclarativeComponent component(&engine, TEST_FILE("objectConversion.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);
    QVariant retn;
    QMetaObject::invokeMethod(object, "circularObject", Q_RETURN_ARG(QVariant, retn));
    QCOMPARE(retn.value<QVariantMap>().value("test"), QVariant(100));

    delete object;
}


// QTBUG-20242
void tst_qdeclarativeecmascript::booleanConversion()
{
    QDeclarativeComponent component(&engine, TEST_FILE("booleanConversion.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test_true1").toBool(), true);
    QCOMPARE(object->property("test_true2").toBool(), true);
    QCOMPARE(object->property("test_true3").toBool(), true);
    QCOMPARE(object->property("test_true4").toBool(), true);
    QCOMPARE(object->property("test_true5").toBool(), true);

    QCOMPARE(object->property("test_false1").toBool(), false);
    QCOMPARE(object->property("test_false2").toBool(), false);
    QCOMPARE(object->property("test_false3").toBool(), false);

    delete object;
}

void tst_qdeclarativeecmascript::handleReferenceManagement()
{

    int dtorCount = 0;
    {
        // Linear QObject reference
        QDeclarativeEngine hrmEngine;
        QDeclarativeComponent component(&hrmEngine, TEST_FILE("handleReferenceManagement.object.1.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        CircularReferenceObject *cro = object->findChild<CircularReferenceObject*>("cro");
        cro->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object, "createReference");
        QMetaObject::invokeMethod(object, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 0); // second has JS ownership, kept alive by first's reference
        delete object;
        hrmEngine.collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 3);
    }

    dtorCount = 0;
    {
        // Circular QObject reference
        QDeclarativeEngine hrmEngine;
        QDeclarativeComponent component(&hrmEngine, TEST_FILE("handleReferenceManagement.object.2.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        CircularReferenceObject *cro = object->findChild<CircularReferenceObject*>("cro");
        cro->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object, "circularReference");
        QMetaObject::invokeMethod(object, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 2); // both should be cleaned up, since circular references shouldn't keep alive.
        delete object;
        hrmEngine.collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 3);
    }

    dtorCount = 0;
    {
        // Linear handle reference
        QDeclarativeEngine hrmEngine;
        QDeclarativeComponent component(&hrmEngine, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        CircularReferenceHandle *crh = object->findChild<CircularReferenceHandle*>("crh");
        QVERIFY(crh != 0);
        crh->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object, "createReference");
        CircularReferenceHandle *first = object->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second = object->property("second").value<CircularReferenceHandle*>();
        QVERIFY(first != 0);
        QVERIFY(second != 0);
        first->addReference(QDeclarativeData::get(second)->v8object); // create reference
        // now we have to reparent second and make second owned by JS.
        second->setParent(0);
        QDeclarativeEngine::setObjectOwnership(second, QDeclarativeEngine::JavaScriptOwnership);
        QMetaObject::invokeMethod(object, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 0); // due to reference from first to second, second shouldn't be collected.
        delete object;
        hrmEngine.collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 3);
    }

    dtorCount = 0;
    {
        // Circular handle reference
        QDeclarativeEngine hrmEngine;
        QDeclarativeComponent component(&hrmEngine, TEST_FILE("handleReferenceManagement.handle.2.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        CircularReferenceHandle *crh = object->findChild<CircularReferenceHandle*>("crh");
        QVERIFY(crh != 0);
        crh->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object, "circularReference");
        CircularReferenceHandle *first = object->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second = object->property("second").value<CircularReferenceHandle*>();
        QVERIFY(first != 0);
        QVERIFY(second != 0);
        first->addReference(QDeclarativeData::get(second)->v8object); // create circular reference
        second->addReference(QDeclarativeData::get(first)->v8object); // note: must be weak.
        // now we have to reparent and change ownership.
        first->setParent(0);
        second->setParent(0);
        QDeclarativeEngine::setObjectOwnership(first, QDeclarativeEngine::JavaScriptOwnership);
        QDeclarativeEngine::setObjectOwnership(second, QDeclarativeEngine::JavaScriptOwnership);
        QMetaObject::invokeMethod(object, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 2); // despite circular references, both will be collected.
        delete object;
        hrmEngine.collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 3);
    }

    dtorCount = 0;
    {
        // multiple engine interaction - linear reference
        QDeclarativeEngine hrmEngine1;
        QDeclarativeEngine hrmEngine2;
        QDeclarativeComponent component1(&hrmEngine1, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QDeclarativeComponent component2(&hrmEngine2, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QObject *object1 = component1.create();
        QObject *object2 = component2.create();
        QVERIFY(object1 != 0);
        QVERIFY(object2 != 0);
        CircularReferenceHandle *crh1 = object1->findChild<CircularReferenceHandle*>("crh");
        CircularReferenceHandle *crh2 = object2->findChild<CircularReferenceHandle*>("crh");
        QVERIFY(crh1 != 0);
        QVERIFY(crh2 != 0);
        crh1->setDtorCount(&dtorCount);
        crh2->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object1, "createReference");
        QMetaObject::invokeMethod(object2, "createReference");
        CircularReferenceHandle *first1 = object1->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second1 = object1->property("second").value<CircularReferenceHandle*>();
        CircularReferenceHandle *first2 = object2->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second2 = object2->property("second").value<CircularReferenceHandle*>();
        QVERIFY(first1 != 0);
        QVERIFY(second1 != 0);
        QVERIFY(first2 != 0);
        QVERIFY(second2 != 0);
        first1->addReference(QDeclarativeData::get(second2)->v8object); // create reference across engines
        // now we have to reparent second2 and make second2 owned by JS.
        second2->setParent(0);
        QDeclarativeEngine::setObjectOwnership(second2, QDeclarativeEngine::JavaScriptOwnership);
        QMetaObject::invokeMethod(object1, "performGc");
        QMetaObject::invokeMethod(object2, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 0); // due to reference from first1 to second2, second2 shouldn't be collected.
        delete object1;
        delete object2;
        hrmEngine1.collectGarbage();
        hrmEngine2.collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 6);
    }

    dtorCount = 0;
    {
        // multiple engine interaction - circular reference
        QDeclarativeEngine hrmEngine1;
        QDeclarativeEngine hrmEngine2;
        QDeclarativeComponent component1(&hrmEngine1, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QDeclarativeComponent component2(&hrmEngine2, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QObject *object1 = component1.create();
        QObject *object2 = component2.create();
        QVERIFY(object1 != 0);
        QVERIFY(object2 != 0);
        CircularReferenceHandle *crh1 = object1->findChild<CircularReferenceHandle*>("crh");
        CircularReferenceHandle *crh2 = object2->findChild<CircularReferenceHandle*>("crh");
        QVERIFY(crh1 != 0);
        QVERIFY(crh2 != 0);
        crh1->setDtorCount(&dtorCount);
        crh2->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object1, "createReference");
        QMetaObject::invokeMethod(object2, "createReference");
        CircularReferenceHandle *first1 = object1->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second1 = object1->property("second").value<CircularReferenceHandle*>();
        CircularReferenceHandle *first2 = object2->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second2 = object2->property("second").value<CircularReferenceHandle*>();
        QVERIFY(first1 != 0);
        QVERIFY(second1 != 0);
        QVERIFY(first2 != 0);
        QVERIFY(second2 != 0);
        first1->addReference(QDeclarativeData::get(second1)->v8object);  // create linear reference within engine1
        second1->addReference(QDeclarativeData::get(second2)->v8object); // create linear reference across engines
        second2->addReference(QDeclarativeData::get(first2)->v8object);  // create linear reference within engine2
        first2->addReference(QDeclarativeData::get(first1)->v8object);   // close the loop - circular ref across engines
        // now we have to reparent and change ownership to JS.
        first1->setParent(0);
        second1->setParent(0);
        first2->setParent(0);
        second2->setParent(0);
        QDeclarativeEngine::setObjectOwnership(first1, QDeclarativeEngine::JavaScriptOwnership);
        QDeclarativeEngine::setObjectOwnership(second1, QDeclarativeEngine::JavaScriptOwnership);
        QDeclarativeEngine::setObjectOwnership(first2, QDeclarativeEngine::JavaScriptOwnership);
        QDeclarativeEngine::setObjectOwnership(second2, QDeclarativeEngine::JavaScriptOwnership);
        QMetaObject::invokeMethod(object1, "performGc");
        QMetaObject::invokeMethod(object2, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 4); // circular references shouldn't keep them alive.
        delete object1;
        delete object2;
        hrmEngine1.collectGarbage();
        hrmEngine2.collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 6);
    }

    dtorCount = 0;
    {
        // multiple engine interaction - linear reference with engine deletion
        QDeclarativeEngine *hrmEngine1 = new QDeclarativeEngine;
        QDeclarativeEngine *hrmEngine2 = new QDeclarativeEngine;
        QDeclarativeComponent component1(hrmEngine1, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QDeclarativeComponent component2(hrmEngine2, TEST_FILE("handleReferenceManagement.handle.1.qml"));
        QObject *object1 = component1.create();
        QObject *object2 = component2.create();
        QVERIFY(object1 != 0);
        QVERIFY(object2 != 0);
        CircularReferenceHandle *crh1 = object1->findChild<CircularReferenceHandle*>("crh");
        CircularReferenceHandle *crh2 = object2->findChild<CircularReferenceHandle*>("crh");
        QVERIFY(crh1 != 0);
        QVERIFY(crh2 != 0);
        crh1->setDtorCount(&dtorCount);
        crh2->setDtorCount(&dtorCount);
        QMetaObject::invokeMethod(object1, "createReference");
        QMetaObject::invokeMethod(object2, "createReference");
        CircularReferenceHandle *first1 = object1->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second1 = object1->property("second").value<CircularReferenceHandle*>();
        CircularReferenceHandle *first2 = object2->property("first").value<CircularReferenceHandle*>();
        CircularReferenceHandle *second2 = object2->property("second").value<CircularReferenceHandle*>();
        QVERIFY(first1 != 0);
        QVERIFY(second1 != 0);
        QVERIFY(first2 != 0);
        QVERIFY(second2 != 0);
        first1->addReference(QDeclarativeData::get(second1)->v8object);  // create linear reference within engine1
        second1->addReference(QDeclarativeData::get(second2)->v8object); // create linear reference across engines
        second2->addReference(QDeclarativeData::get(first2)->v8object);  // create linear reference within engine2
        // now we have to reparent and change ownership to JS.
        first1->setParent(crh1);
        second1->setParent(0);
        first2->setParent(0);
        second2->setParent(0);
        QDeclarativeEngine::setObjectOwnership(second1, QDeclarativeEngine::JavaScriptOwnership);
        QDeclarativeEngine::setObjectOwnership(first2, QDeclarativeEngine::JavaScriptOwnership);
        QDeclarativeEngine::setObjectOwnership(second2, QDeclarativeEngine::JavaScriptOwnership);
        QMetaObject::invokeMethod(object1, "performGc");
        QMetaObject::invokeMethod(object2, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 0);
        delete hrmEngine2;
        QMetaObject::invokeMethod(object1, "performGc");
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 0);
        delete object1;
        delete object2;
        hrmEngine1->collectGarbage();
        QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
        QCOMPARE(dtorCount, 6);
        delete hrmEngine1;
    }
}

void tst_qdeclarativeecmascript::stringArg()
{
    QDeclarativeComponent component(&engine, TEST_FILE("stringArg.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    QMetaObject::invokeMethod(object, "success");
    QVERIFY(object->property("returnValue").toBool());

    QString w1 = TEST_FILE("stringArg.qml").toString() + QLatin1String(":45: Error: String.arg(): Invalid arguments");
    QTest::ignoreMessage(QtWarningMsg, w1.toAscii().constData());
    QMetaObject::invokeMethod(object, "failure");
    QVERIFY(object->property("returnValue").toBool());

    delete object;
}

// Test that assigning a null object works 
// Regressed with: df1788b4dbbb2826ae63f26bdf166342595343f4
void tst_qdeclarativeecmascript::nullObjectBinding()
{
    QDeclarativeComponent component(&engine, TEST_FILE("nullObjectBinding.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QVERIFY(object->property("test") == QVariant::fromValue((QObject *)0));

    delete object;
}

// Test that bindings don't evaluate once the engine has been destroyed
void tst_qdeclarativeecmascript::deletedEngine()
{
    QDeclarativeEngine *engine = new QDeclarativeEngine;
    QDeclarativeComponent component(engine, TEST_FILE("deletedEngine.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("a").toInt(), 39);
    object->setProperty("b", QVariant(9));
    QCOMPARE(object->property("a").toInt(), 117);

    delete engine;

    QCOMPARE(object->property("a").toInt(), 117);
    object->setProperty("b", QVariant(10));
    QCOMPARE(object->property("a").toInt(), 117);

    delete object;
}

// Test the crashing part of QTBUG-9705
void tst_qdeclarativeecmascript::libraryScriptAssert()
{
    QDeclarativeComponent component(&engine, TEST_FILE("libraryScriptAssert.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    delete object;
}

void tst_qdeclarativeecmascript::variantsAssignedUndefined()
{
    QDeclarativeComponent component(&engine, TEST_FILE("variantsAssignedUndefined.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toInt(), 10);
    QCOMPARE(object->property("test2").toInt(), 11);

    object->setProperty("runTest", true);

    QCOMPARE(object->property("test1"), QVariant());
    QCOMPARE(object->property("test2"), QVariant());


    delete object;
}

void tst_qdeclarativeecmascript::qtbug_9792()
{
    QDeclarativeComponent component(&engine, TEST_FILE("qtbug_9792.qml"));

    QDeclarativeContext *context = new QDeclarativeContext(engine.rootContext());

    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create(context));
    QVERIFY(object != 0);

    QTest::ignoreMessage(QtDebugMsg, "Hello world!");
    object->basicSignal();

    delete context;

    transientErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(transientErrorsMsgHandler);

    object->basicSignal();
    
    qInstallMsgHandler(old);

    QCOMPARE(transientErrorsMsgCount, 0);

    delete object;
}

// Verifies that QDeclarativeGuard<>s used in the vmemetaobject are cleaned correctly
void tst_qdeclarativeecmascript::qtcreatorbug_1289()
{
    QDeclarativeComponent component(&engine, TEST_FILE("qtcreatorbug_1289.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QObject *nested = qvariant_cast<QObject *>(o->property("object"));
    QVERIFY(nested != 0);

    QVERIFY(qvariant_cast<QObject *>(nested->property("nestedObject")) == o);

    delete nested;
    nested = qvariant_cast<QObject *>(o->property("object"));
    QVERIFY(nested == 0);

    // If the bug is present, the next line will crash
    delete o;
}

// Test that we shut down without stupid warnings
void tst_qdeclarativeecmascript::noSpuriousWarningsAtShutdown()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("noSpuriousWarningsAtShutdown.qml"));

    QObject *o = component.create();

    transientErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(transientErrorsMsgHandler);

    delete o;

    qInstallMsgHandler(old);

    QCOMPARE(transientErrorsMsgCount, 0);
    }


    {
    QDeclarativeComponent component(&engine, TEST_FILE("noSpuriousWarningsAtShutdown.2.qml"));

    QObject *o = component.create();

    transientErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(transientErrorsMsgHandler);

    delete o;

    qInstallMsgHandler(old);

    QCOMPARE(transientErrorsMsgCount, 0);
    }
}

void tst_qdeclarativeecmascript::canAssignNullToQObject()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("canAssignNullToQObject.1.qml"));

    MyQmlObject *o = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(o != 0);

    QVERIFY(o->objectProperty() != 0);

    o->setProperty("runTest", true);

    QVERIFY(o->objectProperty() == 0);

    delete o;
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("canAssignNullToQObject.2.qml"));

    MyQmlObject *o = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(o != 0);

    QVERIFY(o->objectProperty() == 0);

    delete o;
    }
}

void tst_qdeclarativeecmascript::functionAssignment_fromBinding()
{
    QDeclarativeComponent component(&engine, TEST_FILE("functionAssignment.1.qml"));

    QString url = component.url().toString();
    QString warning = url + ":4: Unable to assign a function to a property.";
    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    
    MyQmlObject *o = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(o != 0);

    QVERIFY(!o->property("a").isValid());

    delete o;
}

void tst_qdeclarativeecmascript::functionAssignment_fromJS()
{
    QFETCH(QString, triggerProperty);

    QDeclarativeComponent component(&engine, TEST_FILE("functionAssignment.2.qml"));
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));

    MyQmlObject *o = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(o != 0);
    QVERIFY(!o->property("a").isValid());

    o->setProperty("aNumber", QVariant(5));
    o->setProperty(triggerProperty.toUtf8().constData(), true);
    QCOMPARE(o->property("a"), QVariant(50));

    o->setProperty("aNumber", QVariant(10));
    QCOMPARE(o->property("a"), QVariant(100));

    delete o;
}

void tst_qdeclarativeecmascript::functionAssignment_fromJS_data()
{
    QTest::addColumn<QString>("triggerProperty");

    QTest::newRow("assign to property") << "assignToProperty";
    QTest::newRow("assign to property, from JS file") << "assignToPropertyFromJsFile";

    QTest::newRow("assign to value type") << "assignToValueType";

    QTest::newRow("use 'this'") << "assignWithThis";
    QTest::newRow("use 'this' from JS file") << "assignWithThisFromJsFile";
}

void tst_qdeclarativeecmascript::functionAssignmentfromJS_invalid()
{
    QDeclarativeComponent component(&engine, TEST_FILE("functionAssignment.2.qml"));
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));

    MyQmlObject *o = qobject_cast<MyQmlObject *>(component.create());
    QVERIFY(o != 0);
    QVERIFY(!o->property("a").isValid());

    o->setProperty("assignFuncWithoutReturn", true);
    QVERIFY(!o->property("a").isValid());

    QString url = component.url().toString();
    QString warning = url + ":67: Unable to assign QString to int";
    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    o->setProperty("assignWrongType", true);

    warning = url + ":71: Unable to assign QString to int";
    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());
    o->setProperty("assignWrongTypeToValueType", true);

    delete o;
}

void tst_qdeclarativeecmascript::eval()
{
    QDeclarativeComponent component(&engine, TEST_FILE("eval.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);

    delete o;
}

void tst_qdeclarativeecmascript::function()
{
    QDeclarativeComponent component(&engine, TEST_FILE("function.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);

    delete o;
}

// Test the "Qt.include" method
void tst_qdeclarativeecmascript::include()
{
    // Non-library relative include
    {
    QDeclarativeComponent component(&engine, TEST_FILE("include.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test0").toInt(), 99);
    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test2_1").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test3_1").toBool(), true);

    delete o;
    }

    // Library relative include
    {
    QDeclarativeComponent component(&engine, TEST_FILE("include_shared.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test0").toInt(), 99);
    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test2_1").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test3_1").toBool(), true);

    delete o;
    }

    // Callback
    {
    QDeclarativeComponent component(&engine, TEST_FILE("include_callback.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toBool(), true);

    delete o;
    }

    // Including file with ".pragma library"
    {
    QDeclarativeComponent component(&engine, TEST_FILE("include_pragma.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test1").toInt(), 100);

    delete o;
    }

    // Remote - success
    {
    TestHTTPServer server(8111);
    QVERIFY(server.isValid());
    server.serveDirectory(SRCDIR "/data");

    QDeclarativeComponent component(&engine, TEST_FILE("include_remote.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QTRY_VERIFY(o->property("done").toBool() == true);
    QTRY_VERIFY(o->property("done2").toBool() == true);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);

    QCOMPARE(o->property("test6").toBool(), true);
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);

    delete o;
    }

    // Remote - error
    {
    TestHTTPServer server(8111);
    QVERIFY(server.isValid());
    server.serveDirectory(SRCDIR "/data");

    QDeclarativeComponent component(&engine, TEST_FILE("include_remote_missing.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QTRY_VERIFY(o->property("done").toBool() == true);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);

    delete o;
    }
}

void tst_qdeclarativeecmascript::signalHandlers()
{
    QDeclarativeComponent component(&engine, TEST_FILE("signalHandlers.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QVERIFY(o->property("count").toInt() == 0);
    QMetaObject::invokeMethod(o, "testSignalCall");
    QCOMPARE(o->property("count").toInt(), 1);

    QMetaObject::invokeMethod(o, "testSignalHandlerCall");
    QCOMPARE(o->property("count").toInt(), 1);
    QCOMPARE(o->property("errorString").toString(), QLatin1String("TypeError: Property 'onTestSignal' of object [object Object] is not a function"));

    QVERIFY(o->property("funcCount").toInt() == 0);
    QMetaObject::invokeMethod(o, "testSignalConnection");
    QCOMPARE(o->property("funcCount").toInt(), 1);

    QMetaObject::invokeMethod(o, "testSignalHandlerConnection");
    QCOMPARE(o->property("funcCount").toInt(), 2);

    QMetaObject::invokeMethod(o, "testSignalDefined");
    QCOMPARE(o->property("definedResult").toBool(), true);

    QMetaObject::invokeMethod(o, "testSignalHandlerDefined");
    QCOMPARE(o->property("definedHandlerResult").toBool(), true);

    delete o;
}

void tst_qdeclarativeecmascript::qtbug_10696()
{
    QDeclarativeComponent component(&engine, TEST_FILE("qtbug_10696.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    delete o;
}

void tst_qdeclarativeecmascript::qtbug_11606()
{
    QDeclarativeComponent component(&engine, TEST_FILE("qtbug_11606.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
}

void tst_qdeclarativeecmascript::qtbug_11600()
{
    QDeclarativeComponent component(&engine, TEST_FILE("qtbug_11600.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
}

// Reading and writing non-scriptable properties should fail
void tst_qdeclarativeecmascript::nonscriptable()
{
    QDeclarativeComponent component(&engine, TEST_FILE("nonscriptable.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("readOk").toBool(), true);
    QCOMPARE(o->property("writeOk").toBool(), true);
    delete o;
}

// deleteLater() should not be callable from QML
void tst_qdeclarativeecmascript::deleteLater()
{
    QDeclarativeComponent component(&engine, TEST_FILE("deleteLater.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
}

void tst_qdeclarativeecmascript::in()
{
    QDeclarativeComponent component(&engine, TEST_FILE("in.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    delete o;
}

void tst_qdeclarativeecmascript::sharedAttachedObject()
{
    QDeclarativeComponent component(&engine, TEST_FILE("sharedAttachedObject.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    delete o;
}

// QTBUG-13999
void tst_qdeclarativeecmascript::objectName()
{
    QDeclarativeComponent component(&engine, TEST_FILE("objectName.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toString(), QString("hello"));
    QCOMPARE(o->property("test2").toString(), QString("ell"));

    o->setObjectName("world");

    QCOMPARE(o->property("test1").toString(), QString("world"));
    QCOMPARE(o->property("test2").toString(), QString("orl"));

    delete o;
}

void tst_qdeclarativeecmascript::writeRemovesBinding()
{
    QDeclarativeComponent component(&engine, TEST_FILE("writeRemovesBinding.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
}

// Test bindings assigned to alias properties actually assign to the alias' target
void tst_qdeclarativeecmascript::aliasBindingsAssignCorrectly()
{
    QDeclarativeComponent component(&engine, TEST_FILE("aliasBindingsAssignCorrectly.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
}

// Test bindings assigned to alias properties override a binding on the target (QTBUG-13719)
void tst_qdeclarativeecmascript::aliasBindingsOverrideTarget()
{
    { 
    QDeclarativeComponent component(&engine, TEST_FILE("aliasBindingsOverrideTarget.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("aliasBindingsOverrideTarget.2.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("aliasBindingsOverrideTarget.3.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
    }
}

// Test that writes to alias properties override bindings on the alias target (QTBUG-13719)
void tst_qdeclarativeecmascript::aliasWritesOverrideBindings()
{
    {
    QDeclarativeComponent component(&engine, TEST_FILE("aliasWritesOverrideBindings.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("aliasWritesOverrideBindings.2.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("aliasWritesOverrideBindings.3.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
    }
}

// Allow an alais to a composite element
// QTBUG-20200
void tst_qdeclarativeecmascript::aliasToCompositeElement()
{
    QDeclarativeComponent component(&engine, TEST_FILE("aliasToCompositeElement.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    delete object;
}

void tst_qdeclarativeecmascript::revisionErrors()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevisionErrors.qml"));
        QString url = component.url().toString();

        QString warning1 = url + ":8: ReferenceError: Can't find variable: prop2";
        QString warning2 = url + ":11: ReferenceError: Can't find variable: prop2";
        QString warning3 = url + ":13: ReferenceError: Can't find variable: method2";

        QTest::ignoreMessage(QtWarningMsg, warning1.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning2.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning3.toLatin1().constData());
        MyRevisionedClass *object = qobject_cast<MyRevisionedClass *>(component.create());
        QVERIFY(object != 0);
        delete object;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevisionErrors2.qml"));
        QString url = component.url().toString();

        // MyRevisionedSubclass 1.0 uses MyRevisionedClass revision 0
        // method2, prop2 from MyRevisionedClass not available
        // method4, prop4 from MyRevisionedSubclass not available
        QString warning1 = url + ":8: ReferenceError: Can't find variable: prop2";
        QString warning2 = url + ":14: ReferenceError: Can't find variable: prop2";
        QString warning3 = url + ":10: ReferenceError: Can't find variable: prop4";
        QString warning4 = url + ":16: ReferenceError: Can't find variable: prop4";
        QString warning5 = url + ":20: ReferenceError: Can't find variable: method2";

        QTest::ignoreMessage(QtWarningMsg, warning1.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning2.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning3.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning4.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning5.toLatin1().constData());
        MyRevisionedClass *object = qobject_cast<MyRevisionedClass *>(component.create());
        QVERIFY(object != 0);
        delete object;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevisionErrors3.qml"));
        QString url = component.url().toString();

        // MyRevisionedSubclass 1.1 uses MyRevisionedClass revision 1
        // All properties/methods available, except MyRevisionedBaseClassUnregistered rev 1
        QString warning1 = url + ":30: ReferenceError: Can't find variable: methodD";
        QString warning2 = url + ":10: ReferenceError: Can't find variable: propD";
        QString warning3 = url + ":20: ReferenceError: Can't find variable: propD";
        QTest::ignoreMessage(QtWarningMsg, warning1.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning2.toLatin1().constData());
        QTest::ignoreMessage(QtWarningMsg, warning3.toLatin1().constData());
        MyRevisionedClass *object = qobject_cast<MyRevisionedClass *>(component.create());
        QVERIFY(object != 0);
        delete object;
    }
}

void tst_qdeclarativeecmascript::revision()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevision.qml"));
        QString url = component.url().toString();

        MyRevisionedClass *object = qobject_cast<MyRevisionedClass *>(component.create());
        QVERIFY(object != 0);
        delete object;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevision2.qml"));
        QString url = component.url().toString();

        MyRevisionedClass *object = qobject_cast<MyRevisionedClass *>(component.create());
        QVERIFY(object != 0);
        delete object;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevision3.qml"));
        QString url = component.url().toString();

        MyRevisionedClass *object = qobject_cast<MyRevisionedClass *>(component.create());
        QVERIFY(object != 0);
        delete object;
    }
    // Test that non-root classes can resolve revisioned methods
    {
        QDeclarativeComponent component(&engine, TEST_FILE("metaobjectRevision4.qml"));

        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("test").toReal(), 11.);
        delete object;
    }
}

void tst_qdeclarativeecmascript::realToInt()
{
    QDeclarativeComponent component(&engine, TEST_FILE("realToInt.qml"));
    MyQmlObject *object = qobject_cast<MyQmlObject*>(component.create());
    QVERIFY(object != 0);

    QMetaObject::invokeMethod(object, "test1");
    QCOMPARE(object->value(), int(4));
    QMetaObject::invokeMethod(object, "test2");
    QCOMPARE(object->value(), int(8));
}
void tst_qdeclarativeecmascript::dynamicString()
{
    QDeclarativeComponent component(&engine, TEST_FILE("dynamicString.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("stringProperty").toString(),
             QString::fromLatin1("string:Hello World false:0 true:1 uint32:100 int32:-100 double:3.14159 date:2011-02-11 05::30:50!"));
}

void tst_qdeclarativeecmascript::automaticSemicolon()
{
    QDeclarativeComponent component(&engine, TEST_FILE("automaticSemicolon.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
}

QTEST_MAIN(tst_qdeclarativeecmascript)

#include "tst_qdeclarativeecmascript.moc"
