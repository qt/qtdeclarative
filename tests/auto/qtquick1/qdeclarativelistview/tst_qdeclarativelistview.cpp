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
#include <QtGui/QStringListModel>
#include <QtQuick1/qdeclarativeview.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtQuick1/private/qdeclarativeitem_p.h>
#include <QtQuick1/private/qdeclarativelistview_p.h>
#include <QtQuick1/private/qdeclarativetext_p.h>
#include <QtQuick1/private/qdeclarativevisualitemmodel_p.h>
#include <QtDeclarative/private/qlistmodelinterface_p.h>
#include "../../../shared/util.h"
#include "incrementalmodel.h"

#ifdef Q_OS_SYMBIAN
// In Symbian OS test data is located in applications private dir
#define SRCDIR "."
#endif

class tst_QDeclarative1ListView : public QObject
{
    Q_OBJECT
public:
    tst_QDeclarative1ListView();

private slots:
    // Test both QListModelInterface and QAbstractItemModel model types
    void qListModelInterface_items();
    void qAbstractItemModel_items();

    void qListModelInterface_changed();
    void qAbstractItemModel_changed();

    void qListModelInterface_inserted();
    void qAbstractItemModel_inserted();

    void qListModelInterface_removed();
    void qAbstractItemModel_removed();

    void qListModelInterface_moved();
    void qAbstractItemModel_moved();

    void qListModelInterface_clear();
    void qAbstractItemModel_clear();

    void itemList();
    void currentIndex();
    void noCurrentIndex();
    void enforceRange();
    void spacing();
    void sections();
    void sectionsDelegate();
    void cacheBuffer();
    void positionViewAtIndex();
    void resetModel();
    void propertyChanges();
    void componentChanges();
    void modelChanges();
    void QTBUG_9791();
    void manualHighlight();
    void QTBUG_11105();
    void header();
    void footer();
    void headerFooter();
    void resizeView();
    void sizeLessThan1();
    void QTBUG_14821();
    void resizeDelegate();
    void QTBUG_16037();
    void indexAt();
    void incrementalModel();
    void onAdd();
    void onAdd_data();
    void onRemove();
    void onRemove_data();
    void testQtQuick11Attributes();
    void testQtQuick11Attributes_data();
    void rightToLeft();
    void test_mirroring();
    void orientationChange();
    void contentPosJump();

private:
    template <class T> void items();
    template <class T> void changed();
    template <class T> void inserted();
    template <class T> void removed(bool animated);
    template <class T> void moved();
    template <class T> void clear();
    QDeclarativeView *createView();
    template<typename T>
    T *findItem(QGraphicsObject *parent, const QString &id, int index=-1);
    template<typename T>
    QList<T*> findItems(QGraphicsObject *parent, const QString &objectName);
    void dumpTree(QDeclarativeItem *parent, int depth = 0);
};

class TestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool error READ error WRITE setError NOTIFY changedError)
    Q_PROPERTY(bool animate READ animate NOTIFY changedAnim)
    Q_PROPERTY(bool invalidHighlight READ invalidHighlight NOTIFY changedHl)
    Q_PROPERTY(int cacheBuffer READ cacheBuffer NOTIFY changedCacheBuffer)

public:
    TestObject(QObject *parent = 0)
        : QObject(parent), mError(true), mAnimate(false), mInvalidHighlight(false)
        , mCacheBuffer(0) {}

    bool error() const { return mError; }
    void setError(bool err) { mError = err; emit changedError(); }

    bool animate() const { return mAnimate; }
    void setAnimate(bool anim) { mAnimate = anim; emit changedAnim(); }

    bool invalidHighlight() const { return mInvalidHighlight; }
    void setInvalidHighlight(bool invalid) { mInvalidHighlight = invalid; emit changedHl(); }

    int cacheBuffer() const { return mCacheBuffer; }
    void setCacheBuffer(int buffer) { mCacheBuffer = buffer; emit changedCacheBuffer(); }

signals:
    void changedError();
    void changedAnim();
    void changedHl();
    void changedCacheBuffer();

public:
    bool mError;
    bool mAnimate;
    bool mInvalidHighlight;
    int mCacheBuffer;
};

class TestModel : public QListModelInterface
{
    Q_OBJECT
public:
    TestModel(QObject *parent = 0) : QListModelInterface(parent) {}
    ~TestModel() {}

    enum Roles { Name, Number };

    QString name(int index) const { return list.at(index).first; }
    QString number(int index) const { return list.at(index).second; }

    int count() const { return list.count(); }

    QList<int> roles() const { return QList<int>() << Name << Number; }
    QString toString(int role) const {
        switch(role) {
        case Name:
            return "name";
        case Number:
            return "number";
        default:
            return "";
        }
    }

    QVariant data(int index, int role) const
    {
        if (role==0)
            return list.at(index).first;
        if (role==1)
            return list.at(index).second;
        return QVariant();
    }
    QHash<int, QVariant> data(int index, const QList<int> &roles) const {
        QHash<int,QVariant> returnHash;

        for (int i = 0; i < roles.size(); ++i) {
            int role = roles.at(i);
            QVariant info;
            switch(role) {
            case Name:
                info = list.at(index).first;
                break;
            case Number:
                info = list.at(index).second;
                break;
            default:
                break;
            }
            returnHash.insert(role, info);
        }
        return returnHash;
    }

    void addItem(const QString &name, const QString &number) {
        list.append(QPair<QString,QString>(name, number));
        emit itemsInserted(list.count()-1, 1);
    }

    void insertItem(int index, const QString &name, const QString &number) {
        list.insert(index, QPair<QString,QString>(name, number));
        emit itemsInserted(index, 1);
    }

    void removeItem(int index) {
        list.removeAt(index);
        emit itemsRemoved(index, 1);
    }

    void removeItems(int index, int count) {
        int c = count;
        while (c--)
            list.removeAt(index);
        emit itemsRemoved(index, count);
    }

    void moveItem(int from, int to) {
        list.move(from, to);
        emit itemsMoved(from, to, 1);
    }

    void modifyItem(int index, const QString &name, const QString &number) {
        list[index] = QPair<QString,QString>(name, number);
        emit itemsChanged(index, 1, roles());
    }

    void clear() {
        int count = list.count();
        list.clear();
        emit itemsRemoved(0, count);
    }

private:
    QList<QPair<QString,QString> > list;
};


class TestModel2 : public QAbstractListModel
{
public:
    enum Roles { Name = Qt::UserRole+1, Number = Qt::UserRole+2 };

