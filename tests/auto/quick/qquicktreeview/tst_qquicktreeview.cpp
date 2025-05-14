// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QStringListModel>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuick/private/qquicktreeview_p_p.h>
#include <QtQuick/private/qquicktextinput_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>

#include "testmodel.h"

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

using namespace Qt::StringLiterals;

#define LOAD_TREEVIEW(fileName) \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    view->requestActivate(); \
    QVERIFY(QTest::qWaitForWindowActive(view)); \
    auto treeView = view->rootObject()->property("treeView").value<QQuickTreeView *>(); \
    QVERIFY(treeView); \
    auto treeViewPrivate = QQuickTreeViewPrivate::get(treeView); \
    Q_UNUSED(treeViewPrivate) \
    auto model = treeView->model().value<TestModel *>(); \
    Q_UNUSED(model)

#define WAIT_UNTIL_POLISHED_ARG(item) \
    QVERIFY(QQuickTest::qIsPolishScheduled(item)); \
    QVERIFY(QQuickTest::qWaitForPolish(item))
#define WAIT_UNTIL_POLISHED WAIT_UNTIL_POLISHED_ARG(treeView)

// ########################################################

class tst_qquicktreeview : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktreeview();

private:
    QQuickView *view = nullptr;

private slots:
    void initTestCase() override;
    void showTreeView();
    void invalidArguments();
    void expandAndCollapseRoot();
    void toggleExpanded();
    void expandAndCollapseChildren();
    void expandChildPendingToBeVisible();
    void expandRecursivelyRoot_data();
    void expandRecursivelyRoot();
    void expandRecursivelyChild_data();
    void expandRecursivelyChild();
    void expandRecursivelyWholeTree();
    void collapseRecursivelyRoot();
    void collapseRecursivelyChild();
    void collapseRecursivelyWholeTree();
    void expandToIndex();
    void requiredPropertiesRoot();
    void requiredPropertiesChildren();
    void emptyModel();
    void updatedModifiedModel();
    void insertRows();
    void insertColumns();
    void toggleExpandedUsingArrowKeys();
    void expandAndCollapsUsingDoubleClick();
    void selectionBehaviorCells_data();
    void selectionBehaviorCells();
    void selectionBehaviorRows();
    void selectionBehaviorColumns();
    void selectionBehaviorDisabled();
    void sortTreeModel_data();
    void sortTreeModel();
    void sortTreeModelDynamic_data();
    void sortTreeModelDynamic();
    void setRootIndex();
    void setRootIndexToLeaf();
    void editUsingEditTriggers_data();
    void editUsingEditTriggers();
    void editOnNonEditableCell_data();
    void editOnNonEditableCell();
};

tst_qquicktreeview::tst_qquicktreeview()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquicktreeview::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestModel>("TestModel", 1, 0, "TestModel");
    view = createView();
}

void tst_qquicktreeview::showTreeView()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view is showing the root of the tree
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}


void tst_qquicktreeview::invalidArguments()
{
    // Check that we handle gracefully invalid arguments
    LOAD_TREEVIEW("normaltreeview.qml");

    treeView->expand(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->expandRecursively(200);
    QCOMPARE(treeView->rows(), 1);
    treeView->expandRecursively(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->expandRecursively(200);
    QCOMPARE(treeView->rows(), 1);

    treeView->collapse(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->collapseRecursively(200);
    QCOMPARE(treeView->rows(), 1);
    treeView->collapseRecursively(-2);
    QCOMPARE(treeView->rows(), 1);
    treeView->collapseRecursively(200);
    QCOMPARE(treeView->rows(), 1);
}

void tst_qquicktreeview::expandAndCollapseRoot()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    QSignalSpy expandedSpy(treeView, SIGNAL(expanded(int,int)));

    // Expand the root
    treeView->expand(0);

    QCOMPARE(expandedSpy.size(), 1);
    auto signalArgs = expandedSpy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == 0);
    QVERIFY(signalArgs.at(1).toInt() == 1);

    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    treeView->collapse(0);
    WAIT_UNTIL_POLISHED;
    // Check that the view only has one row loaded again (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeview::toggleExpanded()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    // Expand the root
    treeView->toggleExpanded(0);
    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    treeView->toggleExpanded(0);
    WAIT_UNTIL_POLISHED;
    // Check that the view only has one row loaded again (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeview::expandAndCollapseChildren()
{
    // Check that we can expand and collapse children, and that
    // the tree ends up with the expected rows
    LOAD_TREEVIEW("normaltreeview.qml");

    const int childCount = 4;
    QSignalSpy expandedSpy(treeView, SIGNAL(expanded(int,int)));

    // Expand the last child of a parent recursively four times
    for (int level = 0; level < 4; ++level) {
        const int nodeToExpand = level * childCount;
        const int firstChildRow = nodeToExpand + 1; // (+ 1 for the root)
        const int lastChildRow = firstChildRow + 4;

        treeView->expand(nodeToExpand);

        QCOMPARE(expandedSpy.size(), 1);
        auto signalArgs = expandedSpy.takeFirst();
        QVERIFY(signalArgs.at(0).toInt() == nodeToExpand);
        QVERIFY(signalArgs.at(1).toInt() == 1);

        WAIT_UNTIL_POLISHED;

        QCOMPARE(treeView->rows(), lastChildRow);

        auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow))->item;
        QCOMPARE(childItem1->property("text").toString(), "0, 0");
        auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 1))->item;
        QCOMPARE(childItem2->property("text").toString(), "1, 0");
        auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 2))->item;
        QCOMPARE(childItem3->property("text").toString(), "2, 0");
    }

    // Collapse down from level 2 (deliberatly not mirroring the expansion by
    // instead collapsing both level 3 and 4 in one go)
    for (int level = 2; level > 0; --level) {
        const int nodeToCollapse = level * childCount;
        const int firstChildRow = nodeToCollapse - childCount + 1;
        const int lastChildRow = nodeToCollapse + 1; // (+ 1 for the root)

        treeView->collapse(nodeToCollapse);
        WAIT_UNTIL_POLISHED;
        QCOMPARE(treeView->rows(), lastChildRow);

        auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow))->item;
        QCOMPARE(childItem1->property("text").toString(), "0, 0");
        auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 1))->item;
        QCOMPARE(childItem2->property("text").toString(), "1, 0");
        auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, firstChildRow + 2))->item;
        QCOMPARE(childItem3->property("text").toString(), "2, 0");
    }

    // Collapse the root
    treeView->collapse(0);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(treeView->rows(), 1);
}

