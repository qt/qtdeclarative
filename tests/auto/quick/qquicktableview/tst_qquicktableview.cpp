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

private slots:
    void initTestCase() override;

    void setAndGetModel_data();
    void setAndGetModel();
    void emptyModel_data();
    void emptyModel();
    void countDelegateItems_data();
    void countDelegateItems();
    void checkLayoutOfEqualSizedDelegateItems_data();
    void checkLayoutOfEqualSizedDelegateItems();
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

    QTest::newRow("QAIM 1x1 1,1") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1);
    QTest::newRow("QAIM 5x5 0,0") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(0, 0);
    QTest::newRow("QAIM 5x5 1,0") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(1, 0);
    QTest::newRow("QAIM 5x5 0,1") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(0, 1);
}

void tst_QQuickTableView::checkLayoutOfEqualSizedDelegateItems()
{
    // Check that the geometry of the delegate items are correct
    QFETCH(QVariant, model);
    QFETCH(QSize, tableSize);
    QFETCH(QSizeF, spacing);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());

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
        qreal expectedX = column * (expectedWidth + spacing.width());
        qreal expectedY = row * (expectedHeight + spacing.height());
        QCOMPARE(item->x(), expectedX);
        QCOMPARE(item->y(), expectedY);
        QCOMPARE(item->width(), expectedWidth);
        QCOMPARE(item->height(), expectedHeight);
    }
}

QTEST_MAIN(tst_QQuickTableView)

#include "tst_qquicktableview.moc"