    TestModel2(QObject *parent=0) : QAbstractListModel(parent) {
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

    void addItems(const QList<QPair<QString, QString> > &items) {
        emit beginInsertRows(QModelIndex(), list.count(), list.count()+items.count()-1);
        for (int i=0; i<items.count(); i++)
            list.append(QPair<QString,QString>(items[i].first, items[i].second));
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

    void removeItems(int index, int count) {
        emit beginRemoveRows(QModelIndex(), index, index+count-1);
        while (count--)
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

    void clear() {
        int count = list.count();
        emit beginRemoveRows(QModelIndex(), 0, count-1);
        list.clear();
        emit endRemoveRows();
    }

private:
    QList<QPair<QString,QString> > list;
};

tst_QDeclarative1ListView::tst_QDeclarative1ListView()
{
}

template <class T>
void tst_QDeclarative1ListView::items()
{
    QDeclarativeView *canvas = createView();

    T model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QTRY_VERIFY(testObject->error() == false);

    QTRY_VERIFY(listview->highlightItem() != 0);
    QTRY_COMPARE(listview->count(), model.count());
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QTRY_COMPARE(contentItem->childItems().count(), model.count()+1); // assumes all are visible, +1 for the (default) highlight item

    // current item should be first item
    QTRY_COMPARE(listview->currentItem(), findItem<QDeclarativeItem>(contentItem, "wrapper", 0));

    for (int i = 0; i < model.count(); ++i) {
        QDeclarative1Text *name = findItem<QDeclarative1Text>(contentItem, "textName", i);
        QTRY_VERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        QDeclarative1Text *number = findItem<QDeclarative1Text>(contentItem, "textNumber", i);
        QTRY_VERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));
    }

    // switch to other delegate
    testObject->setAnimate(true);
    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QTRY_VERIFY(testObject->error() == false);
    QTRY_VERIFY(listview->currentItem());

    // set invalid highlight
    testObject->setInvalidHighlight(true);
    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QTRY_VERIFY(testObject->error() == false);
    QTRY_VERIFY(listview->currentItem());
    QTRY_VERIFY(listview->highlightItem() == 0);

    // back to normal highlight
    testObject->setInvalidHighlight(false);
    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QTRY_VERIFY(testObject->error() == false);
    QTRY_VERIFY(listview->currentItem());
    QTRY_VERIFY(listview->highlightItem() != 0);

    // set an empty model and confirm that items are destroyed
    T model2;
    ctxt->setContextProperty("testModel", &model2);

    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    QTRY_VERIFY(itemCount == 0);

    QTRY_COMPARE(listview->highlightResizeSpeed(), 1000.0);
    QTRY_COMPARE(listview->highlightMoveSpeed(), 1000.0);

    delete canvas;
    delete testObject;
}


template <class T>
void tst_QDeclarative1ListView::changed()
{
    QDeclarativeView *canvas = createView();

    T model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1Flickable *listview = findItem<QDeclarative1Flickable>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    model.modifyItem(1, "Will", "9876");
    QDeclarative1Text *name = findItem<QDeclarative1Text>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QDeclarative1Text *number = findItem<QDeclarative1Text>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    delete canvas;
    delete testObject;
}

template <class T>
void tst_QDeclarative1ListView::inserted()
{
    QDeclarativeView *canvas = createView();

    T model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    model.insertItem(1, "Will", "9876");

    QTRY_COMPARE(contentItem->childItems().count(), model.count()+1); // assumes all are visible, +1 for the (default) highlight item

    QDeclarative1Text *name = findItem<QDeclarative1Text>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QDeclarative1Text *number = findItem<QDeclarative1Text>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    // Confirm items positioned correctly
    for (int i = 0; i < model.count(); ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QTRY_COMPARE(item->y(), i*20.0);
    }

    model.insertItem(0, "Foo", "1111"); // zero index, and current item

    QCOMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QTRY_COMPARE(contentItem->childItems().count(), model.count()+1); // assumes all are visible, +1 for the (default) highlight item

    name = findItem<QDeclarative1Text>(contentItem, "textName", 0);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(0));
    number = findItem<QDeclarative1Text>(contentItem, "textNumber", 0);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(0));

    QTRY_COMPARE(listview->currentIndex(), 1);

    // Confirm items positioned correctly
    for (int i = 0; i < model.count(); ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QTRY_COMPARE(item->y(), i*20.0);
    }

    for (int i = model.count(); i < 30; ++i)
        model.insertItem(i, "Hello", QString::number(i));

    listview->setContentY(80);

    // Insert item outside visible area
    model.insertItem(1, "Hello", "1324");

    QTRY_VERIFY(listview->contentY() == 80);

    // Confirm items positioned correctly
    for (int i = 5; i < 5+15; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.0 - 20.0);
    }

//    QTRY_COMPARE(listview->contentItemHeight(), model.count() * 20.0);

    // QTBUG-19675
    model.clear();
    model.insertItem(0, "Hello", "1234");
    QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->y(), 0.);
    QVERIFY(listview->contentY() == 0);

    delete canvas;
    delete testObject;
}

template <class T>
void tst_QDeclarative1ListView::removed(bool animated)
{
    QDeclarativeView *canvas = createView();

    T model;
    for (int i = 0; i < 50; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    testObject->setAnimate(animated);
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    model.removeItem(1);
    QCOMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    QDeclarative1Text *name = findItem<QDeclarative1Text>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QDeclarative1Text *number = findItem<QDeclarative1Text>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    // Remove first item (which is the current item);
    model.removeItem(0);  // post: top item starts at 20

    QTest::qWait(300);

    name = findItem<QDeclarative1Text>(contentItem, "textName", 0);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(0));
    number = findItem<QDeclarative1Text>(contentItem, "textNumber", 0);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(0));

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),i*20.0 + 20.0);
    }

    // Remove items not visible
    model.removeItem(18);
    qApp->processEvents();

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),i*20.0+20.0);
    }

    // Remove items before visible
    listview->setContentY(80);
    listview->setCurrentIndex(10);

    model.removeItem(1); // post: top item will be at 40
    qApp->processEvents();

    // Confirm items positioned correctly
    for (int i = 2; i < 18; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),40+i*20.0);
    }

    // Remove current index
    QTRY_VERIFY(listview->currentIndex() == 9);
    QDeclarativeItem *oldCurrent = listview->currentItem();
    model.removeItem(9);

    QTRY_COMPARE(listview->currentIndex(), 9);
    QTRY_VERIFY(listview->currentItem() != oldCurrent);

    listview->setContentY(40); // That's the top now
    // let transitions settle.
    QTest::qWait(300);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),40+i*20.0);
    }

    // remove current item beyond visible items.
    listview->setCurrentIndex(20);
    listview->setContentY(40);
    model.removeItem(20);

    QTRY_COMPARE(listview->currentIndex(), 20);
    QTRY_VERIFY(listview->currentItem() != 0);

    // remove item before current, but visible
    listview->setCurrentIndex(8);
    oldCurrent = listview->currentItem();
    model.removeItem(6);

    QTRY_COMPARE(listview->currentIndex(), 7);
    QTRY_VERIFY(listview->currentItem() == oldCurrent);

    listview->setContentY(80);
    QTest::qWait(300);

    // remove all visible items
    model.removeItems(1, 18);
    QTest::qWait(300);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount-1; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i+2);
        if (!item) qWarning() << "Item" << i+2 << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),80+i*20.0);
    }

    model.removeItems(1, 17);
//    QTest::qWait(300);

    model.removeItems(2, 1);
    model.addItem("New", "1");

    QTRY_VERIFY(name = findItem<QDeclarative1Text>(contentItem, "textName", model.count()-1));
    QCOMPARE(name->text(), QString("New"));

    // Add some more items so that we don't run out
    model.clear();
    for (int i = 0; i < 50; i++)
        model.addItem("Item" + QString::number(i), "");

    // QTBUG-QTBUG-20575
    listview->setCurrentIndex(0);
    listview->setContentY(30);
    model.removeItem(0);
    QTRY_VERIFY(name = findItem<QDeclarative1Text>(contentItem, "textName", 0));

    // QTBUG-19198 move to end and remove all visible items one at a time.
    listview->positionViewAtEnd();
    for (int i = 0; i < 18; ++i)
        model.removeItems(model.count() - 1, 1);
    QTRY_VERIFY(findItems<QDeclarativeItem>(contentItem, "wrapper").count() > 16);

    delete canvas;
    delete testObject;
}

template <class T>
void tst_QDeclarative1ListView::clear()
{
    QDeclarativeView *canvas = createView();

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    model.clear();

    QTRY_VERIFY(listview->count() == 0);
    QTRY_VERIFY(listview->currentItem() == 0);
    QTRY_VERIFY(listview->contentY() == 0);
    QVERIFY(listview->currentIndex() == -1);

    // confirm sanity when adding an item to cleared list
    model.addItem("New", "1");
    QTRY_VERIFY(listview->count() == 1);
    QVERIFY(listview->currentItem() != 0);
    QVERIFY(listview->currentIndex() == 0);

    delete canvas;
    delete testObject;
}


