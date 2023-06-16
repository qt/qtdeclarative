// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "interfaces.h"
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/private/qqmlproperty_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlboundsignal_p.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif
#if QT_CONFIG(process)
#include <QtCore/qprocess.h>
#endif
#include <QtCore/private/qobject_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include "qobject.h"
#include <QtQml/QQmlPropertyMap>

using namespace Qt::StringLiterals;

#include <QDebug>
class MyQmlObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPoint pointProperty MEMBER m_point)
public:
    MyQmlObject(QObject *parent = nullptr) : QObject(parent) {}

private:
    QPoint m_point;
};

QML_DECLARE_TYPE(MyQmlObject);

class MyQObject : public QObject
{
    Q_OBJECT
public:
    MyQObject(QObject *parent = nullptr) : QObject(parent), m_i(0) {}

    int inc() { return ++m_i; }

private:
  int m_i;
};


class MyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo)
public:
    MyAttached(QObject *parent) : QObject(parent), m_foo(13) {}

    int foo() const { return m_foo; }
    void setFoo(int f) { m_foo = f; }

private:
    int m_foo;
};

class MyContainer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<MyQmlObject> children READ children)

public:
    MyContainer() {}

    QQmlListProperty<MyQmlObject> children() { return QQmlListProperty<MyQmlObject>(this, &m_children); }

    static MyAttached *qmlAttachedProperties(QObject *o) {
        return new MyAttached(o);
    }

private:
    QList<MyQmlObject*> m_children;
};

QML_DECLARE_TYPE(MyContainer);
QML_DECLARE_TYPEINFO(MyContainer, QML_HAS_ATTACHED_PROPERTIES)

class MyReplaceIfNotDefaultBehaviorContainer : public MyContainer
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<MyQmlObject> defaultList READ defaultList)

    QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT
    Q_CLASSINFO("DefaultProperty", "defaultList")
public:
    MyReplaceIfNotDefaultBehaviorContainer() {}

    QQmlListProperty<MyQmlObject> defaultList() { return QQmlListProperty<MyQmlObject>(this, &m_defaultList); }

private:
    QList<MyQmlObject*> m_defaultList;
};

QML_DECLARE_TYPE(MyReplaceIfNotDefaultBehaviorContainer);

class MyAlwaysReplaceBehaviorContainer : public MyContainer
{
    Q_OBJECT

    QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE
public:
    MyAlwaysReplaceBehaviorContainer() {}
};

QML_DECLARE_TYPE(MyAlwaysReplaceBehaviorContainer);

class ListHolder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList varList READ varList NOTIFY varListChanged)
    Q_PROPERTY(QList<double> doubleList READ doubleList WRITE setDoubleList NOTIFY doubleListChanged)
public:
    explicit ListHolder(QObject *parent = nullptr) : QObject(parent) {}

    QVariantList varList() const { return {1.1, 2.2, 3.3, 11, 5.25f, QStringLiteral("11")}; }

    QList<double> doubleList() const { return m_doubleList; }

    void setDoubleList(const QList<double> &newDoubleList)
    {
        if (m_doubleList == newDoubleList)
            return;
        m_doubleList = newDoubleList;
        emit doubleListChanged();
    }

signals:
    void varListChanged();
    void doubleListChanged();

private:
    QList<double> m_doubleList;
};


class tst_qqmlproperty : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlproperty() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;

    // Constructors
    void qmlmetaproperty();
    void qmlmetaproperty_object();
    void qmlmetaproperty_object_string();
    void qmlmetaproperty_object_context();
    void qmlmetaproperty_object_string_context();

    // Methods
    void name();
    void read();
    void write();
    void reset();

    // Functionality
    void writeObjectToList();
    void writeListToList();
    void listOverrideBehavior();

    //writeToReadOnly();

    void urlHandling_data();
    void urlHandling();

    void variantMapHandling_data();
    void variantMapHandling();

    // Bugs
    void crashOnValueProperty();
    void aliasPropertyBindings_data();
    void aliasPropertyBindings();
    void noContext();
    void assignEmptyVariantMap();
    void warnOnInvalidBinding();
    void registeredCompositeTypeProperty();
    void deeplyNestedObject();
    void readOnlyDynamicProperties();
    void aliasToIdWithMatchingQmlFileNameOnCaseInsensitiveFileSystem();
    void nullPropertyBinding();
    void interfaceBinding();

    void floatToStringPrecision_data();
    void floatToStringPrecision();

    void copy();

    void bindingToAlias();

    void nestedQQmlPropertyMap();

    void underscorePropertyChangeHandler();

    void signalExpressionWithoutObject();

    void dontRemoveQPropertyBinding();
    void compatResolveUrls();

    void initFlags_data();
    void initFlags();

    void constructFromPlainMetaObject();

    void bindToNonQObjectTarget();
    void assignVariantList();

    void listAssignmentSignals();

    void invalidateQPropertyChangeTriggers();

private:
    QQmlEngine engine;
};

void tst_qqmlproperty::qmlmetaproperty()
{
    QQmlProperty prop;

    QScopedPointer<QObject> obj(new QObject);

    QQmlAbstractBinding::Ptr binding(QQmlBinding::create(nullptr, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
    QVERIFY(binding);
    QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(
                obj.data(), QObjectPrivate::get(obj.data())->signalIndex("destroyed()"),
                QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"),
                QString(), -1, -1);
    QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
    QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

    QCOMPARE(prop.name(), QString());
    QCOMPARE(prop.read(), QVariant());
    QCOMPARE(prop.write(QVariant()), false);
    QCOMPARE(prop.hasNotifySignal(), false);
    QCOMPARE(prop.needsNotifySignal(), false);
    QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
    QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
    QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
    QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                          "deleteLater()")), false);
    QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                          "deleteLater()")), false);
    QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
    QVERIFY(!prop.method().isValid());
    QCOMPARE(prop.type(), QQmlProperty::Invalid);
    QCOMPARE(prop.isProperty(), false);
    QCOMPARE(prop.isWritable(), false);
    QCOMPARE(prop.isDesignable(), false);
    QCOMPARE(prop.isResettable(), false);
    QCOMPARE(prop.isSignalProperty(), false);
    QCOMPARE(prop.isValid(), false);
    QCOMPARE(prop.object(), (QObject *)nullptr);
    QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
    QCOMPARE(prop.propertyType(), 0);
    QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
    QVERIFY(!prop.property().name());
    QVERIFY(!QQmlPropertyPrivate::binding(prop));
    QQmlPropertyPrivate::setBinding(prop, binding.data());
    QVERIFY(binding->ref == 1);
    QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
    QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
    QVERIFY(sigExprWatcher.wasDeleted());
    QCOMPARE(prop.index(), -1);
    QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
    QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
}

// 1 = equal, 0 = unknown, -1 = not equal.
static int compareVariantAndListReference(const QVariant &v, QQmlListReference &r)
{
    if (QLatin1String(v.typeName()) != QLatin1String("QQmlListReference"))
        return -1;

    QQmlListReference lhs = v.value<QQmlListReference>();
    if (lhs.isValid() != r.isValid())
        return -1;

    if (lhs.canCount() != r.canCount())
        return -1;

    if (!lhs.canCount()) {
        if (lhs.canAt() != r.canAt())
            return -1; // not equal.
        return 0; // not sure if they're equal or not, and no way to tell.
    }

    // if we get here, we must be able to count.
    if (lhs.count() != r.count())
        return -1;

    if (lhs.canAt() != r.canAt())
        return -1;

    if (!lhs.canAt())
        return 0; // can count, but can't check element equality.

    for (int i = 0; i < lhs.count(); ++i) {
        if (lhs.at(i) != r.at(i)) {
            return -1; // different elements :. not equal.
        }
    }

    return 1; // equal.
}

