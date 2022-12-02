// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlpropertymap.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QSignalSpy>
#include <QDebug>

using namespace Qt::StringLiterals;

class tst_QQmlPropertyMap : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQmlPropertyMap() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;

    void insert();
    void insertMany();
    void insertDuplicate();
    void operatorInsert();
    void operatorValue();
    void clear();
    void changed();
    void count();
    void controlledWrite();

    void crashBug();
    void QTBUG_17868();
    void metaObjectAccessibility();
    void QTBUG_31226();
    void QTBUG_29836();
    void QTBUG_35233();
    void disallowExtending();
    void QTBUG_35906();
    void QTBUG_48136();
    void lookupsInSubTypes();
    void freeze();
    void cachedSignals();
    void signalIndices();
};

class LazyPropertyMap : public QQmlPropertyMap, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int someFixedProperty READ someFixedProperty WRITE setSomeFixedProperty NOTIFY someFixedPropertyChanged)
public:
    LazyPropertyMap()
        : QQmlPropertyMap(this, /*parent*/nullptr)
    {}

    void classBegin() override {}
    void componentComplete() override {
        insert(QStringLiteral("lateProperty"), QStringLiteral("lateValue"));
    }

    int someFixedProperty() const { return value; }
    void setSomeFixedProperty(int v) { value = v; emit someFixedPropertyChanged(); }

signals:
    void someFixedPropertyChanged();

private:
    int value = 0;
};

class SimplePropertyMap: public QQmlPropertyMap
{
    Q_OBJECT
public:
    SimplePropertyMap() : QQmlPropertyMap(this, nullptr) {}
};

void tst_QQmlPropertyMap::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<LazyPropertyMap>("QTBUG_35233", 1, 0, "LazyPropertyMap");
    qmlRegisterType<SimplePropertyMap>("Test", 1, 0, "SimplePropertyMap");
}

void tst_QQmlPropertyMap::insert()
{
    QQmlPropertyMap map;
    map.insert(QLatin1String("key1"),100);
    map.insert(QLatin1String("key2"),200);
    QCOMPARE(map.keys().size(), 2);
    QVERIFY(map.contains(QLatin1String("key1")));

    QCOMPARE(map.value(QLatin1String("key1")), QVariant(100));
    QCOMPARE(map.value(QLatin1String("key2")), QVariant(200));

    map.insert(QLatin1String("key1"),"Hello World");
    QCOMPARE(map.value(QLatin1String("key1")), QVariant("Hello World"));

    //inserting property names same with existing method(signal, slot, method) names is not allowed
    //QQmlPropertyMap has an invokable keys() method
    QTest::ignoreMessage(QtWarningMsg, "Creating property with name \"keys\" is not permitted, conflicts with internal symbols.");
    map.insert(QLatin1String("keys"), 1);
    QCOMPARE(map.keys().size(), 2);
    QVERIFY(!map.contains(QLatin1String("keys")));
    QVERIFY(map.value(QLatin1String("keys")).isNull());

    //QQmlPropertyMap has a deleteLater() slot
    QTest::ignoreMessage(QtWarningMsg, "Creating property with name \"deleteLater\" is not permitted, conflicts with internal symbols.");
    map.insert(QLatin1String("deleteLater"), 1);
    QCOMPARE(map.keys().size(), 2);
    QVERIFY(!map.contains(QLatin1String("deleteLater")));
    QVERIFY(map.value(QLatin1String("deleteLater")).isNull());

    //QQmlPropertyMap has an valueChanged() signal
    QTest::ignoreMessage(QtWarningMsg, "Creating property with name \"valueChanged\" is not permitted, conflicts with internal symbols.");
    map.insert(QLatin1String("valueChanged"), 1);
    QCOMPARE(map.keys().size(), 2);
    QVERIFY(!map.contains(QLatin1String("valueChanged")));
    QVERIFY(map.value(QLatin1String("valueChanged")).isNull());

    //but 'valueChange' should be ok
    map.insert(QLatin1String("valueChange"), 1);
    QCOMPARE(map.keys().size(), 3);
    QVERIFY(map.contains(QLatin1String("valueChange")));
    QCOMPARE(map.value(QLatin1String("valueChange")), QVariant(1));

    //'valueCHANGED' should be ok, too
    map.insert(QLatin1String("valueCHANGED"), 1);
    QCOMPARE(map.keys().size(), 4);
    QVERIFY(map.contains(QLatin1String("valueCHANGED")));
    QCOMPARE(map.value(QLatin1String("valueCHANGED")), QVariant(1));
}