template <class T>
void tst_QDeclarative1ListView::moved()
{
    QDeclarativeView *canvas = createView();

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    model.moveItem(1, 4);

    QDeclarative1Text *name = findItem<QDeclarative1Text>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QDeclarative1Text *number = findItem<QDeclarative1Text>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    name = findItem<QDeclarative1Text>(contentItem, "textName", 4);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(4));
    number = findItem<QDeclarative1Text>(contentItem, "textNumber", 4);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(4));

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    listview->setContentY(80);

    // move outside visible area
    model.moveItem(1, 18);

    // Confirm items positioned correctly and indexes correct
    for (int i = 3; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.0 + 20);
        name = findItem<QDeclarative1Text>(contentItem, "textName", i);
        QTRY_VERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        number = findItem<QDeclarative1Text>(contentItem, "textNumber", i);
        QTRY_VERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));
    }

    // move from outside visible into visible
    model.moveItem(20, 4);

    // Confirm items positioned correctly and indexes correct
    for (int i = 3; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.0 + 20);
        name = findItem<QDeclarative1Text>(contentItem, "textName", i);
        QTRY_VERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        number = findItem<QDeclarative1Text>(contentItem, "textNumber", i);
        QTRY_VERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::enforceRange()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listview-enforcerange.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QTRY_COMPARE(listview->preferredHighlightBegin(), 100.0);
    QTRY_COMPARE(listview->preferredHighlightEnd(), 100.0);
    QTRY_COMPARE(listview->highlightRangeMode(), QDeclarative1ListView::StrictlyEnforceRange);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // view should be positioned at the top of the range.
    QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", 0);
    QTRY_VERIFY(item);
    QTRY_COMPARE(listview->contentY(), -100.0);

    QDeclarative1Text *name = findItem<QDeclarative1Text>(contentItem, "textName", 0);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(0));
    QDeclarative1Text *number = findItem<QDeclarative1Text>(contentItem, "textNumber", 0);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(0));

    // Check currentIndex is updated when contentItem moves
    listview->setContentY(20);

    QTRY_COMPARE(listview->currentIndex(), 6);

    // change model
    TestModel model2;
    for (int i = 0; i < 5; i++)
        model2.addItem("Item" + QString::number(i), "");

    ctxt->setContextProperty("testModel", &model2);
    QCOMPARE(listview->count(), 5);

    delete canvas;
}

void tst_QDeclarative1ListView::spacing()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    listview->setSpacing(10);
    QTRY_VERIFY(listview->spacing() == 10);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*30);
    }

    listview->setSpacing(0);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.0);
    }

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::sections()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i/5));

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listview-sections.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20 + ((i+4)/5) * 20));
        QDeclarative1Text *next = findItem<QDeclarative1Text>(item, "nextSection");
        QCOMPARE(next->text().toInt(), (i+1)/5);
    }

    QSignalSpy currentSectionChangedSpy(listview, SIGNAL(currentSectionChanged()));

    // Remove section boundary
    model.removeItem(5);

    // New section header created
    QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", 5);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    model.insertItem(3, "New Item", "0");

    // Section header moved
    item = findItem<QDeclarativeItem>(contentItem, "wrapper", 5);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 20.0);

    item = findItem<QDeclarativeItem>(contentItem, "wrapper", 6);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    // insert item which will become a section header
    model.insertItem(6, "Replace header", "1");

    item = findItem<QDeclarativeItem>(contentItem, "wrapper", 6);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    item = findItem<QDeclarativeItem>(contentItem, "wrapper", 7);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 20.0);

    QTRY_COMPARE(listview->currentSection(), QString("0"));

    listview->setContentY(140);
    QTRY_COMPARE(listview->currentSection(), QString("1"));

    QTRY_COMPARE(currentSectionChangedSpy.count(), 1);

    listview->setContentY(20);
    QTRY_COMPARE(listview->currentSection(), QString("0"));

    QTRY_COMPARE(currentSectionChangedSpy.count(), 2);

    item = findItem<QDeclarativeItem>(contentItem, "wrapper", 1);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 20.0);

    // check that headers change when item changes
    listview->setContentY(0);
    model.modifyItem(0, "changed", "2");

    item = findItem<QDeclarativeItem>(contentItem, "wrapper", 1);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    delete canvas;
}

void tst_QDeclarative1ListView::sectionsDelegate()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i/5));

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listview-sections_delegate.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20 + ((i+5)/5) * 20));
        QDeclarative1Text *next = findItem<QDeclarative1Text>(item, "nextSection");
        QCOMPARE(next->text().toInt(), (i+1)/5);
    }

    for (int i = 0; i < 3; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "sect_" + QString::number(i));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    model.modifyItem(0, "One", "aaa");
    model.modifyItem(1, "Two", "aaa");
    model.modifyItem(2, "Three", "aaa");
    model.modifyItem(3, "Four", "aaa");
    model.modifyItem(4, "Five", "aaa");

    for (int i = 0; i < 3; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem,
                "sect_" + (i == 0 ? QString("aaa") : QString::number(i)));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    // remove section boundary
    model.removeItem(5);
    qApp->processEvents();
    for (int i = 0; i < 3; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem,
                "sect_" + (i == 0 ? QString("aaa") : QString::number(i)));
        QVERIFY(item);
    }

    // QTBUG-17606
    QList<QDeclarativeItem*> items = findItems<QDeclarativeItem>(contentItem, "sect_1");
    QCOMPARE(items.count(), 1);

    // QTBUG-17759
    model.modifyItem(0, "One", "aaa");
    model.modifyItem(1, "One", "aaa");
    model.modifyItem(2, "One", "aaa");
    model.modifyItem(3, "Four", "aaa");
    model.modifyItem(4, "Four", "aaa");
    model.modifyItem(5, "Four", "aaa");
    model.modifyItem(6, "Five", "aaa");
    model.modifyItem(7, "Five", "aaa");
    model.modifyItem(8, "Five", "aaa");
    model.modifyItem(9, "Two", "aaa");
    model.modifyItem(10, "Two", "aaa");
    model.modifyItem(11, "Two", "aaa");
    QTRY_COMPARE(findItems<QDeclarativeItem>(contentItem, "sect_aaa").count(), 1);
    canvas->rootObject()->setProperty("sectionProperty", "name");
    // ensure view has settled.
    QTRY_COMPARE(findItems<QDeclarativeItem>(contentItem, "sect_Four").count(), 1);
    for (int i = 0; i < 4; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem,
                "sect_" + model.name(i*3));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*4));
    }

    // QTBUG-17769
    model.removeItems(10, 20);
    // ensure view has settled.
    QTRY_COMPARE(findItems<QDeclarativeItem>(contentItem, "wrapper").count(), 10);
    // Drag view up beyond bounds
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(20,20)));
    {
        QMouseEvent mv(QEvent::MouseMove, canvas->mapFromScene(QPoint(20,0)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(canvas->viewport(), &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, canvas->mapFromScene(QPoint(20,-50)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(canvas->viewport(), &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, canvas->mapFromScene(QPoint(20,-200)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(canvas->viewport(), &mv);
    }
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(20,-200)));
    // view should settle back at 0
    QTRY_COMPARE(listview->contentY(), 0.0);

    delete canvas;
}

void tst_QDeclarative1ListView::currentIndex()
{
    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i));

    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("testWrap", QVariant(false));

    QString filename(SRCDIR "/data/listview-initCurrent.qml");
    canvas->setSource(QUrl::fromLocalFile(filename));

    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // current item should be 20th item at startup
    // and current item should be in view
    QCOMPARE(listview->currentIndex(), 20);
    QCOMPARE(listview->contentY(), 100.0);
    QCOMPARE(listview->currentItem(), findItem<QDeclarativeItem>(contentItem, "wrapper", 20));
    QCOMPARE(listview->highlightItem()->y(), listview->currentItem()->y());

    // no wrap
    listview->setCurrentIndex(0);
    QCOMPARE(listview->currentIndex(), 0);
    // confirm that the velocity is updated
    QTRY_VERIFY(listview->verticalVelocity() != 0.0);

    listview->incrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 1);
    listview->decrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 0);

    listview->decrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 0);

    // with wrap
    ctxt->setContextProperty("testWrap", QVariant(true));
    QVERIFY(listview->isWrapEnabled());

    listview->decrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), model.count()-1);

    QTRY_COMPARE(listview->contentY(), 280.0);

    listview->incrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 0);

    QTRY_COMPARE(listview->contentY(), 0.0);

    // Test keys
    canvas->show();
    qApp->setActiveWindow(canvas);
#ifdef Q_WS_X11
    // to be safe and avoid failing setFocus with window managers
    qt_x11_wait_for_window_manager(canvas);
