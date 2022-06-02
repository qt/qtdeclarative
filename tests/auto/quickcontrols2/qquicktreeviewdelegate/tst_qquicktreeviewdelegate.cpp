/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuick/private/qquicktreeview_p_p.h>

#include <QtQuickTemplates2/private/qquicktreeviewdelegate_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>

#include "testmodel.h"

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

#define LOAD_TREEVIEW(fileName) \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    QVERIFY(QTest::qWaitForWindowActive(view)); \
    auto treeView = view->rootObject()->property("treeView").value<QQuickTreeView *>(); \
    QVERIFY(treeView); \
    auto treeViewPrivate = QQuickTreeViewPrivate::get(treeView); \
    Q_UNUSED(treeViewPrivate) \
    auto model = treeView->model().value<TestModel *>(); \
    Q_UNUSED(model)

#define WAIT_UNTIL_POLISHED_ARG(item) \
    QVERIFY(QQuickTest::qWaitForPolish(item))
#define WAIT_UNTIL_POLISHED WAIT_UNTIL_POLISHED_ARG(treeView)

// ########################################################

class tst_qquicktreeviewdelegate : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktreeviewdelegate();

private:
    QQuickView *view = nullptr;

private slots:
    void initTestCase() override;
    void showTreeView();
    void expandAndCollapsUsingDoubleClick();
    void expandAndCollapseClickOnIndicator_data();
    void expandAndCollapseClickOnIndicator();
    void pointerNavigationDisabled();
    void checkPropertiesRoot();
    void checkPropertiesChildren();
    void checkCurrentIndex();
    void checkClickedSignal();
    void clearSelectionOnClick();
};

tst_qquicktreeviewdelegate::tst_qquicktreeviewdelegate()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquicktreeviewdelegate::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestModel>("TestModel", 1, 0, "TestModel");
    view = createView();
}