void tst_QQmlPropertyMap::insertMany()
{
    QHash<QString, QVariant> values;
    values.insert(QLatin1String("key2"), 200);
    values.insert(QLatin1String("key1"), "Hello World");
    values.insert(QLatin1String("valueChange"), 1);
    values.insert(QLatin1String("valueCHANGED"), 1);

    QQmlPropertyMap map;
    map.insert(values);
    QCOMPARE(map.keys().size(), 4);
    QVERIFY(map.contains(QLatin1String("key1")));
    QCOMPARE(map.value(QLatin1String("key2")), QVariant(200));
    QCOMPARE(map.value(QLatin1String("key1")), QVariant("Hello World"));
    //but 'valueChange' should be ok
    QVERIFY(map.contains(QLatin1String("valueChange")));
    QCOMPARE(map.value(QLatin1String("valueChange")), QVariant(1));
    //'valueCHANGED' should be ok, too
    QVERIFY(map.contains(QLatin1String("valueCHANGED")));
    QCOMPARE(map.value(QLatin1String("valueCHANGED")), QVariant(1));

    values.insert(QLatin1String("keys"), 1);
    values.insert(QStringLiteral("foobar"), 12);
    values[QStringLiteral("key1")] = 100;
    //inserting property names same with existing method(signal, slot, method) names is not allowed
    //QQmlPropertyMap has an invokable keys() method
    QTest::ignoreMessage(QtWarningMsg, "Creating property with name \"keys\" is not permitted, conflicts with internal symbols.");
    map.insert(values);
    QCOMPARE(map.keys().size(), 4);
    QVERIFY(!map.contains(QLatin1String("keys")));
    QVERIFY(map.value(QLatin1String("keys")).isNull());

    values.remove(QStringLiteral("keys"));
    values.insert(QLatin1String("deleteLater"), 1);
    //QQmlPropertyMap has a deleteLater() slot
    QTest::ignoreMessage(QtWarningMsg, "Creating property with name \"deleteLater\" is not permitted, conflicts with internal symbols.");
    map.insert(values);
    QCOMPARE(map.keys().size(), 4);
    QVERIFY(!map.contains(QLatin1String("deleteLater")));
    QVERIFY(map.value(QLatin1String("deleteLater")).isNull());

    values.remove(QStringLiteral("deleteLater"));
    values.insert(QLatin1String("valueChanged"), 1);
    //QQmlPropertyMap has an valueChanged() signal
    QTest::ignoreMessage(QtWarningMsg, "Creating property with name \"valueChanged\" is not permitted, conflicts with internal symbols.");
    map.insert(values);
    QCOMPARE(map.keys().size(), 4);
    QVERIFY(!map.contains(QLatin1String("valueChanged")));
    QVERIFY(map.value(QLatin1String("valueChanged")).isNull());

    values.remove(QStringLiteral("valueChanged"));
    map.insert(values); // Adds "foobar" and changes "key1"
    QCOMPARE(map.keys().size(), 5);
    QCOMPARE(map.value(QStringLiteral("foobar")).toInt(), 12);
    QCOMPARE(map.value(QStringLiteral("key1")).toInt(), 100);
}