#endif
    QTRY_VERIFY(canvas->hasFocus());
    QTRY_VERIFY(canvas->scene()->hasFocus());
    qApp->processEvents();

    QTest::keyClick(canvas, Qt::Key_Down);
    QCOMPARE(listview->currentIndex(), 1);

    QTest::keyClick(canvas, Qt::Key_Up);
    QCOMPARE(listview->currentIndex(), 0);

    // turn off auto highlight
    listview->setHighlightFollowsCurrentItem(false);
    QVERIFY(listview->highlightFollowsCurrentItem() == false);

    QVERIFY(listview->highlightItem());
    qreal hlPos = listview->highlightItem()->y();

    listview->setCurrentIndex(4);
    QTRY_COMPARE(listview->highlightItem()->y(), hlPos);

    // insert item before currentIndex
    listview->setCurrentIndex(28);
    model.insertItem(0, "Foo", "1111");
    QTRY_COMPARE(canvas->rootObject()->property("current").toInt(), 29);

    // check removing highlight by setting currentIndex to -1;
    listview->setCurrentIndex(-1);

    QCOMPARE(listview->currentIndex(), -1);
    QVERIFY(!listview->highlightItem());
    QVERIFY(!listview->currentItem());

    delete canvas;
}

void tst_QDeclarative1ListView::noCurrentIndex()
{
    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i));

    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    QString filename(SRCDIR "/data/listview-noCurrent.qml");
    canvas->setSource(QUrl::fromLocalFile(filename));

    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // current index should be -1 at startup
    // and we should not have a currentItem or highlightItem
    QCOMPARE(listview->currentIndex(), -1);
    QCOMPARE(listview->contentY(), 0.0);
    QVERIFY(!listview->highlightItem());
    QVERIFY(!listview->currentItem());

    listview->setCurrentIndex(2);
    QCOMPARE(listview->currentIndex(), 2);
    QVERIFY(listview->highlightItem());
    QVERIFY(listview->currentItem());

    delete canvas;
}

void tst_QDeclarative1ListView::itemList()
{
    QDeclarativeView *canvas = createView();

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/itemlist.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "view");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QDeclarative1VisualItemModel *model = canvas->rootObject()->findChild<QDeclarative1VisualItemModel*>("itemModel");
    QTRY_VERIFY(model != 0);

    QTRY_VERIFY(model->count() == 3);
    QTRY_COMPARE(listview->currentIndex(), 0);

    QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "item1");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), 0.0);
    QCOMPARE(item->height(), listview->height());

    QDeclarative1Text *text = findItem<QDeclarative1Text>(contentItem, "text1");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 0"));

    listview->setCurrentIndex(2);

    item = findItem<QDeclarativeItem>(contentItem, "item3");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), 480.0);

    text = findItem<QDeclarative1Text>(contentItem, "text3");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 2"));

    delete canvas;
}