void tst_qqmlproperty::registeredCompositeTypeProperty()
{
    // Composite type properties
    {
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("registeredCompositeTypeProperty.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(obj);

        // create property accessors and check types.
        QQmlProperty p1(obj.data(), "first");
        QQmlProperty p2(obj.data(), "second");
        QQmlProperty p3(obj.data(), "third");
        QQmlProperty p1e(obj.data(), "first", &engine);
        QQmlProperty p2e(obj.data(), "second", &engine);
        QQmlProperty p3e(obj.data(), "third", &engine);
        QCOMPARE(p1.propertyType(), p2.propertyType());
        QVERIFY(p1.propertyType() != p3.propertyType());

        // check that the values are retrievable from CPP
        QVariant first = obj->property("first");
        QVariant second = obj->property("second");
        QVariant third = obj->property("third");
        QVERIFY(first.isValid());
        QVERIFY(second.isValid());
        QVERIFY(third.isValid());
        // ensure that conversion from qobject-derived-ptr to qobject-ptr works.
        QVERIFY(first.value<QObject*>());
        QVERIFY(second.value<QObject*>());
        QVERIFY(third.value<QObject*>());

        // check that the values retrieved via QQmlProperty match those retrieved via QMetaProperty::read().
        QCOMPARE(p1.read().value<QObject*>(), first.value<QObject*>());
        QCOMPARE(p2.read().value<QObject*>(), second.value<QObject*>());
        QCOMPARE(p3.read().value<QObject*>(), third.value<QObject*>());
        QCOMPARE(p1e.read().value<QObject*>(), first.value<QObject*>());
        QCOMPARE(p2e.read().value<QObject*>(), second.value<QObject*>());
        QCOMPARE(p3e.read().value<QObject*>(), third.value<QObject*>());
    }

    // List-of-composite-type type properties
    {
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("registeredCompositeTypeProperty.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(obj);

        // create list property accessors and check types
        QQmlProperty lp1e(obj.data(), "fclist", &engine);
        QQmlProperty lp2e(obj.data(), "sclistOne", &engine);
        QQmlProperty lp3e(obj.data(), "sclistTwo", &engine);
        QVERIFY(lp1e.propertyType() != lp2e.propertyType());
        QCOMPARE(lp2e.propertyType(), lp3e.propertyType());

        // check that the list values are retrievable from CPP
        QVariant firstList = obj->property("fclist");
        QVariant secondList = obj->property("sclistOne");
        QVariant thirdList = obj->property("sclistTwo");
        QVERIFY(firstList.isValid());
        QVERIFY(secondList.isValid());
        QVERIFY(thirdList.isValid());

        // check that the value returned by QQmlProperty::read() is equivalent to the list reference.
        QQmlListReference r1(obj.data(), "fclist");
        QQmlListReference r2(obj.data(), "sclistOne");
        QQmlListReference r3(obj.data(), "sclistTwo");
        QCOMPARE(compareVariantAndListReference(lp1e.read(), r1), 1);
        QCOMPARE(compareVariantAndListReference(lp2e.read(), r2), 1);
        QCOMPARE(compareVariantAndListReference(lp3e.read(), r3), 1);
    }
}

class PropertyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int defaultProperty READ defaultProperty)
    Q_PROPERTY(QRect rectProperty READ rectProperty)
    Q_PROPERTY(QRect wrectProperty READ wrectProperty WRITE setWRectProperty)
    Q_PROPERTY(QUrl url READ url WRITE setUrl)
    Q_PROPERTY(QVariantMap variantMap READ variantMap WRITE setVariantMap)
    Q_PROPERTY(int resettableProperty READ resettableProperty WRITE setResettableProperty RESET resetProperty)
    Q_PROPERTY(int propertyWithNotify READ propertyWithNotify WRITE setPropertyWithNotify NOTIFY oddlyNamedNotifySignal)
    Q_PROPERTY(MyQmlObject *qmlObject READ qmlObject)
    Q_PROPERTY(MyQObject *qObject READ qObject WRITE setQObject NOTIFY qObjectChanged)
    Q_PROPERTY(QString stringProperty READ stringProperty WRITE setStringProperty)
    Q_PROPERTY(QChar qcharProperty READ qcharProperty WRITE setQcharProperty)
    Q_PROPERTY(QChar constQChar READ constQChar STORED false CONSTANT FINAL)

    Q_CLASSINFO("DefaultProperty", "defaultProperty")
public:
    PropertyObject() : m_resetProperty(9), m_qObject(nullptr), m_stringProperty("foo") {}

    int defaultProperty() { return 10; }
    QRect rectProperty() { return QRect(10, 10, 1, 209); }

    QRect wrectProperty() { return m_rect; }
    void setWRectProperty(const QRect &r) { m_rect = r; }

    QUrl url() { return m_url; }
    void setUrl(const QUrl &u) { m_url = u; }

    QVariantMap variantMap() const { return m_variantMap; }
    void setVariantMap(const QVariantMap &variantMap) { m_variantMap = variantMap; }

    int resettableProperty() const { return m_resetProperty; }
    void setResettableProperty(int r) { m_resetProperty = r; }
    void resetProperty() { m_resetProperty = 9; }

    int propertyWithNotify() const { return m_propertyWithNotify; }
    void setPropertyWithNotify(int i) { m_propertyWithNotify = i; emit oddlyNamedNotifySignal(); }

    MyQmlObject *qmlObject() { return &m_qmlObject; }

    MyQObject *qObject() { return m_qObject; }
    void setQObject(MyQObject *object)
    {
        if (m_qObject != object) {
            m_qObject = object;
            emit qObjectChanged();
        }
    }

    QString stringProperty() const { return m_stringProperty;}
    QChar qcharProperty() const { return m_qcharProperty; }

    QChar constQChar() const { return u'\u25cf'; /* Unicode: black circle */ }

    void setStringProperty(QString arg) { m_stringProperty = arg; }
    void setQcharProperty(QChar arg) { m_qcharProperty = arg; }

signals:
    void clicked();
    void oddlyNamedNotifySignal();
    void qObjectChanged();

private:
    int m_resetProperty;
    QRect m_rect;
    QUrl m_url;
    QVariantMap m_variantMap;
    int m_propertyWithNotify;
    MyQmlObject m_qmlObject;
    MyQObject *m_qObject;
    QString m_stringProperty;
    QChar m_qcharProperty;
};

QML_DECLARE_TYPE(PropertyObject);

void tst_qqmlproperty::qmlmetaproperty_object()
{
    QObject object; // Has no default property
    PropertyObject dobject; // Has default property

    {
        QQmlProperty prop(&object);

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&object, QObjectPrivate::get(&object)->signalIndex("destroyed()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString());
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Invalid);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), false);
        QCOMPARE(prop.object(), (QObject *)nullptr);
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QVERIFY(!prop.property().name());
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), -1);
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject);

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QObjectPrivate::get(&dobject)->signalIndex("clicked()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("defaultProperty"));
        QCOMPARE(prop.read(), QVariant(10));
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), true);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Property);
        QCOMPARE(prop.isProperty(), true);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), true);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::Normal);
        QCOMPARE(prop.propertyType(), QMetaType::Int);
        QCOMPARE(prop.propertyTypeName(), "int");
        QCOMPARE(QString(prop.property().name()), QString("defaultProperty"));
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Unable to assign null to int");
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding);
        QCOMPARE(QQmlPropertyPrivate::binding(prop), binding.data());
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfProperty("defaultProperty"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }
}

void tst_qqmlproperty::qmlmetaproperty_object_string()
{
    QObject object;
    PropertyObject dobject;

    {
        QQmlProperty prop(&object, QString("defaultProperty"));

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&object, QObjectPrivate::get(&object)->signalIndex("destroyed()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString());
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Invalid);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), false);
        QCOMPARE(prop.object(), (QObject *)nullptr);
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QVERIFY(!prop.property().name());
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), -1);
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, QString("defaultProperty"));

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QObjectPrivate::get(&dobject)->signalIndex("clicked()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("defaultProperty"));
        QCOMPARE(prop.read(), QVariant(10));
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), true);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Property);
        QCOMPARE(prop.isProperty(), true);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), true);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::Normal);
        QCOMPARE(prop.propertyType(), QMetaType::Int);
        QCOMPARE(prop.propertyTypeName(), "int");
        QCOMPARE(QString(prop.property().name()), QString("defaultProperty"));
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Unable to assign null to int");
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding);
        QCOMPARE(QQmlPropertyPrivate::binding(prop), binding.data());
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfProperty("defaultProperty"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, QString("onClicked"));

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QQmlPropertyPrivate::get(prop)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("onClicked"));
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant("Hello")), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QCOMPARE(QString(prop.method().methodSignature()), QString("clicked()"));
        QCOMPARE(prop.type(), QQmlProperty::SignalProperty);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), true);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QCOMPARE(prop.property().name(), (const char *)nullptr);
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(!sigExprWatcher.wasDeleted());
        QCOMPARE(QQmlPropertyPrivate::signalExpression(prop), sigExpr);
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfMethod("clicked()"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, QString("onPropertyWithNotifyChanged"));

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QQmlPropertyPrivate::get(prop)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("onOddlyNamedNotifySignal"));
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant("Hello")), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QCOMPARE(QString(prop.method().methodSignature()), QString("oddlyNamedNotifySignal()"));
        QCOMPARE(prop.type(), QQmlProperty::SignalProperty);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), true);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QCOMPARE(prop.property().name(), (const char *)nullptr);
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(!sigExprWatcher.wasDeleted());
        QCOMPARE(QQmlPropertyPrivate::signalExpression(prop), sigExpr);
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfMethod("oddlyNamedNotifySignal()"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }
}

void tst_qqmlproperty::qmlmetaproperty_object_context()
{
    QObject object; // Has no default property
    PropertyObject dobject; // Has default property

    {
        QQmlProperty prop(&object, engine.rootContext());

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&object, QObjectPrivate::get(&object)->signalIndex("destroyed()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString());
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Invalid);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), false);
        QCOMPARE(prop.object(), (QObject *)nullptr);
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QVERIFY(!prop.property().name());
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), -1);
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, engine.rootContext());

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QObjectPrivate::get(&dobject)->signalIndex("clicked()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("defaultProperty"));
        QCOMPARE(prop.read(), QVariant(10));
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), true);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Property);
        QCOMPARE(prop.isProperty(), true);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), true);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::Normal);
        QCOMPARE(prop.propertyType(), QMetaType::Int);
        QCOMPARE(prop.propertyTypeName(), "int");
        QCOMPARE(QString(prop.property().name()), QString("defaultProperty"));
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Unable to assign null to int");
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding);
        QCOMPARE(QQmlPropertyPrivate::binding(prop), binding.data());
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfProperty("defaultProperty"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }
}