void tst_qquicktreeview::requiredPropertiesRoot()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    const auto rootFxItem = treeViewPrivate->loadedTableItem(QPoint(0, 0));
    QVERIFY(rootFxItem);
    const auto rootItem = rootFxItem->item;
    QVERIFY(rootItem);

    const auto context = qmlContext(rootItem.data());
    const auto viewProp = context->contextProperty("treeView").value<QQuickTreeView *>();
    const auto isTreeNode = context->contextProperty("isTreeNode").toBool();
    const auto expanded = context->contextProperty("expanded").toBool();
    const auto hasChildren = context->contextProperty("hasChildren").toBool();
    const auto depth = context->contextProperty("depth").toInt();

    QCOMPARE(viewProp, treeView);
    QCOMPARE(isTreeNode, true);
    QCOMPARE(expanded, false);
    QCOMPARE(hasChildren, true);
    QCOMPARE(depth, 0);

    treeView->expand(0);
    WAIT_UNTIL_POLISHED;

    const auto isExpandedAfterExpand = context->contextProperty("expanded").toBool();
    QCOMPARE(isExpandedAfterExpand, true);
}

void tst_qquicktreeview::requiredPropertiesChildren()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    // The last child has it's own children, so expand that one as well
    const auto rootIndex = model->index(0, 0);
    const int childCount = model->rowCount(rootIndex);
    QCOMPARE(childCount, 4);
    const auto lastChildIndex = model->index(childCount - 1, 0, rootIndex);
    QVERIFY(lastChildIndex.isValid());
    QCOMPARE(model->hasChildren(lastChildIndex), true);

    treeView->expand(4);
    WAIT_UNTIL_POLISHED;
    // We now expect root + 4 children + 4 children = 9 rows
    const int rowCount = treeViewPrivate->loadedRows.count();
    QCOMPARE(rowCount, 9);

    // Go through all rows in the view, except the root
    for (int row = 1; row < rowCount; ++row) {
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);

        const auto context = qmlContext(childItem.data());
        const auto viewProp = context->contextProperty("treeView").value<QQuickTreeView *>();
        const auto isTreeNode = context->contextProperty("isTreeNode").toBool();
        const auto expanded = context->contextProperty("expanded").toBool();
        const auto hasChildren = context->contextProperty("hasChildren").toBool();
        const auto depth = context->contextProperty("depth").toInt();

        QCOMPARE(viewProp, treeView);
        QCOMPARE(isTreeNode, true);
        QCOMPARE(expanded, row == 4);
        QCOMPARE(hasChildren, model->hasChildren(treeView->index(row, 0)));
        QCOMPARE(depth, row <= 4 ? 1 : 2);
    }
}

void tst_qquicktreeview::emptyModel()
{
    // Check that you can assign an empty model, and that
    // calling functions will work as expected (and at least not crash)
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->setModel(QVariant());
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeViewPrivate->loadedItems.size(), 0);
    QCOMPARE(treeView->rows(), 0);
    QCOMPARE(treeView->columns(), 0);

    // Check that we don't crash:
    treeView->expand(10);
    treeView->collapse(5);

    QCOMPARE(treeView->depth(0), -1);
    QCOMPARE(treeView->isExpanded(0), false);
    QVERIFY(!treeView->index(10, 10).isValid());
    QCOMPARE(treeView->rowAtIndex(QModelIndex()), -1);
    QCOMPARE(treeView->columnAtIndex(QModelIndex()), -1);
}

void tst_qquicktreeview::updatedModifiedModel()
{
    // Check that if we change the data in the model, the
    // delegate items get updated.
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root plus it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    auto rootFxItem = treeViewPrivate->loadedTableItem(QPoint(0, 0));
    QVERIFY(rootFxItem);
    auto rootItem = rootFxItem->item;
    QVERIFY(rootItem);
    QCOMPARE(rootItem->property("text"), "0, 0");
    model->setData(model->index(0, 0), QVariant(QString("Changed")));
    QCOMPARE(rootItem->property("text"), "Changed");

    auto rootFxItemCol1 = treeViewPrivate->loadedTableItem(QPoint(1, 2));
    QVERIFY(rootFxItemCol1);
    auto rootItemCol1 = rootFxItemCol1->item;
    QVERIFY(rootItemCol1);
    QCOMPARE(rootItemCol1->property("text"), "1, 1");
    model->setData(model->index(1, 1, model->index(0,0)), QVariant(QString("Changed")));
    QCOMPARE(rootItemCol1->property("text"), "Changed");
}

void tst_qquicktreeview::insertRows()
{
    // Check that if we add new rows to the model, TreeView gets updated
    // to contain the new expected number of rows (flattened to a list)
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 5);

    const QModelIndex rootNode = model->index(0, 0, QModelIndex());
    model->insertRows(0, 2, rootNode);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 7);
    auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, 1))->item;
    QCOMPARE(childItem1->property("text").toString(), "0, 0 (inserted)");
    auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, 2))->item;
    QCOMPARE(childItem2->property("text").toString(), "1, 0 (inserted)");
    auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, 3))->item;
    QCOMPARE(childItem3->property("text").toString(), "0, 0");

    const QModelIndex indexOfInsertedChild = model->index(1, 0, rootNode);
    model->insertRows(0, 2, indexOfInsertedChild);
    treeView->expand(2);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 9);
}

void tst_qquicktreeview::insertColumns()
{
    // Check that if we add new columns to the model, TreeView gets updated
    // to contain the new expected number of rows (flattened to a list)
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->columns(), 5);

    const QModelIndex rootNode = model->index(0, 0, QModelIndex());
    model->insertColumns(0, 2, rootNode);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->columns(), 7);
    auto childItem1 = treeViewPrivate->loadedTableItem(QPoint(0, 1))->item;
    QCOMPARE(childItem1->property("text").toString(), "0, 0 (inserted)");
    auto childItem2 = treeViewPrivate->loadedTableItem(QPoint(0, 2))->item;
    QCOMPARE(childItem2->property("text").toString(), "1, 0 (inserted)");
    auto childItem3 = treeViewPrivate->loadedTableItem(QPoint(0, 3))->item;
    QCOMPARE(childItem3->property("text").toString(), "2, 0 (inserted)");
    auto childItem4 = treeViewPrivate->loadedTableItem(QPoint(3, 0))->item;
    QCOMPARE(childItem4->property("text").toString(), "0, 1");
    auto childItem5 = treeViewPrivate->loadedTableItem(QPoint(3, 1))->item;
    QCOMPARE(childItem5->property("text").toString(), "0, 1");

    const QModelIndex indexOfInsertedChild = model->index(1, 0, rootNode);
    model->insertRows(0, 2, indexOfInsertedChild);
    treeView->expand(2);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 7);
    QCOMPARE(treeView->columns(), 7);

    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            auto childItem = treeViewPrivate->loadedTableItem(QPoint(j, i))->item;
            QVERIFY(childItem);
        }
    }
}

