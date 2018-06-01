/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktableview_p.h>
#include <QtQuick/private/qquicktableview_p_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/private/qqmlobjectmodel_p.h>
#include <QtQml/private/qqmllistmodel_p.h>
#include <QtQml/private/qqmldelegatemodel_p.h>

#include "testmodel.h"

#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

static const char* kTableViewPropName = "tableView";
static const char* kDelegateObjectName = "tableViewDelegate";

Q_DECLARE_METATYPE(QMarginsF);

#define LOAD_TABLEVIEW(fileName) \
    QScopedPointer<QQuickView> view(createView()); \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    QVERIFY(QTest::qWaitForWindowActive(view.data())); \
    auto tableView = view->rootObject()->property(kTableViewPropName).value<QQuickTableView *>(); \
    QVERIFY(tableView); \
    auto tableViewPrivate = QQuickTableViewPrivate::get(tableView); \
    Q_UNUSED(tableViewPrivate)

#define WAIT_UNTIL_POLISHED \
    QVERIFY(tableViewPrivate->polishScheduled); \
    QTRY_VERIFY(!tableViewPrivate->polishScheduled)

class tst_QQuickTableView : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickTableView();

    QQuickTableViewAttached *getAttachedObject(const QObject *object) const;
    FxTableItem *findFxTableItem(int row, int column, const QList<FxTableItem *> items) const;
    FxTableItem *findLoadedBottomRightItem(const QList<FxTableItem *> items) const;

private slots:
    void initTestCase() override;

    void setAndGetModel_data();
    void setAndGetModel();
    void emptyModel_data();
    void emptyModel();
    void checkZeroSizedDelegate();
    void checkImplicitSizeDelegate();
    void noDelegate();
    void countDelegateItems_data();
    void countDelegateItems();
    void checkLayoutOfEqualSizedDelegateItems_data();
    void checkLayoutOfEqualSizedDelegateItems();
    void checkTableMargins_data();
    void checkTableMargins();
    void fillTableViewButNothingMore_data();
    void fillTableViewButNothingMore();
    void flick_data();
    void flick();
    void flickOvershoot_data();
    void flickOvershoot();
    void checkRowColumnCount();
};

tst_QQuickTableView::tst_QQuickTableView()
{
}

void tst_QQuickTableView::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestModel>("TestModel", 0, 1, "TestModel");
}

QQuickTableViewAttached *tst_QQuickTableView::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickTableView>(object);
    return static_cast<QQuickTableViewAttached *>(attachedObject);
}

FxTableItem *tst_QQuickTableView::findFxTableItem(int row, int column, const QList<FxTableItem *> items) const
{
    for (int i = 0; i < items.count(); ++i) {
        FxTableItem *fxitem = items[i];
        auto attached = getAttachedObject(fxitem->item);
        if (row == attached->row() && column == attached->column())
            return fxitem;
    }
    return nullptr;
}

FxTableItem *tst_QQuickTableView::findLoadedBottomRightItem(const QList<FxTableItem *> items) const
{
    FxTableItem *bottomRightItem = nullptr;
    int bottomRightIndex = 0;

    for (int i = items.count() - 1; i > 0; --i) {
        FxTableItem *fxitem = items[i];
        if (fxitem->index > bottomRightIndex) {
            bottomRightItem = fxitem;
            bottomRightIndex = fxitem->index;
        }
    }
    return bottomRightItem;
}

void tst_QQuickTableView::setAndGetModel_data()
{
    QTest::addColumn<QVariant>("model");

    QTest::newRow("QAIM 1x1") << TestModelAsVariant(1, 1);
    QTest::newRow("Number model 1") << QVariant::fromValue(1);
    QTest::newRow("QStringList 1") << QVariant::fromValue(QStringList() << "one");
}

void tst_QQuickTableView::setAndGetModel()
{
    // Test that we can set and get different kind of models
    QFETCH(QVariant, model);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    QCOMPARE(model, tableView->model());
}

void tst_QQuickTableView::emptyModel_data()
{
    QTest::addColumn<QVariant>("model");

    QTest::newRow("QAIM") << TestModelAsVariant(0, 0);
    QTest::newRow("Number model") << QVariant::fromValue(0);
    QTest::newRow("QStringList") << QVariant::fromValue(QStringList());
}

void tst_QQuickTableView::emptyModel()
{
    // Check that if we assign an empty model to
    // TableView, no delegate items will be created.
    QFETCH(QVariant, model);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableViewPrivate->loadedItems.count(), 0);
}

