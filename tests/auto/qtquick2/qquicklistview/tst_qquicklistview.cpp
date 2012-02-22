/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QStringListModel>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeincubator.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickvisualitemmodel_p.h>
#include <QtDeclarative/private/qdeclarativelistmodel_p.h>
#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"
#include "incrementalmodel.h"
#include <math.h>

Q_DECLARE_METATYPE(Qt::LayoutDirection)
Q_DECLARE_METATYPE(QQuickListView::Orientation)

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

class tst_QQuickListView : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_QQuickListView();

private slots:
    // Test both QListModelInterface and QAbstractItemModel model types
    void qListModelInterface_items();
    void qListModelInterface_package_items();
    void qAbstractItemModel_items();

    void qListModelInterface_changed();
    void qListModelInterface_package_changed();
    void qAbstractItemModel_changed();

    void qListModelInterface_inserted();
    void qListModelInterface_inserted_more();
    void qListModelInterface_inserted_more_data();
    void qListModelInterface_package_inserted();
    void qAbstractItemModel_inserted();
    void qAbstractItemModel_inserted_more();
    void qAbstractItemModel_inserted_more_data();

    void qListModelInterface_removed();
    void qListModelInterface_removed_more();
    void qListModelInterface_removed_more_data();
    void qListModelInterface_package_removed();
    void qAbstractItemModel_removed();
    void qAbstractItemModel_removed_more();
    void qAbstractItemModel_removed_more_data();

    void qListModelInterface_moved();
    void qListModelInterface_moved_data();
    void qListModelInterface_package_moved();
    void qListModelInterface_package_moved_data();
    void qAbstractItemModel_moved();
    void qAbstractItemModel_moved_data();

    void multipleChanges();
    void multipleChanges_data();

    void qListModelInterface_clear();
    void qListModelInterface_package_clear();
    void qAbstractItemModel_clear();

    void insertBeforeVisible();
    void insertBeforeVisible_data();
    void swapWithFirstItem();
    void itemList();
    void currentIndex_delayedItemCreation();
    void currentIndex_delayedItemCreation_data();
    void currentIndex();
    void noCurrentIndex();
    void enforceRange();
    void enforceRange_withoutHighlight();
    void spacing();
    void qListModelInterface_sections();
    void qListModelInterface_package_sections();
    void qAbstractItemModel_sections();
    void sectionsPositioning();
    void sectionsDelegate();
    void sectionPropertyChange();
    void cacheBuffer();
    void positionViewAtIndex();
    void resetModel();
    void propertyChanges();
    void componentChanges();
    void modelChanges();
    void manualHighlight();
    void header();
    void header_data();
    void header_delayItemCreation();
    void footer();
    void footer_data();
    void headerFooter();
    void resizeView();
    void resizeViewAndRepaint();
    void sizeLessThan1();
    void QTBUG_14821();
    void resizeDelegate();
    void resizeFirstDelegate();
    void QTBUG_16037();
    void indexAt_itemAt_data();
    void indexAt_itemAt();
    void incrementalModel();
    void onAdd();
    void onAdd_data();
    void onRemove();
    void onRemove_data();
    void rightToLeft();
    void test_mirroring();
    void margins();
    void marginsResize();
    void marginsResize_data();
    void creationContext();
    void snapToItem_data();
    void snapToItem();
    void snapOneItem_data();
    void snapOneItem();

    void QTBUG_9791();
    void QTBUG_11105();
    void QTBUG_21742();

    void asynchronous();
    void unrequestedVisibility();

    void populateTransitions();
    void populateTransitions_data();
    void addTransitions();
    void addTransitions_data();
    void moveTransitions();
    void moveTransitions_data();
    void removeTransitions();
    void removeTransitions_data();
    void displacedTransitions();
    void displacedTransitions_data();
    void multipleTransitions();
    void multipleTransitions_data();

private:
    template <class T> void items(const QUrl &source, bool forceLayout);
    template <class T> void changed(const QUrl &source, bool forceLayout);
    template <class T> void inserted(const QUrl &source);
    template <class T> void inserted_more();
    template <class T> void removed(const QUrl &source, bool animated);
    template <class T> void removed_more(const QUrl &source);
    template <class T> void moved(const QUrl &source);
    template <class T> void clear(const QUrl &source);
    template <class T> void sections(const QUrl &source);

    QList<int> toIntList(const QVariantList &list);
    void matchIndexLists(const QVariantList &indexLists, const QList<int> &expectedIndexes);
    void matchItemsAndIndexes(const QVariantMap &items, const QaimModel &model, const QList<int> &expectedIndexes);
    void matchItemLists(const QVariantList &itemLists, const QList<QQuickItem *> &expectedItems);

    void inserted_more_data();
    void removed_more_data();
    void moved_data();
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

tst_QQuickListView::tst_QQuickListView()
{
}

template <class T>
void tst_QQuickListView::items(const QUrl &source, bool forceLayout)
{
    QQuickView *canvas = createView();

    T model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QTRY_VERIFY(testObject->error() == false);

    QTRY_VERIFY(listview->highlightItem() != 0);
    QTRY_COMPARE(listview->count(), model.count());
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QTRY_COMPARE(contentItem->childItems().count(), model.count()+1); // assumes all are visible, +1 for the (default) highlight item

    // current item should be first item
    QTRY_COMPARE(listview->currentItem(), findItem<QQuickItem>(contentItem, "wrapper", 0));

    for (int i = 0; i < model.count(); ++i) {
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QTRY_VERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        QQuickText *number = findItem<QQuickText>(contentItem, "textNumber", i);
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

    // Force a layout, necessary if ListView is completed before VisualDataModel.
    if (forceLayout)
        QCOMPARE(listview->property("count").toInt(), 0);

    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    QTRY_VERIFY(itemCount == 0);

    QTRY_COMPARE(listview->highlightResizeSpeed(), 1000.0);
    QTRY_COMPARE(listview->highlightMoveSpeed(), 1000.0);

    delete canvas;
    delete testObject;
}


template <class T>
void tst_QQuickListView::changed(const QUrl &source, bool forceLayout)
{
    QQuickView *canvas = createView();

    T model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    qApp->processEvents();

    QQuickFlickable *listview = findItem<QQuickFlickable>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // Force a layout, necessary if ListView is completed before VisualDataModel.
    if (forceLayout)
        QCOMPARE(listview->property("count").toInt(), model.count());

    model.modifyItem(1, "Will", "9876");
    QQuickText *name = findItem<QQuickText>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QQuickText *number = findItem<QQuickText>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    delete canvas;
    delete testObject;
}

template <class T>
void tst_QQuickListView::inserted(const QUrl &source)
{
    QQuickView *canvas = createView();
    canvas->show();

    T model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    model.insertItem(1, "Will", "9876");

    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QTRY_COMPARE(contentItem->childItems().count(), model.count()+1); // assumes all are visible, +1 for the (default) highlight item

    QQuickText *name = findItem<QQuickText>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QQuickText *number = findItem<QQuickText>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    // Confirm items positioned correctly
    for (int i = 0; i < model.count(); ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_COMPARE(item->y(), i*20.0);
    }

    model.insertItem(0, "Foo", "1111"); // zero index, and current item

    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QTRY_COMPARE(contentItem->childItems().count(), model.count()+1); // assumes all are visible, +1 for the (default) highlight item

    name = findItem<QQuickText>(contentItem, "textName", 0);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(0));
    number = findItem<QQuickText>(contentItem, "textNumber", 0);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(0));

    QTRY_COMPARE(listview->currentIndex(), 1);

    // Confirm items positioned correctly
    for (int i = 0; i < model.count(); ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
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
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.0 - 20.0);
    }

//    QTRY_COMPARE(listview->contentItemHeight(), model.count() * 20.0);

    // QTBUG-19675
    model.clear();
    model.insertItem(0, "Hello", "1234");
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->y(), 0.);
    QTRY_VERIFY(listview->contentY() == 0);

    delete canvas;
    delete testObject;
}

template <class T>
void tst_QQuickListView::inserted_more()
{
    QFETCH(qreal, contentY);
    QFETCH(int, insertIndex);
    QFETCH(int, insertCount);
    QFETCH(qreal, itemsOffsetAfterMove);

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    listview->setContentY(contentY);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QList<QPair<QString, QString> > newData;
    for (int i=0; i<insertCount; i++)
        newData << qMakePair(QString("value %1").arg(i), QString::number(i));
    model.insertItems(insertIndex, newData);
    QTRY_COMPARE(listview->property("count").toInt(), model.count());

    // check visibleItems.first() is in correct position
    QQuickItem *item0 = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item0);
    QCOMPARE(item0->y(), itemsOffsetAfterMove);

    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    int firstVisibleIndex = -1;
    for (int i=0; i<items.count(); i++) {
        if (items[i]->y() >= contentY) {
            QDeclarativeExpression e(qmlContext(items[i]), items[i], "index");
            firstVisibleIndex = e.evaluate().toInt();
            break;
        }
    }
    QVERIFY2(firstVisibleIndex >= 0, QTest::toString(firstVisibleIndex));

    // Confirm items positioned correctly and indexes correct
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    QQuickText *name;
    QQuickText *number;
    for (int i = firstVisibleIndex; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->y(), i*20.0 + itemsOffsetAfterMove);
        name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        number = findItem<QQuickText>(contentItem, "textNumber", i);
        QVERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::inserted_more_data()
{
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<int>("insertIndex");
    QTest::addColumn<int>("insertCount");
    QTest::addColumn<qreal>("itemsOffsetAfterMove");

    QTest::newRow("add 1, before visible items")
            << 80.0     // show 4-19
            << 3 << 1
            << -20.0;   // insert above first visible i.e. 0 is at -20, first visible should not move

    QTest::newRow("add multiple, before visible")
            << 80.0     // show 4-19
            << 3 << 3
            << -20.0 * 3;   // again first visible should not move

    QTest::newRow("add 1, at start of visible, content at start")
            << 0.0
            << 0 << 1
            << 0.0;

    QTest::newRow("add multiple, start of visible, content at start")
            << 0.0
            << 0 << 3
            << 0.0;

    QTest::newRow("add 1, at start of visible, content not at start")
            << 80.0     // show 4-19
            << 4 << 1
            << 0.0;

    QTest::newRow("add multiple, at start of visible, content not at start")
            << 80.0     // show 4-19
            << 4 << 3
            << 0.0;


    QTest::newRow("add 1, at end of visible, content at start")
            << 0.0
            << 15 << 1
            << 0.0;

    QTest::newRow("add 1, at end of visible, content at start")
            << 0.0
            << 15 << 3
            << 0.0;

    QTest::newRow("add 1, at end of visible, content not at start")
            << 80.0     // show 4-19
            << 19 << 1
            << 0.0;

    QTest::newRow("add multiple, at end of visible, content not at start")
            << 80.0     // show 4-19
            << 19 << 3
            << 0.0;


    QTest::newRow("add 1, after visible, content at start")
            << 0.0
            << 16 << 1
            << 0.0;

    QTest::newRow("add 1, after visible, content at start")
            << 0.0
            << 16 << 3
            << 0.0;

    QTest::newRow("add 1, after visible, content not at start")
            << 80.0     // show 4-19
            << 20 << 1
            << 0.0;

    QTest::newRow("add multiple, after visible, content not at start")
            << 80.0     // show 4-19
            << 20 << 3
            << 0.0;
}

void tst_QQuickListView::insertBeforeVisible()
{
    QFETCH(int, insertIndex);
    QFETCH(int, insertCount);
    QFETCH(int, cacheBuffer);

    QQuickText *name;
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    listview->setCacheBuffer(cacheBuffer);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // trigger a refill (not just setting contentY) so that the visibleItems grid is updated
    int firstVisibleIndex = 20;     // move to an index where the top item is not visible
    listview->setContentY(firstVisibleIndex * 20.0);
    listview->setCurrentIndex(firstVisibleIndex);

    qApp->processEvents();
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    QTRY_COMPARE(listview->currentIndex(), firstVisibleIndex);
    QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", firstVisibleIndex);
    QVERIFY(item);
    QCOMPARE(item->y(), listview->contentY());

    QList<QPair<QString, QString> > newData;
    for (int i=0; i<insertCount; i++)
        newData << qMakePair(QString("value %1").arg(i), QString::number(i));
    model.insertItems(insertIndex, newData);
    QTRY_COMPARE(listview->property("count").toInt(), model.count());

    // now, moving to the top of the view should position the inserted items correctly
    int itemsOffsetAfterMove = -(insertCount * 20);
    listview->setCurrentIndex(0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    QTRY_COMPARE(listview->currentIndex(), 0);
    QTRY_COMPARE(listview->contentY(), 0.0 + itemsOffsetAfterMove);

    // Confirm items positioned correctly and indexes correct
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->y(), i*20.0 + itemsOffsetAfterMove);
        name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::insertBeforeVisible_data()
{
    QTest::addColumn<int>("insertIndex");
    QTest::addColumn<int>("insertCount");
    QTest::addColumn<int>("cacheBuffer");

    QTest::newRow("insert 1 at 0, 0 buffer") << 0 << 1 << 0;
    QTest::newRow("insert 1 at 0, 100 buffer") << 0 << 1 << 100;
    QTest::newRow("insert 1 at 0, 500 buffer") << 0 << 1 << 500;

    QTest::newRow("insert 1 at 1, 0 buffer") << 1 << 1 << 0;
    QTest::newRow("insert 1 at 1, 100 buffer") << 1 << 1 << 100;
    QTest::newRow("insert 1 at 1, 500 buffer") << 1 << 1 << 500;

    QTest::newRow("insert multiple at 0, 0 buffer") << 0 << 3 << 0;
    QTest::newRow("insert multiple at 0, 100 buffer") << 0 << 3 << 100;
    QTest::newRow("insert multiple at 0, 500 buffer") << 0 << 3 << 500;

    QTest::newRow("insert multiple at 1, 0 buffer") << 1 << 3 << 0;
    QTest::newRow("insert multiple at 1, 100 buffer") << 1 << 3 << 100;
    QTest::newRow("insert multiple at 1, 500 buffer") << 1 << 3 << 500;
}

template <class T>
void tst_QQuickListView::removed(const QUrl &source, bool /* animated */)
{
    QQuickView *canvas = createView();

    T model;
    for (int i = 0; i < 50; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    canvas->show();
    qApp->processEvents();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    model.removeItem(1);
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    QQuickText *name = findItem<QQuickText>(contentItem, "textName", 1);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(1));
    QQuickText *number = findItem<QQuickText>(contentItem, "textNumber", 1);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(1));

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    // Remove first item (which is the current item);
    model.removeItem(0);
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    name = findItem<QQuickText>(contentItem, "textName", 0);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(0));
    number = findItem<QQuickText>(contentItem, "textNumber", 0);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(0));

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),i*20.0);
    }

    // Remove items not visible
    model.removeItem(18);
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),i*20.0);
    }

    // Remove items before visible
    listview->setContentY(80);
    listview->setCurrentIndex(10);

    model.removeItem(1); // post: top item will be at 20
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    // Confirm items positioned correctly
    for (int i = 2; i < 18; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),20+i*20.0);
    }

    // Remove current index
    QTRY_VERIFY(listview->currentIndex() == 9);
    QQuickItem *oldCurrent = listview->currentItem();
    model.removeItem(9);

    QTRY_COMPARE(listview->currentIndex(), 9);
    QTRY_VERIFY(listview->currentItem() != oldCurrent);

    listview->setContentY(20); // That's the top now
    // let transitions settle.
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    QTest::qWait(300);

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),20+i*20.0);
    }

    // remove current item beyond visible items.
    listview->setCurrentIndex(20);
    listview->setContentY(40);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    model.removeItem(20);
    QTRY_COMPARE(listview->currentIndex(), 20);
    QTRY_VERIFY(listview->currentItem() != 0);

    // remove item before current, but visible
    listview->setCurrentIndex(8);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    oldCurrent = listview->currentItem();
    model.removeItem(6);

    QTRY_COMPARE(listview->currentIndex(), 7);
    QTRY_VERIFY(listview->currentItem() == oldCurrent);

    listview->setContentY(80);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    QTest::qWait(300);

    // remove all visible items
    model.removeItems(1, 18);
    QTRY_COMPARE(listview->count() , model.count());

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount-1; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i+1);
        if (!item) qWarning() << "Item" << i+1 << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(),80+i*20.0);
    }

    model.removeItems(1, 17);
    QTRY_COMPARE(listview->count() , model.count());

    model.removeItems(2, 1);
    QTRY_COMPARE(listview->count() , model.count());

    model.addItem("New", "1");
    QTRY_COMPARE(listview->count() , model.count());

    QTRY_VERIFY(name = findItem<QQuickText>(contentItem, "textName", model.count()-1));
    QCOMPARE(name->text(), QString("New"));

    // Add some more items so that we don't run out
    model.clear();
    for (int i = 0; i < 50; i++)
        model.addItem("Item" + QString::number(i), "");

    // QTBUG-QTBUG-20575
    listview->setCurrentIndex(0);
    listview->setContentY(30);
    model.removeItem(0);
    QTRY_VERIFY(name = findItem<QQuickText>(contentItem, "textName", 0));

    // QTBUG-19198 move to end and remove all visible items one at a time.
    listview->positionViewAtEnd();
    for (int i = 0; i < 18; ++i)
        model.removeItems(model.count() - 1, 1);
    QTRY_VERIFY(findItems<QQuickItem>(contentItem, "wrapper").count() > 16);

    delete canvas;
    delete testObject;
}

