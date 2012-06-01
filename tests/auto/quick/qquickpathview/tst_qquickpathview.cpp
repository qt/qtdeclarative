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
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQuick/private/qquickpathview_p.h>
#include <QtQuick/private/qquickpath_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQml/private/qquicklistmodel_p.h>
#include <QtQml/private/qqmlvaluetype_p.h>
#include <QStringListModel>
#include <QFile>

#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

Q_DECLARE_METATYPE(QQuickPathView::HighlightRangeMode)

#ifndef QT_NO_WIDGETS
#include <QStandardItemModel>
static void initStandardTreeModel(QStandardItemModel *model)
{
    QStandardItem *item;
    item = new QStandardItem(QLatin1String("Row 1 Item"));
    model->insertRow(0, item);

    item = new QStandardItem(QLatin1String("Row 2 Item"));
    item->setCheckable(true);
    model->insertRow(1, item);

    QStandardItem *childItem = new QStandardItem(QLatin1String("Row 2 Child Item"));
    item->setChild(0, childItem);

    item = new QStandardItem(QLatin1String("Row 3 Item"));
    item->setIcon(QIcon());
    model->insertRow(2, item);
}
#endif


class tst_QQuickPathView : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickPathView();

private slots:
    void initValues();
    void items();
    void dataModel();
    void pathview2();
    void pathview3();
    void insertModel_data();
    void insertModel();
    void removeModel_data();
    void removeModel();
    void moveModel_data();
    void moveModel();
    void consecutiveModelChanges_data();
    void consecutiveModelChanges();
    void path();
    void pathMoved();
    void setCurrentIndex();
    void resetModel();
    void propertyChanges();
    void pathChanges();
    void componentChanges();
    void modelChanges();
    void pathUpdateOnStartChanged();
    void package();
    void emptyModel();
    void closed();
    void pathUpdate();
    void visualDataModel();
    void undefinedPath();
    void mouseDrag();
#ifndef QT_NO_WIDGETS
    void treeModel();
#endif
    void changePreferredHighlight();
    void missingPercent();
    void creationContext();
    void currentOffsetOnInsertion();
    void asynchronous();
    void cancelDrag();
    void maximumFlickVelocity();
    void snapToItem();
    void snapToItem_data();
    void snapOneItem();
    void snapOneItem_data();
};

class TestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool error READ error WRITE setError)
    Q_PROPERTY(bool useModel READ useModel NOTIFY useModelChanged)
    Q_PROPERTY(int pathItemCount READ pathItemCount NOTIFY pathItemCountChanged)

public:
    TestObject() : QObject(), mError(true), mUseModel(true), mPathItemCount(-1) {}

    bool error() const { return mError; }
    void setError(bool err) { mError = err; }

    bool useModel() const { return mUseModel; }
    void setUseModel(bool use) { mUseModel = use; emit useModelChanged(); }

    int pathItemCount() const { return mPathItemCount; }
    void setPathItemCount(int count) { mPathItemCount = count; emit pathItemCountChanged(); }

signals:
    void useModelChanged();
    void pathItemCountChanged();

private:
    bool mError;
    bool mUseModel;
    int mPathItemCount;
};

tst_QQuickPathView::tst_QQuickPathView()
{
}

void tst_QQuickPathView::initValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathview1.qml"));
    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->path() == 0);
    QVERIFY(obj->delegate() == 0);
    QCOMPARE(obj->model(), QVariant());
    QCOMPARE(obj->currentIndex(), 0);
    QCOMPARE(obj->offset(), 0.);
    QCOMPARE(obj->preferredHighlightBegin(), 0.);
    QCOMPARE(obj->dragMargin(), 0.);
    QCOMPARE(obj->count(), 0);
    QCOMPARE(obj->pathItemCount(), -1);

    delete obj;
}

void tst_QQuickPathView::items()
{
    QQuickView *canvas = createView();

    QaimModel model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QCOMPARE(pathview->count(), model.count());
    QCOMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QCOMPARE(pathview->childItems().count(), model.count()+1); // assumes all are visible, including highlight

    for (int i = 0; i < model.count(); ++i) {
        QQuickText *name = findItem<QQuickText>(pathview, "textName", i);
        QVERIFY(name != 0);
        QCOMPARE(name->text(), model.name(i));
        QQuickText *number = findItem<QQuickText>(pathview, "textNumber", i);
        QVERIFY(number != 0);
        QCOMPARE(number->text(), model.number(i));
    }

    QQuickPath *path = qobject_cast<QQuickPath*>(pathview->path());
    QVERIFY(path);

    QVERIFY(pathview->highlightItem());
    QPointF start = path->pointAt(0.0);
    QPointF offset;
    offset.setX(pathview->highlightItem()->width()/2);
    offset.setY(pathview->highlightItem()->height()/2);
    QCOMPARE(pathview->highlightItem()->pos() + offset, start);

    delete canvas;
}

void tst_QQuickPathView::pathview2()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathview2.qml"));
    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->path() != 0);
    QVERIFY(obj->delegate() != 0);
    QVERIFY(obj->model() != QVariant());
    QCOMPARE(obj->currentIndex(), 0);
    QCOMPARE(obj->offset(), 0.);
    QCOMPARE(obj->preferredHighlightBegin(), 0.);
    QCOMPARE(obj->dragMargin(), 0.);
    QCOMPARE(obj->count(), 8);
    QCOMPARE(obj->pathItemCount(), 10);

    delete obj;
}