void tst_qquicktreeview::expandChildPendingToBeVisible()
{
    // Check that if we expand a row r1, and that row has a child r2 that can
    // be expanded, we can continue to expand c2 immediately, even if r1 is
    // still pending to be shown as expanded in the view.
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expand(0);
    QVERIFY(treeView->isExpanded(0));
    // The view has not yet been updated at this point to show
    // the newly expanded children, so it still has only one row.
    QCOMPARE(treeView->rows(), 1);
    // ...but we still expand row 5, which is a child that has children
    // in the proxy model
    treeView->expand(4);
    QVERIFY(treeView->isExpanded(4));
    QCOMPARE(treeView->rows(), 1);

    WAIT_UNTIL_POLISHED;

    // Now the view have updated to show
    // all the rows that has been expanded.
       QCOMPARE(treeView->rows(), 9);
}

void tst_qquicktreeview::expandRecursivelyRoot_data()
{
    QTest::addColumn<int>("rowToExpand");
    QTest::addColumn<int>("depth");

    QTest::newRow("0, -1") << 0 << -1;
    QTest::newRow("0, 0") << 0 << 0;
    QTest::newRow("0, 1") << 0 << 1;
    QTest::newRow("0, 2") << 0 << 2;
}

void tst_qquicktreeview::expandRecursivelyRoot()
{
    // Check that we can expand the root node (row 0), and that
    // all its children are expanded recursively down to the
    // given depth.
    QFETCH(int, rowToExpand);
    QFETCH(int, depth);

    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int,int)));

    treeView->expandRecursively(rowToExpand, depth);

    if (depth == 0) {
        QCOMPARE(spy.size(), 0);
    } else {

        QCOMPARE(spy.size(), 1);
        const auto signalArgs = spy.takeFirst();
        QVERIFY(signalArgs.at(0).toInt() == rowToExpand);
        QVERIFY(signalArgs.at(1).toInt() == depth);
    }

    WAIT_UNTIL_POLISHED;

    const int rowToExpandDepth = treeView->depth(rowToExpand);
    const int effectiveMaxDepth = depth != -1 ? rowToExpandDepth + depth : model->maxDepth();

    if (depth > 0 || depth == -1)
        QVERIFY(treeView->isExpanded(rowToExpand));
    else
        QVERIFY(!treeView->isExpanded(rowToExpand));

    // Check that all rows after rowToExpand, that are also
    // children of that row, is expanded (down to depth)
    for (int currentRow = rowToExpand + 1; currentRow < treeView->rows(); ++currentRow) {
        const auto modelIndex = treeView->index(currentRow, 0);
        const int currentDepth = treeView->depth(currentRow);
        const bool isChild = currentDepth > rowToExpandDepth;
        const bool isExpandable = model->rowCount(modelIndex) > 0;
        const bool shouldBeExpanded = isChild && isExpandable && currentDepth < effectiveMaxDepth;
        QCOMPARE(treeView->isExpanded(currentRow), shouldBeExpanded);
    }
}

void tst_qquicktreeview::expandRecursivelyChild_data()
{
    QTest::addColumn<int>("rowToExpand");
    QTest::addColumn<int>("depth");

    QTest::newRow("5, -1") << 4 << -1;
    QTest::newRow("5, 0") << 4 << 0;
    QTest::newRow("5, 1") << 4 << 1;
    QTest::newRow("5, 2") << 4 << 2;
    QTest::newRow("5, 3") << 4 << 3;
}

void tst_qquicktreeview::expandRecursivelyChild()
{
    // Check that we can first expand the root node, and the expand
    // recursive the first child node with children (row 4), and that all
    // its children of that node are expanded recursively according to depth.
    QFETCH(int, rowToExpand);
    QFETCH(int, depth);

    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int,int)));

    treeView->expand(0);

    QCOMPARE(spy.size(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == 0);
    QVERIFY(signalArgs.at(1).toInt() == 1);

    treeView->expandRecursively(rowToExpand, depth);

    if (depth == 0) {
        QCOMPARE(spy.size(), 0);
    } else {
        QCOMPARE(spy.size(), 1);
        signalArgs = spy.takeFirst();
        QVERIFY(signalArgs.at(0).toInt() == rowToExpand);
        QVERIFY(signalArgs.at(1).toInt() == depth);
    }

    WAIT_UNTIL_POLISHED;

    const int rowToExpandDepth = treeView->depth(rowToExpand);
    const int effectiveMaxDepth = depth != -1 ? rowToExpandDepth + depth : model->maxDepth();

    // Check that none of the rows before rowToExpand are expanded
    // (except the root node)
    for (int currentRow = 1; currentRow < rowToExpand; ++currentRow)
        QVERIFY(!treeView->isExpanded(currentRow));

    // Check if rowToExpand is expanded
    if (depth > 0 || depth == -1)
        QVERIFY(treeView->isExpanded(rowToExpand));
    else
        QVERIFY(!treeView->isExpanded(rowToExpand));

    // Check that any row after rowToExpand, that is also
    // a child of that row, is expanded (down to depth)
    for (int currentRow = rowToExpand + 1; currentRow < treeView->rows(); ++currentRow) {
        const int currentDepth = treeView->depth(currentRow);
        const bool isChild = currentDepth > rowToExpandDepth;
        const auto modelIndex = treeView->index(currentRow, 0);
        const bool isExpandable = model->rowCount(modelIndex) > 0;
        const bool shouldBeExpanded = isChild && isExpandable && currentDepth < effectiveMaxDepth;
        QCOMPARE(treeView->isExpanded(currentRow), shouldBeExpanded);
    }
}

void tst_qquicktreeview::expandRecursivelyWholeTree()
{
    // Check that we expand the whole tree recursively by passing -1, -1
    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int,int)));
    treeView->expandRecursively(-1, -1);

    QCOMPARE(spy.size(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == -1);
    QVERIFY(signalArgs.at(1).toInt() == -1);

    WAIT_UNTIL_POLISHED;

    // Check that all rows that have children are expanded
    for (int currentRow = 0; currentRow < treeView->rows(); ++currentRow) {
        const auto modelIndex = treeView->index(currentRow, 0);
        const bool isExpandable = model->rowCount(modelIndex) > 0;
        QCOMPARE(treeView->isExpanded(currentRow), isExpandable);
    }
}

