// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>
#include <QtQuick/private/qquicktext_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtGui/private/qguiapplication_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickControl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickControl();

private slots:
    void initTestCase() override;
    void flickable();
    void fractionalFontSize();
    void resizeBackgroundKeepsBindings();
    void hoverInMouseArea();
    void hoverAfterTouch();
    void hoverAfterTouchWithTwoFingers();
    void backgroundAlignment();

private:
    QScopedPointer<QPointingDevice> touchDevice;
};

tst_QQuickControl::tst_QQuickControl()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickControl::initTestCase()
{
    QQmlDataTest::initTestCase();
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");

    touchDevice.reset(QTest::createTouchDevice());
}

void tst_QQuickControl::flickable()
{
    // Check that when a Button that is inside a Flickable with a pressDelay
    // still gets the released and clicked signals sent due to the fact that
    // Flickable sends a mouse event for the delay and not a touch event
    QQuickApplicationHelper helper(this, QStringLiteral("flickable.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickButton *button = window->property("button").value<QQuickButton *>();
    QVERIFY(button);

    QSignalSpy buttonPressedSpy(button, SIGNAL(pressed()));
    QVERIFY(buttonPressedSpy.isValid());

    QSignalSpy buttonReleasedSpy(button, SIGNAL(released()));
    QVERIFY(buttonReleasedSpy.isValid());

    QSignalSpy buttonClickedSpy(button, SIGNAL(clicked()));
    QVERIFY(buttonClickedSpy.isValid());

    QPoint p = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
    QTest::touchEvent(window, touchDevice.data()).press(0, p);
    QTRY_COMPARE(buttonPressedSpy.size(), 1);
    p += QPoint(1, 1); // less than the drag threshold
    QTest::touchEvent(window, touchDevice.data()).move(0, p);
    QTest::touchEvent(window, touchDevice.data()).release(0, p);
    QTRY_COMPARE(buttonReleasedSpy.size(), 1);
    QTRY_COMPARE(buttonClickedSpy.size(), 1);
}

void tst_QQuickControl::fractionalFontSize()
{
    QQuickApplicationHelper helper(this, QStringLiteral("fractionalFontSize.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QQuickControl *control = window->property("control").value<QQuickControl *>();
    QVERIFY(control);
    QQuickText *contentItem = qobject_cast<QQuickText *>(control->contentItem());
    QVERIFY(contentItem);

    QVERIFY(!contentItem->truncated());

    QVERIFY2(qFuzzyCompare(contentItem->contentWidth(),
            QQuickTextPrivate::get(contentItem)->layout.boundingRect().width()),
            "The QQuickText::contentWidth() doesn't match the layout's preferred text width");
}

void tst_QQuickControl::resizeBackgroundKeepsBindings()
{
    QQuickApplicationHelper helper(this, QStringLiteral("resizeBackgroundKeepsBindings.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto ctxt = qmlContext(window);
    QVERIFY(ctxt);
    auto background = qobject_cast<QQuickItem *>(ctxt->objectForName("background"));
    QVERIFY(background);
    QCOMPARE(background->height(), 4);
    QVERIFY(background->bindableHeight().hasBinding());
}

void tst_QQuickControl::hoverInMouseArea()
{
    SKIP_IF_NO_MOUSE_HOVER;

    QQuickApplicationHelper helper(this, QStringLiteral("hoverInMouseArea.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const auto *control = window->property("control").value<QQuickControl *>();
    QVERIFY(control);
    PointLerper pointLerper(window);
    pointLerper.move(mapCenterToWindow(control));
    // It's necessary to use PointLerper here,
    // otherwise the isHovered checks are flaky on most platforms.
    QVERIFY(control->isHovered());

    const auto *textArea = window->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    pointLerper.move(mapCenterToWindow(textArea));
    QVERIFY(textArea->isHovered());

    const auto *textField = window->property("textField").value<QQuickTextField *>();
    QVERIFY(textField);
    pointLerper.move(mapCenterToWindow(textField));
    QVERIFY(textField->isHovered());
}

/*
    Puts the window into the state that the hover-after-touch tests below need: nothing hovered,
    and no hovering device anywhere near the Buttons, whatever the platform did while the
    window was being mapped. Moving the mouse to an empty spot does not get us there:

    - the platform may already have established hover from an enter event at wherever it
      thinks the cursor is (the offscreen plugin claims global (0, 0), which lands inside
      upperButton), and flushFrameSynchronousEvents() then keeps re-delivering hover from
      lastMousePosition on every frame;
    - QGuiApplicationPrivate::processMouseEvent() discards a MouseMove whose global
      position equals lastCursorPosition, and every window these tests create is mapped at
      the same place: from the second window onwards, QTest::mouseMove() to a fixed point
      is a no-op, and the stale hover above survives it;
    - on a platform with a real cursor, that position is wherever the pointer happens to be,
      which a test must not depend on.

    So set the state directly instead of trying to arrive at it via events: reset
    lastCursorPosition to the far-offscreen value that means "no cursor has been seen"
    (which is what isHoveredByHoveringDevice() consults), and drop the hover the platform
    established, along with the position that would resurrect it.
*/
static QQuickDeliveryAgentPrivate *prepareForFingertipHover(QQuickWindow *window)
{
    auto *deliveryAgent = QQuickWindowPrivate::get(window)->deliveryAgentPrivate();
    QGuiApplicationPrivate::lastCursorPosition.reset();
    deliveryAgent->clearHover();
    deliveryAgent->lastMousePosition = {};
    // A Control only becomes hovered on touch via flushFrameSynchronousEvents(), so the
    // tests drive that explicitly instead of relying on a frame landing at the right
    // moment; with a zero interval it never postpones the delivery.
    deliveryAgent->frameSynchronousHoverInterval = 0;
    return deliveryAgent;
}

void tst_QQuickControl::hoverAfterTouch() // QTBUG-62912
{
    // A Control that is tapped on a touchscreen becomes hovered while the finger is
    // down, and must stop being hovered when the finger is lifted: a fingertip has no
    // hover state that outlives contact. This used to work only on Windows, where the
    // QPA plugin synthesizes a LeaveEvent; QQuickAbstractButton accepts touch events
    // directly, so it never got the synth-mouse treatment that saved MouseArea either.
    QQuickApplicationHelper helper(this, QStringLiteral("hoverAfterTouch.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *upperButton = window->property("upperButton").value<QQuickButton *>();
    QVERIFY(upperButton);
    auto *lowerButton = window->property("lowerButton").value<QQuickButton *>();
    QVERIFY(lowerButton);

    const QPoint upperCenter = mapCenterToWindow(upperButton);
    const QPoint lowerCenter = mapCenterToWindow(lowerButton);

    auto *deliveryAgent = prepareForFingertipHover(window);
    QVERIFY(!upperButton->isHovered());
    QVERIFY(!lowerButton->isHovered());

    QTest::touchEvent(window, touchDevice.data()).press(0, upperCenter);
    deliveryAgent->flushFrameSynchronousEvents(window);
    QVERIFY(upperButton->isHovered());
    QVERIFY(!lowerButton->isHovered());

    QTest::touchEvent(window, touchDevice.data()).release(0, upperCenter);
    deliveryAgent->flushFrameSynchronousEvents(window);
    QVERIFY(!upperButton->isHovered());

    // Tapping a second Control must not leave the first one stuck (the original
    // QTBUG-40856 complaint), and must not leave the second one stuck either.
    QTest::touchEvent(window, touchDevice.data()).press(1, lowerCenter);
    deliveryAgent->flushFrameSynchronousEvents(window);
    QVERIFY(lowerButton->isHovered());
    QVERIFY(!upperButton->isHovered());

    QTest::touchEvent(window, touchDevice.data()).release(1, lowerCenter);
    deliveryAgent->flushFrameSynchronousEvents(window);
    QVERIFY(!lowerButton->isHovered());
    QVERIFY(!upperButton->isHovered());
}

void tst_QQuickControl::hoverAfterTouchWithTwoFingers() // QTBUG-62912
{
    // Hover must survive until the *last* finger is lifted. The Windows plugin used to
    // send a LeaveEvent per WM_POINTERLEAVE, i.e. per contact, so lifting one of two
    // fingers already cleared hover.
    QQuickApplicationHelper helper(this, QStringLiteral("hoverAfterTouch.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *upperButton = window->property("upperButton").value<QQuickButton *>();
    QVERIFY(upperButton);
    auto *lowerButton = window->property("lowerButton").value<QQuickButton *>();
    QVERIFY(lowerButton);

    const QPoint upperCenter = mapCenterToWindow(upperButton);
    const QPoint lowerCenter = mapCenterToWindow(lowerButton);

    auto *deliveryAgent = prepareForFingertipHover(window);
    QVERIFY(!upperButton->isHovered());
    QVERIFY(!lowerButton->isHovered());

    // One named sequence, committed explicitly: QTest::touchEvent() builds a fresh
    // sequence each time, and stationary() on a fresh sequence has no previous point to
    // copy, so the finger would teleport to global (0, 0).
    auto seq = QTest::touchEvent(window, touchDevice.data(), false);
    seq.press(0, upperCenter).press(1, lowerCenter).commit();
    deliveryAgent->flushFrameSynchronousEvents(window);
    // Only one Control gets hover, because hover is still single-point (see
    // tst_HoverHandler::twoHandlersTwoTouches); which one does not matter here.
    QVERIFY(upperButton->isHovered() || lowerButton->isHovered());

    // Lift only the second finger: the first is still down, so hover must remain.
    // The flush is load-bearing: while delivering this event, deliverUpdatedPoints()
    // transiently sends HoverLeave to upperButton as it processes point 1 down at
    // lowerCenter, and the flush is what re-enters it at lastMousePosition.
    seq.stationary(0).release(1, lowerCenter).commit();
    deliveryAgent->flushFrameSynchronousEvents(window);
    QVERIFY(upperButton->isHovered() || lowerButton->isHovered());

    // Now the last finger goes up.
    seq.release(0, upperCenter).commit();
    deliveryAgent->flushFrameSynchronousEvents(window);
    QVERIFY(!upperButton->isHovered());
    QVERIFY(!lowerButton->isHovered());
}

void tst_QQuickControl::backgroundAlignment()
{
    QQuickApplicationHelper helper(this, QStringLiteral("backgroundAlignment.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto ctxt = qmlContext(window);
    QVERIFY(ctxt);
    auto image = window->grabWindow();
    // If the top inset is applied, the top 6 pixels will appear white.
    QCOMPARE(image.pixelColor(0, 0), QColor(255,255,255));
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickControl)

#include "tst_qquickcontrol.moc"