void tst_QQmlPropertyMap::insertDuplicate()
{
    QHash<QString, QVariant> values;
    values.insert(QLatin1String("key2"), 200);
    values.insert(QLatin1String("key1"), "Hello World");

    auto expectedCount = values.count();

    QQmlPropertyMap map;
    map.insert(values);
    QCOMPARE(map.keys().size(), expectedCount);

    map.insert(QStringLiteral("key2"), 24);
    QCOMPARE(map.keys().size(), expectedCount);
    QCOMPARE(map.value(QStringLiteral("key2")).toInt(), 24);

    map.insert(QString(), QVariant("Empty1"));
    QCOMPARE(map.keys().size(), ++expectedCount);
    map.insert(QString(), QVariant("Empty2"));
    QCOMPARE(map.keys().size(), expectedCount);

    QHash<QString, QVariant> emptyKeyMap;
    emptyKeyMap.insert(QString(), 200);
    emptyKeyMap.insert(QString(), 400);
    map.insert(emptyKeyMap);
    QCOMPARE(map.keys().size(), expectedCount);
}

void tst_QQmlPropertyMap::operatorInsert()
{
    QQmlPropertyMap map;
    map[QLatin1String("key1")] = 100;
    map[QLatin1String("key2")] = 200;
    QCOMPARE(map.keys().size(), 2);

    QCOMPARE(map.value(QLatin1String("key1")), QVariant(100));
    QCOMPARE(map.value(QLatin1String("key2")), QVariant(200));

    map[QLatin1String("key1")] = "Hello World";
    QCOMPARE(map.value(QLatin1String("key1")), QVariant("Hello World"));
}

void tst_QQmlPropertyMap::operatorValue()
{
    QQmlPropertyMap map;
    map.insert(QLatin1String("key1"),100);
    map.insert(QLatin1String("key2"),200);
    QCOMPARE(map.count(), 2);
    QVERIFY(map.contains(QLatin1String("key1")));

    const QQmlPropertyMap &constMap = map;

    QCOMPARE(constMap.value(QLatin1String("key1")), QVariant(100));
    QCOMPARE(constMap.value(QLatin1String("key2")), QVariant(200));
    QCOMPARE(constMap[QLatin1String("key1")], constMap.value(QLatin1String("key1")));
    QCOMPARE(constMap[QLatin1String("key2")], constMap.value(QLatin1String("key2")));
}

void tst_QQmlPropertyMap::clear()
{
    QQmlPropertyMap map;
    map.insert(QLatin1String("key1"),100);
    QCOMPARE(map.keys().size(), 1);

    QCOMPARE(map.value(QLatin1String("key1")), QVariant(100));

    map.clear(QLatin1String("key1"));
    QCOMPARE(map.keys().size(), 1);
    QVERIFY(map.contains(QLatin1String("key1")));
    QCOMPARE(map.value(QLatin1String("key1")), QVariant());
}

void tst_QQmlPropertyMap::changed()
{
    QQmlPropertyMap map;
    QSignalSpy spy(&map, SIGNAL(valueChanged(QString,QVariant)));
    map.insert(QLatin1String("key1"),100);
    map.insert(QLatin1String("key2"),200);
    QCOMPARE(spy.size(), 0);

    map.clear(QLatin1String("key1"));
    QCOMPARE(spy.size(), 0);

    //make changes in QML
    QQmlEngine engine;
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty(QLatin1String("testdata"), &map);
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nText { text: { testdata.key1 = 'Hello World'; 'X' } }",
            QUrl::fromLocalFile(""));
    QVERIFY(component.isReady());
    QScopedPointer<QQuickText> txt(qobject_cast<QQuickText*>(component.create()));
    QVERIFY(txt);
    QCOMPARE(txt->text(), QString('X'));
    QCOMPARE(spy.size(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 2);
    QCOMPARE(arguments.at(0).toString(),QLatin1String("key1"));
    QCOMPARE(arguments.at(1).value<QVariant>(),QVariant("Hello World"));
    QCOMPARE(map.value(QLatin1String("key1")), QVariant("Hello World"));
}