void tst_QQuickPathView::pathview3()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathview3.qml"));
    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->path() != 0);
    QVERIFY(obj->delegate() != 0);
    QVERIFY(obj->model() != QVariant());
    QCOMPARE(obj->currentIndex(), 0);
    QCOMPARE(obj->offset(), 1.0);
    QCOMPARE(obj->preferredHighlightBegin(), 0.5);
    QCOMPARE(obj->dragMargin(), 24.);
    QCOMPARE(obj->count(), 8);
    QCOMPARE(obj->pathItemCount(), 4);

    delete obj;
}

void tst_QQuickPathView::insertModel_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("idx");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<int>("currentIndex");

    // We have 8 items, with currentIndex == 4
    QTest::newRow("insert after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 6 << 1 << 5. << 4;
    QTest::newRow("insert before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 1 << 4. << 5;
    QTest::newRow("insert multiple after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 2 << 6. << 4;
    QTest::newRow("insert multiple before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 1 << 2 << 4. << 6;
    QTest::newRow("insert at end")
        << int(QQuickPathView::StrictlyEnforceRange) << 8 << 1 << 5. << 4;
    QTest::newRow("insert at beginning")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 1 << 4. << 5;
    QTest::newRow("insert at current")
        << int(QQuickPathView::StrictlyEnforceRange) << 4 << 1 << 4. << 5;

    QTest::newRow("no range - insert after current")
        << int(QQuickPathView::NoHighlightRange) << 6 << 1 << 5. << 4;
    QTest::newRow("no range - insert before current")
        << int(QQuickPathView::NoHighlightRange) << 2 << 1 << 4. << 5;
    QTest::newRow("no range - insert multiple after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 2 << 6. << 4;
    QTest::newRow("no range - insert multiple before current")
        << int(QQuickPathView::NoHighlightRange) << 1 << 2 << 4. << 6;
    QTest::newRow("no range - insert at end")
        << int(QQuickPathView::NoHighlightRange) << 8 << 1 << 5. << 4;
    QTest::newRow("no range - insert at beginning")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 4. << 5;
    QTest::newRow("no range - insert at current")
        << int(QQuickPathView::NoHighlightRange) << 4 << 1 << 4. << 5;
}

void tst_QQuickPathView::insertModel()
{
    QFETCH(int, mode);
    QFETCH(int, idx);
    QFETCH(int, count);
    QFETCH(qreal, offset);
    QFETCH(int, currentIndex);

    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode((QQuickPathView::HighlightRangeMode)mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    QList<QPair<QString, QString> > items;
    for (int i = 0; i < count; ++i)
        items.append(qMakePair(QString("New"), QString::number(i)));

    model.insertItems(idx, items);
    QTRY_COMPARE(pathview->offset(), offset);

    QCOMPARE(pathview->currentIndex(), currentIndex);

    delete canvas;
}

void tst_QQuickPathView::removeModel_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("idx");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<int>("currentIndex");

    // We have 8 items, with currentIndex == 4
    QTest::newRow("remove after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 6 << 1 << 3. << 4;
    QTest::newRow("remove before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 1 << 4. << 3;
    QTest::newRow("remove multiple after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 2 << 2. << 4;
    QTest::newRow("remove multiple before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 1 << 2 << 4. << 2;
    QTest::newRow("remove last")
        << int(QQuickPathView::StrictlyEnforceRange) << 7 << 1 << 3. << 4;
    QTest::newRow("remove first")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 1 << 4. << 3;
    QTest::newRow("remove current")
        << int(QQuickPathView::StrictlyEnforceRange) << 4 << 1 << 3. << 4;
    QTest::newRow("remove all")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 8 << 0. << 0;

    QTest::newRow("no range - remove after current")
        << int(QQuickPathView::NoHighlightRange) << 6 << 1 << 3. << 4;
    QTest::newRow("no range - remove before current")
        << int(QQuickPathView::NoHighlightRange) << 2 << 1 << 4. << 3;
    QTest::newRow("no range - remove multiple after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 2 << 2. << 4;
    QTest::newRow("no range - remove multiple before current")
        << int(QQuickPathView::NoHighlightRange) << 1 << 2 << 4. << 2;
    QTest::newRow("no range - remove last")
        << int(QQuickPathView::NoHighlightRange) << 7 << 1 << 3. << 4;
    QTest::newRow("no range - remove first")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 4. << 3;
    QTest::newRow("no range - remove current offset")
        << int(QQuickPathView::NoHighlightRange) << 4 << 1 << 4. << 4;
    QTest::newRow("no range - remove all")
        << int(QQuickPathView::NoHighlightRange) << 0 << 8 << 0. << 0;
}

void tst_QQuickPathView::removeModel()
{
    QFETCH(int, mode);
    QFETCH(int, idx);
    QFETCH(int, count);
    QFETCH(qreal, offset);
    QFETCH(int, currentIndex);

    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode((QQuickPathView::HighlightRangeMode)mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    model.removeItems(idx, count);
    QTRY_COMPARE(pathview->offset(), offset);

    QCOMPARE(pathview->currentIndex(), currentIndex);

    delete canvas;
}


void tst_QQuickPathView::moveModel_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<int>("currentIndex");

    // We have 8 items, with currentIndex == 4
    QTest::newRow("move after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 6 << 1 << 4. << 4;
    QTest::newRow("move before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 3 << 1 << 4. << 4;
    QTest::newRow("move before current to after")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 6 << 1 << 5. << 3;
    QTest::newRow("move multiple after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 6 << 2 << 4. << 4;
    QTest::newRow("move multiple before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 1 << 2 << 4. << 4;
    QTest::newRow("move before current to end")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 7 << 1 << 5. << 3;
    QTest::newRow("move last to beginning")
        << int(QQuickPathView::StrictlyEnforceRange) << 7 << 0 << 1 << 3. << 5;
    QTest::newRow("move current")
        << int(QQuickPathView::StrictlyEnforceRange) << 4 << 6 << 1 << 2. << 6;

    QTest::newRow("no range - move after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 6 << 1 << 4. << 4;
    QTest::newRow("no range - move before current")
        << int(QQuickPathView::NoHighlightRange) << 2 << 3 << 1 << 4. << 4;
    QTest::newRow("no range - move before current to after")
        << int(QQuickPathView::NoHighlightRange) << 2 << 6 << 1 << 5. << 3;
    QTest::newRow("no range - move multiple after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 6 << 2 << 4. << 4;
    QTest::newRow("no range - move multiple before current")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 2 << 4. << 4;
    QTest::newRow("no range - move before current to end")
        << int(QQuickPathView::NoHighlightRange) << 2 << 7 << 1 << 5. << 3;
    QTest::newRow("no range - move last to beginning")
        << int(QQuickPathView::NoHighlightRange) << 7 << 0 << 1 << 3. << 5;
    QTest::newRow("no range - move current")
        << int(QQuickPathView::NoHighlightRange) << 4 << 6 << 1 << 4. << 6;
    QTest::newRow("no range - move multiple incl. current")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 5 << 4. << 5;
}

void tst_QQuickPathView::moveModel()
{
    QFETCH(int, mode);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(int, count);
    QFETCH(qreal, offset);
    QFETCH(int, currentIndex);

    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode((QQuickPathView::HighlightRangeMode)mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    model.moveItems(from, to, count);
    QTRY_COMPARE(pathview->offset(), offset);

    QCOMPARE(pathview->currentIndex(), currentIndex);

    delete canvas;
}

void tst_QQuickPathView::consecutiveModelChanges_data()
{
    QTest::addColumn<QQuickPathView::HighlightRangeMode>("mode");
    QTest::addColumn<QList<ListChange> >("changes");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<int>("currentIndex");

    QTest::newRow("no range - insert after, insert before")
            << QQuickPathView::NoHighlightRange
            << (QList<ListChange>()
                << ListChange::insert(7, 2)
                << ListChange::insert(1, 3))
            << 13
            << 6.
            << 7;
    QTest::newRow("no range - remove after, remove before")
            << QQuickPathView::NoHighlightRange
            << (QList<ListChange>()
                << ListChange::remove(6, 2)
                << ListChange::remove(1, 3))
            << 3
            << 2.
            << 1;

    QTest::newRow("no range - remove after, insert before")
            << QQuickPathView::NoHighlightRange
            << (QList<ListChange>()
                << ListChange::remove(5, 2)
                << ListChange::insert(1, 3))
            << 9
            << 2.
            << 7;

    QTest::newRow("no range - insert after, remove before")
            << QQuickPathView::NoHighlightRange
            << (QList<ListChange>()
                << ListChange::insert(6, 2)
                << ListChange::remove(1, 3))
            << 7
            << 6.
            << 1;

    QTest::newRow("no range - insert, remove all, polish, insert")
            << QQuickPathView::NoHighlightRange
            << (QList<ListChange>()
                << ListChange::insert(3, 1)
                << ListChange::remove(0, 9)
                << ListChange::polish()
                << ListChange::insert(0, 3))
            << 3
            << 0.
            << 0;
}

void tst_QQuickPathView::consecutiveModelChanges()
{
    QFETCH(QQuickPathView::HighlightRangeMode, mode);
    QFETCH(QList<ListChange>, changes);
    QFETCH(int, count);
    QFETCH(qreal, offset);
    QFETCH(int, currentIndex);

    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode(mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    for (int i=0; i<changes.count(); i++) {
        switch (changes[i].type) {
            case ListChange::Inserted:
            {
                QList<QPair<QString, QString> > items;
                for (int j=changes[i].index; j<changes[i].index + changes[i].count; ++j)
                    items << qMakePair(QString("new item %1").arg(j), QString::number(j));
                model.insertItems(changes[i].index, items);
                break;
            }
            case ListChange::Removed:
                model.removeItems(changes[i].index, changes[i].count);
                break;
            case ListChange::Moved:
                model.moveItems(changes[i].index, changes[i].to, changes[i].count);
                break;
            case ListChange::SetCurrent:
                pathview->setCurrentIndex(changes[i].index);
                break;
        case ListChange::Polish:
                QQUICK_VERIFY_POLISH(pathview);
                break;
            default:
                continue;
        }
    }
    QQUICK_VERIFY_POLISH(pathview);

    QCOMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), count);
    QCOMPARE(pathview->count(), count);
    QTRY_COMPARE(pathview->offset(), offset);

    QCOMPARE(pathview->currentIndex(), currentIndex);

    delete canvas;
}

void tst_QQuickPathView::path()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathtest.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->startX(), 120.);
    QCOMPARE(obj->startY(), 100.);
    QVERIFY(obj->path() != QPainterPath());

    QQmlListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 5);

    QQuickPathAttribute* attr = qobject_cast<QQuickPathAttribute*>(list.at(0));
    QVERIFY(attr != 0);
    QCOMPARE(attr->name(), QString("scale"));
    QCOMPARE(attr->value(), 1.0);

    QQuickPathQuad* quad = qobject_cast<QQuickPathQuad*>(list.at(1));
    QVERIFY(quad != 0);
    QCOMPARE(quad->x(), 120.);
    QCOMPARE(quad->y(), 25.);
    QCOMPARE(quad->controlX(), 260.);
    QCOMPARE(quad->controlY(), 75.);

    QQuickPathPercent* perc = qobject_cast<QQuickPathPercent*>(list.at(2));
    QVERIFY(perc != 0);
    QCOMPARE(perc->value(), 0.3);

    QQuickPathLine* line = qobject_cast<QQuickPathLine*>(list.at(3));
    QVERIFY(line != 0);
    QCOMPARE(line->x(), 120.);
    QCOMPARE(line->y(), 100.);

    QQuickPathCubic* cubic = qobject_cast<QQuickPathCubic*>(list.at(4));
    QVERIFY(cubic != 0);
    QCOMPARE(cubic->x(), 180.);
    QCOMPARE(cubic->y(), 0.);
    QCOMPARE(cubic->control1X(), -10.);
    QCOMPARE(cubic->control1Y(), 90.);
    QCOMPARE(cubic->control2X(), 210.);
    QCOMPARE(cubic->control2Y(), 90.);

    delete obj;
}

void tst_QQuickPathView::dataModel()
{
    QQuickView *canvas = createView();
    canvas->show();

    QQmlContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    QaimModel model;
    model.addItem("red", "1");
    model.addItem("green", "2");
    model.addItem("blue", "3");
    model.addItem("purple", "4");
    model.addItem("gray", "5");
    model.addItem("brown", "6");
    model.addItem("yellow", "7");
    model.addItem("thistle", "8");
    model.addItem("cyan", "9");
    model.addItem("peachpuff", "10");
    model.addItem("powderblue", "11");
    model.addItem("gold", "12");
    model.addItem("sandybrown", "13");

    ctxt->setContextProperty("testData", &model);

    canvas->setSource(testFileUrl("datamodel.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QVERIFY(testObject->error() == false);

    QQuickItem *item = findItem<QQuickItem>(pathview, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->x(), 110.0);
    QCOMPARE(item->y(), 10.0);

    model.insertItem(4, "orange", "10");
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("viewCount").toInt(), model.count());
    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 14);

    QVERIFY(pathview->currentIndex() == 0);
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 0));

    QQuickText *text = findItem<QQuickText>(pathview, "myText", 4);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(4));

    model.removeItem(2);
    QCOMPARE(canvas->rootObject()->property("viewCount").toInt(), model.count());
    text = findItem<QQuickText>(pathview, "myText", 2);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(2));
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 0));

    testObject->setPathItemCount(5);
    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QVERIFY(testObject->error() == false);

    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);

    QQuickRectangle *testItem = findItem<QQuickRectangle>(pathview, "wrapper", 4);
    QVERIFY(testItem != 0);
    testItem = findItem<QQuickRectangle>(pathview, "wrapper", 5);
    QVERIFY(testItem == 0);

    pathview->setCurrentIndex(1);
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));
    QTest::qWait(100);

    model.insertItem(2, "pink", "2");
    QTest::qWait(100);

    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);
    QVERIFY(pathview->currentIndex() == 1);
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));

    text = findItem<QQuickText>(pathview, "myText", 2);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(2));

    model.removeItem(3);
    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);
    text = findItem<QQuickText>(pathview, "myText", 3);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(3));
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));

    model.moveItem(3, 5);
    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);
    QList<QQuickItem*> items = findItems<QQuickItem>(pathview, "wrapper");
    foreach (QQuickItem *item, items) {
        QVERIFY(item->property("onPath").toBool());
    }
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));

    // QTBUG-14199
    pathview->setOffset(7);
    pathview->setOffset(0);
    QCOMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);

    pathview->setCurrentIndex(model.count()-1);
    model.removeItem(model.count()-1);
    QCOMPARE(pathview->currentIndex(), model.count()-1);

    delete canvas;
    delete testObject;
}