void tst_qqmlproperty::qmlmetaproperty_object_string_context()
{
    QObject object;
    PropertyObject dobject;

    {
        QQmlProperty prop(&object, QString("defaultProperty"), engine.rootContext());

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&object, QObjectPrivate::get(&object)->signalIndex("destroyed()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString());
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Invalid);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), false);
        QCOMPARE(prop.object(), (QObject *)nullptr);
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QVERIFY(!prop.property().name());
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), -1);
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, QString("defaultProperty"), engine.rootContext());

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QObjectPrivate::get(&dobject)->signalIndex("clicked()"), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("defaultProperty"));
        QCOMPARE(prop.read(), QVariant(10));
        QCOMPARE(prop.write(QVariant()), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), true);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QVERIFY(!prop.method().isValid());
        QCOMPARE(prop.type(), QQmlProperty::Property);
        QCOMPARE(prop.isProperty(), true);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), true);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), false);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::Normal);
        QCOMPARE(prop.propertyType(), QMetaType::Int);
        QCOMPARE(prop.propertyTypeName(), "int");
        QCOMPARE(QString(prop.property().name()), QString("defaultProperty"));
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Unable to assign null to int");
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding);
        QCOMPARE(QQmlPropertyPrivate::binding(prop), binding.data());
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(sigExprWatcher.wasDeleted());
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfProperty("defaultProperty"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, QString("onClicked"), engine.rootContext());

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QQmlPropertyPrivate::get(prop)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("onClicked"));
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant("Hello")), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QCOMPARE(QString(prop.method().methodSignature()), QString("clicked()"));
        QCOMPARE(prop.type(), QQmlProperty::SignalProperty);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), true);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QCOMPARE(prop.property().name(), (const char *)nullptr);
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(!sigExprWatcher.wasDeleted());
        QCOMPARE(QQmlPropertyPrivate::signalExpression(prop), sigExpr);
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfMethod("clicked()"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }

    {
        QQmlProperty prop(&dobject, QString("onPropertyWithNotifyChanged"), engine.rootContext());

        QQmlAbstractBinding::Ptr binding(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core, QLatin1String("null"), nullptr, QQmlContextData::get(engine.rootContext())));
        static_cast<QQmlBinding *>(binding.data())->setTarget(prop);
        QVERIFY(binding);
        QQmlBoundSignalExpression *sigExpr = new QQmlBoundSignalExpression(&dobject, QQmlPropertyPrivate::get(prop)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1);
        QQmlJavaScriptExpression::DeleteWatcher sigExprWatcher(sigExpr);
        QVERIFY(sigExpr != nullptr && !sigExprWatcher.wasDeleted());

        QScopedPointer<QObject> obj(new QObject);

        QCOMPARE(prop.name(), QString("onOddlyNamedNotifySignal"));
        QCOMPARE(prop.read(), QVariant());
        QCOMPARE(prop.write(QVariant("Hello")), false);
        QCOMPARE(prop.hasNotifySignal(), false);
        QCOMPARE(prop.needsNotifySignal(), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), SLOT(deleteLater())), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), 0), false);
        QCOMPARE(prop.connectNotifySignal(nullptr, obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), obj->metaObject()->indexOfMethod(
                                              "deleteLater()")), false);
        QCOMPARE(prop.connectNotifySignal(obj.data(), -1), false);
        QCOMPARE(QString(prop.method().methodSignature()), QString("oddlyNamedNotifySignal()"));
        QCOMPARE(prop.type(), QQmlProperty::SignalProperty);
        QCOMPARE(prop.isProperty(), false);
        QCOMPARE(prop.isWritable(), false);
        QCOMPARE(prop.isDesignable(), false);
        QCOMPARE(prop.isResettable(), false);
        QCOMPARE(prop.isSignalProperty(), true);
        QCOMPARE(prop.isValid(), true);
        QCOMPARE(prop.object(), qobject_cast<QObject*>(&dobject));
        QCOMPARE(prop.propertyTypeCategory(), QQmlProperty::InvalidCategory);
        QCOMPARE(prop.propertyType(), 0);
        QCOMPARE(prop.propertyTypeName(), (const char *)nullptr);
        QCOMPARE(prop.property().name(), (const char *)nullptr);
        QVERIFY(!QQmlPropertyPrivate::binding(prop));
        QQmlPropertyPrivate::setBinding(prop, binding.data());
        QVERIFY(binding->ref == 1);
        QVERIFY(!QQmlPropertyPrivate::signalExpression(prop));
        QQmlPropertyPrivate::takeSignalExpression(prop, sigExpr);
        QVERIFY(!sigExprWatcher.wasDeleted());
        QCOMPARE(QQmlPropertyPrivate::signalExpression(prop), sigExpr);
        QCOMPARE(prop.index(), dobject.metaObject()->indexOfMethod("oddlyNamedNotifySignal()"));
        QCOMPARE(QQmlPropertyPrivate::propertyIndex(prop).valueTypeIndex(), -1);
        QVERIFY(!QQmlPropertyPrivate::propertyIndex(prop).hasValueTypeIndex());
    }
}

void tst_qqmlproperty::name()
{
    {
        QQmlProperty p;
        QCOMPARE(p.name(), QString());
    }

    {
        PropertyObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.name(), QString("defaultProperty"));
    }

    {
        QObject o;
        QQmlProperty p(&o, QString("objectName"));
        QCOMPARE(p.name(), QString("objectName"));
    }

    {
        PropertyObject o;
        QQmlProperty p(&o, "onClicked");
        QCOMPARE(p.name(), QString("onClicked"));
    }

    {
        QObject o;
        QQmlProperty p(&o, "onClicked");
        QCOMPARE(p.name(), QString());
    }

    {
        PropertyObject o;
        QQmlProperty p(&o, "onPropertyWithNotifyChanged");
        QCOMPARE(p.name(), QString("onOddlyNamedNotifySignal"));
    }

    {
        QObject o;
        QQmlProperty p(&o, "onPropertyWithNotifyChanged");
        QCOMPARE(p.name(), QString());
    }

    {
        QObject o;
        QQmlProperty p(&o, "foo");
        QCOMPARE(p.name(), QString());
    }

    {
        QQmlProperty p(nullptr, "foo");
        QCOMPARE(p.name(), QString());
    }

    {
        PropertyObject o;
        QQmlProperty p(&o, "rectProperty");
        QCOMPARE(p.name(), QString("rectProperty"));
    }

    {
        PropertyObject o;
        QQmlProperty p(&o, "rectProperty.x");
        QCOMPARE(p.name(), QString("rectProperty.x"));
    }

    {
        PropertyObject o;
        QQmlProperty p(&o, "rectProperty.foo");
        QCOMPARE(p.name(), QString());
    }
}