void tst_QQmlPropertyMap::count()
{
    QQmlPropertyMap map;
    QCOMPARE(map.isEmpty(), true);
    map.insert(QLatin1String("key1"),100);
    map.insert(QLatin1String("key2"),200);
    QCOMPARE(map.count(), 2);
    QCOMPARE(map.isEmpty(), false);

    map.insert(QLatin1String("key3"),"Hello World");
    QCOMPARE(map.count(), 3);

    //clearing doesn't remove the key
    map.clear(QLatin1String("key3"));
    QCOMPARE(map.count(), 3);
    QCOMPARE(map.size(), map.count());
}

class MyPropertyMap : public QQmlPropertyMap
{
    Q_OBJECT
protected:
    QVariant updateValue(const QString &key, const QVariant &src) override
    {
        if (key == QLatin1String("key1")) {
            // 'key1' must be all uppercase
            const QString original(src.toString());
            return QVariant(original.toUpper());
        }

        return src;
    }
};

void tst_QQmlPropertyMap::controlledWrite()
{
    MyPropertyMap map;
    QCOMPARE(map.isEmpty(), true);

    //make changes in QML
    QQmlEngine engine;
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty(QLatin1String("testdata"), &map);

    const char *qmlSource =
        "import QtQuick 2.0\n"
        "Item { Component.onCompleted: { testdata.key1 = 'Hello World'; testdata.key2 = 'Goodbye' } }";

    QQmlComponent component(&engine);
    component.setData(qmlSource, QUrl::fromLocalFile(""));
    QVERIFY(component.isReady());

    QObject *obj = component.create();
    QVERIFY(obj);
    delete obj;

    QCOMPARE(map.value(QLatin1String("key1")), QVariant("HELLO WORLD"));
    QCOMPARE(map.value(QLatin1String("key2")), QVariant("Goodbye"));
}

void tst_QQmlPropertyMap::crashBug()
{
    QQmlPropertyMap map;

    QQmlEngine engine;
    QQmlContext context(&engine);
    context.setContextProperty("map", &map);

    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0\nBinding { target: map; property: \"myProp\"; value: 10 + 23 }",QUrl());
    QObject *obj = c.create(&context);
    delete obj;
}

void tst_QQmlPropertyMap::QTBUG_17868()
{
    QQmlPropertyMap map;

    QQmlEngine engine;
    QQmlContext context(&engine);
    context.setContextProperty("map", &map);
    map.insert("key", 1);
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0\nItem {property bool error:false; Component.onCompleted: {try{ map.keys(); }catch(e) {error=true;}}}",QUrl());
    QObject *obj = c.create(&context);
    QVERIFY(obj);
    QVERIFY(!obj->property("error").toBool());
    delete obj;

}

class MyEnhancedPropertyMap : public QQmlPropertyMap
{
    Q_OBJECT
public:
    MyEnhancedPropertyMap() : QQmlPropertyMap(this, nullptr) {}
    bool testSlotCalled() const { return m_testSlotCalled; }

signals:
    void testSignal();

public slots:
    void testSlot() { m_testSlotCalled = true; }

private:
    bool m_testSlotCalled = false;
};

void tst_QQmlPropertyMap::metaObjectAccessibility()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;

    MyEnhancedPropertyMap map;

    // Verify that signals and slots of QQmlPropertyMap-derived classes are accessible
    QObject::connect(&map, SIGNAL(testSignal()), &engine, SIGNAL(quit()));
    QObject::connect(&engine, SIGNAL(quit()), &map, SLOT(testSlot()));

    QCOMPARE(map.metaObject()->className(), "MyEnhancedPropertyMap");

    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));
}

