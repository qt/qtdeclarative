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

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <private/qlistmodelinterface_p.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeincubator.h>
#include <private/qquickrepeater_p.h>
#include <QtQuick/private/qquicktext_p.h>

#include "../../shared/util.h"

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(TESTDATA(filename));
}

class tst_QQuickRepeater : public QObject
{
    Q_OBJECT
public:
    tst_QQuickRepeater();

private slots:
    void numberModel();
    void objectList();
    void stringList();
    void dataModel_adding();
    void dataModel_removing();
    void dataModel_changes();
    void itemModel();
    void resetModel();
    void modelChanged();
    void properties();
    void asynchronous();
    void initParent();

private:
    QQuickView *createView();
    template<typename T>
    T *findItem(QObject *parent, const QString &objectName, int index);
    template<typename T>
    T *findItem(QObject *parent, const QString &id);
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

class TestModel : public QAbstractListModel
{
public:
    enum Roles { Name = Qt::UserRole+1, Number = Qt::UserRole+2 };

    TestModel(QObject *parent=0) : QAbstractListModel(parent) {
        QHash<int, QByteArray> roles;
        roles[Name] = "name";
        roles[Number] = "number";
        setRoleNames(roles);
    }

    int rowCount(const QModelIndex &parent=QModelIndex()) const { Q_UNUSED(parent); return list.count(); }
    QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const {
        QVariant rv;
        if (role == Name)
            rv = list.at(index.row()).first;
        else if (role == Number)
            rv = list.at(index.row()).second;

        return rv;
    }

    int count() const { return rowCount(); }
    QString name(int index) const { return list.at(index).first; }
    QString number(int index) const { return list.at(index).second; }

    void addItem(const QString &name, const QString &number) {
        emit beginInsertRows(QModelIndex(), list.count(), list.count());
        list.append(QPair<QString,QString>(name, number));
        emit endInsertRows();
    }

    void insertItem(int index, const QString &name, const QString &number) {
        emit beginInsertRows(QModelIndex(), index, index);
        list.insert(index, QPair<QString,QString>(name, number));
        emit endInsertRows();
    }

    void removeItem(int index) {
        emit beginRemoveRows(QModelIndex(), index, index);
        list.removeAt(index);
        emit endRemoveRows();
    }

    void moveItem(int from, int to) {
        emit beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        list.move(from, to);
        emit endMoveRows();
    }

