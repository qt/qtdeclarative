// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <private/qquickrepeater_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtGui/qstandarditemmodel.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests.repeater")

class tst_QQuickRepeater : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickRepeater();

private slots:
    void numberModel();
    void objectList_data();
    void objectList();
    void stringList();
    void dataModel_adding();
    void dataModel_removing();
    void dataModel_changes();
    void itemModel();
    void resetModel();
    void modelChanged();
    void modelReset();
    void modelCleared();
    void properties();
    void asynchronous();
    void initParent();
    void dynamicModelCrash();
    void visualItemModelCrash();
    void invalidContextCrash();
    void jsArrayChange();
    void clearRemovalOrder();
    void destroyCount();
    void stackingOrder();
    void objectModel();
    void QTBUG54859_asynchronousMove();
    void package();
    void ownership();
    void requiredProperties();
    void contextProperties();
    void innerRequired();
    void boundDelegateComponent();
};

class TestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool error READ error WRITE setError)
    Q_PROPERTY(bool useModel READ useModel NOTIFY useModelChanged)

public:
    TestObject() : QObject(), mError(true), mUseModel(false) {}

    bool error() const { return mError; }
    void setError(bool err) { mError = err; }

    bool useModel() const { return mUseModel; }
    void setUseModel(bool use) { mUseModel = use; emit useModelChanged(); }

signals:
    void useModelChanged();

private:
    bool mError;
    bool mUseModel;
};

tst_QQuickRepeater::tst_QQuickRepeater()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickRepeater::numberModel()
{
    std::unique_ptr<QQuickView> window { createView() };

    QQmlContext *ctxt = window->rootContext();
    ctxt->setContextProperty("testData", 5);
    std::unique_ptr<TestObject> testObject = std::make_unique<TestObject>();
    ctxt->setContextProperty("testObject", testObject.get());

    window->setSource(testFileUrl("intmodel.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);
    QCOMPARE(repeater->parentItem()->childItems().size(), 5+1);

    QVERIFY(!repeater->itemAt(-1));
    for (int i=0; i<repeater->count(); i++)
        QCOMPARE(repeater->itemAt(i), repeater->parentItem()->childItems().at(i));
    QVERIFY(!repeater->itemAt(repeater->count()));

    QMetaObject::invokeMethod(window->rootObject(), "checkProperties");
    QVERIFY(!testObject->error());

    ctxt->setContextProperty("testData", std::numeric_limits<int>::max());
    QCOMPARE(repeater->parentItem()->childItems().size(), 1);

    ctxt->setContextProperty("testData", -1234);
    QCOMPARE(repeater->parentItem()->childItems().size(), 1);
}

void tst_QQuickRepeater::objectList_data()
{
    QTest::addColumn<QUrl>("filename");

    QTest::newRow("normal") << testFileUrl("objlist.qml");
    QTest::newRow("required") << testFileUrl("objlist_required.qml");
}

class MyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int idx READ idx CONSTANT)
public:
    MyObject(int i) : QObject(), m_idx(i) {}

    int idx() const { return m_idx; }

    int m_idx;
};

void tst_QQuickRepeater::objectList()
{
    QFETCH(QUrl, filename);
    std::unique_ptr<QQuickView> window { createView() };
    QObjectList data;
    auto cleanup = qScopeGuard([&]() {
        qDeleteAll(data);
    });
    for (int i=0; i<100; i++)
        data << new MyObject(i);

    QQmlContext *ctxt = window->rootContext();
    ctxt->setContextProperty("testData", QVariant::fromValue(data));

    window->setSource(filename);
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);
    QCOMPARE(repeater->property("errors").toInt(), 0);//If this fails either they are out of order or can't find the object's data
    QCOMPARE(repeater->property("instantiated").toInt(), 100);

    QVERIFY(!repeater->itemAt(-1));
    for (int i=0; i<data.size(); i++)
        QCOMPARE(repeater->itemAt(i), repeater->parentItem()->childItems().at(i));
    QVERIFY(!repeater->itemAt(data.size()));

    QSignalSpy addedSpy(repeater, SIGNAL(itemAdded(int,QQuickItem*)));
    QSignalSpy removedSpy(repeater, SIGNAL(itemRemoved(int,QQuickItem*)));
    ctxt->setContextProperty("testData", QVariant::fromValue(data));
    QCOMPARE(addedSpy.size(), data.size());
    QCOMPARE(removedSpy.size(), data.size());
}

