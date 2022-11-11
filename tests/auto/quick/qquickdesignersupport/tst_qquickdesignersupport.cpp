// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickdesignersupportitems_p.h>
#include <private/qquickdesignersupportmetainfo_p.h>
#include <private/qquickdesignersupportproperties_p.h>
#include <private/qquickdesignersupportstates_p.h>
#include <private/qquickdesignersupportpropertychanges_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickstate_p.h>
#include <private/qquickstate_p_p.h>
#include <private/qquickstategroup_p.h>
#include <private/qquickpropertychanges_p.h>
#include <private/qquickrectangle_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickVisualTestUtils;

class tst_qquickdesignersupport : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickdesignersupport() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void customData();
    void customDataBindings();
    void objectProperties();
    void dynamicProperty();
    void createComponent();
    void basicStates();
    void statesPropertyChanges();
    void testNotifyPropertyChangeCallBack();
    void testFixResourcePathsForObjectCallBack();
    void testComponentOnCompleteSignal();
    void testSimpleBindings();
    void testDotProperties();
    void testItemReparenting();
    void testPropertyNames();
};


static bool isList(const QQmlProperty &property)
{
    return property.propertyTypeCategory() == QQmlProperty::List;
}

static bool isObject(const QQmlProperty &property)
{
    return property.isValid() && (property.propertyTypeCategory() == QQmlProperty::Object
                                  || !strcmp(property.propertyTypeName(), "QVariant"));
}

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

void addToNewProperty(QObject *object, QObject *newParent, const QByteArray &newParentProperty)
{
    QQmlProperty property(newParent, QString::fromUtf8(newParentProperty));

    if (object)
        object->setParent(newParent);

    if (isList(property)) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());
        list.append(object);
    } else if (isObject(property)) {
        property.write(objectToVariant(object));

        if (QQuickItem *item = qobject_cast<QQuickItem *>(object))
            if (QQuickItem *newParentItem = qobject_cast<QQuickItem *>(newParent))
                item->setParentItem(newParentItem);
    }

    Q_ASSERT(objectToVariant(object).isValid());
}

static void removeObjectFromList(const QQmlProperty &property, QObject *objectToBeRemoved)
{
    QQmlListReference listReference(property.object(), property.name().toUtf8());

    int count = listReference.count();

    QObjectList objectList;

    for (int i = 0; i < count; i ++) {
        QObject *listItem = listReference.at(i);
        if (listItem && listItem != objectToBeRemoved)
            objectList.append(listItem);
    }

    listReference.clear();

    for (QObject *object : objectList)
        listReference.append(object);
}


void tst_qquickdesignersupport::customData()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QScopedPointer<QObject> newItemScopedPointer(QQuickDesignerSupportItems::createPrimitive(
                                                     QLatin1String("QtQuick/Item"),
                                                     QTypeRevision::fromVersion(2, 6),
                                                     view->rootContext()));
    QObject *newItem = newItemScopedPointer.data();

    QVERIFY(newItem);
    QVERIFY(qobject_cast<QQuickItem*>(newItem));

    QQuickDesignerSupportProperties::registerCustomData(newItem);
    QVERIFY(QQuickDesignerSupportProperties::getResetValue(newItem, "width").isValid());
    int defaultWidth = QQuickDesignerSupportProperties::getResetValue(newItem, "width").toInt();
    QCOMPARE(defaultWidth, 0);

    newItem->setProperty("width", 200);
    QCOMPARE(newItem->property("width").toInt(), 200);

    //Check if reseting property does work
    QQuickDesignerSupportProperties::doResetProperty(newItem, view->rootContext(), "width");
    QCOMPARE(newItem->property("width").toInt(), 0);

    //Setting a binding on width
    QQuickDesignerSupportProperties::setPropertyBinding(newItem,
                                                        view->engine()->contextForObject(newItem),
                                                        "width",
                                                        QLatin1String("Math.max(0, 200)"));
    QCOMPARE(newItem->property("width").toInt(), 200);
    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(newItem,
                                                                   view->engine()->contextForObject(newItem),
                                                                   "width",
                                                                   nullptr));

    //Check if reseting property does work after setting binding
    QQuickDesignerSupportProperties::doResetProperty(newItem, view->rootContext(), "width");
    QCOMPARE(newItem->property("width").toInt(), 0);

    //No custom data available for the rootItem, because not registered by QQuickDesignerSupportProperties::registerCustomData
    QVERIFY(!QQuickDesignerSupportProperties::getResetValue(rootItem, "width").isValid());

    newItemScopedPointer.reset(); //Delete the item and check if item gets removed from the hash and an invalid QVariant is returned.

    QVERIFY(!QQuickDesignerSupportProperties::getResetValue(newItem, "width").isValid());
}