void tst_QDeclarative1ListView::cacheBuffer()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_VERIFY(listview->delegate() != 0);
    QTRY_VERIFY(listview->model() != 0);
    QTRY_VERIFY(listview->highlight() != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    testObject->setCacheBuffer(400);
    QTRY_VERIFY(listview->cacheBuffer() == 400);

    int newItemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    QTRY_VERIFY(newItemCount > itemCount);

    // Confirm items positioned correctly
    for (int i = 0; i < model.count() && i < newItemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::positionViewAtIndex()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 40; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position on a currently visible item
    listview->positionViewAtIndex(3, QDeclarative1ListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 60.);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 3; i < model.count() && i < itemCount-3-1; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position on an item beyond the visible items
    listview->positionViewAtIndex(22, QDeclarative1ListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 440.);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 22; i < model.count() && i < itemCount-22-1; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position on an item that would leave empty space if positioned at the top
    listview->positionViewAtIndex(28, QDeclarative1ListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 480.);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 24; i < model.count() && i < itemCount-24-1; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position at the beginning again
    listview->positionViewAtIndex(0, QDeclarative1ListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 0.);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount-1; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position at End using last index
    listview->positionViewAtIndex(model.count()-1, QDeclarative1ListView::End);
    QTRY_COMPARE(listview->contentY(), 480.);

    // Confirm items positioned correctly
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 24; i < model.count(); ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position at End
    listview->positionViewAtIndex(20, QDeclarative1ListView::End);
    QTRY_COMPARE(listview->contentY(), 100.);

    // Position in Center
    listview->positionViewAtIndex(15, QDeclarative1ListView::Center);
    QTRY_COMPARE(listview->contentY(), 150.);

    // Ensure at least partially visible
    listview->positionViewAtIndex(15, QDeclarative1ListView::Visible);
    QTRY_COMPARE(listview->contentY(), 150.);

    listview->setContentY(302);
    listview->positionViewAtIndex(15, QDeclarative1ListView::Visible);
    QTRY_COMPARE(listview->contentY(), 302.);

    listview->setContentY(320);
    listview->positionViewAtIndex(15, QDeclarative1ListView::Visible);
    QTRY_COMPARE(listview->contentY(), 300.);

    listview->setContentY(85);
    listview->positionViewAtIndex(20, QDeclarative1ListView::Visible);
    QTRY_COMPARE(listview->contentY(), 85.);

    listview->setContentY(75);
    listview->positionViewAtIndex(20, QDeclarative1ListView::Visible);
    QTRY_COMPARE(listview->contentY(), 100.);

    // Ensure completely visible
    listview->setContentY(120);
    listview->positionViewAtIndex(20, QDeclarative1ListView::Contain);
    QTRY_COMPARE(listview->contentY(), 120.);

    listview->setContentY(302);
    listview->positionViewAtIndex(15, QDeclarative1ListView::Contain);
    QTRY_COMPARE(listview->contentY(), 300.);

    listview->setContentY(85);
    listview->positionViewAtIndex(20, QDeclarative1ListView::Contain);
    QTRY_COMPARE(listview->contentY(), 100.);

    // positionAtBeginnging
    listview->positionViewAtBeginning();
    QTRY_COMPARE(listview->contentY(), 0.);

    listview->setContentY(80);
    canvas->rootObject()->setProperty("showHeader", true);
    listview->positionViewAtBeginning();
    QTRY_COMPARE(listview->contentY(), -30.);

    // positionAtEnd
    listview->positionViewAtEnd();
    QTRY_COMPARE(listview->contentY(), 480.); // 40*20 - 320

    listview->setContentY(80);
    canvas->rootObject()->setProperty("showFooter", true);
    listview->positionViewAtEnd();
    QTRY_COMPARE(listview->contentY(), 510.);

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::resetModel()
{
    QDeclarativeView *canvas = createView();

    QStringList strings;
    strings << "one" << "two" << "three";
    QStringListModel model(strings);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/displaylist.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(listview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QDeclarative1Text *display = findItem<QDeclarative1Text>(contentItem, "displayText", i);
        QTRY_VERIFY(display != 0);
        QTRY_COMPARE(display->text(), strings.at(i));
    }

    strings.clear();
    strings << "four" << "five" << "six" << "seven";
    model.setStringList(strings);

    QTRY_COMPARE(listview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QDeclarative1Text *display = findItem<QDeclarative1Text>(contentItem, "displayText", i);
        QTRY_VERIFY(display != 0);
        QTRY_COMPARE(display->text(), strings.at(i));
    }

    delete canvas;
}

void tst_QDeclarative1ListView::propertyChanges()
{
    QDeclarativeView *canvas = createView();
    QTRY_VERIFY(canvas);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/propertychangestest.qml"));

    QDeclarative1ListView *listView = canvas->rootObject()->findChild<QDeclarative1ListView*>("listView");
    QTRY_VERIFY(listView);

    QSignalSpy highlightFollowsCurrentItemSpy(listView, SIGNAL(highlightFollowsCurrentItemChanged()));
    QSignalSpy preferredHighlightBeginSpy(listView, SIGNAL(preferredHighlightBeginChanged()));
    QSignalSpy preferredHighlightEndSpy(listView, SIGNAL(preferredHighlightEndChanged()));
    QSignalSpy highlightRangeModeSpy(listView, SIGNAL(highlightRangeModeChanged()));
    QSignalSpy keyNavigationWrapsSpy(listView, SIGNAL(keyNavigationWrapsChanged()));
    QSignalSpy cacheBufferSpy(listView, SIGNAL(cacheBufferChanged()));
    QSignalSpy snapModeSpy(listView, SIGNAL(snapModeChanged()));

    QTRY_COMPARE(listView->highlightFollowsCurrentItem(), true);
    QTRY_COMPARE(listView->preferredHighlightBegin(), 0.0);
    QTRY_COMPARE(listView->preferredHighlightEnd(), 0.0);
    QTRY_COMPARE(listView->highlightRangeMode(), QDeclarative1ListView::ApplyRange);
    QTRY_COMPARE(listView->isWrapEnabled(), true);
    QTRY_COMPARE(listView->cacheBuffer(), 10);
    QTRY_COMPARE(listView->snapMode(), QDeclarative1ListView::SnapToItem);

    listView->setHighlightFollowsCurrentItem(false);
    listView->setPreferredHighlightBegin(1.0);
    listView->setPreferredHighlightEnd(1.0);
    listView->setHighlightRangeMode(QDeclarative1ListView::StrictlyEnforceRange);
    listView->setWrapEnabled(false);
    listView->setCacheBuffer(3);
    listView->setSnapMode(QDeclarative1ListView::SnapOneItem);

    QTRY_COMPARE(listView->highlightFollowsCurrentItem(), false);
    QTRY_COMPARE(listView->preferredHighlightBegin(), 1.0);
    QTRY_COMPARE(listView->preferredHighlightEnd(), 1.0);
    QTRY_COMPARE(listView->highlightRangeMode(), QDeclarative1ListView::StrictlyEnforceRange);
    QTRY_COMPARE(listView->isWrapEnabled(), false);
    QTRY_COMPARE(listView->cacheBuffer(), 3);
    QTRY_COMPARE(listView->snapMode(), QDeclarative1ListView::SnapOneItem);

    QTRY_COMPARE(highlightFollowsCurrentItemSpy.count(),1);
    QTRY_COMPARE(preferredHighlightBeginSpy.count(),1);
    QTRY_COMPARE(preferredHighlightEndSpy.count(),1);
    QTRY_COMPARE(highlightRangeModeSpy.count(),1);
    QTRY_COMPARE(keyNavigationWrapsSpy.count(),1);
    QTRY_COMPARE(cacheBufferSpy.count(),1);
    QTRY_COMPARE(snapModeSpy.count(),1);

    listView->setHighlightFollowsCurrentItem(false);
    listView->setPreferredHighlightBegin(1.0);
    listView->setPreferredHighlightEnd(1.0);
    listView->setHighlightRangeMode(QDeclarative1ListView::StrictlyEnforceRange);
    listView->setWrapEnabled(false);
    listView->setCacheBuffer(3);
    listView->setSnapMode(QDeclarative1ListView::SnapOneItem);

    QTRY_COMPARE(highlightFollowsCurrentItemSpy.count(),1);
    QTRY_COMPARE(preferredHighlightBeginSpy.count(),1);
    QTRY_COMPARE(preferredHighlightEndSpy.count(),1);
    QTRY_COMPARE(highlightRangeModeSpy.count(),1);
    QTRY_COMPARE(keyNavigationWrapsSpy.count(),1);
    QTRY_COMPARE(cacheBufferSpy.count(),1);
    QTRY_COMPARE(snapModeSpy.count(),1);

    delete canvas;
}

void tst_QDeclarative1ListView::componentChanges()
{
    QDeclarativeView *canvas = createView();
    QTRY_VERIFY(canvas);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/propertychangestest.qml"));

    QDeclarative1ListView *listView = canvas->rootObject()->findChild<QDeclarative1ListView*>("listView");
    QTRY_VERIFY(listView);

    QDeclarativeComponent component(canvas->engine());
    component.setData("import QtQuick 1.0; Rectangle { color: \"blue\"; }", QUrl::fromLocalFile(""));

    QDeclarativeComponent delegateComponent(canvas->engine());
    delegateComponent.setData("import QtQuick 1.0; Text { text: '<b>Name:</b> ' + name }", QUrl::fromLocalFile(""));

    QSignalSpy highlightSpy(listView, SIGNAL(highlightChanged()));
    QSignalSpy delegateSpy(listView, SIGNAL(delegateChanged()));
    QSignalSpy headerSpy(listView, SIGNAL(headerChanged()));
    QSignalSpy footerSpy(listView, SIGNAL(footerChanged()));

    listView->setHighlight(&component);
    listView->setHeader(&component);
    listView->setFooter(&component);
    listView->setDelegate(&delegateComponent);

    QTRY_COMPARE(listView->highlight(), &component);
    QTRY_COMPARE(listView->header(), &component);
    QTRY_COMPARE(listView->footer(), &component);
    QTRY_COMPARE(listView->delegate(), &delegateComponent);

    QTRY_COMPARE(highlightSpy.count(),1);
    QTRY_COMPARE(delegateSpy.count(),1);
    QTRY_COMPARE(headerSpy.count(),1);
    QTRY_COMPARE(footerSpy.count(),1);

    listView->setHighlight(&component);
    listView->setHeader(&component);
    listView->setFooter(&component);
    listView->setDelegate(&delegateComponent);

    QTRY_COMPARE(highlightSpy.count(),1);
    QTRY_COMPARE(delegateSpy.count(),1);
    QTRY_COMPARE(headerSpy.count(),1);
    QTRY_COMPARE(footerSpy.count(),1);

    delete canvas;
}

void tst_QDeclarative1ListView::modelChanges()
{
    QDeclarativeView *canvas = createView();
    QTRY_VERIFY(canvas);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/propertychangestest.qml"));

    QDeclarative1ListView *listView = canvas->rootObject()->findChild<QDeclarative1ListView*>("listView");
    QTRY_VERIFY(listView);

    QObject *alternateModel = canvas->rootObject()->findChild<QObject*>("alternateModel");
    QTRY_VERIFY(alternateModel);
    QVariant modelVariant = QVariant::fromValue(alternateModel);
    QSignalSpy modelSpy(listView, SIGNAL(modelChanged()));

    listView->setModel(modelVariant);
    QTRY_COMPARE(listView->model(), modelVariant);
    QTRY_COMPARE(modelSpy.count(),1);

    listView->setModel(modelVariant);
    QTRY_COMPARE(modelSpy.count(),1);

    listView->setModel(QVariant());
    QTRY_COMPARE(modelSpy.count(),2);

    delete canvas;
}

void tst_QDeclarative1ListView::QTBUG_9791()
{
    QDeclarativeView *canvas = createView();

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/strictlyenforcerange.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = qobject_cast<QDeclarative1ListView*>(canvas->rootObject());
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_VERIFY(listview->delegate() != 0);
    QTRY_VERIFY(listview->model() != 0);

    QMetaObject::invokeMethod(listview, "fillModel");
    qApp->processEvents();

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    QCOMPARE(itemCount, 3);

    for (int i = 0; i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->x(), i*300.0);
    }

    // check that view is positioned correctly
    QTRY_COMPARE(listview->contentX(), 590.0);

    delete canvas;
}

void tst_QDeclarative1ListView::manualHighlight()
{
    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);

    QString filename(SRCDIR "/data/manual-highlight.qml");
    canvas->setSource(QUrl::fromLocalFile(filename));

    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(listview->currentIndex(), 0);
    QTRY_COMPARE(listview->currentItem(), findItem<QDeclarativeItem>(contentItem, "wrapper", 0));
    QTRY_COMPARE(listview->highlightItem()->y() - 5, listview->currentItem()->y());

    listview->setCurrentIndex(2);

    QTRY_COMPARE(listview->currentIndex(), 2);
    QTRY_COMPARE(listview->currentItem(), findItem<QDeclarativeItem>(contentItem, "wrapper", 2));
    QTRY_COMPARE(listview->highlightItem()->y() - 5, listview->currentItem()->y());

    // QTBUG-15972
    listview->positionViewAtIndex(3, QDeclarative1ListView::Contain);

    QTRY_COMPARE(listview->currentIndex(), 2);
    QTRY_COMPARE(listview->currentItem(), findItem<QDeclarativeItem>(contentItem, "wrapper", 2));
    QTRY_COMPARE(listview->highlightItem()->y() - 5, listview->currentItem()->y());

    delete canvas;
}