void tst_qqmlproperty::read()
{
    // Invalid
    {
        QQmlProperty p;
        QCOMPARE(p.read(), QVariant());
    }

    // Default prop
    {
        PropertyObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.read(), QVariant(10));
    }

    // Invalid default prop
    {
        QObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.read(), QVariant());
    }

    // Value prop by name
    {
        QObject o;

        QQmlProperty p(&o, "objectName");
        QCOMPARE(p.read(), QVariant(QString()));

        o.setObjectName("myName");

        QCOMPARE(p.read(), QVariant("myName"));
    }

    // Value prop by name (static)
    {
        QObject o;

        QCOMPARE(QQmlProperty::read(&o, "objectName"), QVariant(QString()));

        o.setObjectName("myName");

        QCOMPARE(QQmlProperty::read(&o, "objectName"), QVariant("myName"));
    }

    // Value-type prop
    {
        PropertyObject o;
        QQmlProperty p(&o, "rectProperty.x");
        QCOMPARE(p.read(), QVariant(10));
    }

    // Invalid value-type prop
    {
        PropertyObject o;
        QQmlProperty p(&o, "rectProperty.foo");
        QCOMPARE(p.read(), QVariant());
    }

    // Signal property
    {
        PropertyObject o;
        QQmlProperty p(&o, "onClicked");
        QCOMPARE(p.read(), QVariant());

        QQmlPropertyPrivate::takeSignalExpression(p, new QQmlBoundSignalExpression(&o, QQmlPropertyPrivate::get(p)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1));
        QVERIFY(nullptr != QQmlPropertyPrivate::signalExpression(p));

        QCOMPARE(p.read(), QVariant());
    }

    // Automatic signal property
    {
        PropertyObject o;
        QQmlProperty p(&o, "onPropertyWithNotifyChanged");
        QCOMPARE(p.read(), QVariant());

        QQmlPropertyPrivate::takeSignalExpression(p, new QQmlBoundSignalExpression(&o, QQmlPropertyPrivate::get(p)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1));
        QVERIFY(nullptr != QQmlPropertyPrivate::signalExpression(p));

        QCOMPARE(p.read(), QVariant());
    }

    // Deleted object
    {
        QScopedPointer<PropertyObject> o(new PropertyObject);
        QQmlProperty p(o.data(), "rectProperty.x");
        QCOMPARE(p.read(), QVariant(10));
        o.reset();
        QCOMPARE(p.read(), QVariant());
    }

    // Object property registered with Qt, but not registered with QML.
    {
        PropertyObject o;
        QQmlProperty p(&o, "qObject");
        QCOMPARE(p.propertyTypeCategory(), QQmlProperty::Object);

        QCOMPARE(p.propertyType(), qMetaTypeId<MyQObject*>());
        QVariant v = p.read();
        QVERIFY(v.canConvert(QMetaType(QMetaType::QObjectStar)));
        QVERIFY(qvariant_cast<QObject *>(v) == o.qObject());
    }
    {
        QQmlEngine engine;
        PropertyObject o;
        QQmlProperty p(&o, "qObject", &engine);
        QCOMPARE(p.propertyTypeCategory(), QQmlProperty::Object);

        QCOMPARE(p.propertyType(), qMetaTypeId<MyQObject*>());
        QVariant v = p.read();
        QVERIFY(v.canConvert(QMetaType(QMetaType::QObjectStar)));
        QVERIFY(qvariant_cast<QObject *>(v) == o.qObject());
    }

    // Object property
    {
        PropertyObject o;
        QQmlProperty p(&o, "qmlObject");
        QCOMPARE(p.propertyTypeCategory(), QQmlProperty::Object);
        QCOMPARE(p.propertyType(), qMetaTypeId<MyQmlObject*>());
        QVariant v = p.read();
        QCOMPARE(v.typeId(), QMetaType::QObjectStar);
        QVERIFY(qvariant_cast<QObject *>(v) == o.qmlObject());
    }
    {
        QQmlComponent component(&engine, testFileUrl("readSynthesizedObject.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QQmlProperty p(object.data(), "test", &engine);

        QCOMPARE(p.propertyTypeCategory(), QQmlProperty::Object);
        QVERIFY(p.propertyType() != QMetaType::QObjectStar);

        QVariant v = p.read();
        QCOMPARE(v.typeId(), QMetaType::QObjectStar);
        QCOMPARE(qvariant_cast<QObject *>(v)->property("a").toInt(), 10);
        QCOMPARE(qvariant_cast<QObject *>(v)->property("b").toInt(), 19);
    }
    {   // static
        QQmlComponent component(&engine, testFileUrl("readSynthesizedObject.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QVariant v = QQmlProperty::read(object.data(), "test", &engine);
        QCOMPARE(v.typeId(), QMetaType::QObjectStar);
        QCOMPARE(qvariant_cast<QObject *>(v)->property("a").toInt(), 10);
        QCOMPARE(qvariant_cast<QObject *>(v)->property("b").toInt(), 19);
    }

    // Attached property
    {
        QQmlComponent component(&engine);
        component.setData("import Test 1.0\nMyContainer { }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QQmlProperty p(object.data(), "MyContainer.foo", qmlContext(object.data()));
        QCOMPARE(p.read(), QVariant(13));
    }
    {
        QQmlComponent component(&engine);
        component.setData("import Test 1.0\nMyContainer { MyContainer.foo: 10 }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QQmlProperty p(object.data(), "MyContainer.foo", qmlContext(object.data()));
        QCOMPARE(p.read(), QVariant(10));
    }
    {
        QQmlComponent component(&engine);
        component.setData("import Test 1.0 as Foo\nFoo.MyContainer { Foo.MyContainer.foo: 10 }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QQmlProperty p(object.data(), "Foo.MyContainer.foo", qmlContext(object.data()));
        QCOMPARE(p.read(), QVariant(10));
    }
    {   // static
        QQmlComponent component(&engine);
        component.setData("import Test 1.0 as Foo\nFoo.MyContainer { Foo.MyContainer.foo: 10 }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(QQmlProperty::read(object.data(), "Foo.MyContainer.foo",
                                    qmlContext(object.data())), QVariant(10));
    }
}

void tst_qqmlproperty::write()
{
    // Invalid
    {
        QQmlProperty p;
        QCOMPARE(p.write(QVariant(10)), false);
    }

    // Read-only default prop
    {
        PropertyObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.write(QVariant(10)), false);
    }

    // Invalid default prop
    {
        QObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.write(QVariant(10)), false);
    }

    // Read-only prop by name
    {
        PropertyObject o;
        QQmlProperty p(&o, QString("defaultProperty"));
        QCOMPARE(p.write(QVariant(10)), false);
    }

    // Writable prop by name
    {
        PropertyObject o;
        QQmlProperty p(&o, QString("objectName"));
        QCOMPARE(o.objectName(), QString());
        QCOMPARE(p.write(QVariant(QString("myName"))), true);
        QCOMPARE(o.objectName(), QString("myName"));
    }

    // Writable prop by name (static)
    {
        PropertyObject o;
        QCOMPARE(QQmlProperty::write(&o, QString("objectName"), QVariant(QString("myName"))), true);
        QCOMPARE(o.objectName(), QString("myName"));
    }

    // Deleted object
    {
        QScopedPointer<PropertyObject> o(new PropertyObject);
        QQmlProperty p(o.data(), QString("objectName"));
        QCOMPARE(p.write(QVariant(QString("myName"))), true);
        QCOMPARE(o->objectName(), QString("myName"));

        o.reset();

        QCOMPARE(p.write(QVariant(QString("myName"))), false);
    }

    // Signal property
    {
        PropertyObject o;
        QQmlProperty p(&o, "onClicked");
        QCOMPARE(p.write(QVariant("console.log(1921)")), false);

        QQmlPropertyPrivate::takeSignalExpression(p, new QQmlBoundSignalExpression(&o, QQmlPropertyPrivate::get(p)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1));
        QVERIFY(nullptr != QQmlPropertyPrivate::signalExpression(p));

        QCOMPARE(p.write(QVariant("console.log(1921)")), false);

        QVERIFY(nullptr != QQmlPropertyPrivate::signalExpression(p));
    }

    // Automatic signal property
    {
        PropertyObject o;
        QQmlProperty p(&o, "onPropertyWithNotifyChanged");
        QCOMPARE(p.write(QVariant("console.log(1921)")), false);

        QQmlPropertyPrivate::takeSignalExpression(p, new QQmlBoundSignalExpression(&o, QQmlPropertyPrivate::get(p)->signalIndex(), QQmlContextData::get(engine.rootContext()), nullptr, QLatin1String("null"), QString(), -1, -1));
        QVERIFY(nullptr != QQmlPropertyPrivate::signalExpression(p));

        QCOMPARE(p.write(QVariant("console.log(1921)")), false);

        QVERIFY(nullptr != QQmlPropertyPrivate::signalExpression(p));
    }

    // Value-type property
    {
        PropertyObject o;
        QQmlProperty p(&o, "wrectProperty");

        QCOMPARE(o.wrectProperty(), QRect());
        QCOMPARE(p.write(QRect(1, 13, 99, 8)), true);
        QCOMPARE(o.wrectProperty(), QRect(1, 13, 99, 8));

        QQmlProperty p2(&o, "wrectProperty.x");
        QCOMPARE(p2.read(), QVariant(1));
        QCOMPARE(p2.write(QVariant(6)), true);
        QCOMPARE(p2.read(), QVariant(6));
        QCOMPARE(o.wrectProperty(), QRect(6, 13, 99, 8));
    }

    // URL-property
    {
        PropertyObject o;
        QQmlProperty p(&o, "url");
        const QUrl url = QUrl("main.qml");

        QCOMPARE(p.write(url), true);
        QCOMPARE(o.url(), url);

        QQmlProperty p2(&o, "url", engine.rootContext());

        QCOMPARE(p2.write(url), true);
        QCOMPARE(o.url(), url);
    }
    {   // static
        PropertyObject o;
        const QUrl url = QUrl("main.qml");

        QCOMPARE(QQmlProperty::write(&o, "url", url), true);
        QCOMPARE(o.url(), url);

        QCOMPARE(QQmlProperty::write(&o, "url", url, engine.rootContext()), true);
        QCOMPARE(o.url(), url);
    }

    // Char/string-property
    {
        PropertyObject o;
        QQmlProperty qcharProperty(&o, "qcharProperty");
        QQmlProperty stringProperty(&o, "stringProperty");

        const char16_t black_circle = 0x25cf;

        QCOMPARE(qcharProperty.write(QString("foo")), false);
        QCOMPARE(qcharProperty.write('Q'), true);
        QCOMPARE(qcharProperty.read(), QChar('Q'));
        QCOMPARE(qcharProperty.write(QChar(black_circle)), true);
        QCOMPARE(qcharProperty.read(), QChar(black_circle));

        QCOMPARE(o.stringProperty(), QString("foo")); // Default value
        QCOMPARE(stringProperty.write(QString("bar")), true);
        QCOMPARE(o.stringProperty(), QString("bar"));
        QCOMPARE(stringProperty.write(QVariant(1234)), true);
        QCOMPARE(stringProperty.read().toString(), QString::number(1234));
        QCOMPARE(stringProperty.write('A'), true);
        QCOMPARE(stringProperty.read().toString(), QString::number('A'));
        QCOMPARE(stringProperty.write(QChar(black_circle)), true);
        QCOMPARE(stringProperty.read(), QString(black_circle));

        { // QChar -> QString
            QQmlComponent component(&engine);
            component.setData("import Test 1.0\nPropertyObject { stringProperty: constQChar }", QUrl());
            QScopedPointer<QObject> object(component.create());
            PropertyObject *propertyObject = qobject_cast<PropertyObject*>(object.data());
            QVERIFY(propertyObject != nullptr);
            if (propertyObject) {
                QQmlProperty stringProperty(propertyObject, "stringProperty");
                QCOMPARE(stringProperty.read(), QVariant(QString(propertyObject->constQChar())));
            }
        }

    }

    // VariantMap-property
    QVariantMap vm;
    vm.insert("key", "value");

    {
        PropertyObject o;
        QQmlProperty p(&o, "variantMap");

        QCOMPARE(p.write(vm), true);
        QCOMPARE(o.variantMap(), vm);

        QQmlProperty p2(&o, "variantMap", engine.rootContext());

        QCOMPARE(p2.write(vm), true);
        QCOMPARE(o.variantMap(), vm);
    }
    {   // static
        PropertyObject o;

        QCOMPARE(QQmlProperty::write(&o, "variantMap", vm), true);
        QCOMPARE(o.variantMap(), vm);

        QCOMPARE(QQmlProperty::write(&o, "variantMap", vm, engine.rootContext()), true);
        QCOMPARE(o.variantMap(), vm);
    }

    // Attached property
    {
        QQmlComponent component(&engine);
        component.setData("import Test 1.0\nMyContainer { }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QQmlProperty p(object.data(), "MyContainer.foo", qmlContext(object.data()));
        p.write(QVariant(99));
        QCOMPARE(p.read(), QVariant(99));
    }
    {
        QQmlComponent component(&engine);
        component.setData("import Test 1.0 as Foo\nFoo.MyContainer { Foo.MyContainer.foo: 10 }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QQmlProperty p(object.data(), "Foo.MyContainer.foo", qmlContext(object.data()));
        p.write(QVariant(99));
        QCOMPARE(p.read(), QVariant(99));
    }
    // Writable pointer to QObject derived
    {
        PropertyObject o;
        QQmlProperty p(&o, QString("qObject"));
        QCOMPARE(o.qObject(), (QObject*)nullptr);
        QObject *newObject = new MyQObject(this);
        QCOMPARE(p.write(QVariant::fromValue(newObject)), true);
        QCOMPARE(o.qObject(), newObject);
        QVariant data = p.read();
        QCOMPARE(data.value<QObject*>(), newObject);
        QCOMPARE(data.value<MyQObject*>(), newObject);
        // Incompatible types can not be written.
        QCOMPARE(p.write(QVariant::fromValue(new MyQmlObject(this))), false);
        QVariant newData = p.read();
        QCOMPARE(newData.value<QObject*>(), newObject);
        QCOMPARE(newData.value<MyQObject*>(), newObject);
    }
    {
        QQmlEngine engine;
        PropertyObject o;
        QQmlProperty p(&o, QString("qObject"), &engine);
        QCOMPARE(o.qObject(), (QObject*)nullptr);
        QObject *newObject = new MyQObject(this);
        QCOMPARE(p.write(QVariant::fromValue(newObject)), true);
        QCOMPARE(o.qObject(), newObject);
        QVariant data = p.read();
        QCOMPARE(data.value<QObject*>(), newObject);
        QCOMPARE(data.value<MyQObject*>(), newObject);
        // Incompatible types can not be written.
        QCOMPARE(p.write(QVariant::fromValue(new MyQmlObject(this))), false);
        QVariant newData = p.read();
        QCOMPARE(newData.value<QObject*>(), newObject);
        QCOMPARE(newData.value<MyQObject*>(), newObject);
    }
}

void tst_qqmlproperty::reset()
{
    // Invalid
    {
        QQmlProperty p;
        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }

    // Read-only default prop
    {
        PropertyObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }

    // Invalid default prop
    {
        QObject o;
        QQmlProperty p(&o);
        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }

    // Non-resettable-only prop by name
    {
        PropertyObject o;
        QQmlProperty p(&o, QString("defaultProperty"));
        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }

    // Resettable prop by name
    {
        PropertyObject o;
        QQmlProperty p(&o, QString("resettableProperty"));

        QCOMPARE(p.read(), QVariant(9));
        QCOMPARE(p.write(QVariant(11)), true);
        QCOMPARE(p.read(), QVariant(11));

        QCOMPARE(p.isResettable(), true);
        QCOMPARE(p.reset(), true);

        QCOMPARE(p.read(), QVariant(9));
    }

    // Deleted object
    {
        QScopedPointer<PropertyObject> o(new PropertyObject);

        QQmlProperty p(o.data(), QString("resettableProperty"));

        QCOMPARE(p.isResettable(), true);
        QCOMPARE(p.reset(), true);

        o.reset();

        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }

    // Signal property
    {
        PropertyObject o;
        QQmlProperty p(&o, "onClicked");

        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }

    // Automatic signal property
    {
        PropertyObject o;
        QQmlProperty p(&o, "onPropertyWithNotifyChanged");

        QCOMPARE(p.isResettable(), false);
        QCOMPARE(p.reset(), false);
    }
}

void tst_qqmlproperty::writeObjectToList()
{
    QQmlComponent containerComponent(&engine);
    containerComponent.setData("import Test 1.0\nMyContainer { children: MyQmlObject {} }", QUrl());
    QScopedPointer<QObject> object(containerComponent.create());
    MyContainer *container = qobject_cast<MyContainer*>(object.data());
    QVERIFY(container != nullptr);
    QQmlListReference list(container, "children");
    QCOMPARE(list.count(), 1);

    QScopedPointer<MyQmlObject> childObject(new MyQmlObject);
    QQmlProperty prop(container, "children");
    prop.write(QVariant::fromValue(childObject.data()));
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.at(0), qobject_cast<QObject*>(childObject.data()));
}

void tst_qqmlproperty::writeListToList()
{
    QQmlComponent containerComponent(&engine);
    containerComponent.setData("import Test 1.0\nMyContainer { children: MyQmlObject {} }", QUrl());
    QScopedPointer<QObject> object(containerComponent.create());
    MyContainer *container = qobject_cast<MyContainer*>(object.data());
    QVERIFY(container != nullptr);
    QQmlListReference list(container, "children");
    QCOMPARE(list.count(), 1);

    QList<QObject*> objList;
    objList << new MyQmlObject(this) << new MyQmlObject(this)
            << new MyQmlObject(this) << new MyQmlObject(this);
    QQmlProperty prop(container, "children");
    prop.write(QVariant::fromValue(objList));
    QCOMPARE(list.count(), 4);

    //XXX need to try this with read/write prop (for read-only it correctly doesn't write)
    /*QList<MyQmlObject*> typedObjList;
    typedObjList << new MyQmlObject();
    prop.write(QVariant::fromValue(&typedObjList));
    QCOMPARE(container->children()->size(), 1);*/
}

void tst_qqmlproperty::listOverrideBehavior()
{
    QQmlComponent alwaysAppendContainerComponent(&engine, testFileUrl("ListOverrideAlwaysAppendOverridenContainer.qml"));
    QScopedPointer<QObject> alwaysAppendObject(alwaysAppendContainerComponent.create());
    MyContainer *alwaysAppendContainer = qobject_cast<MyContainer*>(alwaysAppendObject.data());
    QVERIFY(alwaysAppendContainer != nullptr);
    QQmlListReference alwaysAppendChildrenList(alwaysAppendContainer, "children");
    QCOMPARE(alwaysAppendChildrenList.count(), 5);
    QQmlComponent replaceIfNotDefaultContainerComponent(&engine, testFileUrl("ListOverrideReplaceIfNotDefaultOverridenContainer.qml"));
    QScopedPointer<QObject> replaceIfNotDefaultObject(replaceIfNotDefaultContainerComponent.create());
    MyReplaceIfNotDefaultBehaviorContainer *replaceIfNotDefaultContainer = qobject_cast<MyReplaceIfNotDefaultBehaviorContainer*>(replaceIfNotDefaultObject.data());
    QVERIFY(replaceIfNotDefaultContainer != nullptr);
    QQmlListReference replaceIfNotDefaultDefaultList(replaceIfNotDefaultContainer, "defaultList");
    QCOMPARE(replaceIfNotDefaultDefaultList.count(), 5);
    QQmlListReference replaceIfNotDefaultChildrenList(replaceIfNotDefaultContainer, "children");
    QCOMPARE(replaceIfNotDefaultChildrenList.count(), 2);
    QQmlComponent alwaysReplaceContainerComponent(&engine, testFileUrl("ListOverrideAlwaysReplaceOverridenContainer.qml"));
    QScopedPointer<QObject> alwaysReplaceObject(alwaysReplaceContainerComponent.create());
    MyContainer *alwaysReplaceContainer = qobject_cast<MyContainer*>(alwaysReplaceObject.data());
    QVERIFY(alwaysReplaceContainer != nullptr);
    QQmlListReference alwaysReplaceChildrenList(alwaysReplaceContainer, "children");
    QCOMPARE(alwaysReplaceChildrenList.count(), 2);

    {
        QQmlComponent appendQml(&engine, testFileUrl("listBehaviorAppendPragma.qml"));
        QVERIFY2(appendQml.isReady(), qPrintable(appendQml.errorString()));
        QScopedPointer<QObject> o(appendQml.create());
        QVERIFY(o);
        QCOMPARE(o->property("length1").toInt(), 2);
        QCOMPARE(o->property("length2").toInt(), 1);
        QCOMPARE(o->property("default1").toInt(), 2);
        QCOMPARE(o->property("default2").toInt(), 1);
    }

    {
        QQmlComponent replaceQml(&engine, testFileUrl("listBehaviorReplacePragma.qml"));
        QVERIFY2(replaceQml.isReady(), qPrintable(replaceQml.errorString()));
        QScopedPointer<QObject> o(replaceQml.create());
        QVERIFY(o);
        QCOMPARE(o->property("length1").toInt(), 1);
        QCOMPARE(o->property("length2").toInt(), 1);
        QCOMPARE(o->property("default1").toInt(), 1);
        QCOMPARE(o->property("default2").toInt(), 1);
    }

    {
        QQmlComponent replaceIfNotDefaultQml(
                    &engine, testFileUrl("listBehaviorReplaceIfNotDefaultPragma.qml"));
        QVERIFY2(replaceIfNotDefaultQml.isReady(),
                 qPrintable(replaceIfNotDefaultQml.errorString()));
        QScopedPointer<QObject> o(replaceIfNotDefaultQml.create());
        QVERIFY(o);
        QCOMPARE(o->property("length1").toInt(), 1);
        QCOMPARE(o->property("length2").toInt(), 1);
        QCOMPARE(o->property("default1").toInt(), 2);
        QCOMPARE(o->property("default2").toInt(), 1);
    }

    {
        QQmlComponent fail1(&engine, testFileUrl("listBehaviorFail1.qml"));
        QVERIFY(fail1.isError());
        QVERIFY(fail1.errorString().contains(
                 QStringLiteral("Unknown list property assign behavior 'Foo' in pragma")));
    }

    {
        QQmlComponent fail2(&engine, testFileUrl("listBehaviorFail2.qml"));
        QVERIFY(fail2.isError());
        QVERIFY(fail2.errorString().contains(
                 QStringLiteral("Multiple list property assign behavior pragmas found")));
    }
}

void tst_qqmlproperty::urlHandling_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("scheme");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QByteArray>("encoded");

    QTest::newRow("unspecifiedFile")
        << QByteArray("main.qml")
        << QString("")
        << QString("main.qml")
        << QByteArray("main.qml");

    QTest::newRow("specifiedFile")
        << QByteArray("file:///main.qml")
        << QString("file")
        << QString("/main.qml")
        << QByteArray("file:///main.qml");

    QTest::newRow("httpFile")
        << QByteArray("http://www.example.com/main.qml")
        << QString("http")
        << QString("/main.qml")
        << QByteArray("http://www.example.com/main.qml");

    QTest::newRow("pathFile")
        << QByteArray("http://www.example.com/resources/main.qml")
        << QString("http")
        << QString("/resources/main.qml")
        << QByteArray("http://www.example.com/resources/main.qml");

    QTest::newRow("encodableName")
        << QByteArray("http://www.example.com/main file.qml")
        << QString("http")
        << QString("/main file.qml")
        << QByteArray("http://www.example.com/main%20file.qml");

    QTest::newRow("preencodedName")
        << QByteArray("http://www.example.com/resources%7Cmain%20file.qml")
        << QString("http")
        << QString("/resources|main file.qml")
        << QByteArray("http://www.example.com/resources%7Cmain%20file.qml");

    QTest::newRow("encodableQuery")
        << QByteArray("http://www.example.com/main.qml?type=text/qml&comment=now working?")
        << QString("http")
        << QString("/main.qml")
        << QByteArray("http://www.example.com/main.qml?type=text/qml&comment=now%20working?");

    QTest::newRow("preencodedQuery")
        << QByteArray("http://www.example.com/main.qml?type=text%2Fqml&comment=now working%3F")
        << QString("http")
        << QString("/main.qml")
        << QByteArray("http://www.example.com/main.qml?type=text%2Fqml&comment=now%20working%3F");

    QTest::newRow("encodableFragment")
        << QByteArray("http://www.example.com/main.qml?type=text/qml#start+30000|volume+50%")
        << QString("http")
        << QString("/main.qml")
        << QByteArray("http://www.example.com/main.qml?type=text/qml#start+30000%7Cvolume+50%25");

    QTest::newRow("improperlyEncodedFragment")
        << QByteArray("http://www.example.com/main.qml?type=text/qml#start+30000%7Cvolume%2B50%")
        << QString("http")
        << QString("/main.qml")
        << QByteArray("http://www.example.com/main.qml?type=text/qml#start+30000%257Cvolume%252B50%25");
}

void tst_qqmlproperty::urlHandling()
{
    QFETCH(QByteArray, input);
    QFETCH(QString, scheme);
    QFETCH(QString, path);
    QFETCH(QByteArray, encoded);

    QString inputString(QString::fromUtf8(input));

    {
        PropertyObject o;
        QQmlProperty p(&o, "url");

        // Test url written as QByteArray
        QCOMPARE(p.write(input), true);
        QUrl byteArrayResult(o.url());

        QCOMPARE(byteArrayResult.scheme(), scheme);
        QCOMPARE(byteArrayResult.path(), path);
        QCOMPARE(byteArrayResult.toString(QUrl::FullyEncoded), QString::fromUtf8(encoded));
        QCOMPARE(byteArrayResult.toEncoded(), encoded);
    }

    {
        PropertyObject o;
        QQmlProperty p(&o, "url");

        // Test url written as QString
        QCOMPARE(p.write(inputString), true);
        QUrl stringResult(o.url());

        QCOMPARE(stringResult.scheme(), scheme);
        QCOMPARE(stringResult.path(), path);
        QCOMPARE(stringResult.toString(QUrl::FullyEncoded), QString::fromUtf8(encoded));
        QCOMPARE(stringResult.toEncoded(), encoded);
    }
}

void tst_qqmlproperty::variantMapHandling_data()
{
    QTest::addColumn<QVariantMap>("vm");

    // Object literals
    {
        QVariantMap m;
        QTest::newRow("{}") << m;
    }
    {
        QVariantMap m;
        m["a"] = QVariantMap();
        QTest::newRow("{ a:{} }") << m;
    }
    {
        QVariantMap m, m2;
        m2["b"] = 10;
        m2["c"] = 20;
        m["a"] = m2;
        QTest::newRow("{ a:{b:10, c:20} }") << m;
    }
    {
        QVariantMap m;
        m["a"] = 10;
        m["b"] = QVariantList() << 20 << 30;
        QTest::newRow("{ a:10, b:[20, 30]}") << m;
    }

    // Cyclic objects
    {
        QVariantMap m;
        m["p"] = QVariantMap();
        QTest::newRow("var o={}; o.p=o") << m;
    }
    {
        QVariantMap m;
        m["p"] = 123;
        m["q"] = QVariantMap();
        QTest::newRow("var o={}; o.p=123; o.q=o") << m;
    }
}

void tst_qqmlproperty::variantMapHandling()
{
    QFETCH(QVariantMap, vm);

    PropertyObject o;
    QQmlProperty p(&o, "variantMap");

    QCOMPARE(p.write(vm), true);
    QCOMPARE(o.variantMap(), vm);
}

void tst_qqmlproperty::crashOnValueProperty()
{
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);
    QQmlComponent component(engine.data());

    component.setData("import Test 1.0\nPropertyObject { wrectProperty.x: 10 }", QUrl());
    QScopedPointer<QObject> object(component.create());
    PropertyObject *propertyObject = qobject_cast<PropertyObject*>(object.data());
    QVERIFY(propertyObject != nullptr);

    QQmlProperty p(propertyObject, "wrectProperty.x", qmlContext(propertyObject));
    QCOMPARE(p.name(), QString("wrectProperty.x"));

    QCOMPARE(p.read(), QVariant(10));

    //don't crash once the engine is deleted
    engine.reset();

    QCOMPARE(p.propertyTypeName(), "int");
    QCOMPARE(p.read(), QVariant(10));
    p.write(QVariant(20));
    QCOMPARE(p.read(), QVariant(20));
}

void tst_qqmlproperty::aliasPropertyBindings_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("subObject");

    QTest::newRow("same object") << "aliasPropertyBindings.qml" << "";
    QTest::newRow("different objects") << "aliasPropertyBindings2.qml" << "innerObject";
}

// QTBUG-13719, QTBUG-58271
void tst_qqmlproperty::aliasPropertyBindings()
{
    QFETCH(QString, file);
    QFETCH(QString, subObject);

    QQmlComponent component(&engine, testFileUrl(file));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    // the object where realProperty lives
    QObject *realPropertyObject = object.data();
    if (!subObject.isEmpty())
       realPropertyObject = object->property(subObject.toLatin1()).value<QObject*>();

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 90.);
    QCOMPARE(object->property("aliasProperty").toReal(), 90.);

    object->setProperty("test", 10);

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 110.);
    QCOMPARE(object->property("aliasProperty").toReal(), 110.);

    QQmlProperty realProperty(realPropertyObject, QLatin1String("realProperty"));
    QQmlProperty aliasProperty(object.data(), QLatin1String("aliasProperty"));

    // Check there is a binding on these two properties
    QVERIFY(QQmlPropertyPrivate::binding(realProperty) != nullptr);
    QVERIFY(QQmlPropertyPrivate::binding(aliasProperty) != nullptr);

    // Check that its the same binding on these two properties
    QCOMPARE(QQmlPropertyPrivate::binding(realProperty),
             QQmlPropertyPrivate::binding(aliasProperty));

    // Change the binding
    object->setProperty("state", QString("switch"));

    QVERIFY(QQmlPropertyPrivate::binding(realProperty) != nullptr);
    QVERIFY(QQmlPropertyPrivate::binding(aliasProperty) != nullptr);
    QCOMPARE(QQmlPropertyPrivate::binding(realProperty),
             QQmlPropertyPrivate::binding(aliasProperty));

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 96.);
    QCOMPARE(object->property("aliasProperty").toReal(), 96.);

    // Check the old binding really has not effect any more
    object->setProperty("test", 4);

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 96.);
    QCOMPARE(object->property("aliasProperty").toReal(), 96.);

    object->setProperty("test2", 9);

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 288.);
    QCOMPARE(object->property("aliasProperty").toReal(), 288.);

    // Revert
    object->setProperty("state", QString(""));

    QVERIFY(QQmlPropertyPrivate::binding(realProperty) != nullptr);
    QVERIFY(QQmlPropertyPrivate::binding(aliasProperty) != nullptr);
    QCOMPARE(QQmlPropertyPrivate::binding(realProperty),
             QQmlPropertyPrivate::binding(aliasProperty));

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 20.);
    QCOMPARE(object->property("aliasProperty").toReal(), 20.);

    object->setProperty("test2", 3);

    QCOMPARE(realPropertyObject->property("realProperty").toReal(), 20.);
    QCOMPARE(object->property("aliasProperty").toReal(), 20.);
}