void tst_QQuickPathView::pathMoved()
{
    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QQuickPath *path = qobject_cast<QQuickPath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.0);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    pathview->setOffset(1.0);

    for (int i=0; i<model.count(); i++) {
        QQuickRectangle *curItem = findItem<QQuickRectangle>(pathview, "wrapper", i);
        QPointF itemPos(path->pointAt(0.25 + i*0.25));
        QCOMPARE(curItem->pos() + offset, QPointF(itemPos.x(), itemPos.y()));
    }

    pathview->setOffset(0.0);
    QCOMPARE(firstItem->pos() + offset, start);

    // Change delegate size
    pathview->setOffset(0.1);
    pathview->setOffset(0.0);
    canvas->rootObject()->setProperty("delegateWidth", 30);
    QCOMPARE(firstItem->width(), 30.0);
    offset.setX(firstItem->width()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);

    // Change delegate scale
    pathview->setOffset(0.1);
    pathview->setOffset(0.0);
    canvas->rootObject()->setProperty("delegateScale", 1.2);
    QTRY_COMPARE(firstItem->pos() + offset, start);

    delete canvas;
}

void tst_QQuickPathView::setCurrentIndex()
{
    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QQuickPath *path = qobject_cast<QQuickPath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.0);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QCOMPARE(firstItem->pos() + offset, start);
    QCOMPARE(canvas->rootObject()->property("currentA").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("currentB").toInt(), 0);

    pathview->setCurrentIndex(2);

    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 2);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(canvas->rootObject()->property("currentA").toInt(), 2);
    QCOMPARE(canvas->rootObject()->property("currentB").toInt(), 2);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 1);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 1);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 3);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 3);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->incrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    // Test positive indexes are wrapped.
    pathview->setCurrentIndex(6);
    QTRY_COMPARE(pathview->currentIndex(), 2);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 2);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    // Test negative indexes are wrapped.
    pathview->setCurrentIndex(-3);
    QTRY_COMPARE(pathview->currentIndex(), 1);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 1);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    // move an item, set move duration to 0, and change currentIndex to moved item. QTBUG-22786
    model.moveItem(0, 3);
    pathview->setHighlightMoveDuration(0);
    pathview->setCurrentIndex(3);
    QCOMPARE(pathview->currentIndex(), 3);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 3);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    model.moveItem(3, 0);
    pathview->setCurrentIndex(0);
    pathview->setHighlightMoveDuration(300);

    // Check the current item is still created when outside the bounds of pathItemCount.
    pathview->setPathItemCount(2);
    pathview->setHighlightRangeMode(QQuickPathView::NoHighlightRange);
    QVERIFY(findItem<QQuickRectangle>(pathview, "wrapper", 0));
    QVERIFY(findItem<QQuickRectangle>(pathview, "wrapper", 1));
    QVERIFY(!findItem<QQuickRectangle>(pathview, "wrapper", 2));
    QVERIFY(!findItem<QQuickRectangle>(pathview, "wrapper", 3));

    pathview->setCurrentIndex(2);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 2);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(false));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 1);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 1);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 3);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 3);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(false));

    pathview->incrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    delete canvas;
}