void tst_qquickdesignersupport::customDataBindings()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QQuickItem *testComponent = findItem<QQuickItem>(view->rootObject(), QLatin1String("testComponent"));

    QVERIFY(testComponent);
    QQuickDesignerSupportProperties::registerCustomData(testComponent);
    QQuickDesignerSupportProperties::hasValidResetBinding(testComponent, "x");
    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(testComponent,
                                                           view->engine()->contextForObject(testComponent),
                                                           "x",
                                                           nullptr));

    QCOMPARE(testComponent->property("x").toInt(), 200);


    //Set property to 100 and ovveride the default binding
    QQmlProperty property(testComponent, "x", view->engine()->contextForObject(testComponent));
    QVERIFY(property.write(100));
    QCOMPARE(testComponent->property("x").toInt(), 100);

    QVERIFY(!QQuickDesignerSupportProperties::hasBindingForProperty(testComponent,
                                                           view->engine()->contextForObject(testComponent),
                                                           "x",
                                                           nullptr));

    //Reset the binding to the default
    QQuickDesignerSupportProperties::doResetProperty(testComponent,
                                                     view->engine()->contextForObject(testComponent),
                                                     "x");

    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(testComponent,
                                                           view->engine()->contextForObject(testComponent),
                                                           "x",
                                                           nullptr));
    QCOMPARE(testComponent->property("x").toInt(), 200);



    //Set a different binding/expression
    QQuickDesignerSupportProperties::setPropertyBinding(testComponent,
                                                        view->engine()->contextForObject(testComponent),
                                                        "x",
                                                        QLatin1String("Math.max(0, 300)"));

    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(testComponent,
                                                           view->engine()->contextForObject(testComponent),
                                                           "x",
                                                           nullptr));

    QCOMPARE(testComponent->property("x").toInt(), 300);



    //Reset the binding to the default
    QQuickDesignerSupportProperties::doResetProperty(testComponent,
                                                     view->engine()->contextForObject(testComponent),
                                                     "x");


    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(testComponent,
                                                           view->engine()->contextForObject(testComponent),
                                                           "x",
                                                           nullptr));
    QCOMPARE(testComponent->property("x").toInt(), 200);
}

void tst_qquickdesignersupport::objectProperties()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QQuickItem *rectangleItem = findItem<QQuickItem>(view->rootObject(), QLatin1String("rectangleItem"));
    QVERIFY(rectangleItem);


    //Read gradient property as QObject
    int propertyIndex = rectangleItem->metaObject()->indexOfProperty("containmentMask");
    QVERIFY(propertyIndex > 0);
    QMetaProperty metaProperty = rectangleItem->metaObject()->property(propertyIndex);
    QVERIFY(metaProperty.isValid());

    QVERIFY(QQuickDesignerSupportProperties::isPropertyQObject(metaProperty));

    QObject *containmentItem = QQuickDesignerSupportProperties::readQObjectProperty(metaProperty, rectangleItem);
    QVERIFY(containmentItem);


    //The width property is not a QObject
    propertyIndex = rectangleItem->metaObject()->indexOfProperty("width");
    QVERIFY(propertyIndex > 0);
    metaProperty = rectangleItem->metaObject()->property(propertyIndex);
    QVERIFY(metaProperty.isValid());
    QVERIFY(!QQuickDesignerSupportProperties::isPropertyQObject(metaProperty));
}