void tst_qqmlproperty::copy()
{
    PropertyObject object;

    QScopedPointer<QQmlProperty> property(
                new QQmlProperty(&object, QLatin1String("defaultProperty")));
    QCOMPARE(property->name(), QString("defaultProperty"));
    QCOMPARE(property->read(), QVariant(10));
    QCOMPARE(property->type(), QQmlProperty::Property);
    QCOMPARE(property->propertyTypeCategory(), QQmlProperty::Normal);
    QCOMPARE(property->propertyType(), QMetaType::Int);

    QQmlProperty p1(*property);
    QCOMPARE(p1.name(), QString("defaultProperty"));
    QCOMPARE(p1.read(), QVariant(10));
    QCOMPARE(p1.type(), QQmlProperty::Property);
    QCOMPARE(p1.propertyTypeCategory(), QQmlProperty::Normal);
    QCOMPARE(p1.propertyType(), QMetaType::Int);

    QQmlProperty p2(&object, QLatin1String("url"));
    QCOMPARE(p2.name(), QString("url"));
    p2 = *property;
    QCOMPARE(p2.name(), QString("defaultProperty"));
    QCOMPARE(p2.read(), QVariant(10));
    QCOMPARE(p2.type(), QQmlProperty::Property);
    QCOMPARE(p2.propertyTypeCategory(), QQmlProperty::Normal);
    QCOMPARE(p2.propertyType(), QMetaType::Int);

    property.reset();

    QCOMPARE(p1.name(), QString("defaultProperty"));
    QCOMPARE(p1.read(), QVariant(10));
    QCOMPARE(p1.type(), QQmlProperty::Property);
    QCOMPARE(p1.propertyTypeCategory(), QQmlProperty::Normal);
    QCOMPARE(p1.propertyType(), QMetaType::Int);

    QCOMPARE(p2.name(), QString("defaultProperty"));
    QCOMPARE(p2.read(), QVariant(10));
    QCOMPARE(p2.type(), QQmlProperty::Property);
    QCOMPARE(p2.propertyTypeCategory(), QQmlProperty::Normal);
    QCOMPARE(p2.propertyType(), QMetaType::Int);

    p1 = QQmlProperty();
    QQmlPropertyPrivate *p2d = QQmlPropertyPrivate::get(p2);
    QCOMPARE(p2d->count(), 1);

    // Use a pointer to avoid compiler warning about self-assignment.
    QQmlProperty *p2p = &p2;
    *p2p = p2;

    QCOMPARE(p2d->count(), 1);
}