template <class T>
void tst_QQuickListView::removed_more(const QUrl &source)
{
    QFETCH(qreal, contentY);
    QFETCH(int, removeIndex);
    QFETCH(int, removeCount);
    QFETCH(qreal, itemsOffsetAfterMove);

    QQuickText *name;
    QQuickText *number;
    QQuickView *canvas = createView();

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    canvas->show();
    qApp->processEvents();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    listview->setContentY(contentY);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // wait for refill (after refill, items above the firstVisibleIndex-1 should not be rendered)
    int firstVisibleIndex = contentY / 20;
    if (firstVisibleIndex - 2 >= 0)
        QTRY_VERIFY(!findItem<QQuickText>(contentItem, "textName", firstVisibleIndex - 2));

    model.removeItems(removeIndex, removeCount);
    QTRY_COMPARE(listview->property("count").toInt(), model.count());

    // check visibleItems.first() is in correct position
    QQuickItem *item0 = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item0);
    QCOMPARE(item0->y(), itemsOffsetAfterMove);

    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    for (int i=0; i<items.count(); i++) {
        if (items[i]->y() >= contentY) {
            QDeclarativeExpression e(qmlContext(items[i]), items[i], "index");
            firstVisibleIndex = e.evaluate().toInt();
            break;
        }
    }
    QVERIFY2(firstVisibleIndex >= 0, QTest::toString(firstVisibleIndex));

    // Confirm items positioned correctly and indexes correct
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = firstVisibleIndex; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->y(), i*20.0 + itemsOffsetAfterMove);
        name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        number = findItem<QQuickText>(contentItem, "textNumber", i);
        QVERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::removed_more_data()
{
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<int>("removeIndex");
    QTest::addColumn<int>("removeCount");
    QTest::addColumn<qreal>("itemsOffsetAfterMove");

    QTest::newRow("remove 1, before visible items")
            << 80.0     // show 4-19
            << 3 << 1
            << 20.0;   // visible items slide down by 1 item so that first visible does not move

    QTest::newRow("remove multiple, all before visible items")
            << 80.0
            << 1 << 3
            << 20.0 * 3;

    QTest::newRow("remove multiple, all before visible items, remove item 0")
            << 80.0
            << 0 << 4
            << 20.0 * 4;

    // remove 1,2,3 before the visible pos, 0 moves down to just before the visible pos,
    // items 4,5 are removed from view, item 6 slides up to original pos of item 4 (80px)
    QTest::newRow("remove multiple, mix of items from before and within visible items")
            << 80.0
            << 1 << 5
            << 20.0 * 3;    // adjust for the 3 items removed before the visible

    QTest::newRow("remove multiple, mix of items from before and within visible items, remove item 0")
            << 80.0
            << 0 << 6
            << 20.0 * 4;    // adjust for the 3 items removed before the visible


    QTest::newRow("remove 1, from start of visible, content at start")
            << 0.0
            << 0 << 1
            << 0.0;

    QTest::newRow("remove multiple, from start of visible, content at start")
            << 0.0
            << 0 << 3
            << 0.0;

    QTest::newRow("remove 1, from start of visible, content not at start")
            << 80.0     // show 4-19
            << 4 << 1
            << 0.0;

    QTest::newRow("remove multiple, from start of visible, content not at start")
            << 80.0     // show 4-19
            << 4 << 3
            << 0.0;


    QTest::newRow("remove 1, from middle of visible, content at start")
            << 0.0
            << 10 << 1
            << 0.0;

    QTest::newRow("remove multiple, from middle of visible, content at start")
            << 0.0
            << 10 << 5
            << 0.0;

    QTest::newRow("remove 1, from middle of visible, content not at start")
            << 80.0     // show 4-19
            << 10 << 1
            << 0.0;

    QTest::newRow("remove multiple, from middle of visible, content not at start")
            << 80.0     // show 4-19
            << 10 << 5
            << 0.0;


    QTest::newRow("remove 1, after visible, content at start")
            << 0.0
            << 16 << 1
            << 0.0;

    QTest::newRow("remove multiple, after visible, content at start")
            << 0.0
            << 16 << 5
            << 0.0;

    QTest::newRow("remove 1, after visible, content not at middle")
            << 80.0     // show 4-19
            << 16+4 << 1
            << 0.0;

    QTest::newRow("remove multiple, after visible, content not at start")
            << 80.0     // show 4-19
            << 16+4 << 5
            << 0.0;

    QTest::newRow("remove multiple, mix of items from within and after visible items")
            << 80.0
            << 18 << 5
            << 0.0;
}

template <class T>
void tst_QQuickListView::clear(const QUrl &source)
{
    QQuickView *canvas = createView();

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

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
void tst_QQuickListView::moved(const QUrl &source)
{
    QFETCH(qreal, contentY);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(int, count);
    QFETCH(qreal, itemsOffsetAfterMove);

    QQuickText *name;
    QQuickText *number;
    QQuickView *canvas = createView();

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(source);
    canvas->show();
    qApp->processEvents();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *currentItem = listview->currentItem();
    QTRY_VERIFY(currentItem != 0);

    if (contentY != 0) {
        listview->setContentY(contentY);
        QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    }

    model.moveItems(from, to, count);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    int firstVisibleIndex = -1;
    for (int i=0; i<items.count(); i++) {
        if (items[i]->y() >= contentY) {
            QDeclarativeExpression e(qmlContext(items[i]), items[i], "index");
            firstVisibleIndex = e.evaluate().toInt();
            break;
        }
    }
    QVERIFY2(firstVisibleIndex >= 0, QTest::toString(firstVisibleIndex));

    // Confirm items positioned correctly and indexes correct
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = firstVisibleIndex; i < model.count() && i < itemCount; ++i) {
        if (i >= firstVisibleIndex + 16)    // index has moved out of view
            continue;
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->y(), i*20.0 + itemsOffsetAfterMove);
        name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        number = findItem<QQuickText>(contentItem, "textNumber", i);
        QVERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));

        // current index should have been updated
        if (item == currentItem)
            QTRY_COMPARE(listview->currentIndex(), i);
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::moved_data()
{
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("itemsOffsetAfterMove");

    // model starts with 30 items, each 20px high, in area 320px high
    // 16 items should be visible at a time
    // itemsOffsetAfterMove should be > 0 whenever items above the visible pos have moved

    QTest::newRow("move 1 forwards, within visible items")
            << 0.0
            << 1 << 4 << 1
            << 0.0;

    QTest::newRow("move 1 forwards, from non-visible -> visible")
            << 80.0     // show 4-19
            << 1 << 18 << 1
            << 20.0;    // removed 1 item above the first visible, so item 0 should drop down by 1 to minimize movement

    QTest::newRow("move 1 forwards, from non-visible -> visible (move first item)")
            << 80.0     // show 4-19
            << 0 << 4 << 1
            << 20.0;    // first item has moved to below item4, everything drops down by size of 1 item

    QTest::newRow("move 1 forwards, from visible -> non-visible")
            << 0.0
            << 1 << 16 << 1
            << 0.0;

    QTest::newRow("move 1 forwards, from visible -> non-visible (move first item)")
            << 0.0
            << 0 << 16 << 1
            << 0.0;


    QTest::newRow("move 1 backwards, within visible items")
            << 0.0
            << 4 << 1 << 1
            << 0.0;

    QTest::newRow("move 1 backwards, within visible items (to first index)")
            << 0.0
            << 4 << 0 << 1
            << 0.0;

    QTest::newRow("move 1 backwards, from non-visible -> visible")
            << 0.0
            << 20 << 4 << 1
            << 0.0;

    QTest::newRow("move 1 backwards, from non-visible -> visible (move last item)")
            << 0.0
            << 29 << 15 << 1
            << 0.0;

    QTest::newRow("move 1 backwards, from visible -> non-visible")
            << 80.0     // show 4-19
            << 16 << 1 << 1
            << -20.0;   // to minimize movement, item 0 moves to -20, and other items do not move

    QTest::newRow("move 1 backwards, from visible -> non-visible (move first item)")
            << 80.0     // show 4-19
            << 16 << 0 << 1
            << -20.0;   // to minimize movement, item 16 (now at 0) moves to -20, and other items do not move


    QTest::newRow("move multiple forwards, within visible items")
            << 0.0
            << 0 << 5 << 3
            << 0.0;

    QTest::newRow("move multiple forwards, before visible items")
            << 140.0     // show 7-22
            << 4 << 5 << 3      // 4,5,6 move to below 7
            << 20.0 * 3;      // 4,5,6 moved down

    QTest::newRow("move multiple forwards, from non-visible -> visible")
            << 80.0     // show 4-19
            << 1 << 5 << 3
            << 20.0 * 3;    // moving 3 from above the content y should adjust y positions accordingly

    QTest::newRow("move multiple forwards, from non-visible -> visible (move first item)")
            << 80.0     // show 4-19
            << 0 << 5 << 3
            << 20.0 * 3;        // moving 3 from above the content y should adjust y positions accordingly

    QTest::newRow("move multiple forwards, mix of non-visible/visible")
            << 40.0
            << 1 << 16 << 2
            << 20.0;    // item 1,2 are removed, item 3 is now first visible

    QTest::newRow("move multiple forwards, to bottom of view")
            << 0.0
            << 5 << 13 << 3
            << 0.0;

    QTest::newRow("move multiple forwards, to bottom of view, first->last")
            << 0.0
            << 0 << 13 << 3
            << 0.0;

    QTest::newRow("move multiple forwards, to bottom of view, content y not 0")
            << 80.0
            << 5+4 << 13+4 << 3
            << 0.0;

    QTest::newRow("move multiple forwards, from visible -> non-visible")
            << 0.0
            << 1 << 16 << 3
            << 0.0;

    QTest::newRow("move multiple forwards, from visible -> non-visible (move first item)")
            << 0.0
            << 0 << 16 << 3
            << 0.0;


    QTest::newRow("move multiple backwards, within visible items")
            << 0.0
            << 4 << 1 << 3
            << 0.0;

    QTest::newRow("move multiple backwards, within visible items (move first item)")
            << 0.0
            << 10 << 0 << 3
            << 0.0;

    QTest::newRow("move multiple backwards, from non-visible -> visible")
            << 0.0
            << 20 << 4 << 3
            << 0.0;

    QTest::newRow("move multiple backwards, from non-visible -> visible (move last item)")
            << 0.0
            << 27 << 10 << 3
            << 0.0;

    QTest::newRow("move multiple backwards, from visible -> non-visible")
            << 80.0     // show 4-19
            << 16 << 1 << 3
            << -20.0 * 3;   // to minimize movement, 0 moves by -60, and other items do not move

    QTest::newRow("move multiple backwards, from visible -> non-visible (move first item)")
            << 80.0     // show 4-19
            << 16 << 0 << 3
            << -20.0 * 3;   // to minimize movement, 16,17,18 move to above item 0, and other items do not move
}

void tst_QQuickListView::multipleChanges()
{
    QFETCH(int, startCount);
    QFETCH(QList<ListChange>, changes);
    QFETCH(int, newCount);
    QFETCH(int, newCurrentIndex);

    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < startCount; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i=0; i<changes.count(); i++) {
        switch (changes[i].type) {
            case ListChange::Inserted:
            {
                QList<QPair<QString, QString> > items;
                for (int j=changes[i].index; j<changes[i].index + changes[i].count; ++j)
                    items << qMakePair(QString("new item %1").arg(j), QString::number(j));
                model.insertItems(changes[i].index, items);
                QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
                break;
            }
            case ListChange::Removed:
                model.removeItems(changes[i].index, changes[i].count);
                QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
                break;
            case ListChange::Moved:
                model.moveItems(changes[i].index, changes[i].to, changes[i].count);
                QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
                break;
            case ListChange::SetCurrent:
                listview->setCurrentIndex(changes[i].index);
                QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
                break;
            case ListChange::SetContentY:
                listview->setContentY(changes[i].pos);
                QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
                break;
        }
    }

    QTRY_COMPARE(listview->count(), newCount);
    QCOMPARE(listview->count(), model.count());
    QTRY_COMPARE(listview->currentIndex(), newCurrentIndex);

    QQuickText *name;
    QQuickText *number;
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i=0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
        number = findItem<QQuickText>(contentItem, "textNumber", i);
        QVERIFY(number != 0);
        QTRY_COMPARE(number->text(), model.number(i));
    }

    delete testObject;
    delete canvas;
}

void tst_QQuickListView::multipleChanges_data()
{
    QTest::addColumn<int>("startCount");
    QTest::addColumn<QList<ListChange> >("changes");
    QTest::addColumn<int>("newCount");
    QTest::addColumn<int>("newCurrentIndex");

    QList<ListChange> changes;

    for (int i=1; i<30; i++)
        changes << ListChange::remove(0);
    QTest::newRow("remove all but 1, first->last") << 30 << changes << 1 << 0;

    changes << ListChange::remove(0);
    QTest::newRow("remove all") << 30 << changes << 0 << -1;

    changes.clear();
    changes << ListChange::setCurrent(29);
    for (int i=29; i>0; i--)
        changes << ListChange::remove(i);
    QTest::newRow("remove last (current) -> first") << 30 << changes << 1 << 0;

    QTest::newRow("remove then insert at 0") << 10 << (QList<ListChange>()
            << ListChange::remove(0, 1)
            << ListChange::insert(0, 1)
            ) << 10 << 1;

    QTest::newRow("remove then insert at non-zero index") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(2)
            << ListChange::remove(2, 1)
            << ListChange::insert(2, 1)
            ) << 10 << 3;

    QTest::newRow("remove current then insert below it") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(1)
            << ListChange::remove(1, 3)
            << ListChange::insert(2, 2)
            ) << 9 << 1;

    QTest::newRow("remove current index then move it down") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(2)
            << ListChange::remove(1, 3)
            << ListChange::move(1, 5, 1)
            ) << 7 << 5;

    QTest::newRow("remove current index then move it up") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(5)
            << ListChange::remove(4, 3)
            << ListChange::move(4, 1, 1)
            ) << 7 << 1;


    QTest::newRow("insert multiple times") << 0 << (QList<ListChange>()
            << ListChange::insert(0, 2)
            << ListChange::insert(0, 4)
            << ListChange::insert(0, 6)
            ) << 12 << 10;

    QTest::newRow("insert multiple times with current index changes") << 0 << (QList<ListChange>()
            << ListChange::insert(0, 2)
            << ListChange::insert(0, 4)
            << ListChange::insert(0, 6)
            << ListChange::setCurrent(3)
            << ListChange::insert(3, 2)
            ) << 14 << 5;

    QTest::newRow("insert and remove all") << 0 << (QList<ListChange>()
            << ListChange::insert(0, 30)
            << ListChange::remove(0, 30)
            ) << 0 << -1;

    QTest::newRow("insert and remove current") << 30 << (QList<ListChange>()
            << ListChange::insert(1)
            << ListChange::setCurrent(1)
            << ListChange::remove(1)
            ) << 30 << 1;

    QTest::newRow("insert before 0, then remove cross section of new and old items") << 10 << (QList<ListChange>()
            << ListChange::insert(0, 10)
            << ListChange::remove(5, 10)
            ) << 10 << 5;

    QTest::newRow("insert multiple, then move new items to end") << 10 << (QList<ListChange>()
            << ListChange::insert(0, 3)
            << ListChange::move(0, 10, 3)
            ) << 13 << 0;

    QTest::newRow("insert multiple, then move new and some old items to end") << 10 << (QList<ListChange>()
            << ListChange::insert(0, 3)
            << ListChange::move(0, 8, 5)
            ) << 13 << 11;

    QTest::newRow("insert multiple at end, then move new and some old items to start") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(9)
            << ListChange::insert(10, 3)
            << ListChange::move(8, 0, 5)
            ) << 13 << 1;


    QTest::newRow("move back and forth to same index") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(1)
            << ListChange::move(1, 2, 2)
            << ListChange::move(2, 1, 2)
            ) << 10 << 1;

    QTest::newRow("move forwards then back") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(2)
            << ListChange::move(1, 2, 3)
            << ListChange::move(3, 0, 5)
            ) << 10 << 0;

    QTest::newRow("move current, then remove it") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(5)
            << ListChange::move(5, 0, 1)
            << ListChange::remove(0)
            ) << 9 << 0;

    QTest::newRow("move current, then insert before it") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(5)
            << ListChange::move(5, 0, 1)
            << ListChange::insert(0)
            ) << 11 << 1;

    QTest::newRow("move multiple, then remove them") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(1)
            << ListChange::move(5, 1, 3)
            << ListChange::remove(1, 3)
            ) << 7 << 1;

    QTest::newRow("move multiple, then insert before them") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(5)
            << ListChange::move(5, 1, 3)
            << ListChange::insert(1, 5)
            ) << 15 << 6;

    QTest::newRow("move multiple, then insert after them") << 10 << (QList<ListChange>()
            << ListChange::setCurrent(3)
            << ListChange::move(0, 1, 2)
            << ListChange::insert(3, 5)
            ) << 15 << 8;


    QTest::newRow("clear current") << 0 << (QList<ListChange>()
            << ListChange::insert(0, 5)
            << ListChange::setCurrent(-1)
            << ListChange::remove(0, 5)
            << ListChange::insert(0, 5)
            ) << 5 << -1;
}

void tst_QQuickListView::swapWithFirstItem()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // ensure content position is stable
    listview->setContentY(0);
    model.moveItem(1, 0);
    QTRY_VERIFY(listview->contentY() == 0);

    delete testObject;
    delete canvas;
}

void tst_QQuickListView::enforceRange()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("listview-enforcerange.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QTRY_COMPARE(listview->preferredHighlightBegin(), 100.0);
    QTRY_COMPARE(listview->preferredHighlightEnd(), 100.0);
    QTRY_COMPARE(listview->highlightRangeMode(), QQuickListView::StrictlyEnforceRange);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // view should be positioned at the top of the range.
    QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QTRY_VERIFY(item);
    QTRY_COMPARE(listview->contentY(), -100.0);

    QQuickText *name = findItem<QQuickText>(contentItem, "textName", 0);
    QTRY_VERIFY(name != 0);
    QTRY_COMPARE(name->text(), model.name(0));
    QQuickText *number = findItem<QQuickText>(contentItem, "textNumber", 0);
    QTRY_VERIFY(number != 0);
    QTRY_COMPARE(number->text(), model.number(0));

    // Check currentIndex is updated when contentItem moves
    listview->setContentY(20);

    QTRY_COMPARE(listview->currentIndex(), 6);

    // change model
    QmlListModel model2;
    for (int i = 0; i < 5; i++)
        model2.addItem("Item" + QString::number(i), "");

    ctxt->setContextProperty("testModel", &model2);
    QCOMPARE(listview->count(), 5);

    delete canvas;
}

void tst_QQuickListView::enforceRange_withoutHighlight()
{
    // QTBUG-20287
    // If no highlight is set but StrictlyEnforceRange is used, the content should still move
    // to the correct position (i.e. to the next/previous item, not next/previous section)
    // when moving up/down via incrementCurrentIndex() and decrementCurrentIndex()

    QQuickView *canvas = createView();

    QmlListModel model;
    model.addItem("Item 0", "a");
    model.addItem("Item 1", "b");
    model.addItem("Item 2", "b");
    model.addItem("Item 3", "c");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("listview-enforcerange-nohighlight.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    qreal expectedPos = -100.0;

    expectedPos += 10.0;    // scroll past 1st section's delegate (10px height)
    QTRY_COMPARE(listview->contentY(), expectedPos);

    expectedPos += 20 + 10;     // scroll past 1st section and section delegate of 2nd section
    QTest::keyClick(canvas, Qt::Key_Down);

    QTRY_COMPARE(listview->contentY(), expectedPos);

    expectedPos += 20;     // scroll past 1st item of 2nd section
    QTest::keyClick(canvas, Qt::Key_Down);
    QTRY_COMPARE(listview->contentY(), expectedPos);

    expectedPos += 20 + 10;     // scroll past 2nd item of 2nd section and section delegate of 3rd section
    QTest::keyClick(canvas, Qt::Key_Down);
    QTRY_COMPARE(listview->contentY(), expectedPos);

    delete canvas;
}

void tst_QQuickListView::spacing()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    listview->setSpacing(10);
    QTRY_VERIFY(listview->spacing() == 10);

    // Confirm items positioned correctly
    QTRY_VERIFY(findItems<QQuickItem>(contentItem, "wrapper").count() == 11);
    for (int i = 0; i < 11; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*30);
    }

    listview->setSpacing(0);

    // Confirm items positioned correctly
    QTRY_VERIFY(findItems<QQuickItem>(contentItem, "wrapper").count() >= 16);
    for (int i = 0; i < 16; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.0);
    }

    delete canvas;
    delete testObject;
}

template <typename T>
void tst_QQuickListView::sections(const QUrl &source)
{
    QQuickView *canvas = createView();

    T model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i/5));

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(source);
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20 + ((i+4)/5) * 20));
        QQuickText *next = findItem<QQuickText>(item, "nextSection");
        QCOMPARE(next->text().toInt(), (i+1)/5);
    }

    QSignalSpy currentSectionChangedSpy(listview, SIGNAL(currentSectionChanged()));

    // Remove section boundary
    model.removeItem(5);
    QTRY_COMPARE(listview->count(), model.count());

    // New section header created
    QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", 5);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    model.insertItem(3, "New Item", "0");
    QTRY_COMPARE(listview->count(), model.count());

    // Section header moved
    item = findItem<QQuickItem>(contentItem, "wrapper", 5);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 20.0);

    item = findItem<QQuickItem>(contentItem, "wrapper", 6);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    // insert item which will become a section header
    model.insertItem(6, "Replace header", "1");
    QTRY_COMPARE(listview->count(), model.count());

    item = findItem<QQuickItem>(contentItem, "wrapper", 6);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    item = findItem<QQuickItem>(contentItem, "wrapper", 7);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 20.0);

    QTRY_COMPARE(listview->currentSection(), QString("0"));

    listview->setContentY(140);
    QTRY_COMPARE(listview->currentSection(), QString("1"));

    QTRY_COMPARE(currentSectionChangedSpy.count(), 1);

    listview->setContentY(20);
    QTRY_COMPARE(listview->currentSection(), QString("0"));

    QTRY_COMPARE(currentSectionChangedSpy.count(), 2);

    item = findItem<QQuickItem>(contentItem, "wrapper", 1);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 20.0);

    // check that headers change when item changes
    listview->setContentY(0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    model.modifyItem(0, "changed", "2");
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    item = findItem<QQuickItem>(contentItem, "wrapper", 1);
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->height(), 40.0);

    delete canvas;
}