/*
The Repeater element creates children at its own position in its parent's
stacking order.  In this test we insert a repeater between two other Text
elements to test this.
*/
void tst_QQuickRepeater::stringList()
{
    std::unique_ptr<QQuickView> window { createView() };

    QStringList data;
    data << "One";
    data << "Two";
    data << "Three";
    data << "Four";

    QQmlContext *ctxt = window->rootContext();
    ctxt->setContextProperty("testData", data);

    window->setSource(testFileUrl("repeater1.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);

    QQuickItem *container = findItem<QQuickItem>(window->rootObject(), "container");
    QVERIFY(container != nullptr);

    QCOMPARE(container->childItems().size(), data.size() + 3);

    bool saw_repeater = false;
    for (int i = 0; i < container->childItems().size(); ++i) {

        if (i == 0) {
            QQuickText *name = qobject_cast<QQuickText*>(container->childItems().at(i));
            QVERIFY(name != nullptr);
            QCOMPARE(name->text(), QLatin1String("Zero"));
        } else if (i == container->childItems().size() - 2) {
            // The repeater itself
            QQuickRepeater *rep = qobject_cast<QQuickRepeater*>(container->childItems().at(i));
            QCOMPARE(rep, repeater);
            saw_repeater = true;
            continue;
        } else if (i == container->childItems().size() - 1) {
            QQuickText *name = qobject_cast<QQuickText*>(container->childItems().at(i));
            QVERIFY(name != nullptr);
            QCOMPARE(name->text(), QLatin1String("Last"));
        } else {
            QQuickText *name = qobject_cast<QQuickText*>(container->childItems().at(i));
            QVERIFY(name != nullptr);
            QCOMPARE(name->text(), data.at(i-1));
        }
    }
    QVERIFY(saw_repeater);
}

void tst_QQuickRepeater::dataModel_adding()
{
    std::unique_ptr<QQuickView> window { createView() };
    QQmlContext *ctxt = window->rootContext();
    std::unique_ptr<TestObject> testObject = std::make_unique<TestObject>();
    ctxt->setContextProperty("testObject", testObject.get());

    QaimModel testModel;
    ctxt->setContextProperty("testData", &testModel);
    window->setSource(testFileUrl("repeater2.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);
    QQuickItem *container = findItem<QQuickItem>(window->rootObject(), "container");
    QVERIFY(container != nullptr);

    QVERIFY(!repeater->itemAt(0));

    QSignalSpy countSpy(repeater, SIGNAL(countChanged()));
    QSignalSpy addedSpy(repeater, SIGNAL(itemAdded(int,QQuickItem*)));

    // add to empty model
    testModel.addItem("two", "2");
    QCOMPARE(repeater->itemAt(0), container->childItems().at(0));
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(addedSpy.size(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(0));
    addedSpy.clear();

    // insert at start
    testModel.insertItem(0, "one", "1");
    QCOMPARE(repeater->itemAt(0), container->childItems().at(0));
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(addedSpy.size(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(0));
    addedSpy.clear();

    // insert at end
    testModel.insertItem(2, "four", "4");
    QCOMPARE(repeater->itemAt(2), container->childItems().at(2));
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(addedSpy.size(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 2);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(2));
    addedSpy.clear();

    // insert in middle
    testModel.insertItem(2, "three", "3");
    QCOMPARE(repeater->itemAt(2), container->childItems().at(2));
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(addedSpy.size(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 2);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(2));
    addedSpy.clear();

    //insert in middle multiple
    int childItemsSize = container->childItems().size();
    QList<QPair<QString, QString> > multiData;
    multiData << qMakePair(QStringLiteral("five"), QStringLiteral("5")) << qMakePair(QStringLiteral("six"), QStringLiteral("6")) << qMakePair(QStringLiteral("seven"), QStringLiteral("7"));
    testModel.insertItems(1, multiData);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(addedSpy.size(), 3);
    QCOMPARE(container->childItems().size(), childItemsSize + 3);
    QCOMPARE(repeater->itemAt(2), container->childItems().at(2));
    addedSpy.clear();
    countSpy.clear();

    testObject.reset();
    addedSpy.clear();
    countSpy.clear();
}

void tst_QQuickRepeater::dataModel_removing()
{
    std::unique_ptr<QQuickView> window { createView() };
    QQmlContext *ctxt = window->rootContext();
    auto testObject = std::make_unique<TestObject>();
    ctxt->setContextProperty("testObject", testObject.get());

    QaimModel testModel;
    testModel.addItem("one", "1");
    testModel.addItem("two", "2");
    testModel.addItem("three", "3");
    testModel.addItem("four", "4");
    testModel.addItem("five", "5");

    ctxt->setContextProperty("testData", &testModel);
    window->setSource(testFileUrl("repeater2.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);
    QQuickItem *container = findItem<QQuickItem>(window->rootObject(), "container");
    QVERIFY(container != nullptr);
    QCOMPARE(container->childItems().size(), repeater->count()+1);

    QSignalSpy countSpy(repeater, SIGNAL(countChanged()));
    QSignalSpy removedSpy(repeater, SIGNAL(itemRemoved(int,QQuickItem*)));

    // remove at start
    QQuickItem *item = repeater->itemAt(0);
    QCOMPARE(item, container->childItems().at(0));

    testModel.removeItem(0);
    QVERIFY(repeater->itemAt(0) != item);
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(removedSpy.at(0).at(1).value<QQuickItem*>(), item);
    removedSpy.clear();

    // remove at end
    int lastIndex = testModel.count()-1;
    item = repeater->itemAt(lastIndex);
    QCOMPARE(item, container->childItems().at(lastIndex));

    testModel.removeItem(lastIndex);
    QVERIFY(repeater->itemAt(lastIndex) != item);
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toInt(), lastIndex);
    QCOMPARE(removedSpy.at(0).at(1).value<QQuickItem*>(), item);
    removedSpy.clear();

    // remove from middle
    item = repeater->itemAt(1);
    QCOMPARE(item, container->childItems().at(1));

    testModel.removeItem(1);
    QVERIFY(repeater->itemAt(lastIndex) != item);
    QCOMPARE(countSpy.size(), 1); countSpy.clear();
    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(removedSpy.at(0).at(1).value<QQuickItem*>(), item);
    removedSpy.clear();
}

void tst_QQuickRepeater::dataModel_changes()
{
    std::unique_ptr<QQuickView> window { createView() };
    QQmlContext *ctxt = window->rootContext();
    auto testObject = std::make_unique<TestObject>();
    ctxt->setContextProperty("testObject", testObject.get());

    QaimModel testModel;
    testModel.addItem("one", "1");
    testModel.addItem("two", "2");
    testModel.addItem("three", "3");

    ctxt->setContextProperty("testData", &testModel);
    window->setSource(testFileUrl("repeater2.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);
    QQuickItem *container = findItem<QQuickItem>(window->rootObject(), "container");
    QVERIFY(container != nullptr);
    QCOMPARE(container->childItems().size(), repeater->count()+1);

    // Check that model changes are propagated
    QQuickText *text = findItem<QQuickText>(window->rootObject(), "myName", 1);
    QVERIFY(text);
    QCOMPARE(text->text(), QString("two"));

    testModel.modifyItem(1, "Item two", "_2");
    text = findItem<QQuickText>(window->rootObject(), "myName", 1);
    QVERIFY(text);
    QCOMPARE(text->text(), QString("Item two"));

    text = findItem<QQuickText>(window->rootObject(), "myNumber", 1);
    QVERIFY(text);
    QCOMPARE(text->text(), QString("_2"));
}

void tst_QQuickRepeater::itemModel()
{
    std::unique_ptr<QQuickView> window { createView() };
    QQmlContext *ctxt = window->rootContext();
    auto testObject = std::make_unique<TestObject>();
    ctxt->setContextProperty("testObject", testObject.get());

    window->setSource(testFileUrl("itemlist.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);

    QQuickItem *container = findItem<QQuickItem>(window->rootObject(), "container");
    QVERIFY(container != nullptr);

    QCOMPARE(container->childItems().size(), 1);

    testObject->setUseModel(true);
    QMetaObject::invokeMethod(window->rootObject(), "checkProperties");
    QVERIFY(!testObject->error());

    if (lcTests().isDebugEnabled()) {
        qCDebug(lcTests) << "=== item tree:";
        window->contentItem()->dumpItemTree();
        qCDebug(lcTests) << "=== object tree:";
        window->dumpObjectTree();
    }

    QCOMPARE(container->childItems().size(), 4);
    QCOMPARE(qobject_cast<QObject*>(container->childItems().at(0))->objectName(), QLatin1String("item1"));
    QCOMPARE(qobject_cast<QObject*>(container->childItems().at(1))->objectName(), QLatin1String("item2"));
    QCOMPARE(qobject_cast<QObject*>(container->childItems().at(2))->objectName(), QLatin1String("item3"));
    QCOMPARE(container->childItems().at(3), repeater);

    QMetaObject::invokeMethod(window->rootObject(), "switchModel");
    QCOMPARE(container->childItems().size(), 3);
    QCOMPARE(qobject_cast<QObject*>(container->childItems().at(0))->objectName(), QLatin1String("item4"));
    QCOMPARE(qobject_cast<QObject*>(container->childItems().at(1))->objectName(), QLatin1String("item5"));
    QCOMPARE(container->childItems().at(2), repeater);

    testObject->setUseModel(false);
    QCOMPARE(container->childItems().size(), 1);
}

void tst_QQuickRepeater::resetModel()
{
    std::unique_ptr<QQuickView> window {  createView() };

    QStringList dataA;
    for (int i=0; i<10; i++)
        dataA << QString::number(i);

    QQmlContext *ctxt = window->rootContext();
    ctxt->setContextProperty("testData", dataA);
    window->setSource(testFileUrl("repeater1.qml"));
    qApp->processEvents();
    QQuickRepeater *repeater = findItem<QQuickRepeater>(window->rootObject(), "repeater");
    QVERIFY(repeater != nullptr);
    QQuickItem *container = findItem<QQuickItem>(window->rootObject(), "container");
    QVERIFY(container != nullptr);

    QCOMPARE(repeater->count(), dataA.size());
    for (int i=0; i<repeater->count(); i++)
        QCOMPARE(repeater->itemAt(i), container->childItems().at(i+1)); // +1 to skip first Text object

    QSignalSpy modelChangedSpy(repeater, SIGNAL(modelChanged()));
    QSignalSpy countSpy(repeater, SIGNAL(countChanged()));
    QSignalSpy addedSpy(repeater, SIGNAL(itemAdded(int,QQuickItem*)));
    QSignalSpy removedSpy(repeater, SIGNAL(itemRemoved(int,QQuickItem*)));

    QStringList dataB;
    for (int i=0; i<20; i++)
        dataB << QString::number(i);

    // reset context property
    ctxt->setContextProperty("testData", dataB);
    QCOMPARE(repeater->count(), dataB.size());

    QCOMPARE(modelChangedSpy.size(), 1);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(removedSpy.size(), dataA.size());
    QCOMPARE(addedSpy.size(), dataB.size());
    for (int i=0; i<dataB.size(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }
    modelChangedSpy.clear();
    countSpy.clear();
    removedSpy.clear();
    addedSpy.clear();

    // reset via setModel()
    repeater->setModel(dataA);
    QCOMPARE(repeater->count(), dataA.size());

    QCOMPARE(modelChangedSpy.size(), 1);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(removedSpy.size(), dataB.size());
    QCOMPARE(addedSpy.size(), dataA.size());
    for (int i=0; i<dataA.size(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }

    modelChangedSpy.clear();
    countSpy.clear();
    removedSpy.clear();
    addedSpy.clear();
}

// QTBUG-17156
void tst_QQuickRepeater::modelChanged()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("modelChanged.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);
    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);

    repeater->setModel(4);
    QCOMPARE(repeater->count(), 4);
    QCOMPARE(repeater->property("itemsCount").toInt(), 4);
    QCOMPARE(repeater->property("itemsFound").toList().size(), 4);

    repeater->setModel(10);
    QCOMPARE(repeater->count(), 10);
    QCOMPARE(repeater->property("itemsCount").toInt(), 10);
    QCOMPARE(repeater->property("itemsFound").toList().size(), 10);
}

void tst_QQuickRepeater::modelReset()
{
    QaimModel model;

    QQmlEngine engine;
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("testData", &model);

    QQmlComponent component(&engine, testFileUrl("repeater2.qml"));
    QScopedPointer<QObject> object(component.create());
    QQuickItem *rootItem = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(rootItem);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootItem, "repeater");
    QVERIFY(repeater != nullptr);
    QQuickItem *container = findItem<QQuickItem>(rootItem, "container");
    QVERIFY(container != nullptr);

    QCOMPARE(repeater->count(), 0);

    QSignalSpy countSpy(repeater, SIGNAL(countChanged()));
    QSignalSpy addedSpy(repeater, SIGNAL(itemAdded(int,QQuickItem*)));
    QSignalSpy removedSpy(repeater, SIGNAL(itemRemoved(int,QQuickItem*)));


    QList<QPair<QString, QString> > items = QList<QPair<QString, QString> >()
            << qMakePair(QString::fromLatin1("one"), QString::fromLatin1("1"))
            << qMakePair(QString::fromLatin1("two"), QString::fromLatin1("2"))
            << qMakePair(QString::fromLatin1("three"), QString::fromLatin1("3"));

    model.resetItems(items);

    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(removedSpy.size(), 0);
    QCOMPARE(addedSpy.size(), items.size());
    for (int i = 0; i< items.size(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }

    countSpy.clear();
    addedSpy.clear();

    model.reset();
    QCOMPARE(countSpy.size(), 0);
    QCOMPARE(removedSpy.size(), 3);
    QCOMPARE(addedSpy.size(), 3);
    for (int i = 0; i< items.size(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }

    addedSpy.clear();
    removedSpy.clear();

    items.append(qMakePair(QString::fromLatin1("four"), QString::fromLatin1("4")));
    items.append(qMakePair(QString::fromLatin1("five"), QString::fromLatin1("5")));

    model.resetItems(items);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(removedSpy.size(), 3);
    QCOMPARE(addedSpy.size(), 5);
    for (int i = 0; i< items.size(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }

    countSpy.clear();
    addedSpy.clear();
    removedSpy.clear();

    items.clear();
    model.resetItems(items);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(removedSpy.size(), 5);
    QCOMPARE(addedSpy.size(), 0);
}

// QTBUG-46828
void tst_QQuickRepeater::modelCleared()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("modelCleared.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);

    // verify no error messages when the model is cleared and the items are destroyed
    QQmlTestMessageHandler messageHandler;
    repeater->setModel(0);
    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));
}

void tst_QQuickRepeater::properties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("properties.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);

    QSignalSpy modelSpy(repeater, SIGNAL(modelChanged()));
    repeater->setModel(3);
    QCOMPARE(modelSpy.size(),1);
    repeater->setModel(3);
    QCOMPARE(modelSpy.size(),1);

    QSignalSpy delegateSpy(repeater, SIGNAL(delegateChanged()));

    QQmlComponent rectComponent(&engine);
    rectComponent.setData("import QtQuick 2.0; Rectangle {}", QUrl::fromLocalFile(""));

    repeater->setDelegate(&rectComponent);
    QCOMPARE(delegateSpy.size(),1);
    repeater->setDelegate(&rectComponent);
    QCOMPARE(delegateSpy.size(),1);
}

void tst_QQuickRepeater::asynchronous()
{
    std::unique_ptr<QQuickView> window { createView() };
    window->show();
    QQmlIncubationController controller;
    window->engine()->setIncubationController(&controller);

    window->setSource(testFileUrl("asyncloader.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootObject);

    QQuickItem *container = findItem<QQuickItem>(rootObject, "container");
    QVERIFY(!container);
    while (!container) {
        std::atomic<bool> b = false;
        controller.incubateWhile(&b);
        container = findItem<QQuickItem>(rootObject, "container");
    }

    QQuickRepeater *repeater = nullptr;
    while (!repeater) {
        std::atomic<bool> b = false;
        controller.incubateWhile(&b);
        repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    }

    // items will be created one at a time
    // the order is incubator/model specific
    for (int i = 9; i >= 0; --i) {
        QString name("delegate");
        name += QString::number(i);
        QVERIFY(findItem<QQuickItem>(container, name) == nullptr);
        QQuickItem *item = nullptr;
        while (!item) {
            std::atomic<bool> b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(container, name);
        }
    }

    {
        std::atomic<bool> b = true;
        controller.incubateWhile(&b);
    }

    // verify positioning
    for (int i = 0; i < 10; ++i) {
        QString name("delegate");
        name += QString::number(i);
        QQuickItem *item = findItem<QQuickItem>(container, name);
        QTRY_COMPARE(item->y(), i * 50.0);
    }
}

void tst_QQuickRepeater::initParent()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("initparent.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QCOMPARE(qvariant_cast<QQuickItem*>(rootObject->property("parentItem")), rootObject);
}

void tst_QQuickRepeater::dynamicModelCrash()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("dynamicmodelcrash.qml"));

    // Don't crash
    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "rep");
    QVERIFY(repeater);
    QVERIFY(qvariant_cast<QObject *>(repeater->model()) == 0);
}

void tst_QQuickRepeater::visualItemModelCrash()
{
    // This used to crash because the model would get
    // deleted before the repeater, leading to double-deletion
    // of the items.
    std::unique_ptr<QQuickView> window { createView() };
    window->setSource(testFileUrl("visualitemmodel.qml"));
    qApp->processEvents();
}

class BadModel : public QAbstractListModel
{
public:
    ~BadModel()
    {
        beginResetModel();
        endResetModel();
    }

    QVariant data(const QModelIndex &, int) const override { return QVariant(); }
    int rowCount(const QModelIndex &) const override { return 0; }
};


void tst_QQuickRepeater::invalidContextCrash()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("invalidContextCrash.qml"));

    BadModel* model = new BadModel;
    engine.rootContext()->setContextProperty("badModel", model);

    QScopedPointer<QObject> root(component.create());
    QCOMPARE(root->children().size(), 1);
    QObject *repeater = root->children().first();

    // Make sure the model comes first in the child list, so it will be
    // deleted first and then the repeater. During deletion the QML context
    // has been deleted already and is invalid.
    model->setParent(root.data());
    repeater->setParent(nullptr);
    repeater->setParent(root.data());

    QCOMPARE(root->children().size(), 2);
    QCOMPARE(root->children().at(0), model);
    QCOMPARE(root->children().at(1), repeater);

    // Delete the root object, which will invalidate/delete the QML context
    // and then delete the child QObjects, which may try to access the context.
    root.reset(nullptr);
}

void tst_QQuickRepeater::jsArrayChange()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.4; Repeater {}", QUrl());

    QScopedPointer<QQuickRepeater> repeater(qobject_cast<QQuickRepeater *>(component.create()));
    QVERIFY(!repeater.isNull());

    QSignalSpy spy(repeater.data(), SIGNAL(modelChanged()));
    QVERIFY(spy.isValid());

    QJSValue array1 = engine.newArray(3);
    QJSValue array2 = engine.newArray(3);
    for (int i = 0; i < 3; ++i) {
        array1.setProperty(i, i);
        array2.setProperty(i, i);
    }

    repeater->setModel(QVariant::fromValue(array1));
    QCOMPARE(spy.size(), 1);

    // no change
    repeater->setModel(QVariant::fromValue(array2));
    QCOMPARE(spy.size(), 1);
}

void tst_QQuickRepeater::clearRemovalOrder()
{
    // Here, we're going to test that when the model is cleared, item removal
    // signals are sent in a sensible order that gives us correct indices.
    // (QTBUG-42243)
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("clearremovalorder.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);
    QCOMPARE(repeater->count(), 3);

    QQmlListModel *model = rootObject->findChild<QQmlListModel*>("secondModel");
    QVERIFY(model);
    QCOMPARE(model->count(), 0);

    // Now change the model
    QSignalSpy removedSpy(repeater, &QQuickRepeater::itemRemoved);
    repeater->setModel(QVariant::fromValue(model));

    // we should have 0 items, and 3 removal signals.
    QCOMPARE(repeater->count(), 0);
    QCOMPARE(removedSpy.size(), 3);

    // column 1 is for the items, we won't bother verifying these. just look at
    // the indices and make sure they're sane.
    QCOMPARE(removedSpy.at(0).at(0).toInt(), 2);
    QCOMPARE(removedSpy.at(1).at(0).toInt(), 1);
    QCOMPARE(removedSpy.at(2).at(0).toInt(), 0);
}

void tst_QQuickRepeater::destroyCount()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("destroycount.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);

    repeater->setProperty("model", QVariant::fromValue<int>(3));
    QCOMPARE(repeater->property("componentCount").toInt(), 3);
    repeater->setProperty("model", QVariant::fromValue<int>(0));
    QCOMPARE(repeater->property("componentCount").toInt(), 0);
    repeater->setProperty("model", QVariant::fromValue<int>(4));
    QCOMPARE(repeater->property("componentCount").toInt(), 4);

    QStringListModel model;
    repeater->setProperty("model", QVariant::fromValue<QStringListModel *>(&model));
    QCOMPARE(repeater->property("componentCount").toInt(), 0);
    QStringList list;
    list << "1" << "2" << "3" << "4";
    model.setStringList(list);
    QCOMPARE(repeater->property("componentCount").toInt(), 4);
    model.insertRows(2,1);
    QModelIndex index = model.index(2);
    model.setData(index, QVariant::fromValue<QString>(QStringLiteral("foobar")));
    QCOMPARE(repeater->property("componentCount").toInt(), 5);

    model.removeRows(2,1);
    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(repeater->property("componentCount").toInt(), 4);
}

void tst_QQuickRepeater::stackingOrder()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("stackingorder.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *rootObject = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);
    int count = 1;
    do {
        bool stackingOrderOk = rootObject->property("stackingOrderOk").toBool();
        QVERIFY(stackingOrderOk);
        repeater->setModel(QVariant(++count));
    } while (count < 3);
}

static bool compareObjectModel(QQuickRepeater *repeater, QQmlObjectModel *model)
{
    if (repeater->count() != model->count())
        return false;
    for (int i = 0; i < repeater->count(); ++i) {
        if (repeater->itemAt(i) != model->get(i))
            return false;
    }
    return true;
}

void tst_QQuickRepeater::objectModel()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("objectmodel.qml"));

    std::unique_ptr<QObject> root { component.create() };
    QQuickItem *positioner = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(positioner);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(positioner, "repeater");
    QVERIFY(repeater);

    QQmlObjectModel *model = repeater->model().value<QQmlObjectModel *>();
    QVERIFY(model);

    QVERIFY(repeater->itemAt(0));
    QVERIFY(repeater->itemAt(1));
    QVERIFY(repeater->itemAt(2));
    QCOMPARE(repeater->itemAt(0)->property("color").toString(), QColor("red").name());
    QCOMPARE(repeater->itemAt(1)->property("color").toString(), QColor("green").name());
    QCOMPARE(repeater->itemAt(2)->property("color").toString(), QColor("blue").name());

    QQuickItem *item0 = new QQuickItem(positioner);
    item0->setSize(QSizeF(20, 20));
    model->append(item0);
    QCOMPARE(model->count(), 4);
    QVERIFY(compareObjectModel(repeater, model));

    QQuickItem *item1 = new QQuickItem(positioner);
    item1->setSize(QSizeF(20, 20));
    model->insert(0, item1);
    QCOMPARE(model->count(), 5);
    QVERIFY(compareObjectModel(repeater, model));

    model->move(1, 2, 3);
    QVERIFY(compareObjectModel(repeater, model));

    model->remove(2, 2);
    QCOMPARE(model->count(), 3);
    QVERIFY(compareObjectModel(repeater, model));

    model->clear();
    QCOMPARE(model->count(), 0);
    QCOMPARE(repeater->count(), 0);
}

class Ctrl : public QObject
{
    Q_OBJECT
public:

    Q_INVOKABLE void wait()
    {
        QTest::qWait(200);
    }
};

void tst_QQuickRepeater::QTBUG54859_asynchronousMove()
{
    Ctrl ctrl;
    std::unique_ptr<QQuickView> view { createView() };
    view->rootContext()->setContextProperty("ctrl", &ctrl);
    view->setSource(testFileUrl("asynchronousMove.qml"));
    view->show();
    QQuickItem* item = view->rootObject();


    QTRY_COMPARE(item->property("finished"), QVariant(true));
}

void tst_QQuickRepeater::package()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("package.qml"));

    QScopedPointer<QObject>o(component.create()); // don't crash!
    QVERIFY(o != nullptr);

    {
        QQuickRepeater *repeater1 = qobject_cast<QQuickRepeater*>(qmlContext(o.data())->contextProperty("repeater1").value<QObject*>());
        QVERIFY(repeater1);
        QCOMPARE(repeater1->count(), 1);
        QCOMPARE(repeater1->itemAt(0)->objectName(), "firstItem");
    }

    {
        QQuickRepeater *repeater2 = qobject_cast<QQuickRepeater*>(qmlContext(o.data())->contextProperty("repeater2").value<QObject*>());
        QVERIFY(repeater2);
        QCOMPARE(repeater2->count(), 1);
        QCOMPARE(repeater2->itemAt(0)->objectName(), "secondItem");
    }

    {
        QQmlComponent component(&engine, testFileUrl("package2.qml"));
        QScopedPointer<QObject> root(component.create());
        QVERIFY(root != nullptr);
        bool returnedValue = false;
        // calling setup should not crash
        QMetaObject::invokeMethod(root.get(), "setup", Q_RETURN_ARG(bool, returnedValue));
        QVERIFY(returnedValue);
    }
}

void tst_QQuickRepeater::ownership()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("ownership.qml"));

    std::unique_ptr<QAbstractItemModel> aim(new QStandardItemModel);
    QPointer<QAbstractItemModel> modelGuard(aim.get());
    QQmlEngine::setObjectOwnership(aim.get(), QQmlEngine::JavaScriptOwnership);
    {
        QJSValue wrapper = engine.newQObject(aim.get());
    }

    std::unique_ptr<QObject> repeater(component.create());
    QVERIFY(repeater);

    QVERIFY(!QQmlData::keepAliveDuringGarbageCollection(aim.get()));

    repeater->setProperty("model", QVariant::fromValue<QObject*>(aim.get()));

    QVERIFY(!QQmlData::keepAliveDuringGarbageCollection(aim.get()));

    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QVERIFY(modelGuard);

    std::unique_ptr<QQmlComponent> delegate(new QQmlComponent(&engine));
    delegate->setData(QByteArrayLiteral("import QtQuick 2.0\nItem{}"), dataDirectoryUrl().resolved(QUrl("inline.qml")));
    QPointer<QQmlComponent> delegateGuard(delegate.get());
    QQmlEngine::setObjectOwnership(delegate.get(), QQmlEngine::JavaScriptOwnership);
    {
        QJSValue wrapper = engine.newQObject(delegate.get());
    }

    QVERIFY(!QQmlData::keepAliveDuringGarbageCollection(delegate.get()));

    repeater->setProperty("delegate", QVariant::fromValue<QObject*>(delegate.get()));

    QVERIFY(!QQmlData::keepAliveDuringGarbageCollection(delegate.get()));

    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QVERIFY(delegateGuard);

    repeater->setProperty("model", QVariant());
    repeater->setProperty("delegate", QVariant());

    QVERIFY(delegateGuard);
    QVERIFY(modelGuard);

    delegate.release();
    aim.release();

    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QVERIFY(!delegateGuard);
    QVERIFY(!modelGuard);
}

void tst_QQuickRepeater::requiredProperties()
{
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "apples0");
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "oranges1");
    QTest::ignoreMessage(QtMsgType::QtInfoMsg, "pears2");
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("requiredProperty.qml"));
    QScopedPointer<QObject> o {component.create()};
    QVERIFY(o);
}

