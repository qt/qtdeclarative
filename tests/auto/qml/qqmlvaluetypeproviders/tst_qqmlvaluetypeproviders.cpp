// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QDebug>
#include <QScopedPointer>
#include <private/qqmlglobal_p.h>
#include <private/qquickvaluetypes_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include "testtypes.h"

QT_BEGIN_NAMESPACE
extern int qt_defaultDpi(void);
QT_END_NAMESPACE

// There is some overlap between the qqmllanguage and qqmlvaluetypes
// test here, but it needs to be separate to ensure that no QML plugins
// are loaded prior to these tests, which could contaminate the type
// system with more providers.

class tst_qqmlvaluetypeproviders : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlvaluetypeproviders() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;

    void qtqmlValueTypes();   // This test function _must_ be the first test function run.
    void qtquickValueTypes();
    void comparisonSemantics();
    void cppIntegration();
    void jsObjectConversion();
    void invokableFunctions();
    void userType();
    void changedSignal();
    void structured();
    void recursive();
    void date();
    void constructors();
};

void tst_qqmlvaluetypeproviders::initTestCase()
{
    QQmlDataTest::initTestCase();
    registerTypes();
}

void tst_qqmlvaluetypeproviders::qtqmlValueTypes()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("qtqmlValueTypes.qml"));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->property("qtqmlTypeSuccess").toBool());
    QVERIFY(object->property("qtquickTypeSuccess").toBool());
}

void tst_qqmlvaluetypeproviders::qtquickValueTypes()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("qtquickValueTypes.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->property("qtqmlTypeSuccess").toBool());
    QVERIFY(object->property("qtquickTypeSuccess").toBool());
}

void tst_qqmlvaluetypeproviders::comparisonSemantics()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("comparisonSemantics.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->property("comparisonSuccess").toBool());
}

void tst_qqmlvaluetypeproviders::cppIntegration()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("cppIntegration.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    // ensure accessing / comparing / assigning cpp-defined props
    // and qml-defined props works in QML.
    QVERIFY(object->property("success").toBool());

    // ensure types match
    QCOMPARE(object->property("g").userType(), object->property("rectf").userType());
    QCOMPARE(object->property("p").userType(), object->property("pointf").userType());
    QCOMPARE(object->property("z").userType(), object->property("sizef").userType());
    QCOMPARE(object->property("v2").userType(), object->property("vector2").userType());
    QCOMPARE(object->property("v3").userType(), object->property("vector").userType());
    QCOMPARE(object->property("v4").userType(), object->property("vector4").userType());
    QCOMPARE(object->property("q").userType(), object->property("quaternion").userType());
    QCOMPARE(object->property("m").userType(), object->property("matrix").userType());
    QCOMPARE(object->property("c").userType(), object->property("color").userType());
    QCOMPARE(object->property("f").userType(), object->property("font").userType());

    // ensure values match
    QCOMPARE(object->property("g").value<QRectF>(), object->property("rectf").value<QRectF>());
    QCOMPARE(object->property("p").value<QPointF>(), object->property("pointf").value<QPointF>());
    QCOMPARE(object->property("z").value<QSizeF>(), object->property("sizef").value<QSizeF>());
    QCOMPARE(object->property("v2").value<QVector2D>(), object->property("vector2").value<QVector2D>());
    QCOMPARE(object->property("v3").value<QVector3D>(), object->property("vector").value<QVector3D>());
    QCOMPARE(object->property("v4").value<QVector4D>(), object->property("vector4").value<QVector4D>());
    QCOMPARE(object->property("q").value<QQuaternion>(), object->property("quaternion").value<QQuaternion>());
    QCOMPARE(object->property("m").value<QMatrix4x4>(), object->property("matrix").value<QMatrix4x4>());
    QCOMPARE(object->property("c").value<QColor>(), object->property("color").value<QColor>());
    QCOMPARE(object->property("f").value<QFont>(), object->property("font").value<QFont>());
}

void tst_qqmlvaluetypeproviders::jsObjectConversion()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("jsObjectConversion.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->property("qtquickTypeSuccess").toBool());
}