void tst_QQuickTableView::checkZeroSizedDelegate()
{
    // Check that if we assign a delegate with empty width and height, we
    // fall back to use kDefaultColumnWidth and kDefaultRowHeight as
    // column/row sizes.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    view->rootObject()->setProperty("delegateWidth", 0);
    view->rootObject()->setProperty("delegateHeight", 0);

    WAIT_UNTIL_POLISHED;

    auto items = tableViewPrivate->loadedItems;
    QVERIFY(!items.isEmpty());

    for (auto fxItem : tableViewPrivate->loadedItems) {
        auto item = fxItem->item;
        QCOMPARE(item->width(), kDefaultColumnWidth);
        QCOMPARE(item->height(), kDefaultRowHeight);
    }
}

void tst_QQuickTableView::checkImplicitSizeDelegate()
{
    // Check that we can set the size of delegate items using
    // implicit width/height, instead of forcing the user to
    // create an attached object by using TableView.cellWidth/Height.
    LOAD_TABLEVIEW("tableviewimplicitsize.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    auto items = tableViewPrivate->loadedItems;
    QVERIFY(!items.isEmpty());

    for (auto fxItem : tableViewPrivate->loadedItems) {
        auto item = fxItem->item;
        QCOMPARE(item->width(), 90);
        QCOMPARE(item->height(), 60);
    }
}

void tst_QQuickTableView::noDelegate()
{
    // Check that you can skip setting a delegate without
    // it causing any problems (like crashing or asserting).
    // And then set a delegate, and do a quick check that the
    // view gets populated as expected.
    LOAD_TABLEVIEW("plaintableview.qml");

    const int rows = 5;
    const int columns = 5;
    auto model = TestModelAsVariant(columns, rows);
    tableView->setModel(model);

    // Start with no delegate, and check
    // that we end up with no items in the table.
    tableView->setDelegate(nullptr);

    WAIT_UNTIL_POLISHED;

    auto items = tableViewPrivate->loadedItems;
    QVERIFY(items.isEmpty());

    // Set a delegate, and check that we end
    // up with the expected number of items.
    auto delegate = view->rootObject()->property("delegate").value<QQmlComponent *>();
    QVERIFY(delegate);
    tableView->setDelegate(delegate);

    WAIT_UNTIL_POLISHED;

    items = tableViewPrivate->loadedItems;
    QCOMPARE(items.count(), rows * columns);

    // And then unset the delegate again, and check
    // that we end up with no items.
    tableView->setDelegate(nullptr);

    WAIT_UNTIL_POLISHED;

    items = tableViewPrivate->loadedItems;
    QVERIFY(items.isEmpty());
}

void tst_QQuickTableView::countDelegateItems_data()
{
    QTest::addColumn<QVariant>("model");
    QTest::addColumn<int>("count");

    QTest::newRow("QAIM 1x1") << TestModelAsVariant(1, 1) << 1;
    QTest::newRow("QAIM 2x1") << TestModelAsVariant(2, 1) << 2;
    QTest::newRow("QAIM 1x2") << TestModelAsVariant(1, 2) << 2;
    QTest::newRow("QAIM 2x2") << TestModelAsVariant(2, 2) << 4;
    QTest::newRow("QAIM 4x4") << TestModelAsVariant(4, 4) << 16;

    QTest::newRow("Number model 1") << QVariant::fromValue(1) << 1;
    QTest::newRow("Number model 4") << QVariant::fromValue(4) << 4;

    QTest::newRow("QStringList 1") << QVariant::fromValue(QStringList() << "one") << 1;
    QTest::newRow("QStringList 4") << QVariant::fromValue(QStringList() << "one" << "two" << "three" << "four") << 4;
}

void tst_QQuickTableView::countDelegateItems()
{
    // Assign different models of various sizes, and check that the number of
    // delegate items in the view matches the size of the model. Note that for
    // this test to be valid, all items must be within the visible area of the view.
    QFETCH(QVariant, model);
    QFETCH(int, count);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;

    // Check that tableview internals contain the expected number of items
    auto const items = tableViewPrivate->loadedItems;
    QCOMPARE(items.count(), count);

    // Check that this also matches the items found in the view
    auto foundItems = findItems<QQuickItem>(tableView, kDelegateObjectName);
    QCOMPARE(foundItems.count(), count);
}

void tst_QQuickTableView::checkLayoutOfEqualSizedDelegateItems_data()
{
    QTest::addColumn<QVariant>("model");
    QTest::addColumn<QSize>("tableSize");
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");

    // Check spacing together with different table setups
    QTest::newRow("QAIM 1x1 1,1") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 5x5 0,0") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 5x5 1,0") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(1, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 5x5 0,1") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(0, 1) << QMarginsF(0, 0, 0, 0);

    // Check spacing together with margins
    QTest::newRow("QAIM 1x1 1,1 5555") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1) << QMarginsF(5, 5, 5, 5);
    QTest::newRow("QAIM 4x4 0,0 3333") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(0, 0) << QMarginsF(3, 3, 3, 3);
    QTest::newRow("QAIM 4x4 2,2 1234") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(2, 2) << QMarginsF(1, 2, 3, 4);
}