void tst_qquicktreeview::collapseRecursivelyRoot()
{
    // Check that we can collapse the root node (row 0), and that
    // all its children are collapsed recursively down to the leaves.
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expandRecursively();
    WAIT_UNTIL_POLISHED;

    // Verify that the tree is now fully expanded
    // The number of rows should be the root, + 4 children per level. All parents
    // (minus the root), will also have a node with 4 non-recursive children.
    const int expectedRowCount = 1 + (model->maxDepth() * 8) - 4;
    QCOMPARE(treeView->rows(), expectedRowCount);

    QSignalSpy spy(treeView, SIGNAL(collapsed(int,bool)));

    // Collapse the whole tree again. This time, only the root should end up visible
    treeView->collapseRecursively();

    QCOMPARE(spy.size(), 1);
    const auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == -1);
    QVERIFY(signalArgs.at(1).toBool() == true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 1);

    // We need to check that all descendants are collapsed as well. But since they're
    // now no longer visible in the view, we need to expand each parent one by one again to make
    // them visible, and check that the child inside that has children is still collapsed.
    // We can do that by simply iterate over the rows in the view as we expand.
    int currentRow = 0;
    while (currentRow < treeView->rows()) {
        const QModelIndex currentIndex = treeView->index(currentRow, 0);
        if (model->hasChildren(currentIndex)) {
            QVERIFY(!treeView->isExpanded(currentRow));
            treeView->expand(currentRow);
            WAIT_UNTIL_POLISHED;
        }
        currentRow++;
    }

    // Sanity check that we ended up with all rows expanded again
    QCOMPARE(currentRow, expectedRowCount);
}

void tst_qquicktreeview::collapseRecursivelyChild()
{
    // Check that we can collapse a child node (row 4), and that all its children
    // are collapsed recursively down to the leaves (without touching the root).
    LOAD_TREEVIEW("normaltreeview.qml");
    treeView->expandRecursively();
    WAIT_UNTIL_POLISHED;

    // Verify that the tree is now fully expanded
    // The number of rows should be the root, + 4 children per level. All parents
    // (minus the root), will also have a node with 4 non-recursive children.
    const int expectedRowCount = 1 + (model->maxDepth() * 8) - 4;
    QCOMPARE(treeView->rows(), expectedRowCount);

    QSignalSpy spy(treeView, SIGNAL(collapsed(int,bool)));

    // Collapse the 8th child recursive
    const int rowToCollapse = 8;
    const QModelIndex collapseIndex = treeView->index(rowToCollapse, 0);
    const auto expectedLabel = model->data(collapseIndex, Qt::DisplayRole);
    QCOMPARE(expectedLabel, QStringLiteral("3, 0"));
    treeView->collapseRecursively(rowToCollapse);

    QCOMPARE(spy.size(), 1);
    const auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == rowToCollapse);
    QVERIFY(signalArgs.at(1).toBool() == true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 9); // root + 4 children + 4 grand children of the 3rd row

    // We need to check that all descendants are collapsed as well. But since they're
    // now no longer visible in the view, we need to expand each parent one by one again to make
    // them visible, and check that the child inside that has children is still collapsed.
    // We can do that by simply iterate over the rows in the view as we expand.
    int currentRow = 1; // start at first child
    while (currentRow < treeView->rows()) {
        const QModelIndex currentIndex = treeView->index(currentRow, 0);
        if (model->hasChildren(currentIndex)) {
            if (treeView->depth(currentRow) == 1 && currentIndex.row() == 2) {
                // We did only recursively expand the 4th child, so the
                // third should still be expanded
                QVERIFY(treeView->isExpanded(currentRow));
            } else {
                QVERIFY(!treeView->isExpanded(currentRow));
                treeView->expand(currentRow);
                WAIT_UNTIL_POLISHED;
            }
        }
        currentRow++;
    }

    // Sanity check that we ended up with all rows expanded again
    QCOMPARE(currentRow, expectedRowCount);
}

void tst_qquicktreeview::collapseRecursivelyWholeTree()
{
    // Check that we collapse the whole tree recursively by passing -1
    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(collapsed(int,bool)));
    treeView->expandRecursively();
    treeView->collapseRecursively();

    QCOMPARE(spy.size(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == -1);
    QVERIFY(signalArgs.at(1).toBool() == true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->rows(), 1); // root
}

void tst_qquicktreeview::expandToIndex()
{
    // Check that expandToIndex(index) expands the tree so
    // that index becomes visible in the view
    LOAD_TREEVIEW("normaltreeview.qml");
    QSignalSpy spy(treeView, SIGNAL(expanded(int,int)));

    const QModelIndex root = model->index(0, 0);
    const QModelIndex child1 = model->index(3, 0, root);
    const QModelIndex child2 = model->index(3, 0, child1);

    QVERIFY(model->hasChildren(root));
    QVERIFY(model->hasChildren(child1));
    QVERIFY(model->hasChildren(child2));

    QVERIFY(!treeView->isExpanded(treeView->rowAtIndex(root)));
    QVERIFY(!treeView->isExpanded(treeView->rowAtIndex(child1)));
    QVERIFY(!treeView->isExpanded(treeView->rowAtIndex(child2)));

    const QModelIndex childToExpand = model->index(1, 0, child2);
    treeView->expandToIndex(childToExpand);

    QVERIFY(treeView->isExpanded(treeView->rowAtIndex(root)));
    QVERIFY(treeView->isExpanded(treeView->rowAtIndex(child1)));
    QVERIFY(treeView->isExpanded(treeView->rowAtIndex(child2)));

    QCOMPARE(spy.size(), 1);
    auto signalArgs = spy.takeFirst();
    QVERIFY(signalArgs.at(0).toInt() == 0);
    QVERIFY(signalArgs.at(1).toInt() == 3);

    WAIT_UNTIL_POLISHED;

    // The view should now have 13 rows:
    // root + 3 expanded nodes that each have 4 children
    QCOMPARE(treeView->rows(), 13);
}

void tst_qquicktreeview::toggleExpandedUsingArrowKeys()
{
    // Check that you can use the left and right arrow key to
    // expand and collapse nodes in the tree.
    LOAD_TREEVIEW("normaltreeview.qml");

    treeView->setFocus(true);
    QQuickWindow *window = treeView->window();

    // Start by making cell 0, 0 current
    treeView->selectionModel()->setCurrentIndex(treeView->index(0, 0), QItemSelectionModel::NoUpdate);

    // Expand row 0
    const int row0 = 0;
    QCOMPARE(treeView->rows(), 1);
    QVERIFY(!treeView->isExpanded(row0));
    QTest::keyPress(window, Qt::Key_Right);
    QVERIFY(treeView->isExpanded(row0));

    // A polish event was scheduled, but since the call to keyPress() processes
    // events, WAIT_UNTIL_POLISHED will be unreliable. So use QTRY_COMPARE instead.
    QTRY_COMPARE(treeView->rows(), 5);

    // Hitting Key_Right again should be a no-op
    QTest::keyPress(window, Qt::Key_Right);
    QVERIFY(treeView->isExpanded(row0));
    QCOMPARE(treeView->selectionModel()->currentIndex(), treeView->index(row0, 0));

    // Move down to row 1 and try to expand it. Since Row 1
    // doesn't have children, expanding it will be a no-op.
    // And also, it shouldn't move currentIndex to the next
    // column either, it should stay in the tree column.
    const int row1 = 1;
    QVERIFY(!treeView->isExpanded(row1));
    QTest::keyPress(window, Qt::Key_Down);
    QTest::keyPress(window, Qt::Key_Right);
    QVERIFY(!treeView->isExpanded(row1));
    QCOMPARE(treeView->selectionModel()->currentIndex(), treeView->index(row1, 0));

    // Move down to row 4 and expand it
    const int row4 = 4;
    QCOMPARE(treeView->rows(), 5);
    while (treeView->currentRow() != row4)
        QTest::keyPress(window, Qt::Key_Down);
    QVERIFY(!treeView->isExpanded(row4));
    QTest::keyPress(window, Qt::Key_Right);
    QVERIFY(treeView->isExpanded(row4));
    QCOMPARE(treeView->selectionModel()->currentIndex(), treeView->index(row4, 0));

    // Move up again to row 0 and collapse it
    while (treeView->currentRow() != row0)
        QTest::keyPress(window, Qt::Key_Up);
    QVERIFY(treeView->isExpanded(row0));
    QTest::keyPress(window, Qt::Key_Left);
    QVERIFY(!treeView->isExpanded(row0));
    QTRY_COMPARE(treeView->rows(), 1);

    // Hitting Key_Left again should be a no-op
    QTest::keyPress(window, Qt::Key_Left);
    QVERIFY(!treeView->isExpanded(row0));
    QCOMPARE(treeView->selectionModel()->currentIndex(), treeView->index(row0, 0));
}