void tst_qqmlproperty::noContext()
{
    QQmlComponent compA(&engine, testFileUrl("NoContextTypeA.qml"));
    QQmlComponent compB(&engine, testFileUrl("NoContextTypeB.qml"));

    QScopedPointer<QObject> a(compA.create());
    QVERIFY(a != nullptr);
    QScopedPointer<QObject> b(compB.create());
    QVERIFY(b != nullptr);

    QVERIFY(QQmlProperty::write(b.data(), "myTypeA", QVariant::fromValue(a.data()), &engine));
}

void tst_qqmlproperty::assignEmptyVariantMap()
{
    PropertyObject o;

    QVariantMap map;
    map.insert("key", "value");
    o.setVariantMap(map);
    QCOMPARE(o.variantMap().size(), 1);
    QCOMPARE(o.variantMap().isEmpty(), false);


    QQmlComponent component(&engine, testFileUrl("assignEmptyVariantMap.qml"));
    QScopedPointer<QObject> obj(
                component.createWithInitialProperties({{"o", QVariant::fromValue(&o)}}));
    QVERIFY(obj);

    QCOMPARE(o.variantMap().size(), 0);
    QCOMPARE(o.variantMap().isEmpty(), true);
}

void tst_qqmlproperty::warnOnInvalidBinding()
{
    QUrl testUrl(testFileUrl("invalidBinding.qml"));
    QString expectedWarning;

    // V4 error message for property-to-property binding
    expectedWarning = testUrl.toString() + QString::fromLatin1(":6:5: Unable to assign QQuickText to QQuickRectangle");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toLatin1().constData());

    // V8 error message for function-to-property binding
    expectedWarning = testUrl.toString() + QString::fromLatin1(":7:5: Unable to assign QQuickText to QQuickRectangle");
    QTest::ignoreMessage(QtWarningMsg, expectedWarning.toLatin1().constData());