void tst_QQuickTableView::checkLayoutOfEqualSizedDelegateItems()
{
    // Check that the geometry of the delegate items are correct
    QFETCH(QVariant, model);
    QFETCH(QSize, tableSize);
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());

    WAIT_UNTIL_POLISHED;

    auto const items = tableViewPrivate->loadedItems;
    QVERIFY(!items.isEmpty());
    const QQuickItem *firstItem = items[0]->item;

    qreal expectedWidth = firstItem->width();
    qreal expectedHeight = firstItem->height();
    int expectedItemCount = tableSize.width() * tableSize.height();

    for (int i = 0; i < expectedItemCount; ++i) {
        const QQuickItem *item = items[i]->item;
        QVERIFY(item);
        QCOMPARE(item->parentItem(), tableView->contentItem());

        auto attached = getAttachedObject(item);
        int row = attached->row();
        int column = attached->column();
        qreal expectedX = margins.left() + (column * (expectedWidth + spacing.width()));
        qreal expectedY = margins.top() + (row * (expectedHeight + spacing.height()));
        QCOMPARE(item->x(), expectedX);
        QCOMPARE(item->y(), expectedY);
        QCOMPARE(item->width(), expectedWidth);
        QCOMPARE(item->height(), expectedHeight);
    }
}

void tst_QQuickTableView::checkTableMargins_data()
{
    QTest::addColumn<QVariant>("model");
    QTest::addColumn<QSize>("tableSize");
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");

    QTest::newRow("QAIM 1x1 1,1 0000") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 4x4 1,1 0000") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(1, 1) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 1x1 1,1 5555") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1) << QMarginsF(5, 5, 5, 5);
    QTest::newRow("QAIM 4x4 0,0 3333") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(0, 0) << QMarginsF(3, 3, 3, 3);
    QTest::newRow("QAIM 4x4 2,2 1234") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(2, 2) << QMarginsF(1, 2, 3, 4);
    QTest::newRow("QAIM 1x1 0,0 3210") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(0, 0) << QMarginsF(3, 2, 1, 0);
}

void tst_QQuickTableView::checkTableMargins()
{
    // Check that the space between the content view and
    // the items matches the margins we set on the tableview.
    QFETCH(QVariant, model);
    QFETCH(QSize, tableSize);
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());

    WAIT_UNTIL_POLISHED;

    auto const items = tableViewPrivate->loadedItems;

    auto const topLeftFxItem = findFxTableItem(0, 0, items);
    auto const bottomRightFxItem = findFxTableItem(tableSize.height() - 1, tableSize.width() - 1, items);
    QVERIFY(topLeftFxItem);
    QVERIFY(bottomRightFxItem);

    auto const topLeftItem = topLeftFxItem->item;
    auto const bottomRightItem = bottomRightFxItem->item;
    qreal leftSpace = topLeftItem->x();
    qreal topSpace = topLeftItem->y();
    qreal rightSpace = tableView->contentWidth() - (bottomRightItem->x() + bottomRightItem->width());
    qreal bottomSpace = tableView->contentHeight() - (bottomRightItem->y() + bottomRightItem->height());
    QCOMPARE(leftSpace, margins.left());
    QCOMPARE(topSpace, margins.top());
    QCOMPARE(rightSpace, margins.right());
    QCOMPARE(bottomSpace, margins.bottom());
}

void tst_QQuickTableView::fillTableViewButNothingMore_data()
{
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");

    QTest::newRow("0 0,0 0") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("0 10,10 0") << QSizeF(10, 10) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("100 10,10 0") << QSizeF(10, 10) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("0 0,0 100") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("0 10,10 100") << QSizeF(10, 10) << QMarginsF(100, 100, 100, 100);
    QTest::newRow("100 10,10 100") << QSizeF(10, 10) << QMarginsF(100, 100, 100, 100);
}