void tst_QQuickListView::sectionsDelegate()
{
    QSKIP("QTBUG-24395");

    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i/5));

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("listview-sections_delegate.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20 + ((i+5)/5) * 20));
        QQuickText *next = findItem<QQuickText>(item, "nextSection");
        QCOMPARE(next->text().toInt(), (i+1)/5);
    }

    for (int i = 0; i < 3; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "sect_" + QString::number(i));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    // ensure section header is maintained in view
    listview->setCurrentIndex(20);
    QTRY_VERIFY(listview->contentY() >= 200.0);
    listview->setCurrentIndex(0);
    QTRY_COMPARE(listview->contentY(), 0.0);

    // change section
    model.modifyItem(0, "One", "aaa");
    model.modifyItem(1, "Two", "aaa");
    model.modifyItem(2, "Three", "aaa");
    model.modifyItem(3, "Four", "aaa");
    model.modifyItem(4, "Five", "aaa");
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i = 0; i < 3; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem,
                "sect_" + (i == 0 ? QString("aaa") : QString::number(i)));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    // remove section boundary
    model.removeItem(5);
    QTRY_COMPARE(listview->count(), model.count());
    for (int i = 0; i < 3; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem,
                "sect_" + (i == 0 ? QString("aaa") : QString::number(i)));
        QVERIFY(item);
    }

    // QTBUG-17606
    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "sect_1");
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
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "sect_aaa").count(), 1);
    canvas->rootObject()->setProperty("sectionProperty", "name");
    // ensure view has settled.
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "sect_Four").count(), 1);
    for (int i = 0; i < 4; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem,
                "sect_" + model.name(i*3));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*4));
    }

    // QTBUG-17769
    model.removeItems(10, 20);
    // ensure view has settled.
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "wrapper").count(), 10);
    // Drag view up beyond bounds
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(20,20));
    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(20,0), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(20,-50), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(20,-200), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(20,-200));
    // view should settle back at 0
    QTRY_COMPARE(listview->contentY(), 0.0);

    delete canvas;
}

void tst_QQuickListView::sectionsPositioning()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i/5));

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("listview-sections_delegate.qml"));
    canvas->show();
    qApp->processEvents();
    canvas->rootObject()->setProperty("sectionPositioning", QVariant(int(QQuickViewSection::InlineLabels | QQuickViewSection::CurrentLabelAtStart | QQuickViewSection::NextLabelAtEnd)));

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i = 0; i < 3; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "sect_" + QString::number(i));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    QQuickItem *topItem = findVisibleChild(contentItem, "sect_0"); // section header
    QVERIFY(topItem);
    QCOMPARE(topItem->y(), 0.);

    QQuickItem *bottomItem = findVisibleChild(contentItem, "sect_3"); // section footer
    QVERIFY(bottomItem);
    QCOMPARE(bottomItem->y(), 300.);

    // move down a little and check that section header is at top
    listview->setContentY(10);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    QCOMPARE(topItem->y(), 0.);

    // push the top header up
    listview->setContentY(110);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    topItem = findVisibleChild(contentItem, "sect_0"); // section header
    QVERIFY(topItem);
    QCOMPARE(topItem->y(), 100.);

    QQuickItem *item = findVisibleChild(contentItem, "sect_1");
    QVERIFY(item);
    QCOMPARE(item->y(), 120.);

    bottomItem = findVisibleChild(contentItem, "sect_4"); // section footer
    QVERIFY(bottomItem);
    QCOMPARE(bottomItem->y(), 410.);

    // Move past section 0
    listview->setContentY(120);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    topItem = findVisibleChild(contentItem, "sect_0"); // section header
    QVERIFY(!topItem);

    // Push section footer down
    listview->setContentY(70);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    bottomItem = findVisibleChild(contentItem, "sect_4"); // section footer
    QVERIFY(bottomItem);
    QCOMPARE(bottomItem->y(), 380.);

    // Change current section
    listview->setContentY(10);
    model.modifyItem(0, "One", "aaa");
    model.modifyItem(1, "Two", "aaa");
    model.modifyItem(2, "Three", "aaa");
    model.modifyItem(3, "Four", "aaa");
    model.modifyItem(4, "Five", "aaa");
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QTRY_COMPARE(listview->currentSection(), QString("aaa"));

    for (int i = 0; i < 3; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem,
                "sect_" + (i == 0 ? QString("aaa") : QString::number(i)));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    QTRY_VERIFY(topItem = findVisibleChild(contentItem, "sect_aaa")); // section header
    QCOMPARE(topItem->y(), 10.);

    // remove section boundary
    listview->setContentY(120);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    model.removeItem(5);
    QTRY_COMPARE(listview->count(), model.count());
    for (int i = 1; i < 3; ++i) {
        QQuickItem *item = findVisibleChild(contentItem,
                "sect_" + QString::number(i));
        QVERIFY(item);
        QTRY_COMPARE(item->y(), qreal(i*20*6));
    }

    QVERIFY(topItem = findVisibleChild(contentItem, "sect_1"));
    QTRY_COMPARE(topItem->y(), 120.);

    // Change the next section
    listview->setContentY(0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    bottomItem = findVisibleChild(contentItem, "sect_3"); // section footer
    QVERIFY(bottomItem);
    QTRY_COMPARE(bottomItem->y(), 300.);

    model.modifyItem(14, "New", "new");
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QTRY_VERIFY(bottomItem = findVisibleChild(contentItem, "sect_new")); // section footer
    QTRY_COMPARE(bottomItem->y(), 300.);

    // Turn sticky footer off
    listview->setContentY(20);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    canvas->rootObject()->setProperty("sectionPositioning", QVariant(int(QQuickViewSection::InlineLabels | QQuickViewSection::CurrentLabelAtStart)));
    QTRY_VERIFY(item = findVisibleChild(contentItem, "sect_new")); // inline label restored
    QCOMPARE(item->y(), 340.);

    // Turn sticky header off
    listview->setContentY(30);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    canvas->rootObject()->setProperty("sectionPositioning", QVariant(int(QQuickViewSection::InlineLabels)));
    QTRY_VERIFY(item = findVisibleChild(contentItem, "sect_aaa")); // inline label restored
    QCOMPARE(item->y(), 0.);

    delete canvas;
}

void tst_QQuickListView::sectionPropertyChange()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("sectionpropertychange.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    for (int i = 0; i < 2; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(25. + i*75.));
    }

    QMetaObject::invokeMethod(canvas->rootObject(), "switchGroups");
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    for (int i = 0; i < 2; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(25. + i*75.));
    }

    QMetaObject::invokeMethod(canvas->rootObject(), "switchGroups");
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    for (int i = 0; i < 2; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), qreal(25. + i*75.));
    }

    delete canvas;
}

void tst_QQuickListView::currentIndex_delayedItemCreation()
{
    QFETCH(bool, setCurrentToZero);

    QQuickView *canvas = createView();

    // test currentIndexChanged() is emitted even if currentIndex = 0 on start up
    // (since the currentItem will have changed and that shares the same index)
    canvas->rootContext()->setContextProperty("setCurrentToZero", setCurrentToZero);

    canvas->setSource(testFileUrl("fillModelOnComponentCompleted.qml"));
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QSignalSpy spy(listview, SIGNAL(currentItemChanged()));
    QCOMPARE(listview->currentIndex(), 0);
    QTRY_COMPARE(spy.count(), 1);

    delete canvas;
}

void tst_QQuickListView::currentIndex_delayedItemCreation_data()
{
    QTest::addColumn<bool>("setCurrentToZero");

    QTest::newRow("set to 0") << true;
    QTest::newRow("don't set to 0") << false;
}

void tst_QQuickListView::currentIndex()
{
    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i));

    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("testWrap", QVariant(false));

    QString filename(testFile("listview-initCurrent.qml"));
    canvas->setSource(QUrl::fromLocalFile(filename));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // current item should be 20th item at startup
    // and current item should be in view
    QCOMPARE(listview->currentIndex(), 20);
    QCOMPARE(listview->contentY(), 100.0);
    QCOMPARE(listview->currentItem(), findItem<QQuickItem>(contentItem, "wrapper", 20));
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


    // footer should become visible if it is out of view, and then current index is set to count-1
    canvas->rootObject()->setProperty("showFooter", true);
    QTRY_VERIFY(listview->footerItem());
    listview->setCurrentIndex(model.count()-2);
    QTRY_VERIFY(listview->footerItem()->y() > listview->contentY() + listview->height());
    listview->setCurrentIndex(model.count()-1);
    QTRY_COMPARE(listview->contentY() + listview->height(), (20.0 * model.count()) + listview->footerItem()->height());
    canvas->rootObject()->setProperty("showFooter", false);

    // header should become visible if it is out of view, and then current index is set to 0
    canvas->rootObject()->setProperty("showHeader", true);
    QTRY_VERIFY(listview->headerItem());
    listview->setCurrentIndex(1);
    QTRY_VERIFY(listview->headerItem()->y() + listview->headerItem()->height() < listview->contentY());
    listview->setCurrentIndex(0);
    QTRY_COMPARE(listview->contentY(), -listview->headerItem()->height());
    canvas->rootObject()->setProperty("showHeader", false);


    // Test keys
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(qGuiApp->focusWindow() == canvas);

    listview->setCurrentIndex(0);

    QTest::keyClick(canvas, Qt::Key_Down);
    QCOMPARE(listview->currentIndex(), 1);

    QTest::keyClick(canvas, Qt::Key_Up);
    QCOMPARE(listview->currentIndex(), 0);

    // hold down Key_Down
    for (int i=0; i<model.count()-1; i++) {
        QTest::simulateEvent(canvas, true, Qt::Key_Down, Qt::NoModifier, "", true);
        QTRY_COMPARE(listview->currentIndex(), i+1);
    }
    QTest::keyRelease(canvas, Qt::Key_Down);
    QTRY_COMPARE(listview->currentIndex(), model.count()-1);
    QTRY_COMPARE(listview->contentY(), 280.0);

    // hold down Key_Up
    for (int i=model.count()-1; i > 0; i--) {
        QTest::simulateEvent(canvas, true, Qt::Key_Up, Qt::NoModifier, "", true);
        QTRY_COMPARE(listview->currentIndex(), i-1);
    }
    QTest::keyRelease(canvas, Qt::Key_Up);
    QTRY_COMPARE(listview->currentIndex(), 0);
    QTRY_COMPARE(listview->contentY(), 0.0);


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

void tst_QQuickListView::noCurrentIndex()
{
    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i));

    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    QString filename(testFile("listview-noCurrent.qml"));
    canvas->setSource(QUrl::fromLocalFile(filename));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

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

void tst_QQuickListView::itemList()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("itemlist.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "view");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QQuickVisualItemModel *model = canvas->rootObject()->findChild<QQuickVisualItemModel*>("itemModel");
    QTRY_VERIFY(model != 0);

    QTRY_VERIFY(model->count() == 3);
    QTRY_COMPARE(listview->currentIndex(), 0);

    QQuickItem *item = findItem<QQuickItem>(contentItem, "item1");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), 0.0);
    QCOMPARE(item->height(), listview->height());

    QQuickText *text = findItem<QQuickText>(contentItem, "text1");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 0"));

    listview->setCurrentIndex(2);

    item = findItem<QQuickItem>(contentItem, "item3");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), 480.0);

    text = findItem<QQuickText>(contentItem, "text3");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 2"));

    delete canvas;
}

void tst_QQuickListView::cacheBuffer()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 90; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_VERIFY(listview->delegate() != 0);
    QTRY_VERIFY(listview->model() != 0);
    QTRY_VERIFY(listview->highlight() != 0);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    QDeclarativeIncubationController controller;
    canvas->engine()->setIncubationController(&controller);

    testObject->setCacheBuffer(200);
    QTRY_VERIFY(listview->cacheBuffer() == 200);

    // items will be created one at a time
    for (int i = itemCount; i < qMin(itemCount+10,model.count()); ++i) {
        QVERIFY(findItem<QQuickItem>(listview, "wrapper", i) == 0);
        QQuickItem *item = 0;
        while (!item) {
            bool b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(listview, "wrapper", i);
        }
    }

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    int newItemCount = 0;
    newItemCount = findItems<QQuickItem>(contentItem, "wrapper").count();

    // Confirm items positioned correctly
    for (int i = 0; i < model.count() && i < newItemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    // move view and confirm items in view are visible immediately and outside are created async
    listview->setContentY(300);

    for (int i = 15; i < 32; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QVERIFY(item);
        QVERIFY(item->y() == i*20);
    }

    QVERIFY(findItem<QQuickItem>(listview, "wrapper", 32) == 0);

    // ensure buffered items are created
    for (int i = 32; i < qMin(41,model.count()); ++i) {
        QQuickItem *item = 0;
        while (!item) {
            qGuiApp->processEvents(); // allow refill to happen
            bool b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(listview, "wrapper", i);
        }
    }

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::positionViewAtIndex()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 40; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);
    canvas->show();
    canvas->setSource(testFileUrl("listviewtest.qml"));
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position on a currently visible item
    listview->positionViewAtIndex(3, QQuickListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 60.);

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 3; i < model.count() && i < itemCount-3-1; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position on an item beyond the visible items
    listview->positionViewAtIndex(22, QQuickListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 440.);

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 22; i < model.count() && i < itemCount-22-1; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position on an item that would leave empty space if positioned at the top
    listview->positionViewAtIndex(28, QQuickListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 480.);

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 24; i < model.count() && i < itemCount-24-1; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position at the beginning again
    listview->positionViewAtIndex(0, QQuickListView::Beginning);
    QTRY_COMPARE(listview->contentY(), 0.);

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount-1; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position at End using last index
    listview->positionViewAtIndex(model.count()-1, QQuickListView::End);
    QTRY_COMPARE(listview->contentY(), 480.);

    // Confirm items positioned correctly
    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 24; i < model.count(); ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    // Position at End
    listview->positionViewAtIndex(20, QQuickListView::End);
    QTRY_COMPARE(listview->contentY(), 100.);

    // Position in Center
    listview->positionViewAtIndex(15, QQuickListView::Center);
    QTRY_COMPARE(listview->contentY(), 150.);

    // Ensure at least partially visible
    listview->positionViewAtIndex(15, QQuickListView::Visible);
    QTRY_COMPARE(listview->contentY(), 150.);

    listview->setContentY(302);
    listview->positionViewAtIndex(15, QQuickListView::Visible);
    QTRY_COMPARE(listview->contentY(), 302.);

    listview->setContentY(320);
    listview->positionViewAtIndex(15, QQuickListView::Visible);
    QTRY_COMPARE(listview->contentY(), 300.);

    listview->setContentY(85);
    listview->positionViewAtIndex(20, QQuickListView::Visible);
    QTRY_COMPARE(listview->contentY(), 85.);

    listview->setContentY(75);
    listview->positionViewAtIndex(20, QQuickListView::Visible);
    QTRY_COMPARE(listview->contentY(), 100.);

    // Ensure completely visible
    listview->setContentY(120);
    listview->positionViewAtIndex(20, QQuickListView::Contain);
    QTRY_COMPARE(listview->contentY(), 120.);

    listview->setContentY(302);
    listview->positionViewAtIndex(15, QQuickListView::Contain);
    QTRY_COMPARE(listview->contentY(), 300.);

    listview->setContentY(85);
    listview->positionViewAtIndex(20, QQuickListView::Contain);
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

    // set current item to outside visible view, position at beginning
    // and ensure highlight moves to current item
    listview->setCurrentIndex(1);
    listview->positionViewAtBeginning();
    QTRY_COMPARE(listview->contentY(), -30.);
    QVERIFY(listview->highlightItem());
    QCOMPARE(listview->highlightItem()->y(), 20.);

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::resetModel()
{
    QQuickView *canvas = createView();

    QStringList strings;
    strings << "one" << "two" << "three";
    QStringListModel model(strings);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("displaylist.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QTRY_COMPARE(listview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QQuickText *display = findItem<QQuickText>(contentItem, "displayText", i);
        QTRY_VERIFY(display != 0);
        QTRY_COMPARE(display->text(), strings.at(i));
    }

    strings.clear();
    strings << "four" << "five" << "six" << "seven";
    model.setStringList(strings);

    QTRY_COMPARE(listview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QQuickText *display = findItem<QQuickText>(contentItem, "displayText", i);
        QTRY_VERIFY(display != 0);
        QTRY_COMPARE(display->text(), strings.at(i));
    }

    delete canvas;
}

void tst_QQuickListView::propertyChanges()
{
    QQuickView *canvas = createView();
    QTRY_VERIFY(canvas);
    canvas->setSource(testFileUrl("propertychangestest.qml"));

    QQuickListView *listView = canvas->rootObject()->findChild<QQuickListView*>("listView");
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
    QTRY_COMPARE(listView->highlightRangeMode(), QQuickListView::ApplyRange);
    QTRY_COMPARE(listView->isWrapEnabled(), true);
    QTRY_COMPARE(listView->cacheBuffer(), 10);
    QTRY_COMPARE(listView->snapMode(), QQuickListView::SnapToItem);

    listView->setHighlightFollowsCurrentItem(false);
    listView->setPreferredHighlightBegin(1.0);
    listView->setPreferredHighlightEnd(1.0);
    listView->setHighlightRangeMode(QQuickListView::StrictlyEnforceRange);
    listView->setWrapEnabled(false);
    listView->setCacheBuffer(3);
    listView->setSnapMode(QQuickListView::SnapOneItem);

    QTRY_COMPARE(listView->highlightFollowsCurrentItem(), false);
    QTRY_COMPARE(listView->preferredHighlightBegin(), 1.0);
    QTRY_COMPARE(listView->preferredHighlightEnd(), 1.0);
    QTRY_COMPARE(listView->highlightRangeMode(), QQuickListView::StrictlyEnforceRange);
    QTRY_COMPARE(listView->isWrapEnabled(), false);
    QTRY_COMPARE(listView->cacheBuffer(), 3);
    QTRY_COMPARE(listView->snapMode(), QQuickListView::SnapOneItem);

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
    listView->setHighlightRangeMode(QQuickListView::StrictlyEnforceRange);
    listView->setWrapEnabled(false);
    listView->setCacheBuffer(3);
    listView->setSnapMode(QQuickListView::SnapOneItem);

    QTRY_COMPARE(highlightFollowsCurrentItemSpy.count(),1);
    QTRY_COMPARE(preferredHighlightBeginSpy.count(),1);
    QTRY_COMPARE(preferredHighlightEndSpy.count(),1);
    QTRY_COMPARE(highlightRangeModeSpy.count(),1);
    QTRY_COMPARE(keyNavigationWrapsSpy.count(),1);
    QTRY_COMPARE(cacheBufferSpy.count(),1);
    QTRY_COMPARE(snapModeSpy.count(),1);

    delete canvas;
}