void tst_QQuickRepeater::contextProperties()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("contextProperty.qml"));
    QScopedPointer<QObject> o {component.create()};
    QVERIFY(o);

    auto *root = qobject_cast<QQuickItem *>(o.get());
    QVERIFY(root);

    QQueue<QQuickItem *> items;
    items.append(root);

    while (!items.isEmpty()) {
        QQuickItem *item = items.dequeue();
        QQmlRefPointer<QQmlContextData> contextData = QQmlContextData::get(qmlContext(item));

        // Context object and extra object should never be the same. There are ways for the extra
        // object to exist even without required properties, though.
        QVERIFY(contextData->contextObject() != contextData->extraObject());
        for (QQuickItem *child : item->childItems())
            items.enqueue(child);
    }
}

void tst_QQuickRepeater::innerRequired()
{
    QQmlEngine engine;
    const QUrl url(testFileUrl("innerRequired.qml"));
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    QQuickRepeater *a = qobject_cast<QQuickRepeater *>(
            qmlContext(o.data())->objectForName(QStringLiteral("repeater")));
    QVERIFY(a);

    QCOMPARE(a->count(), 2);
    QCOMPARE(a->itemAt(0)->property("age").toInt(), 8);
    QCOMPARE(a->itemAt(0)->property("text").toString(), u"meow");
    QCOMPARE(a->itemAt(1)->property("age").toInt(), 5);
    QCOMPARE(a->itemAt(1)->property("text").toString(), u"woof");
}