void tst_qquicktreeview::expandAndCollapsUsingDoubleClick()
{
    LOAD_TREEVIEW("normaltreeview.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    // Expand the root by double clicking on the row
    const auto item = treeView->itemAtIndex(treeView->index(0, 0));
    QVERIFY(item);
    const QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    const QPoint pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseDClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    // We now expect 5 rows, the root pluss it's 4 children. Since
    // mouseDClick calls processEvents(), it becomes random at this
    // point if the view has been polished or not. So use QTRY_COMPARE.
    QTRY_COMPARE(treeViewPrivate->loadedRows.count(), 5);

    // Collapse the root again
    QTest::mouseDClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    QTRY_COMPARE(treeViewPrivate->loadedRows.count(), 1);

    // If edit triggers has DoubleTapped set, we should
    // start to edit instead of expanding.
    treeView->setEditTriggers(QQuickTableView::DoubleTapped);
    QTest::mouseDClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    if (QQuickTest::qIsPolishScheduled(treeView))
        QVERIFY(QQuickTest::qWaitForPolish(treeView));
    QTRY_COMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeview::selectionBehaviorCells_data()
{
    QTest::addColumn<QPoint>("startCell");
    QTest::addColumn<QPoint>("endCellDist");

    QTest::newRow("QPoint(0, 0), QPoint(0, 0)") << QPoint(0, 0) << QPoint(0, 0);

    QTest::newRow("QPoint(0, 1), QPoint(0, 1)") << QPoint(0, 1) << QPoint(0, 1);
    QTest::newRow("QPoint(0, 2), QPoint(0, 2)") << QPoint(0, 2) << QPoint(0, 2);

    QTest::newRow("QPoint(2, 2), QPoint(0, 0)") << QPoint(2, 2) << QPoint(0, 0);
    QTest::newRow("QPoint(2, 2), QPoint(1, 0)") << QPoint(2, 2) << QPoint(1, 0);
    QTest::newRow("QPoint(2, 2), QPoint(2, 0)") << QPoint(2, 2) << QPoint(2, 0);
    QTest::newRow("QPoint(2, 2), QPoint(-1, 0)") << QPoint(2, 2) << QPoint(-1, 0);
    QTest::newRow("QPoint(2, 2), QPoint(-2, 0)") << QPoint(2, 2) << QPoint(-2, 0);
    QTest::newRow("QPoint(2, 2), QPoint(0, 1)") << QPoint(2, 2) << QPoint(0, 1);
    QTest::newRow("QPoint(2, 2), QPoint(0, 2)") << QPoint(2, 2) << QPoint(0, 2);
    QTest::newRow("QPoint(2, 2), QPoint(0, -1)") << QPoint(2, 2) << QPoint(0, -1);
    QTest::newRow("QPoint(2, 2), QPoint(0, -2)") << QPoint(2, 2) << QPoint(0, -2);

    QTest::newRow("QPoint(2, 2), QPoint(1, 1)") << QPoint(2, 2) << QPoint(1, 1);
    QTest::newRow("QPoint(2, 2), QPoint(1, 2)") << QPoint(2, 2) << QPoint(1, 2);
    QTest::newRow("QPoint(2, 2), QPoint(1, -1)") << QPoint(2, 2) << QPoint(1, -1);
    QTest::newRow("QPoint(2, 2), QPoint(1, -2)") << QPoint(2, 2) << QPoint(1, -2);

    QTest::newRow("QPoint(2, 2), QPoint(-1, 1)") << QPoint(2, 2) << QPoint(-1, 1);
    QTest::newRow("QPoint(2, 2), QPoint(-1, 2)") << QPoint(2, 2) << QPoint(-1, 2);
    QTest::newRow("QPoint(2, 2), QPoint(-1, -1)") << QPoint(2, 2) << QPoint(-1, -1);
    QTest::newRow("QPoint(2, 2), QPoint(-1, -2)") << QPoint(2, 2) << QPoint(-1, -2);
}

void tst_qquicktreeview::selectionBehaviorCells()
{
    // Check that the TreeView implement the overridden updateSelection()
    // function correctly wrt QQuickTableView::SelectCells.
    QFETCH(QPoint, startCell);
    QFETCH(QPoint, endCellDist);
    LOAD_TREEVIEW("normaltreeview.qml");

    const auto selectionModel = treeView->selectionModel();
    treeView->expand(0);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel->hasSelection(), false);
    treeView->setSelectionBehavior(QQuickTableView::SelectCells);

    const QPoint endCell = startCell + endCellDist;
    const QPoint endCellWrapped = startCell - endCellDist;

    const QQuickItem *startItem = treeView->itemAtCell(startCell);
    const QQuickItem *endItem = treeView->itemAtCell(endCell);
    const QQuickItem *endItemWrapped = treeView->itemAtCell(endCellWrapped);
    QVERIFY(startItem);
    QVERIFY(endItem);
    QVERIFY(endItemWrapped);

    const QPointF startPos(startItem->x(), startItem->y());
    const QPointF endPos(endItem->x(), endItem->y());
    const QPointF endPosWrapped(endItemWrapped->x(), endItemWrapped->y());

    QVERIFY(treeViewPrivate->startSelection(startPos, Qt::NoModifier));
    treeViewPrivate->setSelectionStartPos(startPos);
    treeViewPrivate->setSelectionEndPos(endPos);

    QCOMPARE(selectionModel->hasSelection(), true);

    const int x1 = qMin(startCell.x(), endCell.x());
    const int x2 = qMax(startCell.x(), endCell.x());
    const int y1 = qMin(startCell.y(), endCell.y());
    const int y2 = qMax(startCell.y(), endCell.y());

    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            const auto index = treeView->index(y, x);
            QVERIFY(selectionModel->isSelected(index));
        }
    }

    const int expectedCount = (x2 - x1 + 1) * (y2 - y1 + 1);
    const int actualCount = selectionModel->selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    // Wrap the selection
    treeViewPrivate->setSelectionEndPos(endPosWrapped);

    for (int x = x2; x < x1; ++x) {
        for (int y = y2; y < y1; ++y) {
            const auto index = model->index(y, x);
            QVERIFY(selectionModel->isSelected(index));
        }
    }

    const int actualCountAfterWrap = selectionModel->selectedIndexes().size();
    QCOMPARE(actualCountAfterWrap, expectedCount);

    treeViewPrivate->clearSelection();
    QCOMPARE(selectionModel->hasSelection(), false);
}

