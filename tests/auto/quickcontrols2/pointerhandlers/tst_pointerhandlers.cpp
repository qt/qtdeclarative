// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickmousearea_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>

#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

class tst_pointerhandlers : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_pointerhandlers();

private slots:
    void hover_controlInsideControl();
    void hover_controlAndMouseArea();
};

tst_pointerhandlers::tst_pointerhandlers()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_pointerhandlers::hover_controlInsideControl()
{
    // Test that if you move the mouse over a control that is
    // a child of another control, both controls end up hovered.
    // A control should basically not block (accept) hover events.
    QQuickView view(testFileUrl("controlinsidecontrol.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);
    QQuickWindow *window = rootItem->window();
    QVERIFY(window);

    const auto context = qmlContext(rootItem);
    auto outerButton = context->contextProperty("outerButton").value<QQuickButton *>();
    auto innerButton = context->contextProperty("innerButton").value<QQuickButton *>();
    QVERIFY(outerButton);
    QVERIFY(innerButton);

    const QPoint posInWindow(1, 1);
    const QPoint posOnOuterButton = rootItem->mapFromItem(outerButton, QPointF(0, 0)).toPoint();
    const QPoint posOnInnerButton = rootItem->mapFromItem(innerButton, QPointF(0, 0)).toPoint();

    // Start by moving the mouse to the window
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerButton->isHovered(), false);
    QCOMPARE(innerButton->isHovered(), false);

    // Move the mouse over the outer control
    QTest::mouseMove(window, posOnOuterButton);
    QCOMPARE(outerButton->isHovered(), true);
    QCOMPARE(innerButton->isHovered(), false);

    // Move the mouse over the inner control
    QTest::mouseMove(window, posOnInnerButton);
    QCOMPARE(outerButton->isHovered(), true);
    QCOMPARE(innerButton->isHovered(), true);

    // Move the mouse over the outer control again
    QTest::mouseMove(window, posOnOuterButton);
    QCOMPARE(outerButton->isHovered(), true);
    QCOMPARE(innerButton->isHovered(), false);

    // Move the mouse outside all controls
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerButton->isHovered(), false);
    QCOMPARE(innerButton->isHovered(), false);
}

void tst_pointerhandlers::hover_controlAndMouseArea()
{
    QQuickView view(testFileUrl("controlandmousearea.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);
    QQuickWindow *window = rootItem->window();
    QVERIFY(window);

    const auto context = qmlContext(rootItem);
    auto outerMouseArea = context->contextProperty("outerMouseArea").value<QQuickMouseArea *>();
    auto buttonInTheMiddle = context->contextProperty("buttonInTheMiddle").value<QQuickButton *>();
    auto innerMouseArea = context->contextProperty("innerMouseArea").value<QQuickMouseArea *>();
    QVERIFY(outerMouseArea);
    QVERIFY(buttonInTheMiddle);
    QVERIFY(innerMouseArea);

    const QPoint posInWindow(1, 1);
    const QPoint posOnOuterMouseArea = rootItem->mapFromItem(outerMouseArea, QPointF(0, 0)).toPoint();
    const QPoint posOnButtonInTheMiddle = rootItem->mapFromItem(buttonInTheMiddle, QPointF(0, 0)).toPoint();
    const QPoint posOnInnerMouseArea = rootItem->mapFromItem(innerMouseArea, QPointF(0, 0)).toPoint();

    // Start by moving the mouse to the window
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerMouseArea->hovered(), false);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the outer mousearea
    QTest::mouseMove(window, posOnOuterMouseArea);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the button in the middle
    QTest::mouseMove(window, posOnButtonInTheMiddle);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), true);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the inner mousearea
    QTest::mouseMove(window, posOnInnerMouseArea);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), true);
    QCOMPARE(innerMouseArea->hovered(), true);

    // Move the mouse over the button in the middle again
    QTest::mouseMove(window, posOnButtonInTheMiddle);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), true);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the outer mousearea again
    QTest::mouseMove(window, posOnOuterMouseArea);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse outside all items
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerMouseArea->hovered(), false);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);
}

QTEST_MAIN(tst_pointerhandlers)

#include "tst_pointerhandlers.moc"