void tst_QQuickPathView::resetModel()
{
    QQuickView *canvas = createView();

    QStringList strings;
    strings << "one" << "two" << "three";
    QStringListModel model(strings);

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("displaypath.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QCOMPARE(pathview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QQuickText *display = findItem<QQuickText>(pathview, "displayText", i);
        QVERIFY(display != 0);
        QCOMPARE(display->text(), strings.at(i));
    }

    strings.clear();
    strings << "four" << "five" << "six" << "seven";
    model.setStringList(strings);

    QCOMPARE(pathview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QQuickText *display = findItem<QQuickText>(pathview, "displayText", i);
        QVERIFY(display != 0);
        QCOMPARE(display->text(), strings.at(i));
    }

    delete canvas;
}

void tst_QQuickPathView::propertyChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QSignalSpy snapPositionSpy(pathView, SIGNAL(preferredHighlightBeginChanged()));
    QSignalSpy dragMarginSpy(pathView, SIGNAL(dragMarginChanged()));

    QCOMPARE(pathView->preferredHighlightBegin(), 0.1);
    QCOMPARE(pathView->dragMargin(), 5.0);

    pathView->setPreferredHighlightBegin(0.4);
    pathView->setPreferredHighlightEnd(0.4);
    pathView->setDragMargin(20.0);

    QCOMPARE(pathView->preferredHighlightBegin(), 0.4);
    QCOMPARE(pathView->preferredHighlightEnd(), 0.4);
    QCOMPARE(pathView->dragMargin(), 20.0);

    QCOMPARE(snapPositionSpy.count(), 1);
    QCOMPARE(dragMarginSpy.count(), 1);

    pathView->setPreferredHighlightBegin(0.4);
    pathView->setPreferredHighlightEnd(0.4);
    pathView->setDragMargin(20.0);

    QCOMPARE(snapPositionSpy.count(), 1);
    QCOMPARE(dragMarginSpy.count(), 1);

    QSignalSpy maximumFlickVelocitySpy(pathView, SIGNAL(maximumFlickVelocityChanged()));
    pathView->setMaximumFlickVelocity(1000);
    QCOMPARE(maximumFlickVelocitySpy.count(), 1);
    pathView->setMaximumFlickVelocity(1000);
    QCOMPARE(maximumFlickVelocitySpy.count(), 1);

    delete canvas;
}