void tst_qquicktreeview::selectionBehaviorRows()
{
    // Check that the TreeView implement the overridden updateSelection()
    // function correctly wrt QQuickTableView::SelectionRows.
    LOAD_TREEVIEW("normaltreeview.qml");

    const auto selectionModel = treeView->selectionModel();
    QCOMPARE(treeView->selectionBehavior(), QQuickTableView::SelectRows);
    treeView->expand(0);
    treeView->setInteractive(false);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel->hasSelection(), false);

    // Drag from row 0 to row 3
    QVERIFY(treeViewPrivate->startSelection(QPointF(0, 0), Qt::NoModifier));
    treeViewPrivate->setSelectionStartPos(QPointF(0, 0));
    treeViewPrivate->setSelectionEndPos(QPointF(80, 60));

    QCOMPARE(selectionModel->hasSelection(), true);

    const int expectedCount = treeView->columns() * 3; // all columns * three rows
    int actualCount = selectionModel->selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < treeView->columns(); ++x) {
        for (int y = 0; y < 3; ++y) {
            const auto index = treeView->index(y, x);
            QVERIFY(selectionModel->isSelected(index));
        }
    }

    selectionModel->clear();
    QCOMPARE(selectionModel->hasSelection(), false);

    // Drag from row 3 to row 0 (and overshoot mouse)
    QVERIFY(treeViewPrivate->startSelection(QPointF(80, 60), Qt::NoModifier));
    treeViewPrivate->setSelectionStartPos(QPointF(80, 60));
    treeViewPrivate->setSelectionEndPos(QPointF(-10, -10));

    QCOMPARE(selectionModel->hasSelection(), true);

    actualCount = selectionModel->selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < treeView->columns(); ++x) {
        for (int y = 0; y < 3; ++y) {
            const auto index = treeView->index(y, x);
            QVERIFY(selectionModel->isSelected(index));
        }
    }
}

void tst_qquicktreeview::selectionBehaviorColumns()
{
    // Check that the TreeView implement the overridden updateSelection()
    // function correctly wrt QQuickTableView::SelectColumns.
    LOAD_TREEVIEW("normaltreeview.qml");

    const auto selectionModel = treeView->selectionModel();
    treeView->setSelectionBehavior(QQuickTableView::SelectColumns);
    treeView->expand(0);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel->hasSelection(), false);

    // Drag from column 0 to column 3
    QVERIFY(treeViewPrivate->startSelection(QPointF(0, 0), Qt::NoModifier));
    treeViewPrivate->setSelectionStartPos(QPointF(0, 0));
    treeViewPrivate->setSelectionEndPos(QPointF(225, 90));

    QCOMPARE(selectionModel->hasSelection(), true);

    const int expectedCount = treeView->rows() * 3; // all rows * three columns
    int actualCount = selectionModel->selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < treeView->rows(); ++y) {
            const auto index = treeView->index(y, x);
            QVERIFY(selectionModel->isSelected(index));
        }
    }

    selectionModel->clear();
    QCOMPARE(selectionModel->hasSelection(), false);

    // Drag from column 3 to column 0 (and overshoot mouse)
    QVERIFY(treeViewPrivate->startSelection(QPointF(225, 90), Qt::NoModifier));
    treeViewPrivate->setSelectionStartPos(QPointF(225, 90));
    treeViewPrivate->setSelectionEndPos(QPointF(-10, -10));

    QCOMPARE(selectionModel->hasSelection(), true);

    actualCount = selectionModel->selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < treeView->rows(); ++y) {
            const auto index = treeView->index(y, x);
            QVERIFY(selectionModel->isSelected(index));
        }
    }
}

void tst_qquicktreeview::selectionBehaviorDisabled()
{
    // Check that the TreeView implement the overridden updateSelection()
    // function correctly wrt QQuickTableView::SelectionDisabled.
    LOAD_TREEVIEW("normaltreeview.qml");

    const auto selectionModel = treeView->selectionModel();
    treeView->setSelectionBehavior(QQuickTableView::SelectionDisabled);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel->hasSelection(), false);

    // Try to start a selection. treeViewPrivate->startSelection() should
    // reject that, and and return false. The selectionFlag will there stay as
    // QItemSelectionModel::NoUpdate, meaning no active selection is ongoing.
    QVERIFY(!treeViewPrivate->startSelection(QPointF(0, 0), Qt::NoModifier));
    QCOMPARE(treeViewPrivate->selectionFlag, QItemSelectionModel::NoUpdate);
    QCOMPARE(selectionModel->hasSelection(), false);
}

void tst_qquicktreeview::sortTreeModel_data()
{
    QTest::addColumn<QSharedPointer<QAbstractItemModel>>("treeModel");

    const auto stringList = QStringList() << "1" << "2" << "3";
    QTest::newRow("TreeModel") << QSharedPointer<QAbstractItemModel>(new TestModel());
    QTest::newRow("Number model") << QSharedPointer<QAbstractItemModel>(new QStringListModel(stringList));
}

void tst_qquicktreeview::sortTreeModel()
{
    // Check that if you assign a QSortFilterProxyModel to to a TreeView, the
    // view will end up sorted correctly if the proxy model is sorted.
    QFETCH(QSharedPointer<QAbstractItemModel>, treeModel);
    LOAD_TREEVIEW("normaltreeview.qml");

    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(treeModel.data());
    treeView->setModel(QVariant::fromValue(&proxyModel));

    // Expand some nodes
    treeView->expand(0);
    treeView->expand(4);
    treeView->expand(3);

    WAIT_UNTIL_POLISHED;

    // Go through all rows in the view, and check that display in the model
    // is the same as in the view. That means that QQmlTreeModelToTableModel
    // and QSortFilterProxyModel are in sync.
    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = treeView->index(row, 0);
        const QString modelDisplay = proxyModel.data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }

    // Now sort the model, and do the same test again
    proxyModel.sort(0, Qt::DescendingOrder);
    WAIT_UNTIL_POLISHED;

    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = treeView->index(row, 0);
        const QString modelDisplay = proxyModel.data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }
}