void tst_QDeclarative1ListView::QTBUG_11105()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    listview->positionViewAtIndex(20, QDeclarative1ListView::Beginning);
    QCOMPARE(listview->contentY(), 280.);

    TestModel model2;
    for (int i = 0; i < 5; i++)
        model2.addItem("Item" + QString::number(i), "");

    ctxt->setContextProperty("testModel", &model2);

    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    QCOMPARE(itemCount, 5);

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::header()
{
    {
        QDeclarativeView *canvas = createView();

        TestModel model;
        for (int i = 0; i < 30; i++)
            model.addItem("Item" + QString::number(i), "");

        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/header.qml"));
        qApp->processEvents();

        QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
        QTRY_VERIFY(listview != 0);

        QDeclarativeItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QDeclarative1Text *header = findItem<QDeclarative1Text>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->y(), 0.0);
        QCOMPARE(header->height(), 20.0);

        QCOMPARE(listview->contentY(), 0.0);

        model.clear();
        QTRY_COMPARE(header->y(), 0.0);

        for (int i = 0; i < 30; i++)
            model.addItem("Item" + QString::number(i), "");

        QMetaObject::invokeMethod(canvas->rootObject(), "changeHeader");

        header = findItem<QDeclarative1Text>(contentItem, "header");
        QVERIFY(!header);
        header = findItem<QDeclarative1Text>(contentItem, "header2");
        QVERIFY(header);

        QCOMPARE(header->y(), 10.0);
        QCOMPARE(header->height(), 10.0);
        QCOMPARE(listview->contentY(), 10.0);

        delete canvas;
    }
    {
        QDeclarativeView *canvas = createView();

        TestModel model;

        canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/header1.qml"));
        qApp->processEvents();

        QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
        QTRY_VERIFY(listview != 0);

        QDeclarativeItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QDeclarative1Text *header = findItem<QDeclarative1Text>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->y(), 0.0);

        QCOMPARE(listview->contentY(), 0.0);

        model.clear();
        QTRY_COMPARE(header->y(), 0.0);

        delete canvas;
    }
    {
        // QTBUG-19844
        QDeclarativeView *canvas = createView();

        TestModel model;

        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/header.qml"));
        qApp->processEvents();

        QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
        QTRY_VERIFY(listview != 0);

        QDeclarativeItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QDeclarative1Text *header = findItem<QDeclarative1Text>(contentItem, "header");
        QVERIFY(header);

        header->setHeight(500);

        model.addItem("Item 0", "");

        header->setHeight(40);
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", 0);
        QVERIFY(item);
        QTRY_VERIFY(header->y() + header->height() == item->y());

        delete canvas;
    }
}

void tst_QDeclarative1ListView::footer()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 3; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/footer.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QDeclarative1Text *footer = findItem<QDeclarative1Text>(contentItem, "footer");
    QVERIFY(footer);
    QCOMPARE(footer->y(), 60.0);
    QCOMPARE(footer->height(), 30.0);

    model.removeItem(1);
    QTRY_COMPARE(footer->y(), 40.0);

    model.clear();
    QTRY_COMPARE(footer->y(), 0.0);

    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QMetaObject::invokeMethod(canvas->rootObject(), "changeFooter");

    footer = findItem<QDeclarative1Text>(contentItem, "footer");
    QVERIFY(!footer);
    footer = findItem<QDeclarative1Text>(contentItem, "footer2");
    QVERIFY(footer);

    QCOMPARE(footer->y(), 600.0);
    QCOMPARE(footer->height(), 20.0);
    QCOMPARE(listview->contentY(), 0.0);

    delete canvas;
}

class LVAccessor : public QDeclarative1ListView
{
public:
    qreal minY() const { return minYExtent(); }
    qreal maxY() const { return maxYExtent(); }
    qreal minX() const { return minXExtent(); }
    qreal maxX() const { return maxXExtent(); }
};

void tst_QDeclarative1ListView::headerFooter()
{
    {
        // Vertical
        QDeclarativeView *canvas = createView();

        TestModel model;
        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/headerfooter.qml"));
        qApp->processEvents();

        QDeclarative1ListView *listview = qobject_cast<QDeclarative1ListView*>(canvas->rootObject());
        QTRY_VERIFY(listview != 0);

        QDeclarativeItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QDeclarativeItem *header = findItem<QDeclarativeItem>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->y(), 0.0);

        QDeclarativeItem *footer = findItem<QDeclarativeItem>(contentItem, "footer");
        QVERIFY(footer);
        QCOMPARE(footer->y(), 20.0);

        QVERIFY(static_cast<LVAccessor*>(listview)->minY() == 0);
        QVERIFY(static_cast<LVAccessor*>(listview)->maxY() == 0);

        delete canvas;
    }
    {
        // Horizontal
        QDeclarativeView *canvas = createView();

        TestModel model;
        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/headerfooter.qml"));
        canvas->rootObject()->setProperty("horizontal", true);
        qApp->processEvents();

        QDeclarative1ListView *listview = qobject_cast<QDeclarative1ListView*>(canvas->rootObject());
        QTRY_VERIFY(listview != 0);

        QDeclarativeItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QDeclarativeItem *header = findItem<QDeclarativeItem>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->x(), 0.0);

        QDeclarativeItem *footer = findItem<QDeclarativeItem>(contentItem, "footer");
        QVERIFY(footer);
        QCOMPARE(footer->x(), 20.0);

        QVERIFY(static_cast<LVAccessor*>(listview)->minX() == 0);
        QVERIFY(static_cast<LVAccessor*>(listview)->maxX() == 0);

        delete canvas;
    }
    {
        // Horizontal RTL
        QDeclarativeView *canvas = createView();

        TestModel model;
        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/headerfooter.qml"));
        canvas->rootObject()->setProperty("horizontal", true);
        canvas->rootObject()->setProperty("rtl", true);
        qApp->processEvents();

        QDeclarative1ListView *listview = qobject_cast<QDeclarative1ListView*>(canvas->rootObject());
        QTRY_VERIFY(listview != 0);

        QDeclarativeItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QDeclarativeItem *header = findItem<QDeclarativeItem>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->x(), -20.0);

        QDeclarativeItem *footer = findItem<QDeclarativeItem>(contentItem, "footer");
        QVERIFY(footer);
        QCOMPARE(footer->x(), -50.0);

        QCOMPARE(static_cast<LVAccessor*>(listview)->minX(), 240.);
        QCOMPARE(static_cast<LVAccessor*>(listview)->maxX(), 240.);

        delete canvas;
    }
}

void tst_QDeclarative1ListView::resizeView()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 40; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    QVariant heightRatio;
    QMetaObject::invokeMethod(canvas->rootObject(), "heightRatio", Q_RETURN_ARG(QVariant, heightRatio));
    QCOMPARE(heightRatio.toReal(), 0.4);

    listview->setHeight(200);

    QMetaObject::invokeMethod(canvas->rootObject(), "heightRatio", Q_RETURN_ARG(QVariant, heightRatio));
    QCOMPARE(heightRatio.toReal(), 0.25);

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::sizeLessThan1()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/sizelessthan1.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*0.5);
    }

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::QTBUG_14821()
{
    QDeclarativeView *canvas = createView();

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/qtbug14821.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = qobject_cast<QDeclarative1ListView*>(canvas->rootObject());
    QVERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    listview->decrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 99);

    listview->incrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 0);

    delete canvas;
}