void tst_QQuickRepeater::boundDelegateComponent()
{
    QQmlEngine engine;
    const QUrl url(testFileUrl("boundDelegateComponent.qml"));
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    for (int i = 0; i < 3; ++i) {
        QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(QLatin1String("%1:12: ReferenceError: modelData is not defined")
                                   .arg(url.toString())));
    }

    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    QQuickRepeater *a = qobject_cast<QQuickRepeater *>(
            qmlContext(o.data())->objectForName(QStringLiteral("undefinedModelData")));
    QVERIFY(a);
    QCOMPARE(a->count(), 3);
    for (int i = 0; i < 3; ++i)
        QCOMPARE(a->itemAt(i)->objectName(), QStringLiteral("rootundefined"));

    QQuickRepeater *b = qobject_cast<QQuickRepeater *>(
            qmlContext(o.data())->objectForName(QStringLiteral("requiredModelData")));
    QVERIFY(b);
    QCOMPARE(b->count(), 3);
    QCOMPARE(b->itemAt(0)->objectName(), QStringLiteral("rootaa"));
    QCOMPARE(b->itemAt(1)->objectName(), QStringLiteral("rootbb"));
    QCOMPARE(b->itemAt(2)->objectName(), QStringLiteral("rootcc"));
}

QTEST_MAIN(tst_QQuickRepeater)

#include "tst_qquickrepeater.moc"