#if QT_CONFIG(regularexpression)
    // V8 error message for invalid binding to anchor
    const QRegularExpression warning(
                "^" + testUrl.toString()
                + ":14:9: Unable to assign QQuickItem_QML_\\d+ to QQuickAnchorLine$");
    QTest::ignoreMessage(QtWarningMsg, warning);
#endif

    QQmlComponent component(&engine, testUrl);
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);
}

void tst_qqmlproperty::deeplyNestedObject()
{
    PropertyObject o;
    QQmlProperty p(&o, "qmlObject.pointProperty.x");
    QCOMPARE(p.isValid(), true);

    p.write(14);
    QCOMPARE(p.read(), QVariant(14));
}

void tst_qqmlproperty::readOnlyDynamicProperties()
{
    QQmlComponent comp(&engine, testFileUrl("readonlyPrimitiveVsVar.qml"));
    QScopedPointer<QObject> obj(comp.create());
    QVERIFY(obj != nullptr);

    QVERIFY(!obj->metaObject()->property(obj->metaObject()->indexOfProperty("r_var")).isWritable());
    QVERIFY(!obj->metaObject()->property(obj->metaObject()->indexOfProperty("r_int")).isWritable());
    QVERIFY(obj->metaObject()->property(obj->metaObject()->indexOfProperty("w_var")).isWritable());
    QVERIFY(obj->metaObject()->property(obj->metaObject()->indexOfProperty("w_int")).isWritable());
}

void tst_qqmlproperty::aliasToIdWithMatchingQmlFileNameOnCaseInsensitiveFileSystem()
{
    const QUrl url = testFileUrl("aliasToIdWithMatchingQmlFileName.qml");
    QQmlEngine engine;
    QQmlComponent component(&engine, url);
    QScopedPointer<QObject> root(component.create());

    QQmlProperty property(root.data(), "testType.objectName", QQmlEngine::contextForObject(root.data()));
    QVERIFY(property.isValid());
}

// QTBUG-77027
void tst_qqmlproperty::nullPropertyBinding()
{
    const QUrl url = testFileUrl("nullPropertyBinding.qml");
    QQmlEngine engine;
    QQmlComponent component(&engine, url);
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "undefined");
    QMetaObject::invokeMethod(root.get(), "tog");
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "defined");
    QMetaObject::invokeMethod(root.get(), "tog");
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "undefined");
    QMetaObject::invokeMethod(root.get(), "tog");
}

void tst_qqmlproperty::interfaceBinding()
{
    qmlRegisterInterface<Interface>("Interface", 1);
    qmlRegisterType<A>("io.qt.bugreports", 1, 0, "A");
    qmlRegisterType<B>("io.qt.bugreports", 1, 0, "B");
    qmlRegisterType<C>("io.qt.bugreports", 1, 0, "C");
    qmlRegisterType<InterfaceConsumer>("io.qt.bugreports", 1, 0, "InterfaceConsumer");

    const QVector<QUrl> urls = {
        testFileUrl("interfaceBinding.qml"),
        testFileUrl("interfaceBinding2.qml")
    };

    for (const QUrl &url : urls) {
        QQmlEngine engine;
        QQmlComponent component(&engine, url);
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));

        QCOMPARE(root->findChild<QObject*>("a1")->property("testValue").toInt(), 42);
        QCOMPARE(root->findChild<QObject*>("a2")->property("testValue").toInt(), 43);
        QCOMPARE(root->findChild<QObject*>("a3")->property("testValue").toInt(), 44);
    }
}

void tst_qqmlproperty::floatToStringPrecision_data()
{
    QTest::addColumn<QString>("propertyName");
    QTest::addColumn<double>("number");
    QTest::addColumn<QString>("qtString");
    QTest::addColumn<QString>("jsString");

    QTest::newRow("3.4")           << "a" << 3.4           << "3.4"          << "3.4";
    QTest::newRow("0.035003945")   << "b" << 0.035003945   << "0.035003945"  << "0.035003945";
    QTest::newRow("0.0000012345")  << "c" << 0.0000012345  << "1.2345e-06"   << "0.0000012345";
    QTest::newRow("0.00000012345") << "d" << 0.00000012345 << "1.2345e-07"   << "1.2345e-7";
    QTest::newRow("1e20")          << "e" << 1e20          << "1e+20"        << "100000000000000000000";
    QTest::newRow("1e21")          << "f" << 1e21          << "1e+21"        << "1e+21";
}

void tst_qqmlproperty::floatToStringPrecision()
{
    QQmlComponent comp(&engine, testFileUrl("floatToStringPrecision.qml"));
    QScopedPointer<QObject> obj(comp.create());
    QVERIFY(obj != nullptr);

    QFETCH(QString, propertyName);
    QFETCH(double, number);
    QFETCH(QString, qtString);
    QFETCH(QString, jsString);

    QByteArray name = propertyName.toLatin1();
    QCOMPARE(obj->property(name).toDouble(), number);
    QCOMPARE(obj->property(name).toString(), qtString);

    QByteArray name1 = (propertyName + QLatin1Char('1')).toLatin1();
    QCOMPARE(obj->property(name1).toDouble(), number);
    QCOMPARE(obj->property(name1).toString(), qtString);

    QByteArray name2 = (propertyName + QLatin1Char('2')).toLatin1();
    QCOMPARE(obj->property(name2).toDouble(), number);
    QCOMPARE(obj->property(name2).toString(), jsString);
}

void tst_qqmlproperty::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<MyQmlObject>("Test",1,0,"MyQmlObject");
    qmlRegisterType<PropertyObject>("Test",1,0,"PropertyObject");
    qmlRegisterType<MyContainer>("Test",1,0,"MyContainer");
    qmlRegisterType<MyReplaceIfNotDefaultBehaviorContainer>("Test",1,0,"MyReplaceIfNotDefaultBehaviorContainer");
    qmlRegisterType<MyAlwaysReplaceBehaviorContainer>("Test",1,0,"MyAlwaysReplaceBehaviorContainer");
    qmlRegisterType<ListHolder>("Test", 1, 0, "ListHolder");
}

