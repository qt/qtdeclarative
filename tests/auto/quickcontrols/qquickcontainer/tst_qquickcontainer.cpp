// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickTest/QtQuickTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

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
};

tst_qquickcontainer::tst_qquickcontainer()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");

    QQuickStyle::setStyle("Basic");
}

enum ContentItemType {
    View,
    Repeater,
    Positioner,
};

void tst_qquickcontainer::zeroSize_data()
{
    QTest::addColumn<QString>("qmlFileName");
    QTest::addColumn<ContentItemType>("contentItemType");

    QTest::newRow("ListView") << "zeroSizeWithListView.qml" << View;
    QTest::newRow("Repeater") << "zeroSizeWithRepeater.qml" << Repeater;
    QTest::newRow("Repeater in Row") << "zeroSizeWithRepeaterInRow.qml" << Positioner;
}

// Tests that a zero-size Container with a QQuickItemView sub-class culls its items.
// Based on a use case involving SwipeView: QTBUG-125416
void tst_qquickcontainer::zeroSize()
{
    QFETCH(QString, qmlFileName);
    QFETCH(ContentItemType, contentItemType);

    const bool isView = contentItemType == View;
    const bool isPositioner = contentItemType == Positioner;

    QQuickControlsApplicationHelper helper(this, qmlFileName);
    QVERIFY2(helper.ready, helper.failureMessage());
    centerOnScreen(helper.window);
    helper.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(helper.window));

    auto *text1 = helper.window->property("text1").value<QQuickItem *>();
    QVERIFY(text1);
    // Since the items also have zero sizes, they will also be culled by positioners.
    QCOMPARE(QQuickItemPrivate::get(text1)->culled, isView || isPositioner);

    auto *text2 = helper.window->property("text2").value<QQuickItem *>();
    QVERIFY(text2);
    QCOMPARE(QQuickItemPrivate::get(text2)->culled, isView || isPositioner);

    auto *text3 = helper.window->property("text3").value<QQuickItem *>();
    QVERIFY(text3);
    QCOMPARE(QQuickItemPrivate::get(text3)->culled, isView || isPositioner);

    // Add an item and check that it's culled appropriately.
    QVERIFY(QMetaObject::invokeMethod(helper.window, "addTextItem"));
    auto *container = helper.window->property("container").value<QQuickContainer *>();
    QVERIFY(container);
    auto *text4 = container->itemAt(3);
    QVERIFY(text4);
    if (isView || isPositioner) {
        QVERIFY(QQuickTest::qIsPolishScheduled(helper.window));
        QVERIFY(QQuickTest::qWaitForPolish(helper.window));
    }
    QCOMPARE(QQuickItemPrivate::get(text4)->culled, isView || isPositioner);

    // Give it a non-zero size (via its parent, which it fills).
    container->parentItem()->setWidth(text1->implicitWidth());
    container->parentItem()->setHeight(text1->implicitHeight());
    if (isView || isPositioner) {
        QVERIFY(QQuickTest::qIsPolishScheduled(helper.window));
        QVERIFY(QQuickTest::qWaitForPolish(helper.window));
    }
    QCOMPARE(QQuickItemPrivate::get(text1)->culled, false);
    // This one won't be culled for views either, because of cacheBuffer (and
    // clipping apparently doesn't affect culling, if we were to set clip to true).
    QCOMPARE(QQuickItemPrivate::get(text2)->culled, false);
    QCOMPARE(QQuickItemPrivate::get(text3)->culled, isView);
    QCOMPARE(QQuickItemPrivate::get(text4)->culled, isView);

    // Go back to a zero size.
    container->parentItem()->setWidth(0);
    container->parentItem()->setHeight(0);
    if (isView || isPositioner) {
        QVERIFY(QQuickTest::qIsPolishScheduled(helper.window));
        QVERIFY(QQuickTest::qWaitForPolish(helper.window));
    }
    QCOMPARE(QQuickItemPrivate::get(text1)->culled, isView || isPositioner);
    QCOMPARE(QQuickItemPrivate::get(text2)->culled, isView || isPositioner);
    QCOMPARE(QQuickItemPrivate::get(text3)->culled, isView || isPositioner);
    QCOMPARE(QQuickItemPrivate::get(text4)->culled, isView || isPositioner);
}

QTEST_MAIN(tst_qquickcontainer)

#include "tst_qquickcontainer.moc"