void tst_QQuickPathView::pathChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QQuickPath *path = canvas->rootObject()->findChild<QQuickPath*>("path");
    QVERIFY(path);

    QSignalSpy startXSpy(path, SIGNAL(startXChanged()));
    QSignalSpy startYSpy(path, SIGNAL(startYChanged()));

    QCOMPARE(path->startX(), 220.0);
    QCOMPARE(path->startY(), 200.0);

    path->setStartX(240.0);
    path->setStartY(220.0);

    QCOMPARE(path->startX(), 240.0);
    QCOMPARE(path->startY(), 220.0);

    QCOMPARE(startXSpy.count(),1);
    QCOMPARE(startYSpy.count(),1);

    path->setStartX(240);
    path->setStartY(220);

    QCOMPARE(startXSpy.count(),1);
    QCOMPARE(startYSpy.count(),1);

    QQuickPath *alternatePath = canvas->rootObject()->findChild<QQuickPath*>("alternatePath");
    QVERIFY(alternatePath);

    QSignalSpy pathSpy(pathView, SIGNAL(pathChanged()));

    QCOMPARE(pathView->path(), path);

    pathView->setPath(alternatePath);
    QCOMPARE(pathView->path(), alternatePath);
    QCOMPARE(pathSpy.count(),1);

    pathView->setPath(alternatePath);
    QCOMPARE(pathSpy.count(),1);

    QQuickPathAttribute *pathAttribute = canvas->rootObject()->findChild<QQuickPathAttribute*>("pathAttribute");
    QVERIFY(pathAttribute);

    QSignalSpy nameSpy(pathAttribute, SIGNAL(nameChanged()));
    QCOMPARE(pathAttribute->name(), QString("opacity"));

    pathAttribute->setName("scale");
    QCOMPARE(pathAttribute->name(), QString("scale"));
    QCOMPARE(nameSpy.count(),1);

    pathAttribute->setName("scale");
    QCOMPARE(nameSpy.count(),1);
    delete canvas;
}