void tst_qquickdesignersupport::dynamicProperty()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QQuickItem *simpleItem = findItem<QQuickItem>(view->rootObject(), QLatin1String("simpleItem"));

    QVERIFY(simpleItem);

    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(simpleItem, view->engine());
    QQuickDesignerSupportProperties::getPropertyCache(simpleItem);

    QQuickDesignerSupportProperties::createNewDynamicProperty(simpleItem, view->engine(), QLatin1String("dynamicProperty"));

    QQmlProperty property(simpleItem, "dynamicProperty", view->engine()->contextForObject(simpleItem));
    QVERIFY(property.isValid());
    QVERIFY(property.write(QLatin1String("test")));


    QCOMPARE(property.read().toString(), QLatin1String("test"));

    //Force evalutation of all bindings
    QQuickDesignerSupport::refreshExpressions(view->rootContext());

    //Check if expression to dynamic property gets properly resolved
    property = QQmlProperty(simpleItem, "testProperty", view->engine()->contextForObject(simpleItem));
    QVERIFY(property.isValid());
    QCOMPARE(property.read().toString(), QLatin1String("test"));
}

void tst_qquickdesignersupport::createComponent()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QScopedPointer<QObject> testComponentObject(
                QQuickDesignerSupportItems::createComponent(
                    testFileUrl("TestComponent.qml"), view->rootContext()));
    QVERIFY(testComponentObject);

    QVERIFY(QQuickDesignerSupportMetaInfo::isSubclassOf(
                testComponentObject.data(), "QtQuick/Item"));
}

void tst_qquickdesignersupport::basicStates()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QQuickStateGroup *stateGroup = QQuickItemPrivate::get(rootItem)->_states();

    QVERIFY(stateGroup);

    QCOMPARE(stateGroup->states().size(), 2 );

    QQuickState *state01 = stateGroup->states().first();
    QQuickState *state02 = stateGroup->states().last();

    QVERIFY(state01);
    QVERIFY(state02);

    QCOMPARE(state01->property("name").toString(), QLatin1String("state01"));
    QCOMPARE(state02->property("name").toString(), QLatin1String("state02"));

    QVERIFY(!QQuickDesignerSupportStates::isStateActive(state01, view->rootContext()));
    QVERIFY(!QQuickDesignerSupportStates::isStateActive(state01, view->rootContext()));

    QQuickDesignerSupportStates::activateState(state01, view->rootContext());
    QVERIFY(QQuickDesignerSupportStates::isStateActive(state01, view->rootContext()));

    QQuickDesignerSupportStates::activateState(state02, view->rootContext());
    QVERIFY(QQuickDesignerSupportStates::isStateActive(state02, view->rootContext()));
    QVERIFY(!QQuickDesignerSupportStates::isStateActive(state01, view->rootContext()));

    QQuickDesignerSupportStates::deactivateState(state02);
    QVERIFY(!QQuickDesignerSupportStates::isStateActive(state01, view->rootContext()));
    QVERIFY(!QQuickDesignerSupportStates::isStateActive(state01, view->rootContext()));
}