void tst_QQuickListView::componentChanges()
{
    QQuickView *canvas = createView();
    QTRY_VERIFY(canvas);
    canvas->setSource(testFileUrl("propertychangestest.qml"));

    QQuickListView *listView = canvas->rootObject()->findChild<QQuickListView*>("listView");
    QTRY_VERIFY(listView);

    QDeclarativeComponent component(canvas->engine());
    component.setData("import QtQuick 2.0; Rectangle { color: \"blue\"; }", QUrl::fromLocalFile(""));

    QDeclarativeComponent delegateComponent(canvas->engine());
    delegateComponent.setData("import QtQuick 2.0; Text { text: '<b>Name:</b> ' + name }", QUrl::fromLocalFile(""));

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

void tst_QQuickListView::modelChanges()
{
    QQuickView *canvas = createView();
    QTRY_VERIFY(canvas);
    canvas->setSource(testFileUrl("propertychangestest.qml"));

    QQuickListView *listView = canvas->rootObject()->findChild<QQuickListView*>("listView");
    QTRY_VERIFY(listView);

    QDeclarativeListModel *alternateModel = canvas->rootObject()->findChild<QDeclarativeListModel*>("alternateModel");
    QTRY_VERIFY(alternateModel);
    QVariant modelVariant = QVariant::fromValue<QObject *>(alternateModel);
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

void tst_QQuickListView::QTBUG_9791()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("strictlyenforcerange.qml"));
    qApp->processEvents();

    QQuickListView *listview = qobject_cast<QQuickListView*>(canvas->rootObject());
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_VERIFY(listview->delegate() != 0);
    QTRY_VERIFY(listview->model() != 0);

    QMetaObject::invokeMethod(listview, "fillModel");
    qApp->processEvents();

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper", false).count();
    QCOMPARE(itemCount, 3);

    for (int i = 0; i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->x(), i*300.0);
    }

    // check that view is positioned correctly
    QTRY_COMPARE(listview->contentX(), 590.0);

    delete canvas;
}

void tst_QQuickListView::manualHighlight()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    QString filename(testFile("manual-highlight.qml"));
    canvas->setSource(QUrl::fromLocalFile(filename));

    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(listview->currentIndex(), 0);
    QTRY_COMPARE(listview->currentItem(), findItem<QQuickItem>(contentItem, "wrapper", 0));
    QTRY_COMPARE(listview->highlightItem()->y() - 5, listview->currentItem()->y());

    listview->setCurrentIndex(2);

    QTRY_COMPARE(listview->currentIndex(), 2);
    QTRY_COMPARE(listview->currentItem(), findItem<QQuickItem>(contentItem, "wrapper", 2));
    QTRY_COMPARE(listview->highlightItem()->y() - 5, listview->currentItem()->y());

    // QTBUG-15972
    listview->positionViewAtIndex(3, QQuickListView::Contain);

    QTRY_COMPARE(listview->currentIndex(), 2);
    QTRY_COMPARE(listview->currentItem(), findItem<QQuickItem>(contentItem, "wrapper", 2));
    QTRY_COMPARE(listview->highlightItem()->y() - 5, listview->currentItem()->y());

    delete canvas;
}

void tst_QQuickListView::QTBUG_11105()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_VERIFY(item->y() == i*20);
    }

    listview->positionViewAtIndex(20, QQuickListView::Beginning);
    QCOMPARE(listview->contentY(), 280.);

    QmlListModel model2;
    for (int i = 0; i < 5; i++)
        model2.addItem("Item" + QString::number(i), "");

    ctxt->setContextProperty("testModel", &model2);

    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    QCOMPARE(itemCount, 5);

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::header()
{
    QFETCH(QQuickListView::Orientation, orientation);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(QPointF, initialHeaderPos);
    QFETCH(QPointF, firstDelegatePos);
    QFETCH(QPointF, initialContentPos);
    QFETCH(QPointF, changedHeaderPos);
    QFETCH(QPointF, changedContentPos);
    QFETCH(QPointF, resizeContentPos);

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QQuickView *canvas = createView();
    canvas->rootContext()->setContextProperty("testModel", &model);
    canvas->rootContext()->setContextProperty("initialViewWidth", 240);
    canvas->rootContext()->setContextProperty("initialViewHeight", 320);
    canvas->setSource(testFileUrl("header.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    listview->setOrientation(orientation);
    listview->setLayoutDirection(layoutDirection);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QQuickText *header = 0;
    QTRY_VERIFY(header = findItem<QQuickText>(contentItem, "header"));
    QVERIFY(header == listview->headerItem());

    QCOMPARE(header->width(), 100.);
    QCOMPARE(header->height(), 30.);
    QCOMPARE(header->pos(), initialHeaderPos);
    QCOMPARE(QPointF(listview->contentX(), listview->contentY()), initialContentPos);

    QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->pos(), firstDelegatePos);

    model.clear();
    QTRY_COMPARE(listview->count(), model.count());
    QCOMPARE(header->pos(), initialHeaderPos); // header should stay where it is

    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QSignalSpy headerItemSpy(listview, SIGNAL(headerItemChanged()));
    QMetaObject::invokeMethod(canvas->rootObject(), "changeHeader");

    QCOMPARE(headerItemSpy.count(), 1);

    header = findItem<QQuickText>(contentItem, "header");
    QVERIFY(!header);
    header = findItem<QQuickText>(contentItem, "header2");
    QVERIFY(header);

    QVERIFY(header == listview->headerItem());

    QCOMPARE(header->pos(), changedHeaderPos);
    QCOMPARE(header->width(), 50.);
    QCOMPARE(header->height(), 20.);
    QTRY_COMPARE(QPointF(listview->contentX(), listview->contentY()), changedContentPos);

    item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->pos(), firstDelegatePos);

    delete canvas;


    // QTBUG-21207 header should become visible if view resizes from initial empty size

    canvas = createView();
    canvas->rootContext()->setContextProperty("testModel", &model);
    canvas->rootContext()->setContextProperty("initialViewWidth", 0.0);
    canvas->rootContext()->setContextProperty("initialViewHeight", 0.0);
    canvas->setSource(testFileUrl("header.qml"));
    canvas->show();
    qApp->processEvents();

    listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    listview->setOrientation(orientation);
    listview->setLayoutDirection(layoutDirection);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    listview->setWidth(240);
    listview->setHeight(320);
    QTRY_COMPARE(listview->headerItem()->pos(), initialHeaderPos);
    QCOMPARE(QPointF(listview->contentX(), listview->contentY()), initialContentPos);


    delete canvas;
}

void tst_QQuickListView::header_data()
{
    QTest::addColumn<QQuickListView::Orientation>("orientation");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<QPointF>("initialHeaderPos");
    QTest::addColumn<QPointF>("changedHeaderPos");
    QTest::addColumn<QPointF>("initialContentPos");
    QTest::addColumn<QPointF>("changedContentPos");
    QTest::addColumn<QPointF>("firstDelegatePos");
    QTest::addColumn<QPointF>("resizeContentPos");

    // header1 = 100 x 30
    // header2 = 50 x 20
    // delegates = 240 x 20
    // view width = 240

    // header above items, top left
    QTest::newRow("vertical, left to right") << QQuickListView::Vertical << Qt::LeftToRight
        << QPointF(0, -30)
        << QPointF(0, -20)
        << QPointF(0, -30)
        << QPointF(0, -20)
        << QPointF(0, 0)
        << QPointF(0, -10);

    // header above items, top right
    QTest::newRow("vertical, layout right to left") << QQuickListView::Vertical << Qt::RightToLeft
        << QPointF(0, -30)
        << QPointF(0, -20)
        << QPointF(0, -30)
        << QPointF(0, -20)
        << QPointF(0, 0)
        << QPointF(0, -10);

    // header to left of items
    QTest::newRow("horizontal, layout left to right") << QQuickListView::Horizontal << Qt::LeftToRight
        << QPointF(-100, 0)
        << QPointF(-50, 0)
        << QPointF(-100, 0)
        << QPointF(-50, 0)
        << QPointF(0, 0)
        << QPointF(-40, 0);

    // header to right of items
    QTest::newRow("horizontal, layout right to left") << QQuickListView::Horizontal << Qt::RightToLeft
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(-240 + 100, 0)
        << QPointF(-240 + 50, 0)
        << QPointF(-240, 0)
        << QPointF(-240 + 40, 0);
}

void tst_QQuickListView::header_delayItemCreation()
{
    QQuickView *canvas = createView();

    QmlListModel model;

    canvas->rootContext()->setContextProperty("setCurrentToZero", QVariant(false));
    canvas->setSource(testFileUrl("fillModelOnComponentCompleted.qml"));
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QQuickText *header = findItem<QQuickText>(contentItem, "header");
    QVERIFY(header);
    QCOMPARE(header->y(), -header->height());

    QCOMPARE(listview->contentY(), -header->height());

    model.clear();
    QTRY_COMPARE(header->y(), -header->height());

    delete canvas;
}

void tst_QQuickListView::footer()
{
    QFETCH(QQuickListView::Orientation, orientation);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(QPointF, initialFooterPos);
    QFETCH(QPointF, firstDelegatePos);
    QFETCH(QPointF, initialContentPos);
    QFETCH(QPointF, changedFooterPos);
    QFETCH(QPointF, changedContentPos);
    QFETCH(QPointF, resizeContentPos);

    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 3; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("footer.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    listview->setOrientation(orientation);
    listview->setLayoutDirection(layoutDirection);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QQuickText *footer = findItem<QQuickText>(contentItem, "footer");
    QVERIFY(footer);

    QVERIFY(footer == listview->footerItem());

    QCOMPARE(footer->pos(), initialFooterPos);
    QCOMPARE(footer->width(), 100.);
    QCOMPARE(footer->height(), 30.);
    QCOMPARE(QPointF(listview->contentX(), listview->contentY()), initialContentPos);

    QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->pos(), firstDelegatePos);

    // remove one item
    model.removeItem(1);

    if (orientation == QQuickListView::Vertical) {
        QTRY_COMPARE(footer->y(), initialFooterPos.y() - 20);   // delegate height = 20
    } else {
        QTRY_COMPARE(footer->x(), layoutDirection == Qt::LeftToRight ?
                initialFooterPos.x() - 40 : initialFooterPos.x() + 40);  // delegate width = 40
    }

    // remove all items
    model.clear();

    QPointF posWhenNoItems(0, 0);
    if (orientation == QQuickListView::Horizontal && layoutDirection == Qt::RightToLeft)
        posWhenNoItems.setX(-100);
    QTRY_COMPARE(footer->pos(), posWhenNoItems);

    // if header is present, it's at a negative pos, so the footer should not move
    canvas->rootObject()->setProperty("showHeader", true);
    QTRY_COMPARE(footer->pos(), posWhenNoItems);
    canvas->rootObject()->setProperty("showHeader", false);

    // add 30 items
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QSignalSpy footerItemSpy(listview, SIGNAL(footerItemChanged()));
    QMetaObject::invokeMethod(canvas->rootObject(), "changeFooter");

    QCOMPARE(footerItemSpy.count(), 1);

    footer = findItem<QQuickText>(contentItem, "footer");
    QVERIFY(!footer);
    footer = findItem<QQuickText>(contentItem, "footer2");
    QVERIFY(footer);

    QVERIFY(footer == listview->footerItem());

    QCOMPARE(footer->pos(), changedFooterPos);
    QCOMPARE(footer->width(), 50.);
    QCOMPARE(footer->height(), 20.);
    QTRY_COMPARE(QPointF(listview->contentX(), listview->contentY()), changedContentPos);

    item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->pos(), firstDelegatePos);

    listview->positionViewAtEnd();
    footer->setHeight(10);
    footer->setWidth(40);
    QTRY_COMPARE(QPointF(listview->contentX(), listview->contentY()), resizeContentPos);

    delete canvas;
}

void tst_QQuickListView::footer_data()
{
    QTest::addColumn<QQuickListView::Orientation>("orientation");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<QPointF>("initialFooterPos");
    QTest::addColumn<QPointF>("changedFooterPos");
    QTest::addColumn<QPointF>("initialContentPos");
    QTest::addColumn<QPointF>("changedContentPos");
    QTest::addColumn<QPointF>("firstDelegatePos");
    QTest::addColumn<QPointF>("resizeContentPos");

    // footer1 = 100 x 30
    // footer2 = 50 x 20
    // delegates = 40 x 20
    // view width = 240
    // view height = 320

    // footer below items, bottom left
    QTest::newRow("vertical, layout left to right") << QQuickListView::Vertical << Qt::LeftToRight
        << QPointF(0, 3 * 20)
        << QPointF(0, 30 * 20)  // added 30 items
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(0, 30 * 20 - 320 + 10);

    // footer below items, bottom right
    QTest::newRow("vertical, layout right to left") << QQuickListView::Vertical << Qt::RightToLeft
        << QPointF(0, 3 * 20)
        << QPointF(0, 30 * 20)
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(0, 30 * 20 - 320 + 10);

    // footer to right of items
    QTest::newRow("horizontal, layout left to right") << QQuickListView::Horizontal << Qt::LeftToRight
        << QPointF(40 * 3, 0)
        << QPointF(40 * 30, 0)
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(0, 0)
        << QPointF(40 * 30 - 240 + 40, 0);

    // footer to left of items
    QTest::newRow("horizontal, layout right to left") << QQuickListView::Horizontal << Qt::RightToLeft
        << QPointF(-(40 * 3) - 100, 0)
        << QPointF(-(40 * 30) - 50, 0)     // 50 = new footer width
        << QPointF(-240, 0)
        << QPointF(-240, 0)
        << QPointF(-40, 0)
        << QPointF(-(40 * 30) - 40, 0);
}

class LVAccessor : public QQuickListView
{
public:
    qreal minY() const { return minYExtent(); }
    qreal maxY() const { return maxYExtent(); }
    qreal minX() const { return minXExtent(); }
    qreal maxX() const { return maxXExtent(); }
};

void tst_QQuickListView::headerFooter()
{
    {
        // Vertical
        QQuickView *canvas = createView();

        QmlListModel model;
        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(testFileUrl("headerfooter.qml"));
        qApp->processEvents();

        QQuickListView *listview = qobject_cast<QQuickListView*>(canvas->rootObject());
        QTRY_VERIFY(listview != 0);

        QQuickItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QQuickItem *header = findItem<QQuickItem>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->y(), -header->height());

        QQuickItem *footer = findItem<QQuickItem>(contentItem, "footer");
        QVERIFY(footer);
        QCOMPARE(footer->y(), 0.);

        QCOMPARE(static_cast<LVAccessor*>(listview)->minY(), header->height());
        QCOMPARE(static_cast<LVAccessor*>(listview)->maxY(), header->height());

        delete canvas;
    }
    {
        // Horizontal
        QQuickView *canvas = createView();

        QmlListModel model;
        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(testFileUrl("headerfooter.qml"));
        canvas->rootObject()->setProperty("horizontal", true);
        qApp->processEvents();

        QQuickListView *listview = qobject_cast<QQuickListView*>(canvas->rootObject());
        QTRY_VERIFY(listview != 0);

        QQuickItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QQuickItem *header = findItem<QQuickItem>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->x(), -header->width());

        QQuickItem *footer = findItem<QQuickItem>(contentItem, "footer");
        QVERIFY(footer);
        QCOMPARE(footer->x(), 0.);

        QCOMPARE(static_cast<LVAccessor*>(listview)->minX(), header->width());
        QCOMPARE(static_cast<LVAccessor*>(listview)->maxX(), header->width());

        delete canvas;
    }
    {
        // Horizontal RTL
        QQuickView *canvas = createView();

        QmlListModel model;
        QDeclarativeContext *ctxt = canvas->rootContext();
        ctxt->setContextProperty("testModel", &model);

        canvas->setSource(testFileUrl("headerfooter.qml"));
        canvas->rootObject()->setProperty("horizontal", true);
        canvas->rootObject()->setProperty("rtl", true);
        qApp->processEvents();

        QQuickListView *listview = qobject_cast<QQuickListView*>(canvas->rootObject());
        QTRY_VERIFY(listview != 0);

        QQuickItem *contentItem = listview->contentItem();
        QTRY_VERIFY(contentItem != 0);

        QQuickItem *header = findItem<QQuickItem>(contentItem, "header");
        QVERIFY(header);
        QCOMPARE(header->x(), 0.);

        QQuickItem *footer = findItem<QQuickItem>(contentItem, "footer");
        QVERIFY(footer);
        QCOMPARE(footer->x(), -footer->width());

        QCOMPARE(static_cast<LVAccessor*>(listview)->minX(), 240. - header->width());
        QCOMPARE(static_cast<LVAccessor*>(listview)->maxX(), 240. - header->width());

        delete canvas;
    }
}

void tst_QQuickListView::resizeView()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 40; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
    }

    QVariant heightRatio;
    QMetaObject::invokeMethod(canvas->rootObject(), "heightRatio", Q_RETURN_ARG(QVariant, heightRatio));
    QCOMPARE(heightRatio.toReal(), 0.4);

    listview->setHeight(200);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QMetaObject::invokeMethod(canvas->rootObject(), "heightRatio", Q_RETURN_ARG(QVariant, heightRatio));
    QCOMPARE(heightRatio.toReal(), 0.25);

    // Ensure we handle -ve sizes
    listview->setHeight(-100);
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "wrapper", false).count(), 1);

    listview->setCacheBuffer(200);
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "wrapper", false).count(), 11);

    // ensure items in cache become visible
    listview->setHeight(200);
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "wrapper", false).count(), 21);

    itemCount = findItems<QQuickItem>(contentItem, "wrapper", false).count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
        QCOMPARE(item->isVisible(), i < 11); // inside view visible, outside not visible
    }

    // ensure items outside view become invisible
    listview->setHeight(100);
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "wrapper", false).count(), 16);

    itemCount = findItems<QQuickItem>(contentItem, "wrapper", false).count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*20.);
        QCOMPARE(item->isVisible(), i < 6); // inside view visible, outside not visible
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::resizeViewAndRepaint()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 40; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("initialHeight", 100);

    canvas->setSource(testFileUrl("resizeview.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // item at index 10 should not be currently visible
    QVERIFY(!findItem<QQuickItem>(contentItem, "wrapper", 10));

    listview->setHeight(320);

    QTRY_VERIFY(findItem<QQuickItem>(contentItem, "wrapper", 10));

    listview->setHeight(100);
    QTRY_VERIFY(!findItem<QQuickItem>(contentItem, "wrapper", 10));

    delete canvas;
}

void tst_QQuickListView::sizeLessThan1()
{
    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("sizelessthan1.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // Confirm items positioned correctly
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i = 0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        if (!item) qWarning() << "Item" << i << "not found";
        QTRY_VERIFY(item);
        QTRY_COMPARE(item->y(), i*0.5);
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::QTBUG_14821()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("qtbug14821.qml"));
    qApp->processEvents();

    QQuickListView *listview = qobject_cast<QQuickListView*>(canvas->rootObject());
    QVERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    listview->decrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 99);

    listview->incrementCurrentIndex();
    QCOMPARE(listview->currentIndex(), 0);

    delete canvas;
}