void tst_QQuickPathView::componentChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QQmlComponent delegateComponent(canvas->engine());
    delegateComponent.setData("import QtQuick 2.0; Text { text: '<b>Name:</b> ' + name }", QUrl::fromLocalFile(""));

    QSignalSpy delegateSpy(pathView, SIGNAL(delegateChanged()));

    pathView->setDelegate(&delegateComponent);
    QCOMPARE(pathView->delegate(), &delegateComponent);
    QCOMPARE(delegateSpy.count(),1);

    pathView->setDelegate(&delegateComponent);
    QCOMPARE(delegateSpy.count(),1);
    delete canvas;
}

void tst_QQuickPathView::modelChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QQuickListModel *alternateModel = canvas->rootObject()->findChild<QQuickListModel*>("alternateModel");
    QVERIFY(alternateModel);
    QVariant modelVariant = QVariant::fromValue<QObject *>(alternateModel);
    QSignalSpy modelSpy(pathView, SIGNAL(modelChanged()));

    pathView->setModel(modelVariant);
    QCOMPARE(pathView->model(), modelVariant);
    QCOMPARE(modelSpy.count(),1);

    pathView->setModel(modelVariant);
    QCOMPARE(modelSpy.count(),1);

    pathView->setModel(QVariant());
    QCOMPARE(modelSpy.count(),2);

    delete canvas;
}

void tst_QQuickPathView::pathUpdateOnStartChanged()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("pathUpdateOnStartChanged.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QQuickPath *path = canvas->rootObject()->findChild<QQuickPath*>("path");
    QVERIFY(path);
    QCOMPARE(path->startX(), 400.0);
    QCOMPARE(path->startY(), 300.0);

    QQuickItem *item = findItem<QQuickItem>(pathView, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->x(), path->startX() - item->width() / 2.0);
    QCOMPARE(item->y(), path->startY() - item->height() / 2.0);

    delete canvas;
}

void tst_QQuickPathView::package()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("pathview_package.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("photoPathView");
    QVERIFY(pathView);

#ifdef Q_OS_MAC
    QSKIP("QTBUG-21590 view does not reliably receive polish without a running animation");
#endif

    QQuickItem *item = findItem<QQuickItem>(pathView, "pathItem");
    QVERIFY(item);
    QVERIFY(item->scale() != 1.0);

    delete canvas;
}

//QTBUG-13017
void tst_QQuickPathView::emptyModel()
{
    QQuickView *canvas = createView();

    QStringListModel model;

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("emptyModel", &model);

    canvas->setSource(testFileUrl("emptymodel.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    QCOMPARE(pathview->offset(), qreal(0.0));

    delete canvas;
}

void tst_QQuickPathView::closed()
{
    QQmlEngine engine;

    {
        QQmlComponent c(&engine, testFileUrl("openPath.qml"));
        QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
        QVERIFY(obj);
        QCOMPARE(obj->isClosed(), false);
        delete obj;
    }

    {
        QQmlComponent c(&engine, testFileUrl("closedPath.qml"));
        QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
        QVERIFY(obj);
        QCOMPARE(obj->isClosed(), true);
        delete obj;
    }
}

// QTBUG-14239
void tst_QQuickPathView::pathUpdate()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("pathUpdate.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QQuickItem *item = findItem<QQuickItem>(pathView, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->x(), 150.0);

    delete canvas;
}

void tst_QQuickPathView::visualDataModel()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("vdm.qml"));

    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->count(), 3);

    delete obj;
}

void tst_QQuickPathView::undefinedPath()
{
    QQmlEngine engine;

    // QPainterPath warnings are only received if QT_NO_DEBUG is not defined
    if (QLibraryInfo::isDebugBuild()) {
        QString warning1("QPainterPath::moveTo: Adding point where x or y is NaN or Inf, ignoring call");
        QTest::ignoreMessage(QtWarningMsg,qPrintable(warning1));

        QString warning2("QPainterPath::lineTo: Adding point where x or y is NaN or Inf, ignoring call");
        QTest::ignoreMessage(QtWarningMsg,qPrintable(warning2));
    }

    QQmlComponent c(&engine, testFileUrl("undefinedpath.qml"));

    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->count(), 3);

    delete obj;
}