void tst_qquicktreeviewdelegate::showTreeView()
{
    LOAD_TREEVIEW("unmodified.qml");
    // Check that the view is showing the root of the tree
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeviewdelegate::expandAndCollapsUsingDoubleClick()
{
    LOAD_TREEVIEW("unmodified.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    // Expand the root by double clicking on the row
    const auto item = treeView->itemAtCell(0, 0);
    QVERIFY(item);
    const QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    const QPoint pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseDClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    // Collapse the root again
    QTest::mouseDClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    WAIT_UNTIL_POLISHED;
    // Check that the view only has one row loaded again (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
}

void tst_qquicktreeviewdelegate::expandAndCollapseClickOnIndicator_data()
{
    QTest::addColumn<bool>("interactive");
    QTest::newRow("interactive") << true;
    QTest::newRow("not interactive") << false;
}

void tst_qquicktreeviewdelegate::expandAndCollapseClickOnIndicator()
{
    QFETCH(bool, interactive);

    LOAD_TREEVIEW("unmodified.qml");

    treeView->setInteractive(interactive);

    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    // Expand the root by clicking on the indicator
    const auto item = qobject_cast<QQuickTreeViewDelegate *>(treeView->itemAtCell(0, 0));
    QVERIFY(item);
    const auto indicator = item->indicator();
    const QPoint localPos = QPoint(indicator->width() / 2, indicator->height() / 2);
    const QPoint pos = item->window()->contentItem()->mapFromItem(indicator, localPos).toPoint();

    // When treeview is interactive, we toggle expanded on pointer release
    // to not interfere with flicking. Otherwise we expand already on press.
    if (interactive)
        QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    else
        QTest::mousePress(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    WAIT_UNTIL_POLISHED;
    // We now expect 5 rows, the root pluss it's 4 children
    QCOMPARE(treeViewPrivate->loadedRows.count(), 5);

    if (!interactive)
        QTest::mouseRelease(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    // Collapse the root again
    if (interactive)
        QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    else
        QTest::mousePress(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    WAIT_UNTIL_POLISHED;
    // Check that the view only has one row loaded again (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    if (!interactive)
        QTest::mouseRelease(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
}

void tst_qquicktreeviewdelegate::pointerNavigationDisabled()
{
    // Check that treeview respects TableView.pointerNavigationEnabled.
    // When set to false, TreeView should not handle any pointer navigation events.
    LOAD_TREEVIEW("unmodified.qml");
    treeView->setPointerNavigationEnabled(false);

    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);
    QVERIFY(!treeView->isExpanded(0));

    // Try to expand the root by clicking on the indicator
    const auto item = qobject_cast<QQuickTreeViewDelegate *>(treeView->itemAtCell(0, 0));
    QVERIFY(item);
    const auto indicator = item->indicator();
    QPoint localPos = QPoint(indicator->width() / 2, indicator->height() / 2);
    QPoint pos = item->window()->contentItem()->mapFromItem(indicator, localPos).toPoint();
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    // The tree should still be collapsed
    QVERIFY(!treeView->isExpanded(0));
    QVERIFY(!QQuickTest::qIsPolishScheduled(item));

    // Try a double click
    localPos = QPoint(item->width() / 2, item->height() / 2);
    pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseDClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY(!treeView->isExpanded(0));
    QVERIFY(!QQuickTest::qIsPolishScheduled(item));
}

void tst_qquicktreeviewdelegate::checkPropertiesRoot()
{
    LOAD_TREEVIEW("unmodified.qml");
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    const auto rootFxItem = treeViewPrivate->loadedTableItem(QPoint(0, 0));
    QVERIFY(rootFxItem);
    const auto rootItem = qobject_cast<QQuickTreeViewDelegate *>(rootFxItem->item);
    QVERIFY(rootItem);

    QCOMPARE(rootItem->treeView(), treeView);
    QCOMPARE(rootItem->isTreeNode(), true);
    QCOMPARE(rootItem->expanded(), false);
    QCOMPARE(rootItem->hasChildren(), true);
    QCOMPARE(rootItem->depth(), 0);
    QCOMPARE(rootItem->leftPadding(), rootItem->leftMargin() + (rootItem->depth() * rootItem->indentation()) + rootItem->indicator()->width() + rootItem->spacing());
    QCOMPARE(rootItem->indicator()->x(), rootItem->leftMargin() + (rootItem->depth() * rootItem->indentation()));

    treeView->expand(0);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(rootItem->expanded(), true);
}

void tst_qquicktreeviewdelegate::checkPropertiesChildren()
{
    LOAD_TREEVIEW("unmodified.qml");
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
        const auto childItem = qobject_cast<QQuickTreeViewDelegate *>(childFxItem->item);
        QVERIFY(childItem);

        QCOMPARE(childItem->treeView(), treeView);
        QCOMPARE(childItem->isTreeNode(), true);
        QCOMPARE(childItem->expanded(), row == 4);
        QCOMPARE(childItem->hasChildren(), row == 4 || row == 8);
        QCOMPARE(childItem->depth(), row <= 4 ? 1 : 2);
        QCOMPARE(childItem->current(), false);
        QCOMPARE(childItem->selected(), false);
        QCOMPARE(childItem->leftPadding(), childItem->leftMargin() + (childItem->depth() * childItem->indentation()) + childItem->indicator()->width() + childItem->spacing());
        QCOMPARE(childItem->indicator()->x(), childItem->leftMargin() + (childItem->depth() * childItem->indentation()));
    }
}

void tst_qquicktreeviewdelegate::checkCurrentIndex()
{
    // Check that that a cell becomes current when you click on it
    LOAD_TREEVIEW("unmodified.qml");
    // Check that the view only has one row loaded so far (the root of the tree)
    QCOMPARE(treeViewPrivate->loadedRows.count(), 1);

    const QPoint cell(0, 0);
    const auto item = qobject_cast<QQuickTreeViewDelegate *>(treeView->itemAtCell(cell));
    QVERIFY(item);
    QVERIFY(!item->current());
    QVERIFY(!item->selected());
    QVERIFY(!treeView->hasActiveFocus());

    // Click on the label
    QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    QPoint pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY(item->current());
    QVERIFY(!item->selected());
    QVERIFY(treeView->hasActiveFocus());

    // Select the cell
    treeView->selectionModel()->setCurrentIndex(treeView->modelIndex(cell), QItemSelectionModel::Select);
    QVERIFY(item->current());
    QVERIFY(item->selected());
}

void tst_qquicktreeviewdelegate::checkClickedSignal()
{
   // Check that the delegate emits clicked when clicking on the
   // label, but not when clicking on the indicator.
    LOAD_TREEVIEW("unmodified.qml");

    const auto item = treeView->itemAtCell(0, 0);
    QVERIFY(item);

    QSignalSpy clickedSpy(item, SIGNAL(clicked()));

    // Click on the label
    QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    QPoint pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(clickedSpy.count(), 1);

    // Click on the indicator
    const auto indicator = item->property("indicator").value<QQuickItem *>();
    QVERIFY(indicator);
    localPos = QPoint(indicator->x() + indicator->width() / 2, indicator->y() + indicator->height() / 2);
    pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(clickedSpy.count(), 1);
}

void tst_qquicktreeviewdelegate::clearSelectionOnClick()
{
    LOAD_TREEVIEW("unmodified.qml");

    // Select root item
    const auto index = treeView->selectionModel()->model()->index(0, 0);
    treeView->selectionModel()->select(index, QItemSelectionModel::Select);
    QCOMPARE(treeView->selectionModel()->selectedIndexes().count(), 1);

    // Click on a cell. This should remove the selection
    const auto item = qobject_cast<QQuickTreeViewDelegate *>(treeView->itemAtCell(0, 0));
    QVERIFY(item);
    QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    QPoint pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(treeView->selectionModel()->selectedIndexes().count(), 0);
}

QTEST_MAIN(tst_qquicktreeviewdelegate)

#include "tst_qquicktreeviewdelegate.moc"