void tst_QQmlPropertyMap::QTBUG_31226()
{
    /* Instantiate a property map from QML, and verify that property changes
     * made from C++ are visible from QML */
    QQmlEngine engine;
    QQmlContext context(&engine);
    qmlRegisterType<QQmlPropertyMap>("QTBUG_31226", 1, 0, "PropertyMap");
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0\nimport QTBUG_31226 1.0\n"
              "Item {\n"
              "  property string myProp\n"
              "  PropertyMap { id: qmlPropertyMap; objectName: \"qmlPropertyMap\" }\n"
              "  Timer { interval: 5; running: true; onTriggered: { myProp = qmlPropertyMap.greeting; } }\n"
              "}",
              QUrl());
    QObject *obj = c.create(&context);
    QVERIFY(obj);

    QQmlPropertyMap *qmlPropertyMap = obj->findChild<QQmlPropertyMap*>(QString("qmlPropertyMap"));
    QVERIFY(qmlPropertyMap);

    qmlPropertyMap->insert("greeting", QString("Hello world!"));
    QTRY_COMPARE(obj->property("myProp").toString(), QString("Hello world!"));

    delete obj;

}

void tst_QQmlPropertyMap::QTBUG_29836()
{
    MyEnhancedPropertyMap map;
    QCOMPARE(map.testSlotCalled(), false);

    QQmlEngine engine;
    QQmlContext context(&engine);
    context.setContextProperty("enhancedMap", &map);
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0\n"
              "Item {\n"
              "  Timer { interval: 5; running: true; onTriggered: enhancedMap.testSlot() }\n"
              "}",
              QUrl());
    QObject *obj = c.create(&context);
    QVERIFY(obj);

    QTRY_COMPARE(map.testSlotCalled(), true);

    delete obj;

}

void tst_QQmlPropertyMap::QTBUG_35233()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml 2.0\n"
                      "import QTBUG_35233 1.0\n"
                      "QtObject {\n"
                      "    property QtObject testMap: LazyPropertyMap {\n"
                      "        id: map\n"
                      "    }\n"
                      "    property QtObject sibling: QtObject {\n"
                      "        objectName: \"sibling\"\n"
                      "        property string testValue: map.lateProperty\n"
                      "    }\n"
                      "}", QUrl());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());

    QObject *sibling = obj->findChild<QObject*>("sibling");
    QVERIFY(sibling);
    QCOMPARE(sibling->property("testValue").toString(), QString("lateValue"));
}

void tst_QQmlPropertyMap::disallowExtending()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml 2.0\n"
                      "import QTBUG_35233 1.0\n"
                      "LazyPropertyMap {\n"
                      "    id: blah\n"
                      "    someFixedProperty: 42\n"
                      "}\n", QUrl());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());

    component.setData("import QtQml 2.0\n"
                      "import QTBUG_35233 1.0\n"
                      "LazyPropertyMap {\n"
                      "    id: blah\n"
                      "    property int someNewProperty;\n"
                      "}\n", QUrl());
    obj.reset(component.create());
    QVERIFY(obj.isNull());
    QCOMPARE(component.errors().size(), 1);
    QCOMPARE(component.errors().at(0).toString(), QStringLiteral("<Unknown File>:3:1: Fully dynamic types cannot declare new properties."));
}

void tst_QQmlPropertyMap::QTBUG_35906()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml 2.0\n"
                      "import QTBUG_35233 1.0\n"
                      "QtObject {\n"
                      "    property int testValue: mapById.someFixedProperty\n"
                      "\n"
                      "    property QtObject maProperty: LazyPropertyMap {\n"
                      "        id: mapById\n"
                      "        someFixedProperty: 42\n"
                      "    }\n"
                      "}\n", QUrl());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QVariant value = obj->property("testValue");
    QCOMPARE(value.typeId(), QMetaType::Int);
    QCOMPARE(value.toInt(), 42);
}