void tst_QDeclarative1ListView::resizeDelegate()
{
    QDeclarativeView *canvas = createView();

    QStringList strings;
    for (int i = 0; i < 30; ++i)
        strings << QString::number(i);
    QStringListModel model(strings);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/displaylist.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(listview->count(), model.rowCount());

    listview->setCurrentIndex(25);
    listview->setContentY(0);

    for (int i = 0; i < 16; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QCOMPARE(item->y(), i*20.0);
    }

    QCOMPARE(listview->currentItem()->y(), 500.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 500.0);

    canvas->rootObject()->setProperty("delegateHeight", 30);
    qApp->processEvents();

    for (int i = 0; i < 11; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QTRY_COMPARE(item->y(), i*30.0);
    }

    QTRY_COMPARE(listview->currentItem()->y(), 750.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 750.0);

    listview->setCurrentIndex(1);
    listview->positionViewAtIndex(25, QDeclarative1ListView::Beginning);
    listview->positionViewAtIndex(5, QDeclarative1ListView::Beginning);

    for (int i = 5; i < 16; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QCOMPARE(item->y(), i*30.0);
    }

    QTRY_COMPARE(listview->currentItem()->y(), 30.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 30.0);

    canvas->rootObject()->setProperty("delegateHeight", 20);
    qApp->processEvents();

    for (int i = 5; i < 11; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QTRY_COMPARE(item->y(), 150 + (i-5)*20.0);
    }

    QTRY_COMPARE(listview->currentItem()->y(), 70.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 70.0);

    delete canvas;
}

void tst_QDeclarative1ListView::QTBUG_16037()
{
    QDeclarativeView *canvas = createView();

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/qtbug16037.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "listview");
    QTRY_VERIFY(listview != 0);

    QVERIFY(listview->contentHeight() <= 0.0);

    QMetaObject::invokeMethod(canvas->rootObject(), "setModel");

    QTRY_COMPARE(listview->contentHeight(), 80.0);

    delete canvas;
}

void tst_QDeclarative1ListView::indexAt()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QCOMPARE(listview->indexAt(0,0), 0);
    QCOMPARE(listview->indexAt(0,19), 0);
    QCOMPARE(listview->indexAt(239,19), 0);
    QCOMPARE(listview->indexAt(0,20), 1);
    QCOMPARE(listview->indexAt(240,20), -1);

    delete canvas;
    delete testObject;
}

void tst_QDeclarative1ListView::incrementalModel()
{
    QDeclarativeView *canvas = createView();

    IncrementalModel model;
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/displaylist.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(listview->count(), 20);

    listview->positionViewAtIndex(10, QDeclarative1ListView::Beginning);

    QTRY_COMPARE(listview->count(), 25);

    delete canvas;
}

void tst_QDeclarative1ListView::onAdd()
{
    QFETCH(int, initialItemCount);
    QFETCH(int, itemsToAdd);

    const int delegateHeight = 10;
    TestModel2 model;

    // these initial items should not trigger ListView.onAdd
    for (int i=0; i<initialItemCount; i++)
        model.addItem("dummy value", "dummy value");

    QDeclarativeView *canvas = createView();
    canvas->setFixedSize(200, delegateHeight * (initialItemCount + itemsToAdd));
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("delegateHeight", delegateHeight);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/attachedSignals.qml"));

    QObject *object = canvas->rootObject();
    object->setProperty("width", canvas->width());
    object->setProperty("height", canvas->height());
    qApp->processEvents();

    QList<QPair<QString, QString> > items;
    for (int i=0; i<itemsToAdd; i++)
        items << qMakePair(QString("value %1").arg(i), QString::number(i));
    model.addItems(items);

    qApp->processEvents();

    QVariantList result = object->property("addedDelegates").toList();
    QCOMPARE(result.count(), items.count());
    for (int i=0; i<items.count(); i++)
        QCOMPARE(result[i].toString(), items[i].first);

    delete canvas;
}

void tst_QDeclarative1ListView::onAdd_data()
{
    QTest::addColumn<int>("initialItemCount");
    QTest::addColumn<int>("itemsToAdd");

    QTest::newRow("0, add 1") << 0 << 1;
    QTest::newRow("0, add 2") << 0 << 2;
    QTest::newRow("0, add 10") << 0 << 10;

    QTest::newRow("1, add 1") << 1 << 1;
    QTest::newRow("1, add 2") << 1 << 2;
    QTest::newRow("1, add 10") << 1 << 10;

    QTest::newRow("5, add 1") << 5 << 1;
    QTest::newRow("5, add 2") << 5 << 2;
    QTest::newRow("5, add 10") << 5 << 10;
}

void tst_QDeclarative1ListView::onRemove()
{
    QFETCH(int, initialItemCount);
    QFETCH(int, indexToRemove);
    QFETCH(int, removeCount);

    const int delegateHeight = 10;
    TestModel2 model;
    for (int i=0; i<initialItemCount; i++)
        model.addItem(QString("value %1").arg(i), "dummy value");

    QDeclarativeView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("delegateHeight", delegateHeight);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/attachedSignals.qml"));
    QObject *object = canvas->rootObject();

    qApp->processEvents();

    model.removeItems(indexToRemove, removeCount);
    qApp->processEvents();
    QCOMPARE(object->property("removedDelegateCount"), QVariant(removeCount));

    delete canvas;
}

void tst_QDeclarative1ListView::onRemove_data()
{
    QTest::addColumn<int>("initialItemCount");
    QTest::addColumn<int>("indexToRemove");
    QTest::addColumn<int>("removeCount");

    QTest::newRow("remove first") << 1 << 0 << 1;
    QTest::newRow("two items, remove first") << 2 << 0 << 1;
    QTest::newRow("two items, remove last") << 2 << 1 << 1;
    QTest::newRow("two items, remove all") << 2 << 0 << 2;

    QTest::newRow("four items, remove first") << 4 << 0 << 1;
    QTest::newRow("four items, remove 0-2") << 4 << 0 << 2;
    QTest::newRow("four items, remove 1-3") << 4 << 1 << 2;
    QTest::newRow("four items, remove 2-4") << 4 << 2 << 2;
    QTest::newRow("four items, remove last") << 4 << 3 << 1;
    QTest::newRow("four items, remove all") << 4 << 0 << 4;

    QTest::newRow("ten items, remove 1-8") << 10 << 0 << 8;
    QTest::newRow("ten items, remove 2-7") << 10 << 2 << 5;
    QTest::newRow("ten items, remove 4-10") << 10 << 4 << 6;
}

void tst_QDeclarative1ListView::testQtQuick11Attributes()
{
    QFETCH(QString, code);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QObject *obj;

    QDeclarativeComponent valid(&engine);
    valid.setData("import QtQuick 1.1; ListView { " + code.toUtf8() + " }", QUrl(""));
    obj = valid.create();
    QVERIFY(obj);
    QVERIFY(valid.errorString().isEmpty());
    delete obj;

    QDeclarativeComponent invalid(&engine);
    invalid.setData("import QtQuick 1.0; ListView { " + code.toUtf8() + " }", QUrl(""));
    QTest::ignoreMessage(QtWarningMsg, warning.toUtf8());
    obj = invalid.create();
    QCOMPARE(invalid.errorString(), error);
    delete obj;
}

void tst_QDeclarative1ListView::testQtQuick11Attributes_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("positionViewAtBeginning") << "Component.onCompleted: positionViewAtBeginning()"
        << "<Unknown File>:1: ReferenceError: Can't find variable: positionViewAtBeginning"
        << "";

    QTest::newRow("positionViewAtEnd") << "Component.onCompleted: positionViewAtEnd()"
        << "<Unknown File>:1: ReferenceError: Can't find variable: positionViewAtEnd"
        << "";
}

void tst_QDeclarative1ListView::rightToLeft()
{
    QDeclarativeView *canvas = createView();
    canvas->setFixedSize(640,320);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/rightToLeft.qml"));
    qApp->processEvents();

    QVERIFY(canvas->rootObject() != 0);
    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "view");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QDeclarative1VisualItemModel *model = canvas->rootObject()->findChild<QDeclarative1VisualItemModel*>("itemModel");
    QTRY_VERIFY(model != 0);

    QTRY_VERIFY(model->count() == 3);
    QTRY_COMPARE(listview->currentIndex(), 0);

    // initial position at first item, right edge aligned
    QCOMPARE(listview->contentX(), -640.);

    QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "item1");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), -100.0);
    QCOMPARE(item->height(), listview->height());

    QDeclarative1Text *text = findItem<QDeclarative1Text>(contentItem, "text1");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 0"));

    listview->setCurrentIndex(2);

    item = findItem<QDeclarativeItem>(contentItem, "item3");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), -540.0);

    text = findItem<QDeclarative1Text>(contentItem, "text3");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 2"));

    QCOMPARE(listview->contentX(), -640.);

    // Ensure resizing maintains position relative to right edge
    qobject_cast<QDeclarativeItem*>(canvas->rootObject())->setWidth(600);
    QTRY_COMPARE(listview->contentX(), -600.);

    delete canvas;
}