void tst_QQuickPathView::mouseDrag()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("dragpath.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    int current = pathview->currentIndex();

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(10,100));
    QTest::qWait(100);

    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(30,100), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }
    // first move beyond threshold does not trigger drag
    QVERIFY(!pathview->isMoving());

    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(90,100), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }
    // next move beyond threshold does trigger drag
    QVERIFY(pathview->isMoving());

    QVERIFY(pathview->currentIndex() != current);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(40,100));

    delete canvas;
}

#ifndef QT_NO_WIDGETS
void tst_QQuickPathView::treeModel()
{
    QQuickView *canvas = createView();
    canvas->show();

    QStandardItemModel model;
    initStandardTreeModel(&model);
    canvas->engine()->rootContext()->setContextProperty("myModel", &model);

    canvas->setSource(testFileUrl("treemodel.qml"));

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);
    QCOMPARE(pathview->count(), 3);

    QQuickText *item = findItem<QQuickText>(pathview, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->text(), QLatin1String("Row 1 Item"));

    QVERIFY(QMetaObject::invokeMethod(pathview, "setRoot", Q_ARG(QVariant, 1)));
    QCOMPARE(pathview->count(), 1);

    QTRY_VERIFY(item = findItem<QQuickText>(pathview, "wrapper", 0));
    QTRY_COMPARE(item->text(), QLatin1String("Row 2 Child Item"));

    delete canvas;
}
#endif

void tst_QQuickPathView::changePreferredHighlight()
{
    QQuickView *canvas = createView();
    canvas->setGeometry(0,0,400,200);
    canvas->setSource(testFileUrl("dragpath.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    int current = pathview->currentIndex();
    QCOMPARE(current, 0);

    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QQuickPath *path = qobject_cast<QQuickPath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.5);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);

    pathview->setPreferredHighlightBegin(0.8);
    pathview->setPreferredHighlightEnd(0.8);
    start = path->pointAt(0.8);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentIndex(), 0);

    delete canvas;
}

void tst_QQuickPathView::creationContext()
{
    QQuickView canvas;
    canvas.setGeometry(0,0,240,320);
    canvas.setSource(testFileUrl("creationContext.qml"));

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(canvas.rootObject());
    QVERIFY(rootItem);
    QVERIFY(rootItem->property("count").toInt() > 0);

    QQuickItem *item;
    QVERIFY(item = findItem<QQuickItem>(rootItem, "listItem", 0));
    QCOMPARE(item->property("text").toString(), QString("Hello!"));
}

// QTBUG-21320
void tst_QQuickPathView::currentOffsetOnInsertion()
{
    QQuickView *canvas = createView();
    canvas->show();

    QaimModel model;

    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathline.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setPreferredHighlightBegin(0.5);
    pathview->setPreferredHighlightEnd(0.5);

    QCOMPARE(pathview->count(), model.count());

    model.addItem("item0", "0");

    QCOMPARE(pathview->count(), model.count());

    QQuickRectangle *item = 0;
    QTRY_VERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 0));

    QQuickPath *path = qobject_cast<QQuickPath*>(pathview->path());
    QVERIFY(path);

    QPointF start = path->pointAt(0.5);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(item->width()/2);
    offset.setY(item->height()/2);
    QCOMPARE(item->pos() + offset, start);

    QSignalSpy currentIndexSpy(pathview, SIGNAL(currentIndexChanged()));

    // insert an item at the beginning
    model.insertItem(0, "item1", "1");
    qApp->processEvents();

    QCOMPARE(currentIndexSpy.count(), 1);

    // currentIndex is now 1
    QVERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 1));

    // verify that current item (item 1) is still at offset 0.5
    QCOMPARE(item->pos() + offset, start);

    // insert another item at the beginning
    model.insertItem(0, "item2", "2");
    qApp->processEvents();

    QCOMPARE(currentIndexSpy.count(), 2);

    // currentIndex is now 2
    QVERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 2));

    // verify that current item (item 2) is still at offset 0.5
    QCOMPARE(item->pos() + offset, start);

    // verify that remove before current maintains current item
    model.removeItem(0);
    qApp->processEvents();

    QCOMPARE(currentIndexSpy.count(), 3);

    // currentIndex is now 1
    QVERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 1));

    // verify that current item (item 1) is still at offset 0.5
    QCOMPARE(item->pos() + offset, start);

    delete canvas;
}