void tst_QQuickListView::resizeDelegate()
{
    QQuickView *canvas = createView();

    QStringList strings;
    for (int i = 0; i < 30; ++i)
        strings << QString::number(i);
    QStringListModel model(strings);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("displaylist.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QVERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QCOMPARE(listview->count(), model.rowCount());

    listview->setCurrentIndex(25);
    listview->setContentY(0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i = 0; i < 16; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QCOMPARE(item->y(), i*20.0);
    }

    QCOMPARE(listview->currentItem()->y(), 500.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 500.0);

    canvas->rootObject()->setProperty("delegateHeight", 30);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i = 0; i < 11; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QTRY_COMPARE(item->y(), i*30.0);
    }

    QTRY_COMPARE(listview->currentItem()->y(), 750.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 750.0);

    listview->setCurrentIndex(1);
    listview->positionViewAtIndex(25, QQuickListView::Beginning);
    listview->positionViewAtIndex(5, QQuickListView::Beginning);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i = 5; i < 16; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QCOMPARE(item->y(), i*30.0);
    }

    QTRY_COMPARE(listview->currentItem()->y(), 30.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 30.0);

    canvas->rootObject()->setProperty("delegateHeight", 20);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    for (int i = 5; i < 11; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QTRY_COMPARE(item->y(), 150 + (i-5)*20.0);
    }

    QTRY_COMPARE(listview->currentItem()->y(), 70.0);
    QTRY_COMPARE(listview->highlightItem()->y(), 70.0);

    delete canvas;
}

void tst_QQuickListView::resizeFirstDelegate()
{
    // QTBUG-20712: Content Y jumps constantly if first delegate height == 0
    // and other delegates have height > 0

    QQuickView *canvas = createView();

    // bug only occurs when all items in the model are visible
    QmlListModel model;
    for (int i = 0; i < 10; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QVERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *item = 0;
    for (int i = 0; i < model.count(); ++i) {
        item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QCOMPARE(item->y(), i*20.0);
    }

    item = findItem<QQuickItem>(contentItem, "wrapper", 0);
    item->setHeight(0);

    // check the content y has not jumped up and down
    QCOMPARE(listview->contentY(), 0.0);
    QSignalSpy spy(listview, SIGNAL(contentYChanged()));
    QTest::qWait(100);
    QCOMPARE(spy.count(), 0);

    for (int i = 1; i < model.count(); ++i) {
        item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY(item != 0);
        QTRY_COMPARE(item->y(), (i-1)*20.0);
    }


    // QTBUG-22014: refill doesn't clear items scrolling off the top of the
    // list if they follow a zero-sized delegate

    for (int i = 0; i < 10; i++)
        model.addItem("Item" + QString::number(i), "");
    QTRY_COMPARE(listview->count(), model.count());

    item = findItem<QQuickItem>(contentItem, "wrapper", 1);
    QVERIFY(item);
    item->setHeight(0);

    listview->setCurrentIndex(19);
    qApp->processEvents();
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // items 0-2 should have been deleted
    for (int i=0; i<3; i++) {
        QTRY_VERIFY(!findItem<QQuickItem>(contentItem, "wrapper", i));
    }

    delete testObject;
    delete canvas;
}

void tst_QQuickListView::QTBUG_16037()
{
    QQuickView *canvas = createView();
    canvas->show();

    canvas->setSource(testFileUrl("qtbug16037.qml"));
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "listview");
    QTRY_VERIFY(listview != 0);

    QVERIFY(listview->contentHeight() <= 0.0);

    QMetaObject::invokeMethod(canvas->rootObject(), "setModel");

    QTRY_COMPARE(listview->contentHeight(), 80.0);

    delete canvas;
}

void tst_QQuickListView::indexAt_itemAt_data()
{
    QTest::addColumn<qreal>("x");
    QTest::addColumn<qreal>("y");
    QTest::addColumn<int>("index");

    QTest::newRow("Item 0 - 0, 0") << 0. << 0. << 0;
    QTest::newRow("Item 0 - 0, 19") << 0. << 19. << 0;
    QTest::newRow("Item 0 - 239, 19") << 239. << 19. << 0;
    QTest::newRow("Item 1 - 0, 20") << 0. << 20. << 1;
    QTest::newRow("No Item - 240, 20") << 240. << 20. << -1;
}

void tst_QQuickListView::indexAt_itemAt()
{
    QFETCH(qreal, x);
    QFETCH(qreal, y);
    QFETCH(int, index);

    QQuickView *canvas = createView();

    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    canvas->setSource(testFileUrl("listviewtest.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *item = 0;
    if (index >= 0) {
        item = findItem<QQuickItem>(contentItem, "wrapper", index);
        QVERIFY(item);
    }
    QCOMPARE(listview->indexAt(x,y), index);
    QVERIFY(listview->itemAt(x,y) == item);

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::incrementalModel()
{
    QQuickView *canvas = createView();

    IncrementalModel model;
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("displaylist.qml"));
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(listview->count(), 20);

    listview->positionViewAtIndex(10, QQuickListView::Beginning);

    QTRY_COMPARE(listview->count(), 25);

    delete canvas;
}

void tst_QQuickListView::onAdd()
{
    QFETCH(int, initialItemCount);
    QFETCH(int, itemsToAdd);

    const int delegateHeight = 10;
    QaimModel model;

    // these initial items should not trigger ListView.onAdd
    for (int i=0; i<initialItemCount; i++)
        model.addItem("dummy value", "dummy value");

    QQuickView *canvas = createView();
    canvas->setGeometry(0,0,200, delegateHeight * (initialItemCount + itemsToAdd));
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("delegateHeight", delegateHeight);
    canvas->setSource(testFileUrl("attachedSignals.qml"));

    QObject *object = canvas->rootObject();
    object->setProperty("width", canvas->width());
    object->setProperty("height", canvas->height());
    qApp->processEvents();

    QList<QPair<QString, QString> > items;
    for (int i=0; i<itemsToAdd; i++)
        items << qMakePair(QString("value %1").arg(i), QString::number(i));
    model.addItems(items);
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    QVariantList result = object->property("addedDelegates").toList();
    QCOMPARE(result.count(), items.count());
    for (int i=0; i<items.count(); i++)
        QCOMPARE(result[i].toString(), items[i].first);

    delete canvas;
}

void tst_QQuickListView::onAdd_data()
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

void tst_QQuickListView::onRemove()
{
    QFETCH(int, initialItemCount);
    QFETCH(int, indexToRemove);
    QFETCH(int, removeCount);

    const int delegateHeight = 10;
    QaimModel model;
    for (int i=0; i<initialItemCount; i++)
        model.addItem(QString("value %1").arg(i), "dummy value");

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("delegateHeight", delegateHeight);
    canvas->setSource(testFileUrl("attachedSignals.qml"));

    QObject *object = canvas->rootObject();

    model.removeItems(indexToRemove, removeCount);
    QTRY_COMPARE(canvas->rootObject()->property("count").toInt(), model.count());

    QCOMPARE(object->property("removedDelegateCount"), QVariant(removeCount));

    delete canvas;
}

void tst_QQuickListView::onRemove_data()
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

void tst_QQuickListView::rightToLeft()
{
    QQuickView *canvas = createView();
    canvas->setGeometry(0,0,640,320);
    canvas->setSource(testFileUrl("rightToLeft.qml"));
    canvas->show();
    qApp->processEvents();

    QVERIFY(canvas->rootObject() != 0);
    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "view");
    QTRY_VERIFY(listview != 0);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickVisualItemModel *model = canvas->rootObject()->findChild<QQuickVisualItemModel*>("itemModel");
    QTRY_VERIFY(model != 0);

    QTRY_VERIFY(model->count() == 3);
    QTRY_COMPARE(listview->currentIndex(), 0);

    // initial position at first item, right edge aligned
    QCOMPARE(listview->contentX(), -640.);

    QQuickItem *item = findItem<QQuickItem>(contentItem, "item1");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), -100.0);
    QCOMPARE(item->height(), listview->height());

    QQuickText *text = findItem<QQuickText>(contentItem, "text1");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 0"));

    listview->setCurrentIndex(2);

    item = findItem<QQuickItem>(contentItem, "item3");
    QTRY_VERIFY(item);
    QTRY_COMPARE(item->x(), -540.0);

    text = findItem<QQuickText>(contentItem, "text3");
    QTRY_VERIFY(text);
    QTRY_COMPARE(text->text(), QLatin1String("index: 2"));

    QCOMPARE(listview->contentX(), -640.);

    // Ensure resizing maintains position relative to right edge
    qobject_cast<QQuickItem*>(canvas->rootObject())->setWidth(600);
    QTRY_COMPARE(listview->contentX(), -600.);

    delete canvas;
}

void tst_QQuickListView::test_mirroring()
{
    QQuickView *canvasA = createView();
    canvasA->setSource(testFileUrl("rightToLeft.qml"));
    QQuickListView *listviewA = findItem<QQuickListView>(canvasA->rootObject(), "view");
    QTRY_VERIFY(listviewA != 0);

    QQuickView *canvasB = createView();
    canvasB->setSource(testFileUrl("rightToLeft.qml"));
    QQuickListView *listviewB = findItem<QQuickListView>(canvasB->rootObject(), "view");
    QTRY_VERIFY(listviewA != 0);
    qApp->processEvents();

    QList<QString> objectNames;
    objectNames << "item1" << "item2"; // << "item3"

    listviewA->setProperty("layoutDirection", Qt::LeftToRight);
    listviewB->setProperty("layoutDirection", Qt::RightToLeft);
    QCOMPARE(listviewA->layoutDirection(), listviewA->effectiveLayoutDirection());

    // LTR != RTL
    foreach (const QString objectName, objectNames)
        QVERIFY(findItem<QQuickItem>(listviewA, objectName)->x() != findItem<QQuickItem>(listviewB, objectName)->x());

    listviewA->setProperty("layoutDirection", Qt::LeftToRight);
    listviewB->setProperty("layoutDirection", Qt::LeftToRight);

    // LTR == LTR
    foreach (const QString objectName, objectNames)
        QCOMPARE(findItem<QQuickItem>(listviewA, objectName)->x(), findItem<QQuickItem>(listviewB, objectName)->x());

    QVERIFY(listviewB->layoutDirection() == listviewB->effectiveLayoutDirection());
    QQuickItemPrivate::get(listviewB)->setLayoutMirror(true);
    QVERIFY(listviewB->layoutDirection() != listviewB->effectiveLayoutDirection());

    // LTR != LTR+mirror
    foreach (const QString objectName, objectNames)
        QVERIFY(findItem<QQuickItem>(listviewA, objectName)->x() != findItem<QQuickItem>(listviewB, objectName)->x());

    listviewA->setProperty("layoutDirection", Qt::RightToLeft);

    // RTL == LTR+mirror
    foreach (const QString objectName, objectNames)
        QCOMPARE(findItem<QQuickItem>(listviewA, objectName)->x(), findItem<QQuickItem>(listviewB, objectName)->x());

    listviewB->setProperty("layoutDirection", Qt::RightToLeft);

    // RTL != RTL+mirror
    foreach (const QString objectName, objectNames)
        QVERIFY(findItem<QQuickItem>(listviewA, objectName)->x() != findItem<QQuickItem>(listviewB, objectName)->x());

    listviewA->setProperty("layoutDirection", Qt::LeftToRight);

    // LTR == RTL+mirror
    foreach (const QString objectName, objectNames)
        QCOMPARE(findItem<QQuickItem>(listviewA, objectName)->x(), findItem<QQuickItem>(listviewB, objectName)->x());

    delete canvasA;
    delete canvasB;
}

void tst_QQuickListView::margins()
{
    QQuickView *canvas = createView();

    QaimModel model;
    for (int i = 0; i < 50; i++)
        model.addItem("Item" + QString::number(i), "");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("margins.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QCOMPARE(listview->contentY(), -30.);
    QCOMPARE(listview->yOrigin(), 0.);

    // check end bound
    listview->positionViewAtEnd();
    qreal pos = listview->contentY();
    listview->setContentY(pos + 80);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    listview->returnToBounds();
    QTRY_COMPARE(listview->contentY(), pos + 50);

    // remove item before visible and check that top margin is maintained
    // and yOrigin is updated
    listview->setContentY(100);
    model.removeItem(1);
    QTRY_COMPARE(listview->count(), model.count());
    listview->setContentY(-50);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    listview->returnToBounds();
    QCOMPARE(listview->yOrigin(), 20.);
    QTRY_COMPARE(listview->contentY(), -10.);

    // reduce top margin
    listview->setTopMargin(20);
    QCOMPARE(listview->yOrigin(), 20.);
    QTRY_COMPARE(listview->contentY(), 0.);

    // check end bound
    listview->positionViewAtEnd();
    pos = listview->contentY();
    listview->setContentY(pos + 80);
    listview->returnToBounds();
    QTRY_COMPARE(listview->contentY(), pos + 50);

    // reduce bottom margin
    pos = listview->contentY();
    listview->setBottomMargin(40);
    QCOMPARE(listview->yOrigin(), 20.);
    QTRY_COMPARE(listview->contentY(), pos-10);

    delete canvas;
}

// QTBUG-24028
void tst_QQuickListView::marginsResize()
{
    QFETCH(QQuickListView::Orientation, orientation);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(qreal, start);
    QFETCH(qreal, end);

    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("margins2.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "listview");
    QTRY_VERIFY(listview != 0);

    listview->setOrientation(orientation);
    listview->setLayoutDirection(layoutDirection);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    // view is resized after componentCompleted - top margin should still be visible
    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), start);
    else
        QCOMPARE(listview->contentX(), start);

    // move to last index and ensure bottom margin is visible.
    listview->setCurrentIndex(19);
    if (orientation == QQuickListView::Vertical)
        QTRY_COMPARE(listview->contentY(), end);
    else
        QTRY_COMPARE(listview->contentX(), end);

    // back to top - top margin should be visible.
    listview->setCurrentIndex(0);
    if (orientation == QQuickListView::Vertical)
        QTRY_COMPARE(listview->contentY(), start);
    else
        QTRY_COMPARE(listview->contentX(), start);

    delete canvas;
}

void tst_QQuickListView::marginsResize_data()
{
    QTest::addColumn<QQuickListView::Orientation>("orientation");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<qreal>("start");
    QTest::addColumn<qreal>("end");

    QTest::newRow("vertical") << QQuickListView::Vertical << Qt::LeftToRight << -20.0 << 1020.0;
    QTest::newRow("horizontal") << QQuickListView::Horizontal << Qt::LeftToRight << -20.0 << 1020.0;
    QTest::newRow("horizontal, rtl") << QQuickListView::Horizontal << Qt::RightToLeft << -180.0 << -1220.0;
}

void tst_QQuickListView::snapToItem_data()
{
    QTest::addColumn<QQuickListView::Orientation>("orientation");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<int>("highlightRangeMode");
    QTest::addColumn<QPoint>("flickStart");
    QTest::addColumn<QPoint>("flickEnd");
    QTest::addColumn<qreal>("snapAlignment");
    QTest::addColumn<qreal>("endExtent");
    QTest::addColumn<qreal>("startExtent");

    QTest::newRow("vertical, left to right") << QQuickListView::Vertical << Qt::LeftToRight << int(QQuickItemView::NoHighlightRange)
        << QPoint(20, 200) << QPoint(20, 20) << 60.0 << 1200.0 << 0.0;

    QTest::newRow("horizontal, left to right") << QQuickListView::Horizontal << Qt::LeftToRight << int(QQuickItemView::NoHighlightRange)
        << QPoint(200, 20) << QPoint(20, 20) << 60.0 << 1200.0 << 0.0;

    QTest::newRow("horizontal, right to left") << QQuickListView::Horizontal << Qt::RightToLeft << int(QQuickItemView::NoHighlightRange)
        << QPoint(20, 20) << QPoint(200, 20) << -60.0 << -1200.0 - 240.0 << -240.0;

    QTest::newRow("vertical, left to right, enforce range") << QQuickListView::Vertical << Qt::LeftToRight << int(QQuickItemView::StrictlyEnforceRange)
        << QPoint(20, 200) << QPoint(20, 20) << 60.0 << 1340.0 << -20.0;

    QTest::newRow("horizontal, left to right, enforce range") << QQuickListView::Horizontal << Qt::LeftToRight << int(QQuickItemView::StrictlyEnforceRange)
        << QPoint(200, 20) << QPoint(20, 20) << 60.0 << 1340.0 << -20.0;

    QTest::newRow("horizontal, right to left, enforce range") << QQuickListView::Horizontal << Qt::RightToLeft << int(QQuickItemView::StrictlyEnforceRange)
        << QPoint(20, 20) << QPoint(200, 20) << -60.0 << -1200.0 - 240.0 - 140.0 << -220.0;
}

void tst_QQuickListView::snapToItem()
{
    QFETCH(QQuickListView::Orientation, orientation);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(int, highlightRangeMode);
    QFETCH(QPoint, flickStart);
    QFETCH(QPoint, flickEnd);
    QFETCH(qreal, snapAlignment);
    QFETCH(qreal, endExtent);
    QFETCH(qreal, startExtent);

    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("snapToItem.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    listview->setOrientation(orientation);
    listview->setLayoutDirection(layoutDirection);
    listview->setHighlightRangeMode(QQuickItemView::HighlightRangeMode(highlightRangeMode));
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    // confirm that a flick hits an item boundary
    flick(canvas, flickStart, flickEnd, 180);
    QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
    if (orientation == QQuickListView::Vertical)
        QCOMPARE(qreal(fmod(listview->contentY(),80.0)), snapAlignment);
    else
        QCOMPARE(qreal(fmod(listview->contentX(),80.0)), snapAlignment);

    // flick to end
    do {
        flick(canvas, flickStart, flickEnd, 180);
        QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
    } while (orientation == QQuickListView::Vertical
           ? !listview->isAtYEnd()
           : layoutDirection == Qt::LeftToRight ? !listview->isAtXEnd() : !listview->isAtXBeginning());

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), endExtent);
    else
        QCOMPARE(listview->contentX(), endExtent);

    // flick to start
    do {
        flick(canvas, flickEnd, flickStart, 180);
        QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
    } while (orientation == QQuickListView::Vertical
           ? !listview->isAtYBeginning()
           : layoutDirection == Qt::LeftToRight ? !listview->isAtXBeginning() : !listview->isAtXEnd());

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), startExtent);
    else
        QCOMPARE(listview->contentX(), startExtent);

    delete canvas;
}

void tst_QQuickListView::qListModelInterface_items()
{
    items<QmlListModel>(testFileUrl("listviewtest.qml"), false);
}

void tst_QQuickListView::qListModelInterface_package_items()
{
    items<QmlListModel>(testFileUrl("listviewtest-package.qml"), true);
}

void tst_QQuickListView::qAbstractItemModel_items()
{
    items<QaimModel>(testFileUrl("listviewtest.qml"), false);
}

void tst_QQuickListView::qListModelInterface_changed()
{
    changed<QmlListModel>(testFileUrl("listviewtest.qml"), false);
}

void tst_QQuickListView::qListModelInterface_package_changed()
{
    changed<QmlListModel>(testFileUrl("listviewtest-package.qml"), true);
}

void tst_QQuickListView::qAbstractItemModel_changed()
{
    changed<QaimModel>(testFileUrl("listviewtest.qml"), false);
}