void tst_qqmlvaluetypeproviders::invokableFunctions()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("invokableFunctions.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->property("complete").toBool());
    QVERIFY(object->property("success").toBool());
}

namespace {

// A value-type class to export to QML
class TestValue
{
public:
    TestValue() : m_p1(0), m_p2(0.0) {}
    TestValue(int p1, double p2) : m_p1(p1), m_p2(p2) {}
    TestValue(const TestValue &other) : m_p1(other.m_p1), m_p2(other.m_p2) {}
    ~TestValue() {}

    TestValue &operator=(const TestValue &other) { m_p1 = other.m_p1; m_p2 = other.m_p2; return *this; }

    int property1() const { return m_p1; }
    void setProperty1(int p1) { m_p1 = p1; }

    double property2() const { return m_p2; }
    void setProperty2(double p2) { m_p2 = p2; }

    bool operator==(const TestValue &other) const { return (m_p1 == other.m_p1) && (m_p2 == other.m_p2); }
    bool operator!=(const TestValue &other) const { return !operator==(other); }

    bool operator<(const TestValue &other) const { if (m_p1 < other.m_p1) return true; return m_p2 < other.m_p2; }

private:
    int m_p1;
    double m_p2;
};

}

Q_DECLARE_METATYPE(TestValue);

namespace {

class TestValueType
{
    TestValue v;
    Q_GADGET
    Q_PROPERTY(int property1 READ property1 WRITE setProperty1)
    Q_PROPERTY(double property2 READ property2 WRITE setProperty2)
public:
    Q_INVOKABLE QString toString() const { return QString::number(property1()) + QLatin1Char(',') + QString::number(property2()); }

    int property1() const { return v.property1(); }
    void setProperty1(int p1) { v.setProperty1(p1); }

    double property2() const { return v.property2(); }
    void setProperty2(double p2) { v.setProperty2(p2); }
};

class TestValueExporter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TestValue testValue READ testValue WRITE setTestValue)
    QML_NAMED_ELEMENT(TestValueExporter)
public:
    TestValue testValue() const { return m_testValue; }
    void setTestValue(const TestValue &v) { m_testValue = v; }

    Q_INVOKABLE TestValue getTestValue() const { return TestValue(333, 666.999); }

private:
    TestValue m_testValue;
};

}

void tst_qqmlvaluetypeproviders::userType()
{
    qmlRegisterExtendedType<TestValue, TestValueType>("Test", 1, 0, "test_value");
    qmlRegisterTypesAndRevisions<TestValueExporter>("Test", 1);
    Q_ASSERT(qMetaTypeId<TestValue>() >= QMetaType::User);

    TestValueExporter exporter;

    QQmlEngine e;

    QQmlComponent component(&e, testFileUrl("userType.qml"));
    QScopedPointer<QObject> obj(component.createWithInitialProperties({{"testValueExporter", QVariant::fromValue(&exporter)}}));
    QVERIFY(obj != nullptr);
    QCOMPARE(obj->property("success").toBool(), true);
}

void tst_qqmlvaluetypeproviders::changedSignal()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("changedSignal.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QVERIFY(object->property("complete").toBool());
    QVERIFY(object->property("success").toBool());
}