void tst_QQuickTableView::fillTableViewButNothingMore()
{
    // Check that we end up filling the whole visible part of
    // the tableview with cells, but nothing more.
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    LOAD_TABLEVIEW("plaintableview.qml");

    const int rows = 100;
    const int columns = 100;
    auto model = TestModelAsVariant(rows, columns);

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());
    tableView->setCacheBuffer(0);

    WAIT_UNTIL_POLISHED;

    auto const items = tableViewPrivate->loadedItems;

    auto const topLeftFxItem = findFxTableItem(0, 0, items);
    QVERIFY(topLeftFxItem);
    auto const topLeftItem = topLeftFxItem->item;

    // Check that the top-left item are at the corner of the view
    QCOMPARE(topLeftItem->x(), margins.left());
    QCOMPARE(topLeftItem->y(), margins.top());

    auto const bottomRightFxItem = findLoadedBottomRightItem(items);
    QVERIFY(bottomRightFxItem);
    auto const bottomRightItem = bottomRightFxItem->item;
    auto bottomRightAttached = getAttachedObject(bottomRightItem);

    // Check that the right-most item is overlapping the right edge of the view
    QVERIFY(bottomRightItem->x() < tableView->width());
    QVERIFY(bottomRightItem->x() + bottomRightItem->width() >= tableView->width() - spacing.width());

    // Check that the actual number of columns matches what we expect
    qreal cellWidth = bottomRightItem->width() + spacing.width();
    qreal availableWidth = tableView->width() - margins.left();
    int expectedColumns = qCeil(availableWidth / cellWidth);
    int actualColumns = bottomRightAttached->column() + 1;
    QCOMPARE(actualColumns, expectedColumns);

    // Check that the bottom-most item is overlapping the bottom edge of the view
    QVERIFY(bottomRightItem->y() < tableView->height());
    QVERIFY(bottomRightItem->y() + bottomRightItem->height() >= tableView->height() - spacing.height());

    // Check that the actual number of rows matches what we expect
    qreal cellHeight = bottomRightItem->height() + spacing.height();
    qreal availableHeight = tableView->height() - margins.top();
    int expectedRows = qCeil(availableHeight / cellHeight);
    int actualRows = bottomRightAttached->row() + 1;
    QCOMPARE(actualRows, expectedRows);
}

void tst_QQuickTableView::flick_data()
{
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");

    QTest::newRow("s:0 m:0") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("s:5 m:0") << QSizeF(5, 5) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("s:0 m:20") << QSizeF(0, 0) << QMarginsF(20, 20, 20, 20);
    QTest::newRow("s:5 m:20") << QSizeF(5, 5) << QMarginsF(20, 20, 20, 20);
}

void tst_QQuickTableView::flick()
{
    // Check that if we end up with the correct start and end column/row as we flick around
    // with different table configurations.
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    LOAD_TABLEVIEW("plaintableview.qml");

    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const int visualColumnCount = 4;
    const int visualRowCount = 4;
    const qreal cellWidth = delegateWidth + spacing.width();
    const qreal cellHeight = delegateHeight + spacing.height();
    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());
    tableView->setCacheBuffer(0);
    tableView->setWidth(margins.left() + (visualColumnCount * cellWidth) - spacing.width());
    tableView->setHeight(margins.top() + (visualRowCount * cellHeight) - spacing.height());

    WAIT_UNTIL_POLISHED;

    // Check the "simple" case if the cells never lands egde-to-edge with the viewport. For
    // that case we only accept that visible row/columns are loaded.
    qreal flickValues[] = {0.5, 1.5, 4.5, 20.5, 10.5, 3.5, 1.5, 0.5};

    for (qreal cellsToFlick : flickValues) {
        // Flick to the beginning of the cell
        tableView->setContentX(cellsToFlick * cellWidth);
        tableView->setContentY(cellsToFlick * cellHeight);
        tableView->polish();

        WAIT_UNTIL_POLISHED;

        const QRect loadedTable = tableViewPrivate->loadedTable;

        const int expectedTableLeft = cellsToFlick - int((margins.left() + spacing.width()) / cellWidth);
        const int expectedTableTop = cellsToFlick - int((margins.top() + spacing.height()) / cellHeight);

        QCOMPARE(loadedTable.left(), expectedTableLeft);
        QCOMPARE(loadedTable.right(), expectedTableLeft + visualColumnCount);
        QCOMPARE(loadedTable.top(), expectedTableTop);
        QCOMPARE(loadedTable.bottom(), expectedTableTop + visualRowCount);
    }
}