void tst_qquicktreeview::sortTreeModelDynamic_data()
{
    QTest::addColumn<QSharedPointer<QAbstractItemModel>>("treeModel");
    QTest::addColumn<int>("row");

    const auto stringList = QStringList() << "1" << "2" << "3";
    QTest::newRow("TreeModel 0") << QSharedPointer<QAbstractItemModel>(new TestModel()) << 0;
    QTest::newRow("TreeModel 1") << QSharedPointer<QAbstractItemModel>(new TestModel()) << 1;
    QTest::newRow("TreeModel 3") << QSharedPointer<QAbstractItemModel>(new TestModel()) << 3;
    QTest::newRow("TreeModel 6") << QSharedPointer<QAbstractItemModel>(new TestModel()) << 6;
    QTest::newRow("Number model") << QSharedPointer<QAbstractItemModel>(new QStringListModel(stringList)) << 1;
}

void tst_qquicktreeview::sortTreeModelDynamic()
{
    // Check that if you assign a QSortFilterProxyModel to a TreeView, and
    // set DynamicSortFilter to true, the view will end up sorted correctly
    // if you change the text for one of the items.
    QFETCH(QSharedPointer<QAbstractItemModel>, treeModel);
    QFETCH(int, row);
    LOAD_TREEVIEW("normaltreeview.qml");

    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(treeModel.data());
    proxyModel.setDynamicSortFilter(true);
    proxyModel.sort(Qt::AscendingOrder);
    treeView->setModel(QVariant::fromValue(&proxyModel));

    // Expand some nodes
    treeView->expand(0);
    treeView->expand(4);
    treeView->expand(3);

    // Go through all rows in the view, and check that display in the model
    // is the same as in the view. That means that QQmlTreeModelToTableModel
    // and QSortFilterProxyModel are in sync.
    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = treeView->index(row, 0);
        const QString modelDisplay = proxyModel.data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }

    // Now change the text in one of the items. This will trigger
    // a sort for only one of the parents in the model.
    proxyModel.setData(treeView->index(row, 0), u"xxx"_s, Qt::DisplayRole);

    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = treeView->index(row, 0);
        const QString modelDisplay = proxyModel.data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }
}

void tst_qquicktreeview::setRootIndex()
{
    // Check that if you can change the root index in the view to point
    // at a child branch in the model
    LOAD_TREEVIEW("normaltreeview.qml");

    const QModelIndex rootIndex = model->index(0, 0);
    const QModelIndex childIndex = model->index(3, 0, rootIndex);
    QVERIFY(model->hasChildren(childIndex));
    treeView->setRootIndex(childIndex);

    // Go through all rows in the view, and check that view shows the
    // same display text as the display role in the model (under the
    // given root).
    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = model->index(row, 0, childIndex);
        const QString modelDisplay = model->data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }

    // Do the same once more, but this time choose a child that is deeper in the model
    const QModelIndex childIndex2 = model->index(3, 0, childIndex);
    QVERIFY(model->hasChildren(childIndex2));
    treeView->setRootIndex(childIndex);

    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = model->index(row, 0, childIndex2);
        const QString modelDisplay = model->data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }

    // Reset rootIndex. This should show the whole model again
    treeView->setRootIndex(QModelIndex());

    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = model->index(row, 0);
        const QString modelDisplay = model->data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }
}

void tst_qquicktreeview::setRootIndexToLeaf()
{
    // When you set a custom root index, the root index itself will not
    // be shown. Therefore, check that if you change the root index to a
    // leaf in the model, TreeView will be empty.
    LOAD_TREEVIEW("normaltreeview.qml");

    const QModelIndex rootIndex = model->index(0, 0);
    const QModelIndex leafIndex = model->index(1, 0, rootIndex);
    QVERIFY(!model->hasChildren(leafIndex));
    treeView->setRootIndex(leafIndex);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(treeView->rows(), 0);

    // According to the docs, you can set rootIndex to undefined
    // in order to show the whole model again. This is the same
    // as calling 'reset' on the property from c++. Verify that this works.
    const QMetaObject *metaObject = treeView->metaObject();
    const int propertyIndex = metaObject->indexOfProperty("rootIndex");
    QVERIFY(propertyIndex != -1);
    metaObject->property(propertyIndex).reset(treeView);

    for (int row = 0; row < treeView->rows(); ++row) {
        const auto index = model->index(row, 0);
        const QString modelDisplay = model->data(index, Qt::DisplayRole).toString();
        const auto childFxItem = treeViewPrivate->loadedTableItem(QPoint(0, row));
        QVERIFY(childFxItem);
        const auto childItem = childFxItem->item;
        QVERIFY(childItem);
        const auto context = qmlContext(childItem.data());
        const auto itemDisplay = context->contextProperty("display").toString();
        QCOMPARE(itemDisplay, modelDisplay);
    }
}

void tst_qquicktreeview::editUsingEditTriggers_data()
{
    QTest::addColumn<QQuickTreeView::EditTriggers>("editTriggers");
    QTest::addColumn<bool>("interactive");

    QTest::newRow("NoEditTriggers") << QQuickTreeView::EditTriggers(QQuickTreeView::NoEditTriggers);
    QTest::newRow("SingleTapped") << QQuickTreeView::EditTriggers(QQuickTreeView::SingleTapped);
    QTest::newRow("DoubleTapped") << QQuickTreeView::EditTriggers(QQuickTreeView::DoubleTapped);
    QTest::newRow("SelectedTapped") << QQuickTreeView::EditTriggers(QQuickTreeView::SelectedTapped);
    QTest::newRow("EditKeyPressed") << QQuickTreeView::EditTriggers(QQuickTreeView::EditKeyPressed);
    QTest::newRow("AnyKeyPressed") << QQuickTreeView::EditTriggers(QQuickTreeView::AnyKeyPressed);
    QTest::newRow("DoubleTapped | EditKeyPressed")
        << QQuickTreeView::EditTriggers(QQuickTreeView::DoubleTapped | QQuickTreeView::EditKeyPressed);
    QTest::newRow("SingleTapped | AnyKeyPressed")
        << QQuickTreeView::EditTriggers(QQuickTreeView::SingleTapped | QQuickTreeView::AnyKeyPressed);
}