// QTBUG-60908
void tst_qqmlproperty::bindingToAlias()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("aliasToBinding.qml"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

void tst_qqmlproperty::nestedQQmlPropertyMap()
{
    QQmlPropertyMap mainPropertyMap;
    QQmlPropertyMap nestedPropertyMap;
    QQmlPropertyMap deeplyNestedPropertyMap;

    mainPropertyMap.insert("nesting1", QVariant::fromValue(&nestedPropertyMap));
    nestedPropertyMap.insert("value", 42);
    nestedPropertyMap.insert("nesting2", QVariant::fromValue(&deeplyNestedPropertyMap));
    deeplyNestedPropertyMap.insert("value", "success");

    QQmlProperty value{&mainPropertyMap, "nesting1.value"};
    QCOMPARE(value.read().toInt(), 42);

    QQmlProperty success{&mainPropertyMap, "nesting1.nesting2.value"};
    QCOMPARE(success.read().toString(), QLatin1String("success"));
}

void tst_qqmlproperty::underscorePropertyChangeHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick 2.12

        Item {
            property int __withUnderScore
        }
    )", QUrl::fromLocalFile("."));
    QScopedPointer<QObject> root { component.create() };
    QVERIFY(root);
    QQmlProperty changeHandler(root.get(), "on__WithUnderScoreChanged");
    QVERIFY(changeHandler.isValid());
    QVERIFY(changeHandler.isSignalProperty());
}

void tst_qqmlproperty::signalExpressionWithoutObject()
{
    QQmlProperty invalid;
    QQmlPropertyPrivate::setSignalExpression(invalid, nullptr);
    QQmlBoundSignalExpression *expr = QQmlPropertyPrivate::signalExpression(invalid);
    QVERIFY(!expr);
}

void tst_qqmlproperty::dontRemoveQPropertyBinding()
{
    QObject object;
    QQmlProperty objectName(&object, "objectName");
    QVERIFY(objectName.isBindable());
    QProperty<QString> name("hello");
    object.bindableObjectName().setBinding(Qt::makePropertyBinding(name));
    QVERIFY(object.bindableObjectName().hasBinding());

    // A write with DontRemoveBinding preserves the binding
    QQmlPropertyPrivate::write(objectName, u"goodbye"_s, QQmlPropertyData::DontRemoveBinding);
    QVERIFY(object.bindableObjectName().hasBinding());
    // but changes the value
    QCOMPARE(object.objectName(), u"goodbye"_s);
    // subsequent binding evaluations change the value again
    name = u"hello, again"_s;
    QCOMPARE(object.objectName(), name.value());

    // The binding is only preserved by the write which had DontRemoveBinding set
    // any further write will remove the binding
    QQmlPropertyPrivate::write(objectName, u"goodbye"_s, QQmlPropertyData::WriteFlags{});
    QCOMPARE(object.objectName(), u"goodbye"_s);
    QVERIFY(!object.bindableObjectName().hasBinding());
}

void tst_qqmlproperty::compatResolveUrls()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(
        import QtQml
        QtObject {
            property url a: "relative/url.png"
        }
    )", QUrl(QStringLiteral("qrc:/some/resource/path.qml")));
    QVERIFY(c.isReady());
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    if (qEnvironmentVariableIsSet("QML_COMPAT_RESOLVE_URLS_ON_ASSIGNMENT")) {
        QCOMPARE(qvariant_cast<QUrl>(o->property("a")),
                 QUrl(QStringLiteral("qrc:/some/resource/relative/url.png")));
        return;
    }

    QCOMPARE(qvariant_cast<QUrl>(o->property("a")), QUrl(QStringLiteral("relative/url.png")));

#ifdef Q_OS_ANDROID
    QSKIP("Can't start QProcess to run a custom user binary on Android");
#endif

#if QT_CONFIG(process)
    QProcess process;
    process.setProgram(QCoreApplication::applicationFilePath());
    process.setEnvironment(QProcess::systemEnvironment()
                           + QStringList(u"QML_COMPAT_RESOLVE_URLS_ON_ASSIGNMENT=1"_s));
    process.setArguments({QStringLiteral("compatResolveUrls")});
    process.start();
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
#else
    QSKIP("Testing the QML_COMPAT_RESOLVE_URLS_ON_ASSIGNMENT "
          "environment variable requires QProcess.");
#endif
}

void tst_qqmlproperty::initFlags_data()
{
    QTest::addColumn<bool>("passObject");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QQmlPropertyPrivate::InitFlags>("flags");

    const QString names[] = {
        QStringLiteral("foo"),
        QStringLiteral("self.foo"),
        QStringLiteral("onFoo"),
        QStringLiteral("self.onFoo"),
        QStringLiteral("bar"),
        QStringLiteral("self.bar"),
        QStringLiteral("abar"),
        QStringLiteral("self.abar"),
    };

    const QQmlPropertyPrivate::InitFlags flagSets[] = {
        QQmlPropertyPrivate::InitFlag::None,
        QQmlPropertyPrivate::InitFlag::AllowId,
        QQmlPropertyPrivate::InitFlag::AllowSignal,
        QQmlPropertyPrivate::InitFlag::AllowId | QQmlPropertyPrivate::InitFlag::AllowSignal,
    };

    for (int i = 0; i < 2; ++i) {
        const bool passObject = (i != 0);
        for (const QString &name : names) {
            for (const auto &flagSet : flagSets) {
                const QString rowName = QStringLiteral("%1,%2,%3")
                        .arg(passObject).arg(name).arg(flagSet.toInt());
                QTest::addRow("%s", qPrintable(rowName)) << passObject << name << flagSet;
            }
        }
    }
}

void tst_qqmlproperty::initFlags()
{
    QFETCH(bool, passObject);
    QFETCH(QString, name);
    QFETCH(QQmlPropertyPrivate::InitFlags, flags);

    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(
        import QtQml
        QtObject {
            id: self
            signal foo()
            property int bar: 12
            property alias abar: self.bar
        }
    )", QUrl());
    QVERIFY(c.isReady());
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlRefPointer<QQmlContextData> context = QQmlContextData::get(qmlContext(o.data()));

    const QQmlProperty property = QQmlPropertyPrivate::create(
                    passObject ? o.data() : nullptr, name, context, flags);

    const bool usesId = name.startsWith(QStringLiteral("self."));
    const bool hasSignal = name.endsWith(QStringLiteral("foo"));
    if (!passObject && !usesId) {
        QVERIFY(!property.isValid());
    } else if (passObject && usesId) {
        QVERIFY(!property.isValid());
    } else if (usesId && !(flags & QQmlPropertyPrivate::InitFlag::AllowId)) {
        QVERIFY(!property.isValid());
    } else if (hasSignal && !(flags & QQmlPropertyPrivate::InitFlag::AllowSignal)) {
        QVERIFY(!property.isValid());
    } else {
        QVERIFY(property.isValid());
        if (name.endsWith(QStringLiteral("bar"))) {
            QVERIFY(property.isProperty());
            QCOMPARE(property.name(), usesId ? name.mid(strlen("self.")) : name);
            QCOMPARE(property.propertyMetaType(), QMetaType::fromType<int>());
        } else {
            QVERIFY(property.isSignalProperty());
            QCOMPARE(property.name(), QStringLiteral("onFoo"));
            QVERIFY(!property.propertyMetaType().isValid());
        }
    }

}

void tst_qqmlproperty::constructFromPlainMetaObject()
{
    QScopedPointer<PropertyObject> obj(new PropertyObject);

    QQmlData *data = QQmlData::get(obj.data());
    QVERIFY(data == nullptr);

    QQmlProperty prop(obj.data(), "rectProperty");
    QVERIFY(prop.isValid());
    QVERIFY(prop.isProperty());
    QCOMPARE(prop.propertyMetaType(), QMetaType::fromType<QRect>());

    QQmlProperty sig(obj.data(), "onOddlyNamedNotifySignal");
    QVERIFY(sig.isValid());
    QVERIFY(sig.isSignalProperty());
    QVERIFY(!sig.propertyMetaType().isValid());

    data = QQmlData::get(obj.data());
    QVERIFY(data == nullptr);
}

void tst_qqmlproperty::bindToNonQObjectTarget()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("bindToNonQObjectTarget.qml");
    QQmlComponent component(&engine, url);

    // Yes, we can still create the component. The result of the script expression will only be
    // known once it's executed.
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QTest::ignoreMessage(QtWarningMsg,
                         qPrintable(url.toString() + ":14:7: Unable to assign QFont to QObject*"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

void tst_qqmlproperty::assignVariantList()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("assignVariantList.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    ListHolder *holder = qobject_cast<ListHolder *>(o.data());
    const QList<double> doubleList = {1.1, 2.2, 3.3, 11, 5.25, 11};
    QCOMPARE(holder->doubleList(), doubleList);
}

void tst_qqmlproperty::listAssignmentSignals()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("listAssignmentSignals.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QCOMPARE(root->property("signalCounter").toInt(), 1);
    QMetaObject::invokeMethod(root.get(), "assignList");
    QCOMPARE(root->property("signalCounter").toInt(), 2);
}

void tst_qqmlproperty::invalidateQPropertyChangeTriggers()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("invalidateQPropertyChangeTriggers.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QStringList names;
    QObject::connect(root.data(), &QObject::objectNameChanged, [&](const QString &name) {
        if (names.length() == 10)
            root->setProperty("running", false);
        else
            names.append(name);
    });

    root->setProperty("running", true);
    QTRY_VERIFY(!root->property("running").toBool());

    QCOMPARE(names, (QStringList {
        u""_s, u"1300"_s, u"Create Object"_s,
        u""_s, u"1300"_s, u"Create Object"_s,
        u""_s, u"1300"_s, u"Create Object"_s,
        u""_s
    }));
}

QTEST_MAIN(tst_qqmlproperty)

#include "tst_qqmlproperty.moc"