void tst_QQmlPropertyMap::QTBUG_48136()
{
    static const char key[] = "mykey";
    QQmlPropertyMap map;

    //
    // Test that the notify signal is emitted correctly
    //

    const int propIndex = map.metaObject()->indexOfProperty(key);
    const QMetaProperty prop = map.metaObject()->property(propIndex);
    QSignalSpy notifySpy(&map, QByteArray::number(QSIGNAL_CODE) + prop.notifySignal().methodSignature());

    map.insert(key, 42);
    QCOMPARE(notifySpy.size(), 1);
    map.insert(key, 43);
    QCOMPARE(notifySpy.size(), 2);
    map.insert(key, 43);
    QCOMPARE(notifySpy.size(), 2);
    map.insert(key, 44);
    QCOMPARE(notifySpy.size(), 3);

    //
    // Test that the valueChanged signal is emitted correctly
    //
    QSignalSpy valueChangedSpy(&map, &QQmlPropertyMap::valueChanged);
    map.setProperty(key, 44);
    QCOMPARE(valueChangedSpy.size(), 0);
    map.setProperty(key, 45);
    QCOMPARE(valueChangedSpy.size(), 1);
    map.setProperty(key, 45);
    QCOMPARE(valueChangedSpy.size(), 1);
}

void tst_QQmlPropertyMap::lookupsInSubTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("PropertyMapSubType.qml"));
    QTest::ignoreMessage(QtDebugMsg, "expected output");
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("newProperty").toInt(), 42);
}

void tst_QQmlPropertyMap::freeze()
{
    QQmlPropertyMap map;

    map.insert(QLatin1String("key1"),100);
    map.insert(QLatin1String("key2"),200);
    QCOMPARE(map.keys().size(), 2);
    QVERIFY(map.contains(QLatin1String("key1")));
    QCOMPARE(map.value(QLatin1String("key1")), QVariant(100));
    QCOMPARE(map.value(QLatin1String("key2")), QVariant(200));

    map.freeze();
    map.insert(QLatin1String("key3"), 32);
    QCOMPARE(map.keys().size(), 2);
    QVERIFY(!map.contains("key3"));

    map.insert(QLatin1String("key1"), QStringLiteral("Hello World"));
    QCOMPARE(map.value("key1").toString(), QStringLiteral("Hello World"));
}

class Map: public QQmlPropertyMap
{
    Q_OBJECT
public:
    Map(QObject *parent = nullptr)
        : QQmlPropertyMap(this, parent)
    {
        insert( "a", u"yayayaya"_s );
        insert( "b", u"yayayayb"_s);
        insert( "c", u"yayayayc"_s);
        insert( "d", u"yayayayd"_s);

        freeze();
    }
};

void tst_QQmlPropertyMap::cachedSignals()
{
    Map foo;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("map", &foo);
    const QUrl url = testFileUrl("cached.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("text").toString(), u"yayayayc"_s);
    foo.setProperty("c", u"something"_s);
    QCOMPARE(o->property("text").toString(), u"something"_s);
    foo.insert("c", u"other"_s);
    QCOMPARE(o->property("text").toString(), u"other"_s);
    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + u":4:5: Unable to assign [undefined] to QString"_s));
    foo.clear("c");
    QCOMPARE(o->property("text").toString(), u"other"_s);
    foo.insert("c", u"final"_s);
    QCOMPARE(o->property("text").toString(), u"final"_s);
}

class NastyMap: public QQmlPropertyMap
{
    Q_OBJECT
    Q_PROPERTY(int a READ a WRITE setA NOTIFY aChanged)
    Q_PROPERTY(int b MEMBER m_b CONSTANT)

public:

    int a() const { return m_a; }
    void setA(int a)
    {
        if (a != m_a) {
            m_a = a;
            emit aChanged();
        }
    }

signals:
    void aChanged();
    void extraSignal();

private:
    int m_a = 0;
    int m_b = 7;
};

void tst_QQmlPropertyMap::signalIndices()
{
    NastyMap map;
    map.insert(QLatin1String("key1"), 100);
    const QMetaObject *mo = map.metaObject();
    const int propertyIndex = mo->indexOfProperty("key1");
    const QMetaProperty property = mo->property(propertyIndex);
    const int signalIndex = property.notifySignalIndex();
    const QMetaMethod method = mo->method(signalIndex);

    QSignalSpy spy(&map, method);
    map.insert(QLatin1String("key1"), 200);
    QCOMPARE(spy.size(), 1);
}

QTEST_MAIN(tst_QQmlPropertyMap)

#include "tst_qqmlpropertymap.moc"