void tst_qqmlvaluetypeproviders::structured()
{
    QQmlEngine e;
    const QUrl url = testFileUrl("structuredValueTypes.qml");
    QQmlComponent component(&e, url);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    const char *warnings[] = {
        "Could not find any constructor for value type ConstructibleValueType to call"
            " with value [object Object]",
        "Could not find any constructor for value type ConstructibleValueType to call"
            " with value QVariant(QJSValue, )",
        "Could not find any constructor for value type ConstructibleValueType to call"
            " with value [object Object]",
        "Could not find any constructor for value type ConstructibleValueType to call"
            " with value QVariant(QVariantMap, QMap())",
        "Could not convert array value at position 5 from QVariantMap to ConstructibleValueType",
        "Could not find any constructor for value type ConstructibleValueType to call"
            " with value [object Object]",
        "Could not find any constructor for value type ConstructibleValueType to call"
            " with value QVariant(QJSValue, )"
    };

    for (const auto warning : warnings)
        QTest::ignoreMessage(QtWarningMsg, warning);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(
                             url.toString()  + QStringLiteral(":44: Error: Cannot assign QJSValue "
                                                              "to ConstructibleValueType")));

    QTest::ignoreMessage(QtWarningMsg, qPrintable(
                             url.toString()  + QStringLiteral(":14:5: Unable to assign QJSValue "
                                                              "to ConstructibleValueType")));

    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    QCOMPARE(o->property("p").value<QPointF>(), QPointF(7, 77));
    QCOMPARE(o->property("s").value<QSizeF>(), QSizeF(7, 77));
    QCOMPARE(o->property("r").value<QRectF>(), QRectF(5, 55, 7, 77));

    QCOMPARE(o->property("p2").value<QPointF>(), QPointF(4, 5));
    QCOMPARE(o->property("s2").value<QSizeF>(), QSizeF(7, 8));
    QCOMPARE(o->property("r2").value<QRectF>(), QRectF(9, 10, 11, 12));

    QCOMPARE(o->property("c1").value<ConstructibleValueType>(), ConstructibleValueType(5));
    QCOMPARE(o->property("c2").value<ConstructibleValueType>(), ConstructibleValueType(0));
    QCOMPARE(o->property("c3").value<ConstructibleValueType>(), ConstructibleValueType(99));
    QCOMPARE(o->property("c4").value<ConstructibleValueType>(), ConstructibleValueType(0));

    const QList<QPointF> actual = o->property("ps").value<QList<QPointF>>();
    const QList<QPointF> expected = {
        QPointF(1, 2), QPointF(3, 4), QPointF(55, std::numeric_limits<double>::quiet_NaN())
    };
    QCOMPARE(actual.size(), expected.size());
    QCOMPARE(actual[0], expected[0]);
    QCOMPARE(actual[1], expected[1]);
    QCOMPARE(actual[2].x(), expected[2].x());
    QVERIFY(std::isnan(actual[2].y()));

    QCOMPARE(o->property("ss").value<QList<QSizeF>>(),
             QList<QSizeF>({QSizeF(5, 6), QSizeF(7, 8), QSizeF(-1, 99)}));
    QCOMPARE(o->property("cs").value<QList<ConstructibleValueType>>(),
             QList<ConstructibleValueType>({1, 2, 3, 4, 5, 0}));

    StructuredValueType b1;
    b1.setI(10);
    b1.setC(14);
    b1.setP(QPointF(1, 44));
    QCOMPARE(o->property("b1").value<StructuredValueType>(), b1);

    StructuredValueType b2;
    b2.setI(11);
    b2.setC(15);
    b2.setP(QPointF(4, 0));
    QCOMPARE(o->property("b2").value<StructuredValueType>(), b2);


    QList<StructuredValueType> bb(3);
    bb[0].setI(21);
    bb[1].setC(22);
    bb[2].setP(QPointF(199, 222));
    QCOMPARE(o->property("bb").value<QList<StructuredValueType>>(), bb);

    MyTypeObject *t = qobject_cast<MyTypeObject *>(o.data());
    QVERIFY(t);

    QCOMPARE(t->constructible(), ConstructibleValueType(47));

    StructuredValueType structured;
    structured.setI(11);
    structured.setC(12);
    structured.setP(QPointF(7, 8));
    QCOMPARE(t->structured(), structured);

    QCOMPARE(o->property("cr1").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(11.25));
    QCOMPARE(o->property("cr2").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(std::numeric_limits<qreal>::infinity()));
    QCOMPARE(o->property("cr3").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(-std::numeric_limits<qreal>::infinity()));
    QCOMPARE(o->property("cr4").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(std::numeric_limits<qreal>::quiet_NaN()));
    QCOMPARE(o->property("cr5").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(0));
    QCOMPARE(o->property("cr6").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(-112.5));
    QCOMPARE(o->property("cr7").value<ConstructibleFromQReal>(),
             ConstructibleFromQReal(50));

    BarrenValueType barren;
    barren.setI(17);
    QCOMPARE(o->property("barren").value<BarrenValueType>(), barren);

    QMetaObject::invokeMethod(o.data(), "changeBarren");
    QCOMPARE(o->property("barren").value<BarrenValueType>(), BarrenValueType(QString()));

    QCOMPARE(o->property("fromObject").value<ConstructibleValueType>(),
             ConstructibleValueType(nullptr));
    QCOMPARE(o->property("aVariant").value<ConstructibleValueType>(),
             ConstructibleValueType(nullptr));

    QCOMPARE(o->property("listResult").toInt(), 12 + 67 + 68);


    // You can store all kinds of insanity in a VariantObject, but we generally don't.
    // Since we cannot rule out the possibility of there being such VariantObjects, we need to test
    // their conversions.


    QCOMPARE(o->property("fromInsanity").value<StructuredValueType>(), StructuredValueType());

    QV4::Scope scope(e.handle());
    QV4::ScopedString name(scope, scope.engine->newString("insanity"));

    QObject *po = o.data();
    QV4::ScopedObject js(
        scope, scope.engine->metaTypeToJS(QMetaType::fromType<MyTypeObject *>(), &po));

    const QVariantHash hash {
        {"i", 12},
        {"c", QUrl("http://example.com")},
        {"p", QVariantMap {
                  {"x", 17},
                  {"y", 18}
              }}
    };
    QV4::ScopedValue hashValue(
        scope, e.handle()->newVariantObject(QMetaType::fromType<QVariantHash>(), &hash));

    js->put(name, hashValue);

    StructuredValueType fromHash;
    fromHash.setI(12);
    fromHash.setC(ConstructibleValueType(QUrl()));
    fromHash.setP(QPointF(17, 18));

    QCOMPARE(o->property("fromInsanity").value<StructuredValueType>(), fromHash);

    const QVariantMap map {
        {"i", 13},
        {"c", QVariant::fromValue(po) },
        {"p", QVariantMap {
                  {"x", 19},
                  {"y", 20}
              }}
    };
    QV4::ScopedValue mapValue(
        scope, e.handle()->newVariantObject(QMetaType::fromType<QVariantMap>(), &map));
    js->put(name, mapValue);

    StructuredValueType fromMap;
    fromMap.setI(13);
    fromMap.setC(ConstructibleValueType(po));
    fromMap.setP(QPointF(19, 20));

    QCOMPARE(o->property("fromInsanity").value<StructuredValueType>(), fromMap);

    BarrenValueType immediate;
    immediate.setI(14);
    QV4::ScopedValue immediateValue(
        scope, e.handle()->newVariantObject(QMetaType::fromType<BarrenValueType>(), &immediate));
    js->put(name, immediateValue);

    StructuredValueType fromImmediate;
    fromImmediate.setI(14);

    QCOMPARE(o->property("fromInsanity").value<StructuredValueType>(), fromImmediate);

    QQmlComponent c2(&e);
    c2.setData(
        "import QtQml; QtObject { property int i: 99; property point p: ({x: 3, y: 4}) }", QUrl());
    QVERIFY(c2.isReady());
    QScopedPointer<QObject> o2(c2.create());
    QVERIFY(!o2.isNull());
    QObject *object = o2.data();
    QV4::ScopedValue objectValue(
        scope, e.handle()->newVariantObject(QMetaType::fromType<QObject *>(), &object));
    js->put(name, objectValue);

    StructuredValueType fromObject;
    fromObject.setI(99);
    fromObject.setP(QPointF(3, 4));

    QCOMPARE(o->property("fromInsanity").value<StructuredValueType>(), fromObject);

    const MyTypeObject *m = static_cast<const MyTypeObject *>(po);
    QVERIFY(!m->hasEffectPadding());
    QMetaObject::invokeMethod(po, "updatePadding");
    QVERIFY(m->hasEffectPadding());
    QCOMPARE(m->effectPadding(), QRectF());
    po->setProperty("newItemPadding", QRectF(1, 2, 3, 4));
    QMetaObject::invokeMethod(po, "updatePadding");
    QVERIFY(m->hasEffectPadding());
    QCOMPARE(m->effectPadding(), QRectF(1, 2, 3, 4));
}