void tst_QQuickListView::qListModelInterface_inserted()
{
    inserted<QmlListModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qListModelInterface_package_inserted()
{
    inserted<QmlListModel>(testFileUrl("listviewtest-package.qml"));
}

void tst_QQuickListView::qListModelInterface_inserted_more()
{
    inserted_more<QmlListModel>();
}

void tst_QQuickListView::qListModelInterface_inserted_more_data()
{
    inserted_more_data();
}

void tst_QQuickListView::qAbstractItemModel_inserted()
{
    inserted<QaimModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qAbstractItemModel_inserted_more()
{
    inserted_more<QaimModel>();
}

void tst_QQuickListView::qAbstractItemModel_inserted_more_data()
{
    inserted_more_data();
}

void tst_QQuickListView::qListModelInterface_removed()
{
    removed<QmlListModel>(testFileUrl("listviewtest.qml"), false);
    removed<QmlListModel>(testFileUrl("listviewtest.qml"), true);
}

void tst_QQuickListView::qListModelInterface_removed_more()
{
    removed_more<QmlListModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qListModelInterface_removed_more_data()
{
    removed_more_data();
}

void tst_QQuickListView::qListModelInterface_package_removed()
{
    removed<QmlListModel>(testFileUrl("listviewtest-package.qml"), false);
    removed<QmlListModel>(testFileUrl("listviewtest-package.qml"), true);
}

void tst_QQuickListView::qAbstractItemModel_removed()
{
    removed<QaimModel>(testFileUrl("listviewtest.qml"), false);
    removed<QaimModel>(testFileUrl("listviewtest.qml"), true);
}

void tst_QQuickListView::qAbstractItemModel_removed_more()
{
    removed_more<QaimModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qAbstractItemModel_removed_more_data()
{
    removed_more_data();
}

void tst_QQuickListView::qListModelInterface_moved()
{
    moved<QmlListModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qListModelInterface_moved_data()
{
    moved_data();
}

void tst_QQuickListView::qListModelInterface_package_moved()
{
    moved<QmlListModel>(testFileUrl("listviewtest-package.qml"));
}

void tst_QQuickListView::qListModelInterface_package_moved_data()
{
    moved_data();
}

void tst_QQuickListView::qAbstractItemModel_moved()
{
    moved<QaimModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qAbstractItemModel_moved_data()
{
    moved_data();
}

void tst_QQuickListView::qListModelInterface_clear()
{
    clear<QmlListModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qListModelInterface_package_clear()
{
    clear<QmlListModel>(testFileUrl("listviewtest-package.qml"));
}

void tst_QQuickListView::qAbstractItemModel_clear()
{
    clear<QaimModel>(testFileUrl("listviewtest.qml"));
}

void tst_QQuickListView::qListModelInterface_sections()
{
    sections<QmlListModel>(testFileUrl("listview-sections.qml"));
}

void tst_QQuickListView::qListModelInterface_package_sections()
{
    sections<QmlListModel>(testFileUrl("listview-sections-package.qml"));
}

void tst_QQuickListView::qAbstractItemModel_sections()
{
    sections<QaimModel>(testFileUrl("listview-sections.qml"));
}

void tst_QQuickListView::creationContext()
{
    QQuickView canvas;
    canvas.setGeometry(0,0,240,320);
    canvas.setSource(testFileUrl("creationContext.qml"));
    qApp->processEvents();

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(canvas.rootObject());
    QVERIFY(rootItem);
    QVERIFY(rootItem->property("count").toInt() > 0);

    QQuickItem *item;
    QVERIFY(item = rootItem->findChild<QQuickItem *>("listItem"));
    QCOMPARE(item->property("text").toString(), QString("Hello!"));
    QVERIFY(item = rootItem->findChild<QQuickItem *>("header"));
    QCOMPARE(item->property("text").toString(), QString("Hello!"));
    QVERIFY(item = rootItem->findChild<QQuickItem *>("footer"));
    QCOMPARE(item->property("text").toString(), QString("Hello!"));
    QVERIFY(item = rootItem->findChild<QQuickItem *>("section"));
    QCOMPARE(item->property("text").toString(), QString("Hello!"));
}

void tst_QQuickListView::QTBUG_21742()
{
    QQuickView canvas;
    canvas.setGeometry(0,0,200,200);
    canvas.setSource(testFileUrl("qtbug-21742.qml"));
    qApp->processEvents();

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(canvas.rootObject());
    QVERIFY(rootItem);
    QCOMPARE(rootItem->property("count").toInt(), 1);
}

void tst_QQuickListView::asynchronous()
{
    QQuickView *canvas = createView();
    canvas->show();
    QDeclarativeIncubationController controller;
    canvas->engine()->setIncubationController(&controller);

    canvas->setSource(testFileUrl("asyncloader.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootObject);

    QQuickListView *listview = 0;
    while (!listview) {
        bool b = false;
        controller.incubateWhile(&b);
        listview = rootObject->findChild<QQuickListView*>("view");
    }

    // items will be created one at a time
    for (int i = 0; i < 8; ++i) {
        QVERIFY(findItem<QQuickItem>(listview, "wrapper", i) == 0);
        QQuickItem *item = 0;
        while (!item) {
            bool b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(listview, "wrapper", i);
        }
    }

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    // verify positioning
    QQuickItem *contentItem = listview->contentItem();
    for (int i = 0; i < 8; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QTRY_COMPARE(item->y(), i*50.0);
    }

    delete canvas;
}

void tst_QQuickListView::snapOneItem_data()
{
    QTest::addColumn<QQuickListView::Orientation>("orientation");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<int>("highlightRangeMode");
    QTest::addColumn<QPoint>("flickStart");
    QTest::addColumn<QPoint>("flickEnd");
    QTest::addColumn<qreal>("snapAlignment");
    QTest::addColumn<qreal>("endExtent");
    QTest::addColumn<qreal>("startExtent");

    QTest::newRow("vertical, left to right") << QQuickListView::Vertical << Qt::LeftToRight << int(QQuickItemView::NoHighlightRange)
        << QPoint(20, 200) << QPoint(20, 20) << 180.0 << 560.0 << 0.0;

    QTest::newRow("horizontal, left to right") << QQuickListView::Horizontal << Qt::LeftToRight << int(QQuickItemView::NoHighlightRange)
        << QPoint(200, 20) << QPoint(20, 20) << 180.0 << 560.0 << 0.0;

    QTest::newRow("horizontal, right to left") << QQuickListView::Horizontal << Qt::RightToLeft << int(QQuickItemView::NoHighlightRange)
        << QPoint(20, 20) << QPoint(200, 20) << -420.0 << -560.0 - 240.0 << -240.0;

    QTest::newRow("vertical, left to right, enforce range") << QQuickListView::Vertical << Qt::LeftToRight << int(QQuickItemView::StrictlyEnforceRange)
        << QPoint(20, 200) << QPoint(20, 20) << 180.0 << 580.0 << -20.0;

    QTest::newRow("horizontal, left to right, enforce range") << QQuickListView::Horizontal << Qt::LeftToRight << int(QQuickItemView::StrictlyEnforceRange)
        << QPoint(200, 20) << QPoint(20, 20) << 180.0 << 580.0 << -20.0;

    QTest::newRow("horizontal, right to left, enforce range") << QQuickListView::Horizontal << Qt::RightToLeft << int(QQuickItemView::StrictlyEnforceRange)
        << QPoint(20, 20) << QPoint(200, 20) << -420.0 << -580.0 - 240.0 << -220.0;
}

void tst_QQuickListView::snapOneItem()
{
    QFETCH(QQuickListView::Orientation, orientation);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(int, highlightRangeMode);
    QFETCH(QPoint, flickStart);
    QFETCH(QPoint, flickEnd);
    QFETCH(qreal, snapAlignment);
    QFETCH(qreal, endExtent);
    QFETCH(qreal, startExtent);

#ifdef Q_OS_MAC
    // This test seems to be unreliable - different test data fails on different runs
    QSKIP("QTBUG-24338");
#endif

    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("snapOneItem.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);

    listview->setOrientation(orientation);
    listview->setLayoutDirection(layoutDirection);
    listview->setHighlightRangeMode(QQuickItemView::HighlightRangeMode(highlightRangeMode));
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != 0);

    QSignalSpy currentIndexSpy(listview, SIGNAL(currentIndexChanged()));

    // confirm that a flick hits the next item boundary
    flick(canvas, flickStart, flickEnd, 180);
    QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), snapAlignment);
    else
        QCOMPARE(listview->contentX(), snapAlignment);

    if (QQuickItemView::HighlightRangeMode(highlightRangeMode) == QQuickItemView::StrictlyEnforceRange) {
        QCOMPARE(listview->currentIndex(), 1);
        QCOMPARE(currentIndexSpy.count(), 1);
    }

    // flick to end
    do {
        flick(canvas, flickStart, flickEnd, 180);
        QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
    } while (orientation == QQuickListView::Vertical
           ? !listview->isAtYEnd()
           : layoutDirection == Qt::LeftToRight ? !listview->isAtXEnd() : !listview->isAtXBeginning());

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), endExtent);
    else
        QCOMPARE(listview->contentX(), endExtent);

    if (QQuickItemView::HighlightRangeMode(highlightRangeMode) == QQuickItemView::StrictlyEnforceRange) {
        QCOMPARE(listview->currentIndex(), 3);
        QCOMPARE(currentIndexSpy.count(), 3);
    }

    // flick to start
    do {
        flick(canvas, flickEnd, flickStart, 180);
        QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
    } while (orientation == QQuickListView::Vertical
           ? !listview->isAtYBeginning()
           : layoutDirection == Qt::LeftToRight ? !listview->isAtXBeginning() : !listview->isAtXEnd());

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), startExtent);
    else
        QCOMPARE(listview->contentX(), startExtent);

    if (QQuickItemView::HighlightRangeMode(highlightRangeMode) == QQuickItemView::StrictlyEnforceRange) {
        QCOMPARE(listview->currentIndex(), 0);
        QCOMPARE(currentIndexSpy.count(), 6);
    }

    delete canvas;
}

void tst_QQuickListView::unrequestedVisibility()
{
    QmlListModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Item" + QString::number(i), QString::number(i));

    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("testWrap", QVariant(false));

    canvas->setSource(testFileUrl("unrequestedItems.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *leftview = findItem<QQuickListView>(canvas->rootObject(), "leftList");
    QTRY_VERIFY(leftview != 0);

    QQuickListView *rightview = findItem<QQuickListView>(canvas->rootObject(), "rightList");
    QTRY_VERIFY(rightview != 0);

    QQuickItem *leftContent = leftview->contentItem();
    QTRY_VERIFY(leftContent != 0);

    QQuickItem *rightContent = rightview->contentItem();
    QTRY_VERIFY(rightContent != 0);

    rightview->setCurrentIndex(20);

    QTRY_COMPARE(leftview->contentY(), 0.0);
    QTRY_COMPARE(rightview->contentY(), 100.0);

    QQuickItem *item;

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 1));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 1));
    QCOMPARE(item->isVisible(), false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 19));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 19));
    QCOMPARE(item->isVisible(), true);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 3));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), true);

    rightview->setCurrentIndex(0);

    QTRY_COMPARE(leftview->contentY(), 0.0);
    QTRY_COMPARE(rightview->contentY(), 0.0);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 1));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 1));
    QTRY_COMPARE(item->isVisible(), true);

    QVERIFY(!findItem<QQuickItem>(leftContent, "wrapper", 19));
    QVERIFY(!findItem<QQuickItem>(rightContent, "wrapper", 19));

    leftview->setCurrentIndex(20);

    QTRY_COMPARE(leftview->contentY(), 100.0);
    QTRY_COMPARE(rightview->contentY(), 0.0);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 1));
    QTRY_COMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 1));
    QCOMPARE(item->isVisible(), true);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 19));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 19));
    QCOMPARE(item->isVisible(), false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 3));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);

    model.moveItems(19, 1, 1);
    QTRY_COMPARE(QQuickItemPrivate::get(leftview)->polishScheduled, false);

    QTRY_VERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 1));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 1));
    QCOMPARE(item->isVisible(), true);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 19));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 19));
    QCOMPARE(item->isVisible(), false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 5));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);

    model.moveItems(3, 4, 1);
    QTRY_COMPARE(QQuickItemPrivate::get(leftview)->polishScheduled, false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 5));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);

    model.moveItems(4, 3, 1);
    QTRY_COMPARE(QQuickItemPrivate::get(leftview)->polishScheduled, false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 5));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);

    model.moveItems(16, 17, 1);
    QTRY_COMPARE(QQuickItemPrivate::get(leftview)->polishScheduled, false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 5));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);

    model.moveItems(17, 16, 1);
    QTRY_COMPARE(QQuickItemPrivate::get(leftview)->polishScheduled, false);

    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 4));
    QCOMPARE(item->isVisible(), false);
    QVERIFY(item = findItem<QQuickItem>(leftContent, "wrapper", 5));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 16));
    QCOMPARE(item->isVisible(), true);
    QVERIFY(item = findItem<QQuickItem>(rightContent, "wrapper", 17));
    QCOMPARE(item->isVisible(), false);

    delete canvas;
}