void tst_QQuickTableView::flickOvershoot_data()
{
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");

    QTest::newRow("s:0 m:0") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("s:5 m:0") << QSizeF(5, 5) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("s:0 m:20") << QSizeF(0, 0) << QMarginsF(20, 20, 20, 20);
    QTest::newRow("s:5 m:20") << QSizeF(5, 5) << QMarginsF(20, 20, 20, 20);
}

void tst_QQuickTableView::flickOvershoot()
{
    // Flick the table completely out and then in again, and see
    // that we still contains the expected rows/columns
    // Note that TableView always keeps top-left item loaded, even
    // when everything is flicked out of view.
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    LOAD_TABLEVIEW("plaintableview.qml");

    const int rowCount = 5;
    const int columnCount = 5;
    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const qreal cellWidth = delegateWidth + spacing.width();
    const qreal cellHeight = delegateHeight + spacing.height();
    const qreal tableWidth = margins.left() + margins.right() + (cellWidth * columnCount) - spacing.width();
    const qreal tableHeight = margins.top() + margins.bottom() + (cellHeight * rowCount) - spacing.height();
    const int outsideMargin = 10;
    auto model = TestModelAsVariant(rowCount, columnCount);

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());
    tableView->setCacheBuffer(0);
    tableView->setWidth(tableWidth - margins.right() - cellWidth / 2);
    tableView->setHeight(tableHeight - margins.bottom() - cellHeight / 2);

    WAIT_UNTIL_POLISHED;

    // Flick table out of view left
    tableView->setContentX(-tableView->width() - outsideMargin);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.right(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.top(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), rowCount - 1);

    // Flick table out of view right
    tableView->setContentX(tableWidth + outsideMargin);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.right(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.top(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), rowCount - 1);

    // Flick table out of view on top
    tableView->setContentX(0);
    tableView->setContentY(-tableView->height() - outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.right(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.top(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), 0);

    // Flick table out of view at the bottom
    tableView->setContentX(0);
    tableView->setContentY(tableHeight + outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.right(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.top(), rowCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), rowCount - 1);

    // Flick table out of view left and top at the same time
    tableView->setContentX(-tableView->width() - outsideMargin);
    tableView->setContentY(-tableView->height() - outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.right(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.top(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), 0);

    // Flick table back to origo
    tableView->setContentX(0);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.right(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.top(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), rowCount - 1);

    // Flick table out of view right and bottom at the same time
    tableView->setContentX(tableWidth + outsideMargin);
    tableView->setContentY(tableHeight + outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.right(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.top(), rowCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), rowCount - 1);

    // Flick table back to origo
    tableView->setContentX(0);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedTable.left(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.right(), columnCount - 1);
    QCOMPARE(tableViewPrivate->loadedTable.top(), 0);
    QCOMPARE(tableViewPrivate->loadedTable.bottom(), rowCount - 1);
}

void tst_QQuickTableView::checkRowColumnCount()
{
    // If we flick several columns (rows) at the same time, check that we don't
    // end up with loading more delegate items into memory than necessary. We
    // should free up columns as we go before loading new ones.
    LOAD_TABLEVIEW("countingtableview.qml");

    const char *maxDelegateCountProp = "maxDelegateCount";
    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const int tableViewCount = tableViewPrivate->loadedItems.count();
    const int qmlCountAfterInit = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(tableViewCount, qmlCountAfterInit);

    // Flick a long distance right
    tableView->setContentX(tableView->width() * 2);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    const int qmlCountAfterRightFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterRightFlick, qmlCountAfterInit);

    // Flick a long distance down
    tableView->setContentX(tableView->height() * 2);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    const int qmlCountAfterDownFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterDownFlick, qmlCountAfterInit);

    // Flick a long distance left
    tableView->setContentX(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    const int qmlCountAfterLeftFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterLeftFlick, qmlCountAfterInit);

    // Flick a long distance up
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    const int qmlCountAfterUpFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterUpFlick, qmlCountAfterInit);
}

QTEST_MAIN(tst_QQuickTableView)

#include "tst_qquicktableview.moc"