void tst_QQuickPathView::asynchronous()
{
    QQuickView *canvas = createView();
    canvas->show();
    QQmlIncubationController controller;
    canvas->engine()->setIncubationController(&controller);

    canvas->setSource(testFileUrl("asyncloader.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootObject);

    QQuickPathView *pathview = 0;
    while (!pathview) {
        bool b = false;
        controller.incubateWhile(&b);
        pathview = rootObject->findChild<QQuickPathView*>("view");
    }

    // items will be created one at a time
    for (int i = 0; i < 5; ++i) {
        QVERIFY(findItem<QQuickItem>(pathview, "wrapper", i) == 0);
        QQuickItem *item = 0;
        while (!item) {
            bool b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(pathview, "wrapper", i);
        }
    }

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    // verify positioning
    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QQuickPath *path = qobject_cast<QQuickPath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.0);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    pathview->setOffset(1.0);

    for (int i=0; i<5; i++) {
        QQuickItem *curItem = findItem<QQuickItem>(pathview, "wrapper", i);
        QPointF itemPos(path->pointAt(0.2 + i*0.2));
        QCOMPARE(curItem->pos() + offset, itemPos);
    }

    delete canvas;
}

void tst_QQuickPathView::missingPercent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("missingPercent.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
    QVERIFY(obj);
    QCOMPARE(obj->attributeAt("_qfx_percent", 1.0), qreal(1.0));
    delete obj;
}

void tst_QQuickPathView::cancelDrag()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("dragpath.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    // drag between snap points
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(10,100));
    QTest::qWait(100);
    QTest::mouseMove(canvas, QPoint(30, 100));
    QTest::mouseMove(canvas, QPoint(85, 100));

    QTRY_VERIFY(pathview->offset() != qFloor(pathview->offset()));
    QTRY_VERIFY(pathview->isMoving());

    // steal mouse grab - cancels PathView dragging
    QQuickItem *item = canvas->rootObject()->findChild<QQuickItem*>("text");
    item->grabMouse();

    // returns to a snap point.
    QTRY_VERIFY(pathview->offset() == qFloor(pathview->offset()));
    QTRY_VERIFY(!pathview->isMoving());

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(40,100));

    delete canvas;
}

void tst_QQuickPathView::maximumFlickVelocity()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("dragpath.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    pathview->setMaximumFlickVelocity(700);
    flick(canvas, QPoint(200,10), QPoint(10,10), 180);
    QVERIFY(pathview->isMoving());
    QVERIFY(pathview->isFlicking());
    QTRY_VERIFY(!pathview->isMoving());

    double dist1 = 100 - pathview->offset();

    pathview->setOffset(0.);
    pathview->setMaximumFlickVelocity(300);
    flick(canvas, QPoint(200,10), QPoint(10,10), 180);
    QVERIFY(pathview->isMoving());
    QVERIFY(pathview->isFlicking());
    QTRY_VERIFY(!pathview->isMoving());

    double dist2 = 100 - pathview->offset();

    pathview->setOffset(0.);
    pathview->setMaximumFlickVelocity(500);
    flick(canvas, QPoint(200,10), QPoint(10,10), 180);
    QVERIFY(pathview->isMoving());
    QVERIFY(pathview->isFlicking());
    QTRY_VERIFY(!pathview->isMoving());

    double dist3 = 100 - pathview->offset();

    QVERIFY(dist1 > dist2);
    QVERIFY(dist3 > dist2);
    QVERIFY(dist2 < dist1);

    delete canvas;
}

void tst_QQuickPathView::snapToItem()
{
    QFETCH(bool, enforceRange);

    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("panels.qml"));
    QQuickPathView *pathview = canvas->rootObject()->findChild<QQuickPathView*>("view");
    QVERIFY(pathview != 0);

    canvas->rootObject()->setProperty("enforceRange", enforceRange);
    QTRY_VERIFY(!pathview->isMoving()); // ensure stable

    int currentIndex = pathview->currentIndex();

    QSignalSpy snapModeSpy(pathview, SIGNAL(snapModeChanged()));

    flick(canvas, QPoint(200,10), QPoint(10,10), 180);

    QVERIFY(pathview->isMoving());
    QTRY_VERIFY(!pathview->isMoving());

    QVERIFY(pathview->offset() == qFloor(pathview->offset()));

    if (enforceRange)
        QVERIFY(pathview->currentIndex() != currentIndex);
    else
        QVERIFY(pathview->currentIndex() == currentIndex);
}

void tst_QQuickPathView::snapToItem_data()
{
    QTest::addColumn<bool>("enforceRange");

    QTest::newRow("no enforce range") << false;
    QTest::newRow("enforce range") << true;
}

void tst_QQuickPathView::snapOneItem()
{
    QFETCH(bool, enforceRange);

    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("panels.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = canvas->rootObject()->findChild<QQuickPathView*>("view");
    QVERIFY(pathview != 0);

    canvas->rootObject()->setProperty("enforceRange", enforceRange);

    QSignalSpy snapModeSpy(pathview, SIGNAL(snapModeChanged()));

    canvas->rootObject()->setProperty("snapOne", true);
    QVERIFY(snapModeSpy.count() == 1);
    QTRY_VERIFY(!pathview->isMoving()); // ensure stable

    int currentIndex = pathview->currentIndex();

    double startOffset = pathview->offset();
    flick(canvas, QPoint(200,10), QPoint(10,10), 180);

    QVERIFY(pathview->isMoving());
    QTRY_VERIFY(!pathview->isMoving());

    // must have moved only one item
    QCOMPARE(pathview->offset(), fmodf(3.0 + startOffset - 1.0, 3.0));

    if (enforceRange)
        QVERIFY(pathview->currentIndex() == currentIndex+1);
    else
        QVERIFY(pathview->currentIndex() == currentIndex);
}

void tst_QQuickPathView::snapOneItem_data()
{
    QTest::addColumn<bool>("enforceRange");

    QTest::newRow("no enforce range") << false;
    QTest::newRow("enforce range") << true;
}


QTEST_MAIN(tst_QQuickPathView)

#include "tst_qquickpathview.moc"