void tst_qquickdesignersupport::statesPropertyChanges()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QQuickItem *simpleItem = findItem<QQuickItem>(view->rootObject(), QLatin1String("simpleItem"));

    QVERIFY(simpleItem);

    QQuickStateGroup *stateGroup = QQuickItemPrivate::get(rootItem)->_states();

    QVERIFY(stateGroup);

    QCOMPARE(stateGroup->states().size(), 2 );

    QQuickState *state01 = stateGroup->states().first();
    QQuickState *state02 = stateGroup->states().last();

    QVERIFY(state01);
    QVERIFY(state02);

    QCOMPARE(state01->property("name").toString(), QLatin1String("state01"));
    QCOMPARE(state02->property("name").toString(), QLatin1String("state02"));

    //PropertyChanges are parsed lazily
    QQuickDesignerSupportStates::activateState(state01, view->rootContext());
    QQuickDesignerSupportStates::deactivateState(state01);

    QQuickStatePrivate *statePrivate01 = static_cast<QQuickStatePrivate *>(QQuickStatePrivate::get(state01));

    QCOMPARE(state01->operationCount(), 1);

    QCOMPARE(statePrivate01->operations.size(), 1);

    QQuickStateOperation *propertyChange = statePrivate01->operations.at(0).data();

    QCOMPARE(QQuickDesignerSupportPropertyChanges::stateObject(propertyChange), state01);

    QQuickDesignerSupportPropertyChanges::changeValue(propertyChange, "width", 300);

    QCOMPARE(simpleItem->property("width").toInt(), 0);
    QQuickDesignerSupportStates::activateState(state01, view->rootContext());
    QCOMPARE(simpleItem->property("width").toInt(), 300);
    QQuickDesignerSupportStates::deactivateState(state01);
    QCOMPARE(simpleItem->property("width").toInt(), 0);

    //Set "base state value" in state1 using the revert list
    QQuickDesignerSupportStates::activateState(state01, view->rootContext());
    QQuickDesignerSupportStates::changeValueInRevertList(state01, simpleItem, "width", 200);
    QCOMPARE(simpleItem->property("width").toInt(), 300);
    QQuickDesignerSupportStates::deactivateState(state01);
    QCOMPARE(simpleItem->property("width").toInt(), 200);


    //Create new PropertyChanges
    QQuickPropertyChanges *newPropertyChange = new QQuickPropertyChanges();
    newPropertyChange->setParent(state01);
    QQmlListProperty<QQuickStateOperation> changes = state01->changes();
    QQuickStatePrivate::operations_append(&changes, newPropertyChange);

    newPropertyChange->setObject(rootItem);

    QQuickDesignerSupportPropertyChanges::attachToState(newPropertyChange);

    QCOMPARE(rootItem, QQuickDesignerSupportPropertyChanges::targetObject(newPropertyChange));

    QCOMPARE(state01->operationCount(), 2);
    QCOMPARE(statePrivate01->operations.size(), 2);

    QCOMPARE(QQuickDesignerSupportPropertyChanges::stateObject(newPropertyChange), state01);

    //Set color for rootItem in state1
    QQuickDesignerSupportPropertyChanges::changeValue(newPropertyChange, "color", QColor(Qt::red));

    QQuickDesignerSupportStates::activateState(state01, view->rootContext());
    QCOMPARE(rootItem->property("color").value<QColor>(), QColor(Qt::red));
    QQuickDesignerSupportStates::deactivateState(state01);
    QCOMPARE(rootItem->property("color").value<QColor>(), QColor(Qt::white));

    QQuickDesignerSupportPropertyChanges::removeProperty(newPropertyChange, "color");
    QQuickDesignerSupportStates::activateState(state01, view->rootContext());
    QCOMPARE(rootItem->property("color").value<QColor>(), QColor(Qt::white));

}

static QObject * s_object = nullptr;
static QQuickDesignerSupport::PropertyName  s_propertyName;

static void notifyPropertyChangeCallBackFunction(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    s_object = object;
    s_propertyName = propertyName;
}

static void (*notifyPropertyChangeCallBackPointer)(QObject *, const QQuickDesignerSupport::PropertyName &) = &notifyPropertyChangeCallBackFunction;


void tst_qquickdesignersupport::testNotifyPropertyChangeCallBack()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    QQuickRectangle *rectangle = static_cast<QQuickRectangle *>(rootItem);
    QVERIFY(rectangle);

    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(rectangle, view->engine());

    QQuickGradient *gradient = new QQuickGradient(rectangle);

    QQuickDesignerSupportMetaInfo::registerNotifyPropertyChangeCallBack(notifyPropertyChangeCallBackPointer);

    rectangle->setProperty("gradient", QVariant::fromValue<QJSValue>(view->engine()->newQObject(gradient)));

    QVERIFY(s_object);
    QCOMPARE(s_object, rootItem);
    QCOMPARE(s_propertyName, QQuickDesignerSupport::PropertyName("gradient"));
}

// We have to use this ugly approach, because the signature of
// registerFixResourcePathsForObjectCallBack doesn't accept
// a proper lambda with a capture list
static QVector<QObject*> s_allSubObjects;

static void fixResourcePathsForObjectCallBackFunction(QObject *object)
{
    s_allSubObjects << object;
}

static void (*fixResourcePathsForObjectCallBackPointer)(QObject *) = &fixResourcePathsForObjectCallBackFunction;