void tst_qquicktreeview::editUsingEditTriggers()
{
    // Check that you can start to edit in treeView
    // using the available edit triggers.
    QFETCH(QQuickTreeView::EditTriggers, editTriggers);
    LOAD_TREEVIEW("editdelegate.qml");

    TestModel testModel;
    treeView->setModel(QVariant::fromValue(&testModel));
    treeView->forceActiveFocus();
    treeView->expand(0);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(treeView->editTriggers(), QQuickTreeView::EditKeyPressed);
    treeView->setEditTriggers(editTriggers);

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());

    const QPoint cell1(0, 0);
    const QPoint cell2(1, 1);
    const QModelIndex index1 = treeView->modelIndex(cell1);
    const QModelIndex index2 = treeView->modelIndex(cell2);
    const auto item1 = treeView->itemAtCell(cell1);
    const auto item2 = treeView->itemAtCell(cell2);
    QVERIFY(item1);
    QVERIFY(item2);

    testModel.m_editableIndices = { index1, index2 };

    QQuickWindow *window = treeView->window();

    const QPoint localPos = QPoint(item1->width() - 1, item1->height() - 1);
    const QPoint localPosOutside = QPoint(treeView->contentWidth() + 10, treeView->contentHeight() + 10);
    const QPoint tapPos1 = window->contentItem()->mapFromItem(item1, localPos).toPoint();
    const QPoint tapPos2 = window->contentItem()->mapFromItem(item2, localPos).toPoint();
    const QPoint tapOutsideContentItem = window->contentItem()->mapFromItem(item2, localPosOutside).toPoint();

    if (editTriggers & QQuickTreeView::SingleTapped) {
        // edit cell 1
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);
        const auto editItem1 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);

        // edit cell 2 (without closing the previous edit session first)
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index2);
        const auto editItem2 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem2);
        QVERIFY(editItem2->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index2);

        // single tap outside content item should close the editor
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapOutsideContentItem);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(treeView->selectionModel()->currentIndex(), index2);
    }

    if (editTriggers & QQuickTreeView::DoubleTapped) {
        // edit cell 1
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);
        const auto editItem1 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);

        // edit cell 2 (without closing the previous edit session first)
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index2);
        const auto editItem2 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem2);
        QVERIFY(editItem2->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index2);

        // single tap outside the edit item should close the editor
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);

        if (!(editTriggers & QQuickTreeView::SingleTapped)) {
            // single tap on a cell should not open the editor
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
            QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
            QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        }

        // single tap outside content item should make sure editing ends
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapOutsideContentItem);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::SelectedTapped) {
        // select cell first, then tap on it
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::Select);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);
        const auto editItem1 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);

        // tap on a non-selected cell. This should close the editor, and move
        // the current index, but not begin to edit the cell.
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index2);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());

        // tap on a non-selected cell while no editor is active
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(treeView->selectionModel()->currentIndex(), index2);

        // tap on the current cell. This alone should not start an edit (unless it's also selected)
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::EditKeyPressed) {
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Return);
        const auto editItem1 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);

        // Pressing escape should close the editor
        QTest::keyClick(window, Qt::Key_Escape);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);

        // Pressing Enter to open the editor again
        QTest::keyClick(window, Qt::Key_Enter);
        const auto editItem2 = treeView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem2);
        QVERIFY(editItem2->hasActiveFocus());
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);

        // single tap outside the edit item should close the editor
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::AnyKeyPressed) {
        // Pressing key x should start to edit. And in case of AnyKeyPressed, we
        // also replay the key event to the focus object.
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_X);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);
        auto textInput1 = treeView->property(kEditItem).value<QQuickTextInput *>();
        QVERIFY(textInput1);
        QVERIFY(textInput1->hasActiveFocus());
        QCOMPARE(textInput1->text(), "x");

        // Pressing escape should close the editor
        QTest::keyClick(window, Qt::Key_Escape);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);

        // Pressing a modifier key alone should not open the editor
        QTest::keyClick(window, Qt::Key_Shift);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QTest::keyClick(window, Qt::Key_Control);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QTest::keyClick(window, Qt::Key_Alt);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QTest::keyClick(window, Qt::Key_Meta);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());

        // Pressing enter should also start to edit. But this is a
        // special case, we don't replay enter into the focus object.
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Enter);
        QCOMPARE(treeView->selectionModel()->currentIndex(), index1);
        QCOMPARE(treeView->property(kEditIndex).value<QModelIndex>(), index1);
        auto textInput2 = treeView->property(kEditItem).value<QQuickTextInput *>();
        QVERIFY(textInput2);
        QVERIFY(textInput2->hasActiveFocus());

        if (!(editTriggers & QQuickTreeView::SingleTapped)) {
            // single tap outside the edit item should close the editor
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
            QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
            QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        }

        // single tap outside content item should make sure editing ends
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapOutsideContentItem);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers == QQuickTreeView::NoEditTriggers) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }
}

void tst_qquicktreeview::editOnNonEditableCell_data()
{
    QTest::addColumn<QQuickTreeView::EditTriggers>("editTriggers");

    QTest::newRow("SingleTapped") << QQuickTreeView::EditTriggers(QQuickTreeView::SingleTapped);
    QTest::newRow("DoubleTapped") << QQuickTreeView::EditTriggers(QQuickTreeView::DoubleTapped);
    QTest::newRow("SelectedTapped") << QQuickTreeView::EditTriggers(QQuickTreeView::SelectedTapped);
    QTest::newRow("EditKeyPressed") << QQuickTreeView::EditTriggers(QQuickTreeView::EditKeyPressed);
    QTest::newRow("AnyKeyPressed") << QQuickTreeView::EditTriggers(QQuickTreeView::EditKeyPressed);
}

void tst_qquicktreeview::editOnNonEditableCell()
{
    // Check that the user cannot edit a non-editable cell from the edit triggers.
    // Note: we don't want TreeView to print out warnings in this case, since
    // the user is not doing anything wrong. We only want to print out warnings if
    // the application is calling edit() explicitly on a cell that cannot be edited
    // (separate test below).
    QFETCH(QQuickTreeView::EditTriggers, editTriggers);
    LOAD_TREEVIEW("editdelegate.qml");

    TestModel testModel;
    treeView->setModel(QVariant::fromValue(&testModel));
    treeView->forceActiveFocus();
    treeView->expand(0);

    WAIT_UNTIL_POLISHED;

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    const QPoint cell(1, 1);
    const QModelIndex index1 = treeView->modelIndex(cell);
    const auto item = treeView->itemAtCell(cell);
    QVERIFY(item);

    QQuickWindow *window = treeView->window();

    const QPoint localPos = QPoint(item->width() - 1, item->height() - 1);
    const QPoint tapPos = window->contentItem()->mapFromItem(item, localPos).toPoint();

    if (editTriggers & QQuickTreeView::SingleTapped) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::DoubleTapped) {
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::SelectedTapped) {
        // select cell first, then tap on it
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::EditKeyPressed) {
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTreeView::AnyKeyPressed) {
        treeView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!treeView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!treeView->property(kEditIndex).value<QModelIndex>().isValid());
    }
}

QTEST_MAIN(tst_qquicktreeview)

#include "tst_qquicktreeview.moc"