void tst_qqmlvaluetypeproviders::recursive()
{
    QQmlEngine e;
    const QUrl url = testFileUrl("recursiveWriteBack.qml");
    QQmlComponent component(&e, url);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    const QList<StructuredValueType> l = o->property("l").value<QList<StructuredValueType>>();
    QCOMPARE(l.size(), 3);
    QCOMPARE(l[2].i(), 4);
    QCOMPARE(l[1].p().x(), 88);
    QCOMPARE(l[0].sizes()[1].width(), 19);

    MyTypeObject *m = qobject_cast<MyTypeObject *>(o.data());
    QCOMPARE(m->structured().p().x(), 76);

    // Recursive write back into a list detached from the property.
    QCOMPARE(m->property("aa").toInt(), 12);
}

void tst_qqmlvaluetypeproviders::date()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("dateWriteBack.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("aDateTime").value<QDateTime>().date().day(), 14);
    QCOMPARE(o->property("aDate").value<QDate>().month(), 9);
    QCOMPARE(o->property("aTime").value<QTime>().hour(), 5);
    QCOMPARE(o->property("aVariant").value<QDateTime>().time().minute(), 44);
}

void tst_qqmlvaluetypeproviders::constructors()
{

    QQmlEngine e;

    {
        const auto guard = qScopeGuard([]() { Padding::log.clear(); });
        QQmlComponent component(&e);
        component.setData("import Test\nMyItem { padding : 50 }", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> o(component.create());
        QVERIFY(!o.isNull());
        MyItem *item = qobject_cast<MyItem *>(o.data());
        QVERIFY(item);
        QCOMPARE(item->padding().left(), 50);
        QCOMPARE(item->padding().right(), 50);

        QCOMPARE(Padding::log.length(), 3);

        // Created by default ctor of MyItem
        QCOMPARE(Padding::log[0].type, Padding::CustomCtor);
        QCOMPARE(Padding::log[0].left, 17);
        QCOMPARE(Padding::log[0].right, 17);

        // Created by assignment of integer
        QCOMPARE(Padding::log[1].type, Padding::InvokableCtor);
        QCOMPARE(Padding::log[1].left, 50);
        QCOMPARE(Padding::log[1].right, 50);

        // In MyItem::setPadding()
        QCOMPARE(Padding::log[2].type, Padding::CopyAssign);
        QCOMPARE(Padding::log[2].left, 50);
        QCOMPARE(Padding::log[2].right, 50);
    }

    {
        const auto guard = qScopeGuard([]() { Padding::log.clear(); });
        QQmlComponent component(&e);
        component.setData("import Test\nMyItem { padding: ({ left: 10, right: 20 }) }", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> o(component.create());
        QVERIFY(!o.isNull());
        MyItem *item = qobject_cast<MyItem *>(o.data());
        QVERIFY(item);
        QCOMPARE(item->padding().left(), 10);
        QCOMPARE(item->padding().right(), 20);

        QCOMPARE(Padding::log.length(), 3);

        // Created by default ctor of MyItem
        QCOMPARE(Padding::log[0].type, Padding::CustomCtor);
        QCOMPARE(Padding::log[0].left, 17);
        QCOMPARE(Padding::log[0].right, 17);

        // Preparing for setting properties of structured value
        QCOMPARE(Padding::log[1].type, Padding::DefaultCtor);
        QCOMPARE(Padding::log[1].left, 0);
        QCOMPARE(Padding::log[1].right, 0);

        // In MyItem::setPadding()
        QCOMPARE(Padding::log[2].type, Padding::CopyAssign);
        QCOMPARE(Padding::log[2].left, 10);
        QCOMPARE(Padding::log[2].right, 20);
    }
}

QTEST_MAIN(tst_qqmlvaluetypeproviders)

#include "tst_qqmlvaluetypeproviders.moc"