void tst_QQuickListView::populateTransitions()
{
    QFETCH(bool, staticallyPopulate);
    QFETCH(bool, dynamicallyPopulate);
    QFETCH(bool, usePopulateTransition);

    QPointF transitionFrom(-50, -50);
    QPointF transitionVia(100, 100);
    QaimModel model_transitionFrom;
    QaimModel model_transitionVia;

    QaimModel model;
    if (staticallyPopulate) {
        for (int i = 0; i < 30; i++)
            model.addItem("item" + QString::number(i), "");
    }

    QQuickView *canvas = createView();
    canvas->rootContext()->setContextProperty("testModel", &model);
    canvas->rootContext()->setContextProperty("testObject", new TestObject(canvas->rootContext()));
    canvas->rootContext()->setContextProperty("usePopulateTransition", usePopulateTransition);
    canvas->rootContext()->setContextProperty("dynamicallyPopulate", dynamicallyPopulate);
    canvas->rootContext()->setContextProperty("transitionFrom", transitionFrom);
    canvas->rootContext()->setContextProperty("transitionVia", transitionVia);
    canvas->rootContext()->setContextProperty("model_transitionFrom", &model_transitionFrom);
    canvas->rootContext()->setContextProperty("model_transitionVia", &model_transitionVia);
    canvas->setSource(testFileUrl("populateTransitions.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QVERIFY(listview);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem);

    if (staticallyPopulate || dynamicallyPopulate) {
        // check the populate transition is run
        if (usePopulateTransition) {
            QTRY_COMPARE(listview->property("countPopulateTransitions").toInt(), 17);
        } else {
            QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
            QTRY_COMPARE(listview->property("countPopulateTransitions").toInt(), 0);
        }
        QTRY_COMPARE(listview->property("countAddTransitions").toInt(), 0);
    } else {
        QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    }

    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    if (usePopulateTransition)
        QCOMPARE(itemCount, listview->property("countPopulateTransitions").toInt());
    for (int i=0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->x(), 0.0);
        QTRY_COMPARE(item->y(), i*20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    // add an item and check this is done with add trantion, not populate
    model.insertItem(0, "another item", "");
    QTRY_COMPARE(listview->property("countAddTransitions").toInt(), 1);
    QTRY_COMPARE(listview->property("countPopulateTransitions").toInt(),
                 (usePopulateTransition && (staticallyPopulate || dynamicallyPopulate)) ? 17 : 0);

    // clear the model
    canvas->rootContext()->setContextProperty("testModel", QVariant());
    QTRY_COMPARE(listview->count(), 0);
    QTRY_COMPARE(findItems<QQuickItem>(contentItem, "wrapper").count(), 0);
    listview->setProperty("countPopulateTransitions", 0);
    listview->setProperty("countAddTransitions", 0);

    // set to a valid model and check populate transition is run a second time
    model.clear();
    for (int i = 0; i < 30; i++)
        model.addItem("item" + QString::number(i), "");
    canvas->rootContext()->setContextProperty("testModel", &model);
    QTRY_COMPARE(listview->property("countPopulateTransitions").toInt(), usePopulateTransition ? 17 : 0);
    QTRY_COMPARE(listview->property("countAddTransitions").toInt(), 0);

    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    if (usePopulateTransition)
        QCOMPARE(itemCount, listview->property("countPopulateTransitions").toInt());
    for (int i=0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->x(), 0.0);
        QTRY_COMPARE(item->y(), i*20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    // reset model and check populate transition is run again
    listview->setProperty("countPopulateTransitions", 0);
    listview->setProperty("countAddTransitions", 0);
    model.reset();
    QTRY_COMPARE(listview->property("countPopulateTransitions").toInt(), usePopulateTransition ? 17 : 0);
    QTRY_COMPARE(listview->property("countAddTransitions").toInt(), 0);

    itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    if (usePopulateTransition)
        QCOMPARE(itemCount, listview->property("countPopulateTransitions").toInt());
    for (int i=0; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->x(), 0.0);
        QTRY_COMPARE(item->y(), i*20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
}

void tst_QQuickListView::populateTransitions_data()
{
    QTest::addColumn<bool>("staticallyPopulate");
    QTest::addColumn<bool>("dynamicallyPopulate");
    QTest::addColumn<bool>("usePopulateTransition");

    QTest::newRow("static") << true << false << true;
    QTest::newRow("static, no populate") << true << false << false;

    QTest::newRow("dynamic") << false << true << true;
    QTest::newRow("dynamic, no populate") << false << true << false;

    QTest::newRow("empty to start with") << false << false << true;
    QTest::newRow("empty to start with, no populate") << false << false << false;
}

void tst_QQuickListView::addTransitions()
{
    QFETCH(int, initialItemCount);
    QFETCH(bool, shouldAnimateTargets);
    QFETCH(qreal, contentY);
    QFETCH(int, insertionIndex);
    QFETCH(int, insertionCount);
    QFETCH(ListRange, expectedDisplacedIndexes);

    // added items should start here
    QPointF targetItems_transitionFrom(-50, -50);

    // displaced items should pass through this point
    QPointF displacedItems_transitionVia(100, 100);

    QaimModel model;
    for (int i = 0; i < initialItemCount; i++)
        model.addItem("Original item" + QString::number(i), "");
    QaimModel model_targetItems_transitionFrom;
    QaimModel model_displacedItems_transitionVia;

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("model_targetItems_transitionFrom", &model_targetItems_transitionFrom);
    ctxt->setContextProperty("model_displacedItems_transitionVia", &model_displacedItems_transitionVia);
    ctxt->setContextProperty("targetItems_transitionFrom", targetItems_transitionFrom);
    ctxt->setContextProperty("displacedItems_transitionVia", displacedItems_transitionVia);
    ctxt->setContextProperty("testObject", testObject);
    canvas->setSource(testFileUrl("addTransitions.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    if (contentY != 0) {
        listview->setContentY(contentY);
        QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    }

    QList<QPair<QString,QString> > expectedDisplacedValues = expectedDisplacedIndexes.getModelDataValues(model);

    // only target items that will become visible should be animated
    QList<QPair<QString, QString> > newData;
    QList<QPair<QString, QString> > expectedTargetData;
    QList<int> targetIndexes;
    if (shouldAnimateTargets) {
        for (int i=insertionIndex; i<insertionIndex+insertionCount; i++) {
            newData << qMakePair(QString("New item %1").arg(i), QString(""));

            if (i >= contentY / 20 && i < (contentY + listview->height()) / 20) {  // only grab visible items
                expectedTargetData << newData.last();
                targetIndexes << i;
            }
        }
        QVERIFY(expectedTargetData.count() > 0);
    }

    // start animation
    if (!newData.isEmpty()) {
        model.insertItems(insertionIndex, newData);
        QTRY_COMPARE(model.count(), listview->count());
    }

    QList<QQuickItem *> targetItems = findItems<QQuickItem>(contentItem, "wrapper", targetIndexes);

    if (shouldAnimateTargets) {
        QTRY_COMPARE(listview->property("targetTransitionsDone").toInt(), expectedTargetData.count());
        QTRY_COMPARE(listview->property("displaceTransitionsDone").toInt(),
                     expectedDisplacedIndexes.isValid() ? expectedDisplacedIndexes.count() : 0);

        // check the target and displaced items were animated
        model_targetItems_transitionFrom.matchAgainst(expectedTargetData, "wasn't animated from target 'from' pos", "shouldn't have been animated from target 'from' pos");
        model_displacedItems_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with displaced anim", "shouldn't have been animated with displaced anim");

        // check attached properties
        matchItemsAndIndexes(listview->property("targetTrans_items").toMap(), model, targetIndexes);
        matchIndexLists(listview->property("targetTrans_targetIndexes").toList(), targetIndexes);
        matchItemLists(listview->property("targetTrans_targetItems").toList(), targetItems);
        if (expectedDisplacedIndexes.isValid()) {
            // adjust expectedDisplacedIndexes to their final values after the move
            QList<int> displacedIndexes = adjustIndexesForAddDisplaced(expectedDisplacedIndexes.indexes, insertionIndex, insertionCount);
            matchItemsAndIndexes(listview->property("displacedTrans_items").toMap(), model, displacedIndexes);
            matchIndexLists(listview->property("displacedTrans_targetIndexes").toList(), targetIndexes);
            matchItemLists(listview->property("displacedTrans_targetItems").toList(), targetItems);
        }

    } else {
        QTRY_COMPARE(model_targetItems_transitionFrom.count(), 0);
        QTRY_COMPARE(model_displacedItems_transitionVia.count(), 0);
    }

    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    int firstVisibleIndex = -1;
    int itemCount = items.count();
    for (int i=0; i<items.count(); i++) {
        if (items[i]->y() >= contentY) {
            QDeclarativeExpression e(qmlContext(items[i]), items[i], "index");
            firstVisibleIndex = e.evaluate().toInt();
            break;
        }
    }
    QVERIFY2(firstVisibleIndex >= 0, QTest::toString(firstVisibleIndex));

    // verify all items moved to the correct final positions
    for (int i=firstVisibleIndex; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->y(), i*20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::addTransitions_data()
{
    QTest::addColumn<int>("initialItemCount");
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<bool>("shouldAnimateTargets");
    QTest::addColumn<int>("insertionIndex");
    QTest::addColumn<int>("insertionCount");
    QTest::addColumn<ListRange>("expectedDisplacedIndexes");

    // if inserting before visible index, items should not appear or animate in, even if there are > 1 new items
    QTest::newRow("insert 1, just before start")
            << 30 << 20.0 << false
            << 0 << 1 << ListRange();
    QTest::newRow("insert 1, way before start")
            << 30 << 20.0 << false
            << 0 << 1 << ListRange();
    QTest::newRow("insert multiple, just before start")
            << 30 << 100.0 << false
            << 0 << 3 << ListRange();
    QTest::newRow("insert multiple, way before start")
            << 30 << 100.0 << false
            << 0 << 3 << ListRange();

    QTest::newRow("insert 1 at start")
            << 30 << 0.0 << true
            << 0 << 1 << ListRange(0, 15);
    QTest::newRow("insert multiple at start")
            << 30 << 0.0 << true
            << 0 << 3 << ListRange(0, 15);
    QTest::newRow("insert 1 at start, content y not 0")
            << 30 << 40.0 << true  // first visible is index 2, so translate the displaced indexes by 2
            << 2 << 1 << ListRange(0 + 2, 15 + 2);
    QTest::newRow("insert multiple at start, content y not 0")
            << 30 << 40.0 << true    // first visible is index 2
            << 2 << 3 << ListRange(0 + 2, 15 + 2);

    QTest::newRow("insert 1 at start, to empty list")
            << 0 << 0.0 << true
            << 0 << 1 << ListRange();
    QTest::newRow("insert multiple at start, to empty list")
            << 0 << 0.0 << true
            << 0 << 3 << ListRange();

    QTest::newRow("insert 1 at middle")
            << 30 << 0.0 << true
            << 5 << 1 << ListRange(5, 15);
    QTest::newRow("insert multiple at middle")
            << 30 << 0.0 << true
            << 5 << 3 << ListRange(5, 15);

    QTest::newRow("insert 1 at bottom")
            << 30 << 0.0 << true
            << 15 << 1 << ListRange(15, 15);
    QTest::newRow("insert multiple at bottom")
            << 30 << 0.0 << true
            << 15 << 3 << ListRange(15, 15);
    QTest::newRow("insert 1 at bottom, content y not 0")
            << 30 << 20.0 * 3 << true
            << 15 + 3 << 1 << ListRange(15 + 3, 15 + 3);
    QTest::newRow("insert multiple at bottom, content y not 0")
            << 30 << 20.0 * 3 << true
            << 15 + 3 << 3 << ListRange(15 + 3, 15 + 3);

    // items added after the last visible will not be animated in, since they
    // do not appear in the final view
    QTest::newRow("insert 1 after end")
            << 30 << 0.0 << false
            << 17 << 1 << ListRange();
    QTest::newRow("insert multiple after end")
            << 30 << 0.0 << false
            << 17 << 3 << ListRange();
}

void tst_QQuickListView::moveTransitions()
{
    QFETCH(int, initialItemCount);
    QFETCH(qreal, contentY);
    QFETCH(qreal, itemsOffsetAfterMove);
    QFETCH(int, moveFrom);
    QFETCH(int, moveTo);
    QFETCH(int, moveCount);
    QFETCH(ListRange, expectedDisplacedIndexes);

    // target and displaced items should pass through these points
    QPointF targetItems_transitionVia(-50, 50);
    QPointF displacedItems_transitionVia(100, 100);

    QaimModel model;
    for (int i = 0; i < initialItemCount; i++)
        model.addItem("Original item" + QString::number(i), "");
    QaimModel model_targetItems_transitionVia;
    QaimModel model_displacedItems_transitionVia;

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("model_targetItems_transitionVia", &model_targetItems_transitionVia);
    ctxt->setContextProperty("model_displacedItems_transitionVia", &model_displacedItems_transitionVia);
    ctxt->setContextProperty("targetItems_transitionVia", targetItems_transitionVia);
    ctxt->setContextProperty("displacedItems_transitionVia", displacedItems_transitionVia);
    ctxt->setContextProperty("testObject", testObject);
    canvas->setSource(testFileUrl("moveTransitions.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QQuickText *name;

    if (contentY != 0) {
        listview->setContentY(contentY);
        QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    }

    QList<QPair<QString,QString> > expectedDisplacedValues = expectedDisplacedIndexes.getModelDataValues(model);

    // Items moving to *or* from visible positions should be animated.
    // Otherwise, they should not be animated.
    QList<QPair<QString, QString> > expectedTargetData;
    QList<int> targetIndexes;
    for (int i=moveFrom; i<moveFrom+moveCount; i++) {
        int toIndex = moveTo + (i - moveFrom);
        if (i <= (contentY + listview->height()) / 20
                || toIndex < (contentY + listview->height()) / 20) {
            expectedTargetData << qMakePair(model.name(i), model.number(i));
            targetIndexes << i;
        }
    }
    // ViewTransition.index provides the indices that items are moving to, not from
    targetIndexes = adjustIndexesForMove(targetIndexes, moveFrom, moveTo, moveCount);

    // start animation
    model.moveItems(moveFrom, moveTo, moveCount);

    QTRY_COMPARE(listview->property("targetTransitionsDone").toInt(), expectedTargetData.count());
    QTRY_COMPARE(listview->property("displaceTransitionsDone").toInt(),
                 expectedDisplacedIndexes.isValid() ? expectedDisplacedIndexes.count() : 0);

    QList<QQuickItem *> targetItems = findItems<QQuickItem>(contentItem, "wrapper", targetIndexes);

    // check the target and displaced items were animated
    model_targetItems_transitionVia.matchAgainst(expectedTargetData, "wasn't animated from target 'from' pos", "shouldn't have been animated from target 'from' pos");
    model_displacedItems_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with displaced anim", "shouldn't have been animated with displaced anim");

    // check attached properties
    matchItemsAndIndexes(listview->property("targetTrans_items").toMap(), model, targetIndexes);
    matchIndexLists(listview->property("targetTrans_targetIndexes").toList(), targetIndexes);
    matchItemLists(listview->property("targetTrans_targetItems").toList(), targetItems);
    if (expectedDisplacedIndexes.isValid()) {
        // adjust expectedDisplacedIndexes to their final values after the move
        QList<int> displacedIndexes = adjustIndexesForMove(expectedDisplacedIndexes.indexes, moveFrom, moveTo, moveCount);
        matchItemsAndIndexes(listview->property("displacedTrans_items").toMap(), model, displacedIndexes);
        matchIndexLists(listview->property("displacedTrans_targetIndexes").toList(), targetIndexes);
        matchItemLists(listview->property("displacedTrans_targetItems").toList(), targetItems);
    }

    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    int firstVisibleIndex = -1;
    for (int i=0; i<items.count(); i++) {
        if (items[i]->y() >= contentY) {
            QDeclarativeExpression e(qmlContext(items[i]), items[i], "index");
            firstVisibleIndex = e.evaluate().toInt();
            break;
        }
    }
    QVERIFY2(firstVisibleIndex >= 0, QTest::toString(firstVisibleIndex));

    // verify all items moved to the correct final positions
    int itemCount = findItems<QQuickItem>(contentItem, "wrapper").count();
    for (int i=firstVisibleIndex; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->y(), i*20.0 + itemsOffsetAfterMove);
        name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::moveTransitions_data()
{
    QTest::addColumn<int>("initialItemCount");
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<qreal>("itemsOffsetAfterMove");
    QTest::addColumn<int>("moveFrom");
    QTest::addColumn<int>("moveTo");
    QTest::addColumn<int>("moveCount");
    QTest::addColumn<ListRange>("expectedDisplacedIndexes");

    // when removing from above the visible, all items shift down depending on how many
    // items have been removed from above the visible
    QTest::newRow("move from above view, outside visible items, move 1") << 30 << 4*20.0 << 20.0
            << 1 << 10 << 1 << ListRange(11, 15+4);
    QTest::newRow("move from above view, outside visible items, move 1 (first item)") << 30 << 4*20.0 << 20.0
            << 0 << 10 << 1 << ListRange(11, 15+4);
    QTest::newRow("move from above view, outside visible items, move multiple") << 30 << 4*20.0 << 2*20.0
            << 1 << 10 << 2 << ListRange(12, 15+4);
    QTest::newRow("move from above view, outside visible items, move multiple (first item)") << 30 << 4*20.0 << 3*20.0
            << 0 << 10 << 3 << ListRange(13, 15+4);
    QTest::newRow("move from above view, mix of visible/non-visible") << 30 << 4*20.0 << 3*20.0
            << 1 << 10 << 5 << ListRange(6, 14) + ListRange(15, 15+4);
    QTest::newRow("move from above view, mix of visible/non-visible (move first)") << 30 << 4*20.0 << 4*20.0
            << 0 << 10 << 5 << ListRange(5, 14) + ListRange(15, 15+4);

    QTest::newRow("move within view, move 1 down") << 30 << 0.0 << 0.0
            << 1 << 10 << 1 << ListRange(2, 10);
    QTest::newRow("move within view, move 1 down, move first item") << 30 << 0.0 << 0.0
            << 0 << 10 << 1 << ListRange(1, 10);
    QTest::newRow("move within view, move 1 down, move first item, contentY not 0") << 30 << 4*20.0 << 0.0
            << 0+4 << 10+4 << 1 << ListRange(1+4, 10+4);
    QTest::newRow("move within view, move 1 down, to last item") << 30 << 0.0 << 0.0
            << 10 << 15 << 1 << ListRange(11, 15);
    QTest::newRow("move within view, move first->last") << 30 << 0.0 << 0.0
            << 0 << 15 << 1 << ListRange(1, 15);

    QTest::newRow("move within view, move multiple down") << 30 << 0.0 << 0.0
            << 1 << 10 << 3 << ListRange(4, 12);
    QTest::newRow("move within view, move multiple down, move first item") << 30 << 0.0 << 0.0
            << 0 << 10 << 3 << ListRange(3, 12);
    QTest::newRow("move within view, move multiple down, move first item, contentY not 0") << 30 << 4*20.0 << 0.0
            << 0+4 << 10+4 << 3 << ListRange(3+4, 12+4);
    QTest::newRow("move within view, move multiple down, displace last item") << 30 << 0.0 << 0.0
            << 5 << 13 << 3 << ListRange(8, 15);
    QTest::newRow("move within view, move multiple down, move first->last") << 30 << 0.0 << 0.0
            << 0 << 13 << 3 << ListRange(3, 15);

    QTest::newRow("move within view, move 1 up") << 30 << 0.0 << 0.0
            << 10 << 1 << 1 << ListRange(1, 9);
    QTest::newRow("move within view, move 1 up, move to first index") << 30 << 0.0 << 0.0
            << 10 << 0 << 1 << ListRange(0, 9);
    QTest::newRow("move within view, move 1 up, move to first index, contentY not 0") << 30 << 4*20.0 << 0.0
            << 10+4 << 0+4 << 1 << ListRange(0+4, 9+4);
    QTest::newRow("move within view, move 1 up, move to first index, contentY not on item border") << 30 << 4*20.0 - 10 << 0.0
            << 10+4 << 0+4 << 1 << ListRange(0+4, 9+4);
    QTest::newRow("move within view, move 1 up, move last item") << 30 << 0.0 << 0.0
            << 15 << 10 << 1 << ListRange(10, 14);
    QTest::newRow("move within view, move 1 up, move last->first") << 30 << 0.0 << 0.0
            << 15 << 0 << 1 << ListRange(0, 14);

    QTest::newRow("move within view, move multiple up") << 30 << 0.0 << 0.0
            << 10 << 1 << 3 << ListRange(1, 9);
    QTest::newRow("move within view, move multiple up, move to first index") << 30 << 0.0 << 0.0
            << 10 << 0 << 3 << ListRange(0, 9);
    QTest::newRow("move within view, move multiple up, move to first index, contentY not 0") << 30 << 4*20.0 << 0.0
            << 10+4 << 0+4 << 3 << ListRange(0+4, 9+4);
    QTest::newRow("move within view, move multiple up, move last item") << 30 << 0.0 << 0.0
            << 13 << 5 << 3 << ListRange(5, 12);
    QTest::newRow("move within view, move multiple up, move last->first") << 30 << 0.0 << 0.0
            << 13 << 0 << 3 << ListRange(0, 12);

    QTest::newRow("move from below view, move 1 up, move to top") << 30 << 0.0 << 0.0
            << 20 << 0 << 1 << ListRange(0, 15);
    QTest::newRow("move from below view, move 1 up, move to top, contentY not 0") << 30 << 4*20.0 << 0.0
            << 25 << 4 << 1 << ListRange(0+4, 15+4);
    QTest::newRow("move from below view, move multiple up, move to top") << 30 << 0.0 << 0.0
            << 20 << 0 << 3 << ListRange(0, 15);
    QTest::newRow("move from below view, move multiple up, move to top, contentY not 0") << 30 << 4*20.0 << 0.0
            << 25 << 4 << 3 << ListRange(0+4, 15+4);

    QTest::newRow("move from below view, move 1 up, move to bottom") << 30 << 0.0 << 0.0
            << 20 << 15 << 1 << ListRange(15, 15);
    QTest::newRow("move from below view, move 1 up, move to bottom, contentY not 0") << 30 << 4*20.0 << 0.0
            << 25 << 15+4 << 1 << ListRange(15+4, 15+4);
    QTest::newRow("move from below view, move multiple up, move to to bottom") << 30 << 0.0 << 0.0
            << 20 << 15 << 3 << ListRange(15, 15);
    QTest::newRow("move from below view, move multiple up, move to bottom, contentY not 0") << 30 << 4*20.0 << 0.0
            << 25 << 15+4 << 3 << ListRange(15+4, 15+4);
}

void tst_QQuickListView::removeTransitions()
{
    QFETCH(int, initialItemCount);
    QFETCH(bool, shouldAnimateTargets);
    QFETCH(qreal, contentY);
    QFETCH(int, removalIndex);
    QFETCH(int, removalCount);
    QFETCH(ListRange, expectedDisplacedIndexes);

    // added items should end here
    QPointF targetItems_transitionTo(-50, -50);

    // displaced items should pass through this points
    QPointF displacedItems_transitionVia(100, 100);

    QaimModel model;
    for (int i = 0; i < initialItemCount; i++)
        model.addItem("Original item" + QString::number(i), "");
    QaimModel model_targetItems_transitionTo;
    QaimModel model_displacedItems_transitionVia;

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("model_targetItems_transitionTo", &model_targetItems_transitionTo);
    ctxt->setContextProperty("model_displacedItems_transitionVia", &model_displacedItems_transitionVia);
    ctxt->setContextProperty("targetItems_transitionTo", targetItems_transitionTo);
    ctxt->setContextProperty("displacedItems_transitionVia", displacedItems_transitionVia);
    ctxt->setContextProperty("testObject", testObject);
    canvas->setSource(testFileUrl("removeTransitions.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    if (contentY != 0) {
        listview->setContentY(contentY);
        QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
    }

    QList<QPair<QString,QString> > expectedDisplacedValues = expectedDisplacedIndexes.getModelDataValues(model);

    // only target items that are visible should be animated
    QList<QPair<QString, QString> > expectedTargetData;
    QList<int> targetIndexes;
    if (shouldAnimateTargets) {
        for (int i=removalIndex; i<removalIndex+removalCount; i++) {
            if (i >= contentY / 20 && i < (contentY + listview->height()) / 20) {
                expectedTargetData << qMakePair(model.name(i), model.number(i));
                targetIndexes << i;
            }
        }
        QVERIFY(expectedTargetData.count() > 0);
    }

    // calculate targetItems and expectedTargets before model changes
    QList<QQuickItem *> targetItems = findItems<QQuickItem>(contentItem, "wrapper", targetIndexes);
    QVariantMap expectedTargets;
    for (int i=0; i<targetIndexes.count(); i++)
        expectedTargets[model.name(targetIndexes[i])] = targetIndexes[i];

    // start animation
    model.removeItems(removalIndex, removalCount);
    QTRY_COMPARE(model.count(), listview->count());

    if (shouldAnimateTargets) {
        QTRY_COMPARE(listview->property("targetTransitionsDone").toInt(), expectedTargetData.count());
        QTRY_COMPARE(listview->property("displaceTransitionsDone").toInt(),
                     expectedDisplacedIndexes.isValid() ? expectedDisplacedIndexes.count() : 0);

        // check the target and displaced items were animated
        model_targetItems_transitionTo.matchAgainst(expectedTargetData, "wasn't animated to target 'to' pos", "shouldn't have been animated to target 'to' pos");
        model_displacedItems_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with displaced anim", "shouldn't have been animated with displaced anim");

        // check attached properties
        QCOMPARE(listview->property("targetTrans_items").toMap(), expectedTargets);
        matchIndexLists(listview->property("targetTrans_targetIndexes").toList(), targetIndexes);
        matchItemLists(listview->property("targetTrans_targetItems").toList(), targetItems);
        if (expectedDisplacedIndexes.isValid()) {
            // adjust expectedDisplacedIndexes to their final values after the move
            QList<int> displacedIndexes = adjustIndexesForRemoveDisplaced(expectedDisplacedIndexes.indexes, removalIndex, removalCount);
            matchItemsAndIndexes(listview->property("displacedTrans_items").toMap(), model, displacedIndexes);
            matchIndexLists(listview->property("displacedTrans_targetIndexes").toList(), targetIndexes);
            matchItemLists(listview->property("displacedTrans_targetItems").toList(), targetItems);
        }
    } else {
        QTRY_COMPARE(model_targetItems_transitionTo.count(), 0);
        QTRY_COMPARE(model_displacedItems_transitionVia.count(), 0);
    }

    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    int firstVisibleIndex = -1;
    int itemCount = items.count();

    for (int i=0; i<items.count(); i++) {
        QDeclarativeExpression e(qmlContext(items[i]), items[i], "index");
        int index = e.evaluate().toInt();
        if (firstVisibleIndex < 0 && items[i]->y() >= contentY)
            firstVisibleIndex = index;
        if (index < 0)
            itemCount--;    // exclude deleted items
    }
    QVERIFY2(firstVisibleIndex >= 0, QTest::toString(firstVisibleIndex));

    // verify all items moved to the correct final positions
    for (int i=firstVisibleIndex; i < model.count() && i < itemCount; ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QCOMPARE(item->x(), 0.0);
        QCOMPARE(item->y(), contentY + (i-firstVisibleIndex) * 20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::removeTransitions_data()
{
    QTest::addColumn<int>("initialItemCount");
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<bool>("shouldAnimateTargets");
    QTest::addColumn<int>("removalIndex");
    QTest::addColumn<int>("removalCount");
    QTest::addColumn<ListRange>("expectedDisplacedIndexes");

    // All items that are visible following the remove operation should be animated.
    // Remove targets that are outside of the view should not be animated.

    QTest::newRow("remove 1 before start")
            << 30 << 20.0 * 3 << false
            << 2 << 1 << ListRange();
    QTest::newRow("remove multiple, all before start")
            << 30 << 20.0 * 3 << false
            << 0 << 3 << ListRange();
    QTest::newRow("remove mix of before and after start")
            << 30 << 20.0 * 3 << true
            << 2 << 3 << ListRange(5, 20);  // 5-20 are visible after the remove

    QTest::newRow("remove 1 from start")
            << 30 << 0.0 << true
            << 0 << 1 << ListRange(1, 16);  // 1-16 are visible after the remove
    QTest::newRow("remove multiple from start")
            << 30 << 0.0 << true
            << 0 << 3 << ListRange(3, 18);  // 3-18 are visible after the remove
    QTest::newRow("remove 1 from start, content y not 0")
            << 30 << 20.0 * 2 << true  // first visible is index 2, so translate the displaced indexes by 2
            << 2 << 1 << ListRange(1 + 2, 16 + 2);
    QTest::newRow("remove multiple from start, content y not 0")
            << 30 << 20.0 * 2 << true    // first visible is index 2
            << 2 << 3 << ListRange(3 + 2, 18 + 2);

    QTest::newRow("remove 1 from middle")
            << 30 << 0.0 << true
            << 5 << 1 << ListRange(6, 16);
    QTest::newRow("remove multiple from middle")
            << 30 << 0.0 << true
            << 5 << 3 << ListRange(8, 18);


    QTest::newRow("remove 1 from bottom")
            << 30 << 0.0 << true
            << 15 << 1 << ListRange(16, 16);

    // remove 15, 16, 17
    // 15 will animate as the target item, 16 & 17 won't be animated since they are outside
    // the view, and 18 will be animated as the displaced item to replace the last item
    QTest::newRow("remove multiple from bottom")
            << 30 << 0.0 << true
            << 15 << 3 << ListRange(18, 18);

    QTest::newRow("remove 1 from bottom, content y not 0")
            << 30 << 20.0 * 2 << true
            << 15 + 2 << 1 << ListRange(16 + 2, 16 + 2);
    QTest::newRow("remove multiple from bottom, content y not 0")
            << 30 << 20.0 * 2 << true
            << 15 + 2 << 3 << ListRange(18 + 2, 18 + 2);


    QTest::newRow("remove 1 after end")
            << 30 << 0.0 << false
            << 17 << 1 << ListRange();
    QTest::newRow("remove multiple after end")
            << 30 << 0.0 << false
            << 17 << 3 << ListRange();
}

void tst_QQuickListView::displacedTransitions()
{
    QFETCH(bool, useDisplaced);
    QFETCH(bool, displacedEnabled);
    QFETCH(bool, useAddDisplaced);
    QFETCH(bool, addDisplacedEnabled);
    QFETCH(bool, useMoveDisplaced);
    QFETCH(bool, moveDisplacedEnabled);
    QFETCH(bool, useRemoveDisplaced);
    QFETCH(bool, removeDisplacedEnabled);
    QFETCH(ListChange, change);
    QFETCH(ListRange, expectedDisplacedIndexes);

    QaimModel model;
    for (int i = 0; i < 30; i++)
        model.addItem("Original item" + QString::number(i), "");
    QaimModel model_displaced_transitionVia;
    QaimModel model_addDisplaced_transitionVia;
    QaimModel model_moveDisplaced_transitionVia;
    QaimModel model_removeDisplaced_transitionVia;

    QPointF displaced_transitionVia(-50, -100);
    QPointF addDisplaced_transitionVia(-150, 100);
    QPointF moveDisplaced_transitionVia(50, -100);
    QPointF removeDisplaced_transitionVia(150, 100);

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject(canvas);
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("testObject", testObject);
    ctxt->setContextProperty("model_displaced_transitionVia", &model_displaced_transitionVia);
    ctxt->setContextProperty("model_addDisplaced_transitionVia", &model_addDisplaced_transitionVia);
    ctxt->setContextProperty("model_moveDisplaced_transitionVia", &model_moveDisplaced_transitionVia);
    ctxt->setContextProperty("model_removeDisplaced_transitionVia", &model_removeDisplaced_transitionVia);
    ctxt->setContextProperty("displaced_transitionVia", displaced_transitionVia);
    ctxt->setContextProperty("addDisplaced_transitionVia", addDisplaced_transitionVia);
    ctxt->setContextProperty("moveDisplaced_transitionVia", moveDisplaced_transitionVia);
    ctxt->setContextProperty("removeDisplaced_transitionVia", removeDisplaced_transitionVia);
    ctxt->setContextProperty("useDisplaced", useDisplaced);
    ctxt->setContextProperty("displacedEnabled", displacedEnabled);
    ctxt->setContextProperty("useAddDisplaced", useAddDisplaced);
    ctxt->setContextProperty("addDisplacedEnabled", addDisplacedEnabled);
    ctxt->setContextProperty("useMoveDisplaced", useMoveDisplaced);
    ctxt->setContextProperty("moveDisplacedEnabled", moveDisplacedEnabled);
    ctxt->setContextProperty("useRemoveDisplaced", useRemoveDisplaced);
    ctxt->setContextProperty("removeDisplacedEnabled", removeDisplacedEnabled);
    canvas->setSource(testFileUrl("displacedTransitions.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    QList<QPair<QString,QString> > expectedDisplacedValues = expectedDisplacedIndexes.getModelDataValues(model);
    listview->setProperty("displaceTransitionsDone", false);

    switch (change.type) {
        case ListChange::Inserted:
        {
            QList<QPair<QString, QString> > targetItemData;
            for (int i=change.index; i<change.index + change.count; ++i)
                targetItemData << qMakePair(QString("new item %1").arg(i), QString::number(i));
            model.insertItems(change.index, targetItemData);
            QTRY_COMPARE(model.count(), listview->count());
            break;
        }
        case ListChange::Removed:
            model.removeItems(change.index, change.count);
            QTRY_COMPARE(model.count(), listview->count());
            break;
        case ListChange::Moved:
            model.moveItems(change.index, change.to, change.count);
            QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
            break;
        case ListChange::SetCurrent:
        case ListChange::SetContentY:
            break;
    }
    if ((useDisplaced && displacedEnabled)
            || (useAddDisplaced && addDisplacedEnabled)
            || (useMoveDisplaced && moveDisplacedEnabled)
            || (useRemoveDisplaced && removeDisplacedEnabled)) {
        QTRY_VERIFY(listview->property("displaceTransitionsDone").toBool());
    }

    if (change.type == ListChange::Inserted && useAddDisplaced && addDisplacedEnabled)
        model_addDisplaced_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with add displaced", "shouldn't have been animated with add displaced");
    else
        QCOMPARE(model_addDisplaced_transitionVia.count(), 0);
    if (change.type == ListChange::Moved && useMoveDisplaced && moveDisplacedEnabled)
        model_moveDisplaced_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with move displaced", "shouldn't have been animated with move displaced");
    else
        QCOMPARE(model_moveDisplaced_transitionVia.count(), 0);
    if (change.type == ListChange::Removed && useRemoveDisplaced && removeDisplacedEnabled)
        model_removeDisplaced_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with remove displaced", "shouldn't have been animated with remove displaced");
    else
        QCOMPARE(model_removeDisplaced_transitionVia.count(), 0);

    if (useDisplaced && displacedEnabled
            && ( (change.type == ListChange::Inserted && (!useAddDisplaced || !addDisplacedEnabled))
                 || (change.type == ListChange::Moved && (!useMoveDisplaced || !moveDisplacedEnabled))
                 || (change.type == ListChange::Removed && (!useRemoveDisplaced || !removeDisplacedEnabled))) ) {
        model_displaced_transitionVia.matchAgainst(expectedDisplacedValues, "wasn't animated with generic displaced", "shouldn't have been animated with generic displaced");
    } else {
        QCOMPARE(model_displaced_transitionVia.count(), 0);
    }

    // verify all items moved to the correct final positions
    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    for (int i=0; i < model.count() && i < items.count(); ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QCOMPARE(item->x(), 0.0);
        QCOMPARE(item->y(), i * 20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
}

void tst_QQuickListView::displacedTransitions_data()
{
    QTest::addColumn<bool>("useDisplaced");
    QTest::addColumn<bool>("displacedEnabled");
    QTest::addColumn<bool>("useAddDisplaced");
    QTest::addColumn<bool>("addDisplacedEnabled");
    QTest::addColumn<bool>("useMoveDisplaced");
    QTest::addColumn<bool>("moveDisplacedEnabled");
    QTest::addColumn<bool>("useRemoveDisplaced");
    QTest::addColumn<bool>("removeDisplacedEnabled");
    QTest::addColumn<ListChange>("change");
    QTest::addColumn<ListRange>("expectedDisplacedIndexes");

    QTest::newRow("no displaced transitions at all")
            << false << false
            << false << false
            << false << false
            << false << false
            << ListChange::insert(0, 1) << ListRange(0, 15);

    QTest::newRow("just displaced")
            << true << true
            << false << false
            << false << false
            << false << false
            << ListChange::insert(0, 1) << ListRange(0, 15);

    QTest::newRow("just displaced (not enabled)")
            << true << false
            << false << false
            << false << false
            << false << false
            << ListChange::insert(0, 1) << ListRange(0, 15);

    QTest::newRow("displaced + addDisplaced")
            << true << true
            << true << true
            << false << false
            << false << false
            << ListChange::insert(0, 1) << ListRange(0, 15);

    QTest::newRow("displaced + addDisplaced (not enabled)")
            << true << true
            << true << false
            << false << false
            << false << false
            << ListChange::insert(0, 1) << ListRange(0, 15);

    QTest::newRow("displaced + moveDisplaced")
            << true << true
            << false << false
            << true << true
            << false << false
            << ListChange::move(0, 10, 1) << ListRange(1, 10);

    QTest::newRow("displaced + moveDisplaced (not enabled)")
            << true << true
            << false << false
            << true << false
            << false << false
            << ListChange::move(0, 10, 1) << ListRange(1, 10);

    QTest::newRow("displaced + removeDisplaced")
            << true << true
            << false << false
            << false << false
            << true << true
            << ListChange::remove(0, 1) << ListRange(1, 16);

    QTest::newRow("displaced + removeDisplaced (not enabled)")
            << true << true
            << false << false
            << false << false
            << true << false
            << ListChange::remove(0, 1) << ListRange(1, 16);


    QTest::newRow("displaced + add, should use generic displaced for a remove")
            << true << true
            << true << true
            << false << false
            << true << false
            << ListChange::remove(0, 1) << ListRange(1, 16);
}

void tst_QQuickListView::multipleTransitions()
{
    QSKIP("QTBUG-24523");

    // Tests that if you interrupt a transition in progress with another action that
    // cancels the previous transition, the resulting items are still placed correctly.

    QFETCH(int, initialCount);
    QFETCH(qreal, contentY);
    QFETCH(QList<ListChange>, changes);

    // add transitions on the left, moves on the right
    QPointF addTargets_transitionFrom(-50, -50);
    QPointF addDisplaced_transitionFrom(-50, 50);
    QPointF moveTargets_transitionFrom(50, -50);
    QPointF moveDisplaced_transitionFrom(50, 50);

    QmlListModel model;
    for (int i = 0; i < initialCount; i++)
        model.addItem("Original item" + QString::number(i), "");

    QQuickView *canvas = createView();
    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testModel", &model);
    ctxt->setContextProperty("testObject", testObject);
    ctxt->setContextProperty("addTargets_transitionFrom", addTargets_transitionFrom);
    ctxt->setContextProperty("addDisplaced_transitionFrom", addDisplaced_transitionFrom);
    ctxt->setContextProperty("moveTargets_transitionFrom", moveTargets_transitionFrom);
    ctxt->setContextProperty("moveDisplaced_transitionFrom", moveDisplaced_transitionFrom);
    canvas->setSource(testFileUrl("multipleTransitions.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickListView *listview = findItem<QQuickListView>(canvas->rootObject(), "list");
    QTRY_VERIFY(listview != 0);
    QQuickItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);
    QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);

    int timeBetweenActions = canvas->rootObject()->property("timeBetweenActions").toInt();

    QList<QPair<QString, QString> > targetItems;
    for (int i=0; i<changes.count(); i++) {
        switch (changes[i].type) {
            case ListChange::Inserted:
            {
                for (int j=changes[i].index; j<changes[i].index + changes[i].count; ++j)
                    targetItems << qMakePair(QString("new item %1").arg(j), QString::number(j));
                model.insertItems(changes[i].index, targetItems);
                QTRY_COMPARE(model.count(), listview->count());
                QTRY_VERIFY(listview->property("runningAddTargets").toBool());
                QTRY_VERIFY(listview->property("runningAddDisplaced").toBool());
                if (i == changes.count() - 1) {
                    QTRY_VERIFY(!listview->property("runningAddTargets").toBool());
                    QTRY_VERIFY(!listview->property("runningAddDisplaced").toBool());
                } else {
                    QTest::qWait(timeBetweenActions);
                }
                break;
            }
            case ListChange::Removed:
                for (int j=changes[i].index; j<changes[i].index + changes[i].count; ++j)
                    targetItems << qMakePair(model.name(i), model.number(i));
                model.removeItems(changes[i].index, changes[i].count);
                QTRY_COMPARE(model.count(), listview->count());
                QTRY_VERIFY(listview->property("runningRemoveTargets").toBool());
                QTRY_VERIFY(listview->property("runningRemoveDisplaced").toBool());
                if (i == changes.count() - 1) {
                    QTRY_VERIFY(!listview->property("runningRemoveTargets").toBool());
                    QTRY_VERIFY(!listview->property("runningRemoveDisplaced").toBool());
                } else {
                    QTest::qWait(timeBetweenActions);
                }
                break;
            case ListChange::Moved:
                for (int j=changes[i].index; j<changes[i].index + changes[i].count; ++j)
                    targetItems << qMakePair(model.name(i), model.number(i));
                model.moveItems(changes[i].index, changes[i].to, changes[i].count);
                QTRY_VERIFY(listview->property("runningMoveTargets").toBool());
                QTRY_VERIFY(listview->property("runningMoveDisplaced").toBool());
                if (i == changes.count() - 1) {
                    QTRY_VERIFY(!listview->property("runningMoveTargets").toBool());
                    QTRY_VERIFY(!listview->property("runningMoveDisplaced").toBool());
                } else {
                    QTest::qWait(timeBetweenActions);
                }
                break;
            case ListChange::SetCurrent:
                listview->setCurrentIndex(changes[i].index);
                break;
            case ListChange::SetContentY:
                listview->setContentY(changes[i].pos);
                QTRY_COMPARE(QQuickItemPrivate::get(listview)->polishScheduled, false);
                break;
        }
    }
    QCOMPARE(listview->count(), model.count());

    // verify all items moved to the correct final positions
    QList<QQuickItem*> items = findItems<QQuickItem>(contentItem, "wrapper");
    for (int i=0; i < model.count() && i < items.count(); ++i) {
        QQuickItem *item = findItem<QQuickItem>(contentItem, "wrapper", i);
        QVERIFY2(item, QTest::toString(QString("Item %1 not found").arg(i)));
        QTRY_COMPARE(item->x(), 0.0);
        QTRY_COMPARE(item->y(), i*20.0);
        QQuickText *name = findItem<QQuickText>(contentItem, "textName", i);
        QVERIFY(name != 0);
        QTRY_COMPARE(name->text(), model.name(i));
    }

    delete canvas;
    delete testObject;
}

void tst_QQuickListView::multipleTransitions_data()
{
    QTest::addColumn<int>("initialCount");
    QTest::addColumn<qreal>("contentY");
    QTest::addColumn<QList<ListChange> >("changes");

    // the added item and displaced items should move to final dest correctly
    QTest::newRow("add item, then move it immediately") << 10 << 0.0 << (QList<ListChange>()
            << ListChange::insert(0, 1)
            << ListChange::move(0, 3, 1)
            );

    // items affected by the add should change from move to add transition
    QTest::newRow("move, then insert item before the moved item") << 20 << 0.0 << (QList<ListChange>()
            << ListChange::move(1, 10, 3)
            << ListChange::insert(0, 1)
            );

    // items should be placed correctly if you trigger a transition then refill for that index
    QTest::newRow("add at 0, flick down, flick back to top and add at 0 again") << 20 << 0.0 << (QList<ListChange>()
            << ListChange::insert(0, 1)
            << ListChange::setContentY(80.0)
            << ListChange::setContentY(0.0)
            << ListChange::insert(0, 1)
            );
}

QList<int> tst_QQuickListView::toIntList(const QVariantList &list)
{
    QList<int> ret;
    bool ok = true;
    for (int i=0; i<list.count(); i++) {
        ret << list[i].toInt(&ok);
        if (!ok)
            qWarning() << "tst_QQuickListView::toIntList(): not a number:" << list[i];
    }

    return ret;
}

void tst_QQuickListView::matchIndexLists(const QVariantList &indexLists, const QList<int> &expectedIndexes)
{
    for (int i=0; i<indexLists.count(); i++) {
        QSet<int> current = indexLists[i].value<QList<int> >().toSet();
        if (current != expectedIndexes.toSet())
            qDebug() << "Cannot match actual targets" << current << "with expected" << expectedIndexes;
        QCOMPARE(current, expectedIndexes.toSet());
    }
}

void tst_QQuickListView::matchItemsAndIndexes(const QVariantMap &items, const QaimModel &model, const QList<int> &expectedIndexes)
{
    for (QVariantMap::const_iterator it = items.begin(); it != items.end(); ++it) {
        QVERIFY(it.value().type() == QVariant::Int);
        QString name = it.key();
        int itemIndex = it.value().toInt();
        QVERIFY2(expectedIndexes.contains(itemIndex), QTest::toString(QString("Index %1 not found in expectedIndexes").arg(itemIndex)));
        if (model.name(itemIndex) != name)
            qDebug() << itemIndex;
        QCOMPARE(model.name(itemIndex), name);
    }
    QCOMPARE(items.count(), expectedIndexes.count());
}

void tst_QQuickListView::matchItemLists(const QVariantList &itemLists, const QList<QQuickItem *> &expectedItems)
{
    for (int i=0; i<itemLists.count(); i++) {
        QVERIFY(itemLists[i].type() == QVariant::List);
        QVariantList current = itemLists[i].toList();
        for (int j=0; j<current.count(); j++) {
            QQuickItem *o = qobject_cast<QQuickItem*>(current[j].value<QObject*>());
            QVERIFY2(o, QTest::toString(QString("Invalid actual item at %1").arg(j)));
            QVERIFY2(expectedItems.contains(o), QTest::toString(QString("Cannot match item %1").arg(j)));
        }
        QCOMPARE(current.count(), expectedItems.count());
    }
}


QTEST_MAIN(tst_QQuickListView)

#include "tst_qquicklistview.moc"