void tst_qquickdesignersupport::testFixResourcePathsForObjectCallBack()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("test.qml"));

    QVERIFY(view->errors().isEmpty());

    QQuickItem *rootItem = view->rootObject();

    QVERIFY(rootItem);

    s_allSubObjects.clear();

    QQuickDesignerSupportItems::registerFixResourcePathsForObjectCallBack(fixResourcePathsForObjectCallBackPointer);

    QQuickItem *simpleItem = findItem<QQuickItem>(view->rootObject(), QLatin1String("simpleItem"));

    QVERIFY(simpleItem);

    QQuickDesignerSupportItems::tweakObjects(simpleItem);

    // Check that the fixResourcePathsForObjectCallBack was called on simpleItem
    // NOTE: more objects are collected now. There is also at least a palette
    //       that created on demand.
    QVERIFY(s_allSubObjects.contains(simpleItem));

    s_allSubObjects.clear();
}

void doComponentCompleteRecursive(QObject *object)
{
    if (object) {
        QQuickItem *item = qobject_cast<QQuickItem*>(object);

        if (item && DesignerSupport::isComponentComplete(item))
            return;

        DesignerSupport::emitComponentCompleteSignalForAttachedProperty(object);

        QList<QObject*> childList = object->children();

        if (item) {
            foreach (QQuickItem *childItem, item->childItems()) {
                if (!childList.contains(childItem))
                    childList.append(childItem);
            }
        }

        foreach (QObject *child, childList)
                doComponentCompleteRecursive(child);

        if (item) {
            static_cast<QQmlParserStatus*>(item)->componentComplete();
        } else {
            QQmlParserStatus *qmlParserStatus = dynamic_cast< QQmlParserStatus*>(object);
            if (qmlParserStatus)
                qmlParserStatus->componentComplete();
        }
    }
}

void tst_qquickdesignersupport::testComponentOnCompleteSignal()
{
    {
        QScopedPointer<QQuickView> view(new QQuickView);
        view->engine()->setOutputWarningsToStandardError(false);
        view->setSource(testFileUrl("componentTest.qml"));

        QVERIFY(view->errors().isEmpty());
        QQuickItem *rootItem = view->rootObject();
        QVERIFY(rootItem);

        QQuickItem *item = findItem<QQuickItem>(view->rootObject(), QLatin1String("topLevelComplete"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("red"));

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("implemented"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("blue"));

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("most inner"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("green"));
    }

    {
        ComponentCompleteDisabler disableComponentComplete;

        QScopedPointer<QQuickView> view(new QQuickView);
        view->engine()->setOutputWarningsToStandardError(false);
        view->setSource(testFileUrl("componentTest.qml"));

        QVERIFY(view->errors().isEmpty());
        QQuickItem *rootItem = view->rootObject();
        QVERIFY(rootItem);

        QQuickItem *item = findItem<QQuickItem>(view->rootObject(), QLatin1String("topLevelComplete"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("white"));

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("implemented"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("white"));

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("most inner"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("white"));

        doComponentCompleteRecursive(rootItem);

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("topLevelComplete"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("red"));

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("implemented"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("blue"));

        item = findItem<QQuickItem>(view->rootObject(), QLatin1String("most inner"));
        QVERIFY(item);
        QCOMPARE(item->property("color").value<QColor>(), QColor("green"));
    }
}


void tst_qquickdesignersupport::testSimpleBindings()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("bindingTest.qml"));

    QVERIFY(view->errors().isEmpty());
    QQuickItem *rootItem = view->rootObject();
    QVERIFY(rootItem);

    QQuickItem *text = findItem<QQuickItem>(rootItem, QLatin1String("text"));
    QVERIFY(text);

    QQuickItem *item = findItem<QQuickItem>(rootItem, QLatin1String("item"));
    QVERIFY(item);

    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(item, view->engine());
    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(text, view->engine());

    QQuickDesignerSupportProperties::registerCustomData(item);
    QQuickDesignerSupportProperties::registerCustomData(text);

    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(text,
                                                                   QQmlEngine::contextForObject(text),
                                                                   "text",
                                                                   nullptr));

    QQuickDesignerSupportProperties::doResetProperty(text, QQmlEngine::contextForObject(text),  "text");


    QQuickDesignerSupportProperties::setPropertyBinding(text,
                                                        QQmlEngine::contextForObject(text),
                                                        "text",
                                                        "qsTr(\"someText\")");

    QVERIFY(QQuickDesignerSupportProperties::hasBindingForProperty(text,
                                                                   QQmlEngine::contextForObject(text),
                                                                   "text",
                                                                   nullptr));
}