    void modifyItem(int idx, const QString &name, const QString &number) {
        list[idx] = QPair<QString,QString>(name, number);
        emit dataChanged(index(idx,0), index(idx,0));
    }

private:
    QList<QPair<QString,QString> > list;
};


tst_QQuickRepeater::tst_QQuickRepeater()
{
}

void tst_QQuickRepeater::numberModel()
{
    QQuickView *canvas = createView();

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testData", 5);
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(TEST_FILE("intmodel.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);
    QCOMPARE(repeater->parentItem()->childItems().count(), 5+1);

    QVERIFY(!repeater->itemAt(-1));
    for (int i=0; i<repeater->count(); i++)
        QCOMPARE(repeater->itemAt(i), repeater->parentItem()->childItems().at(i));
    QVERIFY(!repeater->itemAt(repeater->count()));

    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QVERIFY(testObject->error() == false);

    delete testObject;
    delete canvas;
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
    QQuickView *canvas = createView();
    QObjectList data;
    for (int i=0; i<100; i++)
        data << new MyObject(i);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testData", QVariant::fromValue(data));

    canvas->setSource(TEST_FILE("objlist.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);
    QCOMPARE(repeater->property("errors").toInt(), 0);//If this fails either they are out of order or can't find the object's data
    QCOMPARE(repeater->property("instantiated").toInt(), 100);

    QVERIFY(!repeater->itemAt(-1));
    for (int i=0; i<data.count(); i++)
        QCOMPARE(repeater->itemAt(i), repeater->parentItem()->childItems().at(i));
    QVERIFY(!repeater->itemAt(data.count()));

    QSignalSpy addedSpy(repeater, SIGNAL(itemAdded(int,QQuickItem*)));
    QSignalSpy removedSpy(repeater, SIGNAL(itemRemoved(int,QQuickItem*)));
    ctxt->setContextProperty("testData", QVariant::fromValue(data));
    QCOMPARE(addedSpy.count(), data.count());
    QCOMPARE(removedSpy.count(), data.count());

    qDeleteAll(data);
    delete canvas;
}

/*
The Repeater element creates children at its own position in its parent's
stacking order.  In this test we insert a repeater between two other Text
elements to test this.
*/
void tst_QQuickRepeater::stringList()
{
    QQuickView *canvas = createView();

    QStringList data;
    data << "One";
    data << "Two";
    data << "Three";
    data << "Four";

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testData", data);

    canvas->setSource(TEST_FILE("repeater1.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);

    QQuickItem *container = findItem<QQuickItem>(canvas->rootObject(), "container");
    QVERIFY(container != 0);

    QCOMPARE(container->childItems().count(), data.count() + 3);

    bool saw_repeater = false;
    for (int i = 0; i < container->childItems().count(); ++i) {

        if (i == 0) {
            QQuickText *name = qobject_cast<QQuickText*>(container->childItems().at(i));
            QVERIFY(name != 0);
            QCOMPARE(name->text(), QLatin1String("Zero"));
        } else if (i == container->childItems().count() - 2) {
            // The repeater itself
            QQuickRepeater *rep = qobject_cast<QQuickRepeater*>(container->childItems().at(i));
            QCOMPARE(rep, repeater);
            saw_repeater = true;
            continue;
        } else if (i == container->childItems().count() - 1) {
            QQuickText *name = qobject_cast<QQuickText*>(container->childItems().at(i));
            QVERIFY(name != 0);
            QCOMPARE(name->text(), QLatin1String("Last"));
        } else {
            QQuickText *name = qobject_cast<QQuickText*>(container->childItems().at(i));
            QVERIFY(name != 0);
            QCOMPARE(name->text(), data.at(i-1));
        }
    }
    QVERIFY(saw_repeater);

    delete canvas;
}

void tst_QQuickRepeater::dataModel_adding()
{
    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    TestModel testModel;
    ctxt->setContextProperty("testData", &testModel);
    canvas->setSource(TEST_FILE("repeater2.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);
    QQuickItem *container = findItem<QQuickItem>(canvas->rootObject(), "container");
    QVERIFY(container != 0);

    QVERIFY(!repeater->itemAt(0));

    QSignalSpy countSpy(repeater, SIGNAL(countChanged()));
    QSignalSpy addedSpy(repeater, SIGNAL(itemAdded(int,QQuickItem*)));

    // add to empty model
    testModel.addItem("two", "2");
    QCOMPARE(repeater->itemAt(0), container->childItems().at(0));
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(0));
    addedSpy.clear();

    // insert at start
    testModel.insertItem(0, "one", "1");
    QCOMPARE(repeater->itemAt(0), container->childItems().at(0));
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(0));
    addedSpy.clear();

    // insert at end
    testModel.insertItem(2, "four", "4");
    QCOMPARE(repeater->itemAt(2), container->childItems().at(2));
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 2);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(2));
    addedSpy.clear();

    // insert in middle
    testModel.insertItem(2, "three", "3");
    QCOMPARE(repeater->itemAt(2), container->childItems().at(2));
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(addedSpy.at(0).at(0).toInt(), 2);
    QCOMPARE(addedSpy.at(0).at(1).value<QQuickItem*>(), container->childItems().at(2));
    addedSpy.clear();

    delete testObject;
    addedSpy.clear();
    countSpy.clear();
    delete canvas;
}

void tst_QQuickRepeater::dataModel_removing()
{
    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    TestModel testModel;
    testModel.addItem("one", "1");
    testModel.addItem("two", "2");
    testModel.addItem("three", "3");
    testModel.addItem("four", "4");
    testModel.addItem("five", "5");

    ctxt->setContextProperty("testData", &testModel);
    canvas->setSource(TEST_FILE("repeater2.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);
    QQuickItem *container = findItem<QQuickItem>(canvas->rootObject(), "container");
    QVERIFY(container != 0);
    QCOMPARE(container->childItems().count(), repeater->count()+1);

    QSignalSpy countSpy(repeater, SIGNAL(countChanged()));
    QSignalSpy removedSpy(repeater, SIGNAL(itemRemoved(int,QQuickItem*)));

    // remove at start
    QQuickItem *item = repeater->itemAt(0);
    QCOMPARE(item, container->childItems().at(0));

    testModel.removeItem(0);
    QVERIFY(repeater->itemAt(0) != item);
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(removedSpy.at(0).at(1).value<QQuickItem*>(), item);
    removedSpy.clear();

    // remove at end
    int lastIndex = testModel.count()-1;
    item = repeater->itemAt(lastIndex);
    QCOMPARE(item, container->childItems().at(lastIndex));

    testModel.removeItem(lastIndex);
    QVERIFY(repeater->itemAt(lastIndex) != item);
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toInt(), lastIndex);
    QCOMPARE(removedSpy.at(0).at(1).value<QQuickItem*>(), item);
    removedSpy.clear();

    // remove from middle
    item = repeater->itemAt(1);
    QCOMPARE(item, container->childItems().at(1));

    testModel.removeItem(1);
    QVERIFY(repeater->itemAt(lastIndex) != item);
    QCOMPARE(countSpy.count(), 1); countSpy.clear();
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(removedSpy.at(0).at(1).value<QQuickItem*>(), item);
    removedSpy.clear();

    delete testObject;
    delete canvas;
}

void tst_QQuickRepeater::dataModel_changes()
{
    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    TestModel testModel;
    testModel.addItem("one", "1");
    testModel.addItem("two", "2");
    testModel.addItem("three", "3");

    ctxt->setContextProperty("testData", &testModel);
    canvas->setSource(TEST_FILE("repeater2.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);
    QQuickItem *container = findItem<QQuickItem>(canvas->rootObject(), "container");
    QVERIFY(container != 0);
    QCOMPARE(container->childItems().count(), repeater->count()+1);

    // Check that model changes are propagated
    QQuickText *text = findItem<QQuickText>(canvas->rootObject(), "myName", 1);
    QVERIFY(text);
    QCOMPARE(text->text(), QString("two"));

    testModel.modifyItem(1, "Item two", "_2");
    text = findItem<QQuickText>(canvas->rootObject(), "myName", 1);
    QVERIFY(text);
    QCOMPARE(text->text(), QString("Item two"));

    text = findItem<QQuickText>(canvas->rootObject(), "myNumber", 1);
    QVERIFY(text);
    QCOMPARE(text->text(), QString("_2"));

    delete testObject;
    delete canvas;
}

void tst_QQuickRepeater::itemModel()
{
    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(TEST_FILE("itemlist.qml"));
    qApp->processEvents();

    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);

    QQuickItem *container = findItem<QQuickItem>(canvas->rootObject(), "container");
    QVERIFY(container != 0);

    QCOMPARE(container->childItems().count(), 1);

    testObject->setUseModel(true);
    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QVERIFY(testObject->error() == false);

    QCOMPARE(container->childItems().count(), 4);
    QVERIFY(qobject_cast<QObject*>(container->childItems().at(0))->objectName() == "item1");
    QVERIFY(qobject_cast<QObject*>(container->childItems().at(1))->objectName() == "item2");
    QVERIFY(qobject_cast<QObject*>(container->childItems().at(2))->objectName() == "item3");
    QVERIFY(container->childItems().at(3) == repeater);

    QMetaObject::invokeMethod(canvas->rootObject(), "switchModel");
    QCOMPARE(container->childItems().count(), 3);
    QVERIFY(qobject_cast<QObject*>(container->childItems().at(0))->objectName() == "item4");
    QVERIFY(qobject_cast<QObject*>(container->childItems().at(1))->objectName() == "item5");
    QVERIFY(container->childItems().at(2) == repeater);

    testObject->setUseModel(false);
    QCOMPARE(container->childItems().count(), 1);

    delete testObject;
    delete canvas;
}

void tst_QQuickRepeater::resetModel()
{
    QQuickView *canvas = createView();

    QStringList dataA;
    for (int i=0; i<10; i++)
        dataA << QString::number(i);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testData", dataA);
    canvas->setSource(TEST_FILE("repeater1.qml"));
    qApp->processEvents();
    QQuickRepeater *repeater = findItem<QQuickRepeater>(canvas->rootObject(), "repeater");
    QVERIFY(repeater != 0);
    QQuickItem *container = findItem<QQuickItem>(canvas->rootObject(), "container");
    QVERIFY(container != 0);

    QCOMPARE(repeater->count(), dataA.count());
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
    QCOMPARE(repeater->count(), dataB.count());

    QCOMPARE(modelChangedSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(removedSpy.count(), dataA.count());
    QCOMPARE(addedSpy.count(), dataB.count());
    for (int i=0; i<dataB.count(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }
    modelChangedSpy.clear();
    countSpy.clear();
    removedSpy.clear();
    addedSpy.clear();

    // reset via setModel()
    repeater->setModel(dataA);
    QCOMPARE(repeater->count(), dataA.count());

    QCOMPARE(modelChangedSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(removedSpy.count(), dataB.count());
    QCOMPARE(addedSpy.count(), dataA.count());
    for (int i=0; i<dataA.count(); i++) {
        QCOMPARE(addedSpy.at(i).at(0).toInt(), i);
        QCOMPARE(addedSpy.at(i).at(1).value<QQuickItem*>(), repeater->itemAt(i));
    }

    modelChangedSpy.clear();
    countSpy.clear();
    removedSpy.clear();
    addedSpy.clear();

    delete canvas;
}

// QTBUG-17156
void tst_QQuickRepeater::modelChanged()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, TEST_FILE("modelChanged.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(rootObject);
    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);

    repeater->setModel(4);
    QCOMPARE(repeater->count(), 4);
    QCOMPARE(repeater->property("itemsCount").toInt(), 4);
    QCOMPARE(repeater->property("itemsFound").toList().count(), 4);

    repeater->setModel(10);
    QCOMPARE(repeater->count(), 10);
    QCOMPARE(repeater->property("itemsCount").toInt(), 10);
    QCOMPARE(repeater->property("itemsFound").toList().count(), 10);

    delete rootObject;
}

void tst_QQuickRepeater::properties()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, TEST_FILE("properties.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(rootObject);

    QQuickRepeater *repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    QVERIFY(repeater);

    QSignalSpy modelSpy(repeater, SIGNAL(modelChanged()));
    repeater->setModel(3);
    QCOMPARE(modelSpy.count(),1);
    repeater->setModel(3);
    QCOMPARE(modelSpy.count(),1);

    QSignalSpy delegateSpy(repeater, SIGNAL(delegateChanged()));

    QDeclarativeComponent rectComponent(&engine);
    rectComponent.setData("import QtQuick 2.0; Rectangle {}", QUrl::fromLocalFile(""));

    repeater->setDelegate(&rectComponent);
    QCOMPARE(delegateSpy.count(),1);
    repeater->setDelegate(&rectComponent);
    QCOMPARE(delegateSpy.count(),1);

    delete rootObject;
}

void tst_QQuickRepeater::asynchronous()
{
    QQuickView *canvas = createView();
    canvas->show();
    QDeclarativeIncubationController controller;
    canvas->engine()->setIncubationController(&controller);

    canvas->setSource(TEST_FILE("asyncloader.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootObject);

    QQuickItem *container = findItem<QQuickItem>(rootObject, "container");
    QVERIFY(!container);
    while (!container) {
        bool b = false;
        controller.incubateWhile(&b);
        container = findItem<QQuickItem>(rootObject, "container");
    }

    QQuickRepeater *repeater = 0;
    while (!repeater) {
        bool b = false;
        controller.incubateWhile(&b);
        repeater = findItem<QQuickRepeater>(rootObject, "repeater");
    }

    // items will be created one at a time
    for (int i = 0; i < 10; ++i) {
        QString name("delegate");
        name += QString::number(i);
        QVERIFY(findItem<QQuickItem>(container, name) == 0);
        QQuickItem *item = 0;
        while (!item) {
            bool b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(container, name);
        }
    }

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    // verify positioning
    for (int i = 0; i < 10; ++i) {
        QString name("delegate");
        name += QString::number(i);
        QQuickItem *item = findItem<QQuickItem>(container, name);
        QTRY_COMPARE(item->y(), i * 50.0);
    }

    delete canvas;
}

void tst_QQuickRepeater::initParent()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, TEST_FILE("initparent.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(rootObject);

    QCOMPARE(qvariant_cast<QQuickItem*>(rootObject->property("parentItem")), rootObject);
}

QQuickView *tst_QQuickRepeater::createView()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    return canvas;
}

template<typename T>
T *tst_QQuickRepeater::findItem(QObject *parent, const QString &objectName, int index)
{
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->children().count() << "children";
    for (int i = 0; i < parent->children().count(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(parent->children().at(i));
        if (!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
            if (index != -1) {
                QDeclarativeExpression e(qmlContext(item), item, "index");
                if (e.evaluate().toInt() == index)
                    return static_cast<T*>(item);
            } else {
                return static_cast<T*>(item);
            }
        }
        item = findItem<T>(item, objectName, index);
        if (item)
            return static_cast<T*>(item);
    }

    return 0;
}

template<typename T>
T *tst_QQuickRepeater::findItem(QObject *parent, const QString &objectName)
{
    const QMetaObject &mo = T::staticMetaObject;
    if (mo.cast(parent) && (objectName.isEmpty() || parent->objectName() == objectName))
        return static_cast<T*>(parent);
    for (int i = 0; i < parent->children().count(); ++i) {
        QQuickItem *child = qobject_cast<QQuickItem*>(parent->children().at(i));
        if (!child)
            continue;
        QQuickItem *item = findItem<T>(child, objectName);
        if (item)
            return static_cast<T*>(item);
    }

    return 0;
}

QTEST_MAIN(tst_QQuickRepeater)

#include "tst_qquickrepeater.moc"
