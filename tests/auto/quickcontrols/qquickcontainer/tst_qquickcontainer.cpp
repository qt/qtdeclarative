// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>
#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickTest/QtQuickTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQmlModels/private/qqmlobjectmodel_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

class tst_qquickcontainer : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qquickcontainer();

private slots:
    void zeroSize_data();
    void zeroSize();
    void skipReorderContentModelItem();
};

tst_qquickcontainer::tst_qquickcontainer()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");

    QQuickStyle::setStyle("Basic");
}

void tst_qquickcontainer::zeroSize_data()
{
    QTest::addColumn<QString>("qmlFileName");
    QTest::addColumn<bool>("isItemView");

    QTest::newRow("ListView") << "zeroSizeWithListView.qml" << true;
    // See QQuickContainerPrivate::maybeCullItem for why this is false.
    QTest::newRow("Repeater") << "zeroSizeWithRepeater.qml" << false;
}

// Tests that a zero-size Container with a QQuickItemView sub-class culls its items.
// Based on a use case involving SwipeView: QTBUG-125416
void tst_qquickcontainer::zeroSize()
{
    QFETCH(QString, qmlFileName);
    QFETCH(bool, isItemView);

    QQuickControlsApplicationHelper helper(this, qmlFileName);
    QVERIFY2(helper.ready, helper.failureMessage());
    centerOnScreen(helper.window);
    helper.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(helper.window));

    auto *text1 = helper.window->property("text1").value<QQuickItem *>();
    QVERIFY(text1);
    QCOMPARE(QQuickItemPrivate::get(text1)->culled, isItemView);

    auto *text2 = helper.window->property("text2").value<QQuickItem *>();
    QVERIFY(text2);
    QCOMPARE(QQuickItemPrivate::get(text2)->culled, isItemView);

    auto *text3 = helper.window->property("text3").value<QQuickItem *>();
    QVERIFY(text3);
    QCOMPARE(QQuickItemPrivate::get(text3)->culled, isItemView);

    // Add an item and check that it's culled appropriately.
    QVERIFY(QMetaObject::invokeMethod(helper.window, "addTextItem"));
    auto *container = helper.window->property("container").value<QQuickContainer *>();
    QVERIFY(container);
    auto *text4 = container->itemAt(3);
    QVERIFY(text4);
    QCOMPARE(QQuickItemPrivate::get(text4)->culled, isItemView);

    // Give it a non-zero size (via its parent, which it fills).
    container->parentItem()->setWidth(text1->implicitWidth());
    container->parentItem()->setHeight(text1->implicitHeight());
    if (isItemView) {
        QVERIFY(QQuickTest::qIsPolishScheduled(helper.window));
        QVERIFY(QQuickTest::qWaitForPolish(helper.window));
    }
    QCOMPARE(QQuickItemPrivate::get(text1)->culled, false);
    // This one won't be culled for views either, because of cacheBuffer (and
    // clipping apparently doesn't affect culling, if we were to set clip to true).
    QCOMPARE(QQuickItemPrivate::get(text2)->culled, false);
    QCOMPARE(QQuickItemPrivate::get(text3)->culled, isItemView);
    QCOMPARE(QQuickItemPrivate::get(text4)->culled, isItemView);

    // Go back to a zero size.
    container->parentItem()->setWidth(0);
    container->parentItem()->setHeight(0);
    if (isItemView) {
        QVERIFY(QQuickTest::qIsPolishScheduled(helper.window));
        QVERIFY(QQuickTest::qWaitForPolish(helper.window));
    }
    QCOMPARE(QQuickItemPrivate::get(text1)->culled, isItemView);
    QCOMPARE(QQuickItemPrivate::get(text2)->culled, isItemView);
    QCOMPARE(QQuickItemPrivate::get(text3)->culled, isItemView);
    QCOMPARE(QQuickItemPrivate::get(text4)->culled, isItemView);
}

void tst_qquickcontainer::skipReorderContentModelItem()
{
    QQuickControlsApplicationHelper helper(this, "skipReorderContentModelItem.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    centerOnScreen(helper.window);
    helper.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(helper.window));

    const auto *container = helper.window->property("navigationBar").value<QQuickContainer *>();
    QVERIFY(container);
    const auto *listView = container->property("listView").value<QQuickListView *>();
    QVERIFY(listView);
    const auto *contentModel = container->property("contentModel").value<QQmlObjectModel *>();
    QVERIFY(contentModel);

    QCOMPARE(listView->count(), contentModel->count());

    int totalWidth = listView->spacing() * (contentModel->count() - 1);
    for (int index = 0; index < contentModel->count(); index++)
        totalWidth += qobject_cast<QQuickText *>(listView->itemAtIndex(index))->width();
    QVERIFY(totalWidth > listView->width());

    for (int index = 0; index < listView->count(); index++) {
        const auto *textItem = qobject_cast<QQuickText *>(listView->itemAtIndex(index));
        QCOMPARE(textItem->text().toInt(), index + 1);
    }
}

QTEST_MAIN(tst_qquickcontainer)

#include "tst_qquickcontainer.moc"