void tst_qquickdesignersupport::testDotProperties()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("bindingTest.qml"));

    QVERIFY(view->errors().isEmpty());
    QQuickItem *rootItem = view->rootObject();
    QVERIFY(rootItem);

    QQuickItem *text = findItem<QQuickItem>(rootItem, QLatin1String("text"));
    QVERIFY(text);

    QQuickItem *item = findItem<QQuickItem>(rootItem, QLatin1String("item"));
    QVERIFY(item);

    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(item, view->engine());
    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(text, view->engine());

    QQuickDesignerSupportProperties::registerCustomData(item);
    QQuickDesignerSupportProperties::registerCustomData(text);

    QCOMPARE(text->property("font.bold").value<QColor>(), QColor("true"));
    QCOMPARE(text->property("font.italic").value<QColor>(), QColor("false"));
    QCOMPARE(text->property("font.underline").value<QColor>(), QColor("false"));

    QQmlProperty property(text, "font.capitalization");
}

void  tst_qquickdesignersupport::testItemReparenting()
{

    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("bindingTest.qml"));

    QVERIFY(view->errors().isEmpty());
    QQuickItem *rootItem = view->rootObject();
    QVERIFY(rootItem);

    QQuickItem *text = findItem<QQuickItem>(rootItem, QLatin1String("text"));
    QVERIFY(text);

    QQuickItem *item = findItem<QQuickItem>(rootItem, QLatin1String("item"));
    QVERIFY(item);

    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(item, view->engine());
    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(text, view->engine());

    QQuickDesignerSupportProperties::registerCustomData(item);
    QQuickDesignerSupportProperties::registerCustomData(text);


    QCOMPARE(text->parentItem(), rootItem);
    QQmlProperty childrenProperty(rootItem, "children");
    removeObjectFromList(childrenProperty, text);
    addToNewProperty(text, item, "children");
    QCOMPARE(text->parentItem(), item);
}

void tst_qquickdesignersupport::testPropertyNames()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->engine()->setOutputWarningsToStandardError(false);
    view->setSource(testFileUrl("propertyNameTest.qml"));

    QVERIFY(view->errors().isEmpty());
    QQuickItem *rootItem = view->rootObject();
    QVERIFY(rootItem);

    QQuickDesignerSupport::PropertyNameList names = QQuickDesignerSupportProperties::allPropertyNames(rootItem);
    QVERIFY(!names.isEmpty());
    QVERIFY(names.contains("width"));
    QVERIFY(names.contains("height"));
    QVERIFY(names.contains("clip"));
    QVERIFY(names.contains("childrenRect"));
    QVERIFY(names.contains("activeFocus"));
    QVERIFY(names.contains("border.width"));
    names = QQuickDesignerSupportProperties::propertyNameListForWritableProperties(rootItem);
    QVERIFY(!names.isEmpty());
    QVERIFY(names.contains("width"));
    QVERIFY(names.contains("height"));
    QVERIFY(names.contains("opacity"));
    QVERIFY(!names.contains("childrenRect"));
    QVERIFY(!names.contains("childrenRect"));
    QVERIFY(!names.contains("activeFocus"));
    QVERIFY(names.contains("border.width"));

    QQuickItem *recursiveProperty = findItem<QQuickItem>(rootItem, QLatin1String("recursiveProperty"));
    QVERIFY(recursiveProperty);
    names = QQuickDesignerSupportProperties::allPropertyNames(recursiveProperty);
    QVERIFY(!names.isEmpty());
    QVERIFY(names.contains("testProperty"));
    QVERIFY(names.contains("myproperty.testProperty"));

    names = QQuickDesignerSupportProperties::propertyNameListForWritableProperties(recursiveProperty);
    QVERIFY(!names.isEmpty());
    QVERIFY(!names.contains("testProperty"));
}

QTEST_MAIN(tst_qquickdesignersupport)

#include "tst_qquickdesignersupport.moc"