void tst_QDeclarative1ListView::test_mirroring()
{
    QDeclarativeView *canvasA = createView();
    canvasA->setSource(QUrl::fromLocalFile(SRCDIR "/data/rightToLeft.qml"));
    QDeclarative1ListView *listviewA = findItem<QDeclarative1ListView>(canvasA->rootObject(), "view");
    QTRY_VERIFY(listviewA != 0);

    QDeclarativeView *canvasB = createView();
    canvasB->setSource(QUrl::fromLocalFile(SRCDIR "/data/rightToLeft.qml"));
    QDeclarative1ListView *listviewB = findItem<QDeclarative1ListView>(canvasB->rootObject(), "view");
    QTRY_VERIFY(listviewA != 0);
    qApp->processEvents();

    QList<QString> objectNames;
    objectNames << "item1" << "item2"; // << "item3"

    listviewA->setProperty("layoutDirection", Qt::LeftToRight);
    listviewB->setProperty("layoutDirection", Qt::RightToLeft);
    QCOMPARE(listviewA->layoutDirection(), listviewA->effectiveLayoutDirection());

    // LTR != RTL
    foreach(const QString objectName, objectNames)
        QVERIFY(findItem<QDeclarativeItem>(listviewA, objectName)->x() != findItem<QDeclarativeItem>(listviewB, objectName)->x());

    listviewA->setProperty("layoutDirection", Qt::LeftToRight);
    listviewB->setProperty("layoutDirection", Qt::LeftToRight);

    // LTR == LTR
    foreach(const QString objectName, objectNames)
        QCOMPARE(findItem<QDeclarativeItem>(listviewA, objectName)->x(), findItem<QDeclarativeItem>(listviewB, objectName)->x());

    QVERIFY(listviewB->layoutDirection() == listviewB->effectiveLayoutDirection());
    QDeclarativeItemPrivate::get(listviewB)->setLayoutMirror(true);
    QVERIFY(listviewB->layoutDirection() != listviewB->effectiveLayoutDirection());

    // LTR != LTR+mirror
    foreach(const QString objectName, objectNames)
        QVERIFY(findItem<QDeclarativeItem>(listviewA, objectName)->x() != findItem<QDeclarativeItem>(listviewB, objectName)->x());

    listviewA->setProperty("layoutDirection", Qt::RightToLeft);

    // RTL == LTR+mirror
    foreach(const QString objectName, objectNames)
        QCOMPARE(findItem<QDeclarativeItem>(listviewA, objectName)->x(), findItem<QDeclarativeItem>(listviewB, objectName)->x());

    listviewB->setProperty("layoutDirection", Qt::RightToLeft);

    // RTL != RTL+mirror
    foreach(const QString objectName, objectNames)
        QVERIFY(findItem<QDeclarativeItem>(listviewA, objectName)->x() != findItem<QDeclarativeItem>(listviewB, objectName)->x());

    listviewA->setProperty("layoutDirection", Qt::LeftToRight);

    // LTR == RTL+mirror
    foreach(const QString objectName, objectNames)
        QCOMPARE(findItem<QDeclarativeItem>(listviewA, objectName)->x(), findItem<QDeclarativeItem>(listviewB, objectName)->x());

    delete canvasA;
    delete canvasB;
}

void tst_QDeclarative1ListView::orientationChange()
{
    QDeclarativeView *canvas = createView();

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/orientchange.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = qobject_cast<QDeclarative1ListView*>(canvas->rootObject());
    QVERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    listview->positionViewAtIndex(50, QDeclarative1ListView::Beginning);

    // Confirm items positioned correctly
    for (int i = 50; i < 54; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item);
        QCOMPARE(item->y(), i*80.0);
    }

    listview->setOrientation(QDeclarative1ListView::Horizontal);
    QCOMPARE(listview->contentY(), 0.);

    // Confirm items positioned correctly
    for (int i = 0; i < 3; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item);
        QCOMPARE(item->x(), i*80.0);
    }

    listview->positionViewAtIndex(50, QDeclarative1ListView::Beginning);
    listview->setOrientation(QDeclarative1ListView::Vertical);
    QCOMPARE(listview->contentX(), 0.);
    //
    // Confirm items positioned correctly
    for (int i = 0; i < 4; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        QVERIFY(item);
        QCOMPARE(item->y(), i*80.0);
    }

    delete canvas;
}

void tst_QDeclarative1ListView::contentPosJump()
{
    QDeclarativeView *canvas = createView();

    TestModel model;
    for (int i = 0; i < 50; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/listviewtest.qml"));
    qApp->processEvents();

    QDeclarative1ListView *listview = findItem<QDeclarative1ListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QDeclarativeItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    // Test jumping more than a page of items.
    listview->setContentY(500);
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    for (int i = 25; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    listview->setContentY(-100);
    itemCount = findItems<QDeclarativeItem>(contentItem, "wrapper").count();
    QVERIFY(itemCount < 20);
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QDeclarativeItem *item = findItem<QDeclarativeItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    delete canvas;
}

void tst_QDeclarative1ListView::qListModelInterface_items()
{
    items<TestModel>();
}

void tst_QDeclarative1ListView::qAbstractItemModel_items()
{
    items<TestModel2>();
}

void tst_QDeclarative1ListView::qListModelInterface_changed()
{
    changed<TestModel>();
}

void tst_QDeclarative1ListView::qAbstractItemModel_changed()
{
    changed<TestModel2>();
}

void tst_QDeclarative1ListView::qListModelInterface_inserted()
{
    inserted<TestModel>();
}

void tst_QDeclarative1ListView::qAbstractItemModel_inserted()
{
    inserted<TestModel2>();
}

void tst_QDeclarative1ListView::qListModelInterface_removed()
{
    removed<TestModel>(false);
    removed<TestModel>(true);
}

void tst_QDeclarative1ListView::qAbstractItemModel_removed()
{
    removed<TestModel2>(false);
    removed<TestModel2>(true);
}

void tst_QDeclarative1ListView::qListModelInterface_moved()
{
    moved<TestModel>();
}

void tst_QDeclarative1ListView::qAbstractItemModel_moved()
{
    moved<TestModel2>();
}

void tst_QDeclarative1ListView::qListModelInterface_clear()
{
    clear<TestModel>();
}

void tst_QDeclarative1ListView::qAbstractItemModel_clear()
{
    clear<TestModel2>();
}

QDeclarativeView *tst_QDeclarative1ListView::createView()
{
    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);

    return canvas;
}

/*
   Find an item with the specified objectName.  If index is supplied then the
   item must also evaluate the {index} expression equal to index
*/
template<typename T>
T *tst_QDeclarative1ListView::findItem(QGraphicsObject *parent, const QString &objectName, int index)
{
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->childItems().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(parent->childItems().at(i));
        if(!item)
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
QList<T*> tst_QDeclarative1ListView::findItems(QGraphicsObject *parent, const QString &objectName)
{
    QList<T*> items;
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->childItems().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(parent->childItems().at(i));
        if(!item || !item->isVisible())
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
            items.append(static_cast<T*>(item));
        items += findItems<T>(item, objectName);
    }

    return items;
}

void tst_QDeclarative1ListView::dumpTree(QDeclarativeItem *parent, int depth)
{
    static QString padding("                       ");
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(parent->childItems().at(i));
        if(!item)
            continue;
        qDebug() << padding.left(depth*2) << item;
        dumpTree(item, depth+1);
    }
}


QTEST_MAIN(tst_QDeclarative1ListView)

#include "tst_qdeclarativelistview.moc"
