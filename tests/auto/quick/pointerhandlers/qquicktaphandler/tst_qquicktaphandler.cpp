// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest/QtTest>

#include <QtGui/qstylehints.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qquickwindow_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_TapHandler : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_TapHandler()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void initTestCase() override;

    void touchGesturePolicyDragThreshold();
    void mouseGesturePolicyDragThreshold();
    void touchMouseGesturePolicyDragThreshold();
    void touchGesturePolicyWithinBounds();
    void mouseGesturePolicyWithinBounds();
    void touchGesturePolicyReleaseWithinBounds();
    void mouseGesturePolicyReleaseWithinBounds();
    void gesturePolicyDragWithinBounds_data();
    void gesturePolicyDragWithinBounds();
    void touchMultiTap();
    void mouseMultiTap_data();
    void mouseMultiTap();
    void mouseMultiTapLeftRight_data();
    void mouseMultiTapLeftRight();
    void singleTapDoubleTap_data();
    void singleTapDoubleTap();
    void longPress_data();
    void longPress();
    void buttonsMultiTouch();
    void componentUserBehavioralOverride();
    void rightLongPressIgnoreWheel();
    void negativeZStackingOrder();
    void nonTopLevelParentWindow();
    void nestedDoubleTap_data();
    void nestedDoubleTap();
    void nestedAndSiblingPropagation_data();
    void nestedAndSiblingPropagation();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName,
                    QWindow *parent = nullptr);
    QPointingDevice *touchDevice = QTest::createTouchDevice(); // TODO const after fixing QTBUG-107864
    void mouseEvent(QEvent::Type type, Qt::MouseButton button, const QPoint &point,
                    QWindow *targetWindow, QWindow *mapToWindow);
};

void tst_TapHandler::createView(QScopedPointer<QQuickView> &window, const char *fileName,
                                QWindow *parent)
{
    window.reset(new QQuickView(parent));
    if (parent) {
        parent->show();
        QVERIFY(QTest::qWaitForWindowActive(parent));
    }

    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtils::centerOnScreen(window.data());
    QQuickViewTestUtils::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

void tst_TapHandler::mouseEvent(QEvent::Type type, Qt::MouseButton button, const QPoint &point,
                                QWindow *targetWindow, QWindow *mapToWindow)
{
    QVERIFY(targetWindow);
    QVERIFY(mapToWindow);
    auto buttons = button;
    if (type == QEvent::MouseButtonRelease) {
        buttons = Qt::NoButton;
    }
    QMouseEvent me(type, point, mapToWindow->mapToGlobal(point), button, buttons,
                   Qt::KeyboardModifiers(), QPointingDevice::primaryPointingDevice());
    QVERIFY(qApp->notify(targetWindow, &me));
}

void tst_TapHandler::initTestCase()
{
    // This test assumes that we don't get synthesized mouse events from QGuiApplication
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    QQmlDataTest::initTestCase();
}

void tst_TapHandler::touchGesturePolicyDragThreshold()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonDragThreshold = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(buttonDragThreshold);
    QQuickTapHandler *tapHandler = buttonDragThreshold->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    QSignalSpy dragThresholdTappedSpy(buttonDragThreshold, SIGNAL(tapped()));

    // DragThreshold button stays pressed while touchpoint stays within dragThreshold, emits tapped on release
    QPoint p1 = buttonDragThreshold->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonDragThreshold->property("pressed").toBool());
    QCOMPARE(dragThresholdTappedSpy.size(), 1);
    QCOMPARE(buttonDragThreshold->property("tappedPosition").toPoint(), p1);
    QCOMPARE(tapHandler->point().position(), QPointF());

    // DragThreshold button is no longer pressed if touchpoint goes beyond dragThreshold
    dragThresholdTappedSpy.clear();
    p1 = buttonDragThreshold->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    p1 += QPoint(1, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonDragThreshold->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!buttonDragThreshold->property("pressed").toBool());
    QCOMPARE(dragThresholdTappedSpy.size(), 0);
}

void tst_TapHandler::mouseGesturePolicyDragThreshold()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonDragThreshold = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(buttonDragThreshold);
    QQuickTapHandler *tapHandler = buttonDragThreshold->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    QSignalSpy dragThresholdTappedSpy(buttonDragThreshold, SIGNAL(tapped()));

    // DragThreshold button stays pressed while mouse stays within dragThreshold, emits tapped on release
    QPoint p1 = buttonDragThreshold->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!buttonDragThreshold->property("pressed").toBool());
    QTRY_COMPARE(dragThresholdTappedSpy.size(), 1);
    QCOMPARE(buttonDragThreshold->property("tappedPosition").toPoint(), p1);
    QCOMPARE(tapHandler->point().position(), QPointF());

    // DragThreshold button is no longer pressed if mouse goes beyond dragThreshold
    dragThresholdTappedSpy.clear();
    p1 = buttonDragThreshold->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    p1 += QPoint(1, 0);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(!buttonDragThreshold->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!buttonDragThreshold->property("pressed").toBool());
    QCOMPARE(dragThresholdTappedSpy.size(), 0);
}

void tst_TapHandler::touchMouseGesturePolicyDragThreshold()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonDragThreshold = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(buttonDragThreshold);
    QSignalSpy tappedSpy(buttonDragThreshold, SIGNAL(tapped()));
    QSignalSpy canceledSpy(buttonDragThreshold, SIGNAL(canceled()));

    // Press mouse, drag it outside the button, release
    QPoint p1 = buttonDragThreshold->mapToScene(QPointF(20, 20)).toPoint();
    QPoint p2 = p1 + QPoint(int(buttonDragThreshold->height()), 0);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QTest::mouseMove(window, p2);
    QTRY_COMPARE(canceledSpy.size(), 1);
    QCOMPARE(tappedSpy.size(), 0);
    QCOMPARE(buttonDragThreshold->property("pressed").toBool(), false);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p2);

    // Press and release touch, verify that it still works (QTBUG-71466)
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonDragThreshold->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 1);

    // Press touch, drag it outside the button, release
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).move(1, p2, window);
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(buttonDragThreshold->property("pressed").toBool(), false);
    QTest::touchEvent(window, touchDevice).release(1, p2, window);
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(canceledSpy.size(), 2);
    QCOMPARE(tappedSpy.size(), 1); // didn't increase
    QCOMPARE(buttonDragThreshold->property("pressed").toBool(), false);

    // Press and release mouse, verify that it still works
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(tappedSpy.size(), 2);
    QCOMPARE(canceledSpy.size(), 2); // didn't increase
    QCOMPARE(buttonDragThreshold->property("pressed").toBool(), false);
}

void tst_TapHandler::touchGesturePolicyWithinBounds()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonWithinBounds = window->rootObject()->findChild<QQuickItem*>("WithinBounds");
    QVERIFY(buttonWithinBounds);
    QSignalSpy withinBoundsTappedSpy(buttonWithinBounds, SIGNAL(tapped()));

    // WithinBounds button stays pressed while touchpoint stays within bounds, emits tapped on release
    QPoint p1 = buttonWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonWithinBounds->property("pressed").toBool());
    p1 += QPoint(50, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 1);

    // WithinBounds button is no longer pressed if touchpoint leaves bounds
    withinBoundsTappedSpy.clear();
    p1 = buttonWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonWithinBounds->property("pressed").toBool());
    p1 += QPoint(0, 100);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonWithinBounds->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 0);
}

void tst_TapHandler::mouseGesturePolicyWithinBounds()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonWithinBounds = window->rootObject()->findChild<QQuickItem*>("WithinBounds");
    QVERIFY(buttonWithinBounds);
    QSignalSpy withinBoundsTappedSpy(buttonWithinBounds, SIGNAL(tapped()));

    // WithinBounds button stays pressed while touchpoint stays within bounds, emits tapped on release
    QPoint p1 = buttonWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonWithinBounds->property("pressed").toBool());
    p1 += QPoint(50, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 1);

    // WithinBounds button is no longer pressed if touchpoint leaves bounds
    withinBoundsTappedSpy.clear();
    p1 = buttonWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonWithinBounds->property("pressed").toBool());
    p1 += QPoint(0, 100);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(!buttonWithinBounds->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 0);
}

void tst_TapHandler::touchGesturePolicyReleaseWithinBounds()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonReleaseWithinBounds = window->rootObject()->findChild<QQuickItem*>("ReleaseWithinBounds");
    QVERIFY(buttonReleaseWithinBounds);
    QSignalSpy releaseWithinBoundsTappedSpy(buttonReleaseWithinBounds, SIGNAL(tapped()));

    // ReleaseWithinBounds button stays pressed while touchpoint wanders anywhere,
    // then if it comes back within bounds, emits tapped on release
    QPoint p1 = buttonReleaseWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 += QPoint(50, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 += QPoint(250, 100);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 = buttonReleaseWithinBounds->mapToScene(QPointF(25, 15)).toPoint();
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 1);

    // ReleaseWithinBounds button does not emit tapped if released out of bounds
    releaseWithinBoundsTappedSpy.clear();
    p1 = buttonReleaseWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 += QPoint(0, 100);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 0);
}

void tst_TapHandler::mouseGesturePolicyReleaseWithinBounds()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonReleaseWithinBounds = window->rootObject()->findChild<QQuickItem*>("ReleaseWithinBounds");
    QVERIFY(buttonReleaseWithinBounds);
    QSignalSpy releaseWithinBoundsTappedSpy(buttonReleaseWithinBounds, SIGNAL(tapped()));

    // ReleaseWithinBounds button stays pressed while touchpoint wanders anywhere,
    // then if it comes back within bounds, emits tapped on release
    QPoint p1 = buttonReleaseWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 += QPoint(50, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 += QPoint(250, 100);
    QTest::mouseMove(window, p1);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 = buttonReleaseWithinBounds->mapToScene(QPointF(25, 15)).toPoint();
    QTest::mouseMove(window, p1);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 1);

    // ReleaseWithinBounds button does not emit tapped if released out of bounds
    releaseWithinBoundsTappedSpy.clear();
    p1 = buttonReleaseWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    p1 += QPoint(0, 100);
    QTest::mouseMove(window, p1);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 0);
}

void tst_TapHandler::gesturePolicyDragWithinBounds_data()
{
    QTest::addColumn<const QPointingDevice *>("device");
    QTest::addColumn<QPoint>("dragStart");
    QTest::addColumn<QPoint>("dragDistance");
    QTest::addColumn<QString>("expectedFeedback");

    const QPointingDevice *constTouchDevice = touchDevice;

    QTest::newRow("mouse: click") << QPointingDevice::primaryPointingDevice() << QPoint(200, 200) << QPoint(0, 0) << "middle";
    QTest::newRow("touch: tap") << constTouchDevice << QPoint(200, 200) << QPoint(0, 0) << "middle";
    QTest::newRow("mouse: drag up") << QPointingDevice::primaryPointingDevice() << QPoint(200, 200) << QPoint(0, -20) << "top";
    QTest::newRow("touch: drag up") << constTouchDevice << QPoint(200, 200) << QPoint(0, -20) << "top";
    QTest::newRow("mouse: drag out to cancel") << QPointingDevice::primaryPointingDevice() << QPoint(435, 200) << QPoint(10, 0) << "canceled";
    QTest::newRow("touch: drag out to cancel") << constTouchDevice << QPoint(435, 200) << QPoint(10, 0) << "canceled";
}

void tst_TapHandler::gesturePolicyDragWithinBounds()
{
    QFETCH(const QPointingDevice *, device);
    QFETCH(QPoint, dragStart);
    QFETCH(QPoint, dragDistance);
    QFETCH(QString, expectedFeedback);
    const bool expectedCanceled = expectedFeedback == "canceled";

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragReleaseMenu.qml")));
    QQuickTapHandler *tapHandler = window.rootObject()->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    QSignalSpy canceledSpy(tapHandler, &QQuickTapHandler::canceled);

    QQuickTest::pointerPress(device, &window, 0, dragStart);
    QTRY_VERIFY(tapHandler->isPressed());
    QQuickTest::pointerMove(device, &window, 0, dragStart + dragDistance);
    if (expectedCanceled)
        QTRY_COMPARE(tapHandler->timeHeld(), -1);
    else
        QTRY_VERIFY(tapHandler->timeHeld() > 0.1);
    QQuickTest::pointerRelease(device, &window, 0, dragStart + dragDistance);

    QCOMPARE(window.rootObject()->property("feedbackText"), expectedFeedback);
    if (expectedCanceled)
        QCOMPARE(canceledSpy.size(), 1);
}

void tst_TapHandler::touchMultiTap()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(button);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));

    // Tap once
    QPoint p1 = button->mapToScene(QPointF(2, 2)).toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 1);

    // Tap again in exactly the same place (not likely with touch in the real world)
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 2);

    // Tap a third time, nearby
    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 3);

    // Tap a fourth time, drifting farther away
    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 4);

    // Test a stray touch begin
    tappedSpy.clear();
    constexpr int count = 2;
    for (int i = 0; i < count; ++i) {
        QTest::touchEvent(window, touchDevice).press(1, p1, window);
        QQuickTouchUtils::flush(window);
        p1 -= QPoint(dragThreshold, dragThreshold);
        QTest::touchEvent(window, touchDevice).move(1, p1, window);
        QQuickTouchUtils::flush(window);
        QTest::touchEvent(window, touchDevice).release(1, p1, window);
        QQuickTouchUtils::flush(window);
        p1 += QPoint(dragThreshold, dragThreshold);
        QTest::touchEvent(window, touchDevice).press(1, p1, window);
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(tappedSpy.count(), count);
}

void tst_TapHandler::mouseMultiTap_data()
{
    QTest::addColumn<QQuickTapHandler::ExclusiveSignals>("exclusiveSignals");
    QTest::addColumn<int>("expectedSingleTaps");
    QTest::addColumn<int>("expectedSingleTapsAfterMovingAway");
    QTest::addColumn<int>("expectedSingleTapsAfterWaiting");
    QTest::addColumn<int>("expectedDoubleTaps");

    QTest::newRow("NotExclusive")        << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::NotExclusive)
                                         << 1 << 2 << 3 << 1;
    QTest::newRow("SingleTap")           << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap)
                                         << 1 << 2 << 3 << 0;
    QTest::newRow("DoubleTap")           << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::DoubleTap)
                                         << 0 << 0 << 0 << 1;
    QTest::newRow("SingleTap|DoubleTap") << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap | QQuickTapHandler::DoubleTap)
                                         << 0 << 0 << 0 << 0;
}

void tst_TapHandler::mouseMultiTap()
{
    QFETCH(QQuickTapHandler::ExclusiveSignals, exclusiveSignals);
    QFETCH(int, expectedSingleTaps);
    QFETCH(int, expectedSingleTapsAfterMovingAway);
    QFETCH(int, expectedSingleTapsAfterWaiting);
    QFETCH(int, expectedDoubleTaps);

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(button);
    QQuickTapHandler *tapHandler = button->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    tapHandler->setExclusiveSignals(exclusiveSignals);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));
    QSignalSpy singleTapSpy(tapHandler, &QQuickTapHandler::singleTapped);
    QSignalSpy doubleTapSpy(tapHandler, &QQuickTapHandler::doubleTapped);

    // Click once
    QPoint p1 = button->mapToScene(QPointF(2, 2)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 1);
    // If exclusiveSignals == SingleTap | DoubleTap:
    // This would be a single-click if we waited longer than the double-click interval,
    // but it's too early for the signal at this moment; and we're going to click again.
    // If exclusiveSignals == DoubleTap: singleTapped() won't happen.
    // Otherwise: we got singleTapped() immediately.
    QCOMPARE(singleTapSpy.size(), expectedSingleTaps);
    QCOMPARE(tapHandler->timeHeld(), -1);

    // Click again in exactly the same place
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QCOMPARE(tappedSpy.size(), 2);
    QCOMPARE(singleTapSpy.size(), expectedSingleTaps);
    QCOMPARE(doubleTapSpy.size(), expectedDoubleTaps);

    // Click a third time, nearby: that'll be a triple-click
    p1 += QPoint(1, 1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QCOMPARE(tappedSpy.size(), 3);
    QCOMPARE(singleTapSpy.size(), expectedSingleTaps);
    QCOMPARE(doubleTapSpy.size(), expectedDoubleTaps);
    QCOMPARE(tapHandler->tapCount(), 3);

    // Click a fourth time, drifting farther away: treated as a separate click, regardless of timing
    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p1); // default delay to prevent double-click
    QCOMPARE(tappedSpy.size(), 4);
    QCOMPARE(tapHandler->tapCount(), 1);
    QTRY_COMPARE(singleTapSpy.size(), expectedSingleTapsAfterMovingAway);

    // Click a fifth time later on at the same place: treated as a separate click
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(tappedSpy.size(), 5);
    QCOMPARE(tapHandler->tapCount(), 1);
    QCOMPARE(singleTapSpy.size(), expectedSingleTapsAfterWaiting);
}

void tst_TapHandler::mouseMultiTapLeftRight_data()
{
    QTest::addColumn<QQuickTapHandler::ExclusiveSignals>("exclusiveSignals");
    QTest::addColumn<int>("expectedSingleTaps");
    QTest::addColumn<int>("expectedDoubleTaps");
    QTest::addColumn<int>("expectedTabCount2");
    QTest::addColumn<int>("expectedTabCount3");

    QTest::newRow("NotExclusive")        << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::NotExclusive)
                                         << 3 << 0 << 1 << 1;
    QTest::newRow("SingleTap")           << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap)
                                         << 3 << 0 << 1 << 1;
    QTest::newRow("DoubleTap")           << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::DoubleTap)
                                         << 0 << 0 << 1 << 1;
    QTest::newRow("SingleTap|DoubleTap") << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap | QQuickTapHandler::DoubleTap)
                                         << 0 << 0 << 1 << 1;
}

void tst_TapHandler::mouseMultiTapLeftRight() //QTBUG-111557
{
    QFETCH(QQuickTapHandler::ExclusiveSignals, exclusiveSignals);
    QFETCH(int, expectedSingleTaps);
    QFETCH(int, expectedDoubleTaps);
    QFETCH(int, expectedTabCount2);
    QFETCH(int, expectedTabCount3);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(button);
    QQuickTapHandler *tapHandler = button->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    tapHandler->setExclusiveSignals(exclusiveSignals);
    tapHandler->setAcceptedButtons(Qt::LeftButton | Qt::RightButton);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));
    QSignalSpy singleTapSpy(tapHandler, &QQuickTapHandler::singleTapped);
    QSignalSpy doubleTapSpy(tapHandler, &QQuickTapHandler::doubleTapped);

    // Click once with the left button
    QPoint p1 = button->mapToScene(QPointF(2, 2)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1, 10);

    // Click again with the right button -> should reset tabCount()
    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, p1, 10);
    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, p1, 10);

    QCOMPARE(tapHandler->tapCount(), expectedTabCount2);

    // Click again with the left button -> should reset tabCount()
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1, 10);

    QCOMPARE(tapHandler->tapCount(), expectedTabCount3);
    QCOMPARE(singleTapSpy.size(), expectedSingleTaps);
    QCOMPARE(doubleTapSpy.size(), expectedDoubleTaps);
}

void tst_TapHandler::singleTapDoubleTap_data()
{
    QTest::addColumn<QPointingDevice::DeviceType>("deviceType");
    QTest::addColumn<QQuickTapHandler::ExclusiveSignals>("exclusiveSignals");
    QTest::addColumn<int>("expectedEndingSingleTapCount");
    QTest::addColumn<int>("expectedDoubleTapCount");

    QTest::newRow("mouse:NotExclusive")
            << QPointingDevice::DeviceType::Mouse
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::NotExclusive)
            << 1 << 1;
    QTest::newRow("mouse:SingleTap")
            << QPointingDevice::DeviceType::Mouse
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap)
            << 1 << 0;
    QTest::newRow("mouse:DoubleTap")
            << QPointingDevice::DeviceType::Mouse
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::DoubleTap)
            << 0 << 1;
    QTest::newRow("mouse:SingleTap|DoubleTap")
            << QPointingDevice::DeviceType::Mouse
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap | QQuickTapHandler::DoubleTap)
            << 0 << 1;
    QTest::newRow("touch:NotExclusive")
            << QPointingDevice::DeviceType::TouchScreen
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::NotExclusive)
            << 1 << 1;
    QTest::newRow("touch:SingleTap")
            << QPointingDevice::DeviceType::TouchScreen
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap)
            << 1 << 0;
    QTest::newRow("touch:DoubleTap")
            << QPointingDevice::DeviceType::TouchScreen
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::DoubleTap)
            << 0 << 1;
    QTest::newRow("touch:SingleTap|DoubleTap")
            << QPointingDevice::DeviceType::TouchScreen
            << QQuickTapHandler::ExclusiveSignals(QQuickTapHandler::SingleTap | QQuickTapHandler::DoubleTap)
            << 0 << 1;
}

void tst_TapHandler::singleTapDoubleTap()
{
    QFETCH(QPointingDevice::DeviceType, deviceType);
    QFETCH(QQuickTapHandler::ExclusiveSignals, exclusiveSignals);
    QFETCH(int, expectedEndingSingleTapCount);
    QFETCH(int, expectedDoubleTapCount);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(button);
    QQuickTapHandler *tapHandler = button->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    tapHandler->setExclusiveSignals(exclusiveSignals);
    QSignalSpy tappedSpy(tapHandler, &QQuickTapHandler::tapped);
    QSignalSpy singleTapSpy(tapHandler, &QQuickTapHandler::singleTapped);
    QSignalSpy doubleTapSpy(tapHandler, &QQuickTapHandler::doubleTapped);

    auto tap = [window, tapHandler, deviceType, this](const QPoint &p1, int delay = 10) {
        switch (static_cast<QPointingDevice::DeviceType>(deviceType)) {
        case QPointingDevice::DeviceType::Mouse:
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p1, delay);
            break;
        case QPointingDevice::DeviceType::TouchScreen:
            QTest::qWait(delay);
            QTest::touchEvent(window, touchDevice).press(0, p1, window);
            QTRY_VERIFY(tapHandler->isPressed());
            QTest::touchEvent(window, touchDevice).release(0, p1, window);
            break;
        default:
            break;
        }
    };

    // tap once
    const QPoint p1 = button->mapToScene(QPointF(2, 2)).toPoint();
    tap(p1);
    QCOMPARE(tappedSpy.size(), 1);
    QCOMPARE(doubleTapSpy.size(), 0);

    // tap again immediately afterwards
    tap(p1);
    QTRY_COMPARE(doubleTapSpy.size(), expectedDoubleTapCount);
    QCOMPARE(tappedSpy.size(), 2);
    QCOMPARE(singleTapSpy.size(), expectedEndingSingleTapCount);

    // wait past the double-tap interval, then do it again
    const auto delay = qApp->styleHints()->mouseDoubleClickInterval() + 10;
    tappedSpy.clear();
    singleTapSpy.clear();
    doubleTapSpy.clear();

    // tap once with delay
    tap(p1, delay);
    QCOMPARE(tappedSpy.size(), 1);
    QCOMPARE(doubleTapSpy.size(), 0);

    // tap again immediately afterwards
    tap(p1);
    QTRY_COMPARE(doubleTapSpy.size(), expectedDoubleTapCount);
    QCOMPARE(tappedSpy.size(), 2);
    QCOMPARE(singleTapSpy.size(), expectedEndingSingleTapCount);
}

void tst_TapHandler::longPress_data()
{
    QTest::addColumn<const QPointingDevice *>("device");
    QTest::addColumn<QString>("buttonName");
    QTest::addColumn<qreal>("longPressThreshold");
    QTest::addColumn<QPoint>("releaseOffset");
    QTest::addColumn<bool>("expectLongPress");
    QTest::addColumn<bool>("expectTapped");

    const QPointingDevice *constTouchDevice = touchDevice;

    // Reduce the threshold so that we can get a long press quickly (faster in CI)
    const qreal longPressThreshold = 0.3;
    QTest::newRow("mouse, lpt longPressThreshold: DragThreshold")
            << QPointingDevice::primaryPointingDevice() << "DragThreshold"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: DragThreshold")
            << constTouchDevice << "DragThreshold" << longPressThreshold
            << QPoint(0, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: DragThreshold, drag")
            << QPointingDevice::primaryPointingDevice() << "DragThreshold"
            << longPressThreshold << QPoint(50, 0) << false << false;
    QTest::newRow("touch, lpt longPressThreshold: DragThreshold, drag")
            << constTouchDevice << "DragThreshold"
            << longPressThreshold << QPoint(50, 0) << false << false;

    QTest::newRow("mouse, lpt longPressThreshold: WithinBounds")
            << QPointingDevice::primaryPointingDevice() << "WithinBounds"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: WithinBounds")
            << constTouchDevice << "WithinBounds"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: WithinBounds, drag")
            << QPointingDevice::primaryPointingDevice() << "WithinBounds"
            << longPressThreshold << QPoint(50, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: WithinBounds, drag")
            << constTouchDevice << "WithinBounds"
            << longPressThreshold << QPoint(50, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: WithinBounds, drag out")
            << QPointingDevice::primaryPointingDevice() << "WithinBounds"
            << longPressThreshold << QPoint(155, 0) << false << false;
    QTest::newRow("touch, lpt longPressThreshold: WithinBounds, drag out")
            << constTouchDevice << "WithinBounds"
            << longPressThreshold << QPoint(155, 0) << false << false;

    QTest::newRow("mouse, lpt longPressThreshold: ReleaseWithinBounds")
            << QPointingDevice::primaryPointingDevice() << "ReleaseWithinBounds"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: ReleaseWithinBounds")
            << constTouchDevice << "ReleaseWithinBounds"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: ReleaseWithinBounds, drag")
            << QPointingDevice::primaryPointingDevice() << "ReleaseWithinBounds"
            << longPressThreshold << QPoint(50, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: ReleaseWithinBounds, drag")
            << constTouchDevice << "ReleaseWithinBounds"
            << longPressThreshold << QPoint(50, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: ReleaseWithinBounds, drag out")
            << QPointingDevice::primaryPointingDevice() << "ReleaseWithinBounds"
            << longPressThreshold << QPoint(155, 0) << false << false;
    QTest::newRow("touch, lpt longPressThreshold: ReleaseWithinBounds, drag out")
            << constTouchDevice << "ReleaseWithinBounds"
            << longPressThreshold << QPoint(155, 0) << false << false;

    QTest::newRow("mouse, lpt longPressThreshold: DragWithinBounds")
            << QPointingDevice::primaryPointingDevice() << "DragWithinBounds"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: DragWithinBounds")
            << constTouchDevice << "DragWithinBounds"
            << longPressThreshold << QPoint(0, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: DragWithinBounds, drag")
            << QPointingDevice::primaryPointingDevice() << "DragWithinBounds"
            << longPressThreshold << QPoint(50, 0) << true << false;
    QTest::newRow("touch, lpt longPressThreshold: DragWithinBounds, drag")
            << constTouchDevice << "DragWithinBounds"
            << longPressThreshold << QPoint(50, 0) << true << false;
    QTest::newRow("mouse, lpt longPressThreshold: DragWithinBounds, drag out")
            << QPointingDevice::primaryPointingDevice() << "DragWithinBounds"
            << longPressThreshold << QPoint(155, 0) << false << false;
    QTest::newRow("touch, lpt longPressThreshold: DragWithinBounds, drag out")
            << constTouchDevice << "DragWithinBounds"
            << longPressThreshold << QPoint(155, 0) << false << false;

    // Zero or negative threshold means long press is disabled
    QTest::newRow("mouse, lpt 0: DragThreshold")
            << QPointingDevice::primaryPointingDevice() << "DragThreshold"
            << qreal(0) << QPoint(0, 0) << false << true;
    QTest::newRow("mouse, lpt -1: DragThreshold")
            << QPointingDevice::primaryPointingDevice() << "DragThreshold"
            << qreal(-1) << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt 0: DragThreshold")
            << constTouchDevice << "DragThreshold"
            << qreal(0) << QPoint(0, 0) << false << true;

    QTest::newRow("mouse, lpt 0: WithinBounds")
            << QPointingDevice::primaryPointingDevice() << "WithinBounds"
            << qreal(0) << QPoint(0, 0) << false << true;
    QTest::newRow("mouse, lpt -1: WithinBounds")
            << QPointingDevice::primaryPointingDevice() << "WithinBounds"
            << qreal(-1) << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt 0: WithinBounds")
            << constTouchDevice << "WithinBounds"
            << qreal(0) << QPoint(0, 0) << false << true;

    QTest::newRow("mouse, lpt 0: ReleaseWithinBounds")
            << QPointingDevice::primaryPointingDevice() << "ReleaseWithinBounds"
            << qreal(0) << QPoint(0, 0) << false << true;
    QTest::newRow("mouse, lpt -1: ReleaseWithinBounds")
            << QPointingDevice::primaryPointingDevice() << "ReleaseWithinBounds"
            << qreal(-1) << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt 0: ReleaseWithinBounds")
            << constTouchDevice << "ReleaseWithinBounds"
            << qreal(0) << QPoint(0, 0) << false << true;

    QTest::newRow("mouse, lpt 0: DragWithinBounds")
            << QPointingDevice::primaryPointingDevice() << "DragWithinBounds"
            << qreal(0) << QPoint(0, 0) << false << true;
    QTest::newRow("mouse, lpt -1: DragWithinBounds")
            << QPointingDevice::primaryPointingDevice() << "DragWithinBounds"
            << qreal(-1) << QPoint(0, 0) << true << false;
    QTest::newRow("touch, lpt 0: DragWithinBounds")
            << constTouchDevice << "DragWithinBounds"
            << qreal(0) << QPoint(0, 0) << false << true;
}

void tst_TapHandler::longPress()
{
    QFETCH(const QPointingDevice *, device);
    QFETCH(QString, buttonName);
    QFETCH(qreal, longPressThreshold);
    QFETCH(QPoint, releaseOffset);
    QFETCH(bool, expectLongPress);
    QFETCH(bool, expectTapped);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("buttons.qml")));

    QQuickItem *button = window.rootObject()->findChild<QQuickItem*>(buttonName);
    QVERIFY(button);
    QQuickTapHandler *tapHandler = button->findChild<QQuickTapHandler*>(buttonName);
    QVERIFY(tapHandler);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));
    QSignalSpy longPressThresholdChangedSpy(tapHandler, SIGNAL(longPressThresholdChanged()));
    QSignalSpy timeHeldSpy(tapHandler, SIGNAL(timeHeldChanged()));
    QSignalSpy longPressedSpy(tapHandler, SIGNAL(longPressed()));

    const qreal defaultThreshold = tapHandler->longPressThreshold();
    qsizetype changedCount = 0;
    QCOMPARE_GT(defaultThreshold, 0);
    tapHandler->setLongPressThreshold(longPressThreshold);
    if (longPressThreshold > 0)
        QCOMPARE(longPressThresholdChangedSpy.size(), ++changedCount);
    QVERIFY(QMetaObject::invokeMethod(button, "assignUndefinedLongPressThreshold"));
    if (longPressThreshold > 0)
        QCOMPARE(longPressThresholdChangedSpy.size(), ++changedCount);
    QCOMPARE(tapHandler->longPressThreshold(), defaultThreshold);
    tapHandler->setLongPressThreshold(longPressThreshold);

    // Press and hold
    QPoint p1 = button->mapToScene(button->clipRect().center()).toPoint();
    QQuickTest::pointerPress(device, &window, 1, p1);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTRY_COMPARE(longPressedSpy.size(), expectLongPress ? 1 : 0);
    timeHeldSpy.wait(); // the longer we hold it, the more this will occur
    qDebug() << "held" << tapHandler->timeHeld() << "secs; timeHeld updated" << timeHeldSpy.size() << "times";
    QCOMPARE_GT(timeHeldSpy.size(), 0);
    if (expectLongPress) {
        // Should be > longPressThreshold but slow CI and timer granularity can interfere
        QCOMPARE_GT(tapHandler->timeHeld(), longPressThreshold - 0.1);
    } else {
        // Should be quite small, but event delivery is not instantaneous
        QCOMPARE_LT(tapHandler->timeHeld(), 0.3);
    }

    // If we have an offset, we need a move between press and release for realistic simulation
    if (!releaseOffset.isNull())
        QQuickTest::pointerMove(device, &window, 1, p1 + releaseOffset);

    // Release (optionally at an offset) and check whether tapped was emitted
    QQuickTest::pointerRelease(device, &window, 1, p1 + releaseOffset);
    QTRY_VERIFY(!button->property("pressed").toBool());
    if (expectLongPress)
        QCOMPARE_GT(button->property("timeHeldWhenLongPressed").toReal(), longPressThreshold - 0.1);
    QCOMPARE(tapHandler->timeHeld(), -1);
    QCOMPARE(tappedSpy.size(), expectTapped ? 1 : 0);
    QCOMPARE(longPressedSpy.size(), expectLongPress ? 1 : 0);
}

void tst_TapHandler::buttonsMultiTouch()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttons.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *buttonDragThreshold = window->rootObject()->findChild<QQuickItem*>("DragThreshold");
    QVERIFY(buttonDragThreshold);
    QSignalSpy dragThresholdTappedSpy(buttonDragThreshold, SIGNAL(tapped()));

    QQuickItem *buttonWithinBounds = window->rootObject()->findChild<QQuickItem*>("WithinBounds");
    QVERIFY(buttonWithinBounds);
    QSignalSpy withinBoundsTappedSpy(buttonWithinBounds, SIGNAL(tapped()));

    QQuickItem *buttonReleaseWithinBounds = window->rootObject()->findChild<QQuickItem*>("ReleaseWithinBounds");
    QVERIFY(buttonReleaseWithinBounds);
    QSignalSpy releaseWithinBoundsTappedSpy(buttonReleaseWithinBounds, SIGNAL(tapped()));
    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(window, touchDevice, false);

    // can press multiple buttons at the same time
    QPoint p1 = buttonDragThreshold->mapToScene(QPointF(20, 20)).toPoint();
    touchSeq.press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QPoint p2 = buttonWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    touchSeq.stationary(1).press(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("active").toBool());
    QPoint p3 = buttonReleaseWithinBounds->mapToScene(QPointF(20, 20)).toPoint();
    touchSeq.stationary(1).stationary(2).press(3, p3, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("active").toBool());
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("active").toBool());
    QVERIFY(buttonDragThreshold->property("pressed").toBool());

    // combinations of small touchpoint movements and stationary points should not cause state changes
    p1 += QPoint(2, 0);
    p2 += QPoint(3, 0);
    touchSeq.move(1, p1).move(2, p2).stationary(3).commit();
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("active").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("active").toBool());
    p3 += QPoint(4, 0);
    touchSeq.stationary(1).stationary(2).move(3, p3).commit();
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("active").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("active").toBool());

    // can release top button and press again: others stay pressed the whole time
    touchSeq.stationary(2).stationary(3).release(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonDragThreshold->property("pressed").toBool());
    QCOMPARE(dragThresholdTappedSpy.size(), 1);
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 0);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 0);
    touchSeq.stationary(2).stationary(3).press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());

    // can release middle button and press again: others stay pressed the whole time
    touchSeq.stationary(1).stationary(3).release(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 1);
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    QCOMPARE(dragThresholdTappedSpy.size(), 1);
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 0);
    touchSeq.stationary(1).stationary(3).press(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(buttonDragThreshold->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());

    // can release bottom button and press again: others stay pressed the whole time
    touchSeq.stationary(1).stationary(2).release(3, p3, window).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(releaseWithinBoundsTappedSpy.size(), 1);
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QCOMPARE(withinBoundsTappedSpy.size(), 1);
    QVERIFY(!buttonReleaseWithinBounds->property("pressed").toBool());
    QCOMPARE(dragThresholdTappedSpy.size(), 1);
    touchSeq.stationary(1).stationary(2).press(3, p3, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(buttonDragThreshold->property("pressed").toBool());
    QVERIFY(buttonWithinBounds->property("pressed").toBool());
    QVERIFY(buttonReleaseWithinBounds->property("pressed").toBool());
}

void tst_TapHandler::componentUserBehavioralOverride()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "buttonOverrideHandler.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>("Overridden");
    QVERIFY(button);
    QQuickTapHandler *innerTapHandler = button->findChild<QQuickTapHandler*>("Overridden");
    QVERIFY(innerTapHandler);
    QQuickTapHandler *userTapHandler = button->findChild<QQuickTapHandler*>("override");
    QVERIFY(userTapHandler);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));
    QSignalSpy innerGrabChangedSpy(innerTapHandler, SIGNAL(grabChanged(QPointingDevice::GrabTransition,QEventPoint)));
    QSignalSpy userGrabChangedSpy(userTapHandler, SIGNAL(grabChanged(QPointingDevice::GrabTransition,QEventPoint)));
    QSignalSpy innerPressedChangedSpy(innerTapHandler, SIGNAL(pressedChanged()));
    QSignalSpy userPressedChangedSpy(userTapHandler, SIGNAL(pressedChanged()));

    // Press
    QPoint p1 = button->mapToScene(button->clipRect().center()).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(userPressedChangedSpy.size(), 1);
    QCOMPARE(innerPressedChangedSpy.size(), 0);
    QCOMPARE(innerGrabChangedSpy.size(), 0);
    QCOMPARE(userGrabChangedSpy.size(), 1);

    // Release
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(userPressedChangedSpy.size(), 2);
    QCOMPARE(innerPressedChangedSpy.size(), 0);
    QCOMPARE(tappedSpy.size(), 1); // only because the override handler makes that happen
    QCOMPARE(innerGrabChangedSpy.size(), 0);
    QCOMPARE(userGrabChangedSpy.size(), 2);
}

void tst_TapHandler::rightLongPressIgnoreWheel()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "rightTapHandler.qml");
    QQuickView * window = windowPtr.data();

    QQuickTapHandler *tap = window->rootObject()->findChild<QQuickTapHandler*>();
    QVERIFY(tap);
    QSignalSpy tappedSpy(tap, &QQuickTapHandler::tapped);
    QSignalSpy longPressedSpy(tap, &QQuickTapHandler::longPressed);
    QPoint p1(100, 100);

    // Mouse wheel with ScrollBegin phase (because as soon as two fingers are touching
    // the trackpad, it will send such an event: QTBUG-71955)
    {
        QWheelEvent wheelEvent(p1, p1, QPoint(0, 0), QPoint(0, 0),
                               Qt::NoButton, Qt::NoModifier, Qt::ScrollBegin, false, Qt::MouseEventNotSynthesized);
        QGuiApplication::sendEvent(window, &wheelEvent);
    }

    // Press
    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, p1);
    QTRY_COMPARE(tap->isPressed(), true);

    // Mouse wheel ScrollEnd phase
    QWheelEvent wheelEvent(p1, p1, QPoint(0, 0), QPoint(0, 0),
                           Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false, Qt::MouseEventNotSynthesized);
    QGuiApplication::sendEvent(window, &wheelEvent);
    QTRY_COMPARE(longPressedSpy.size(), 1);
    QCOMPARE(tap->isPressed(), true);
    QCOMPARE(tappedSpy.size(), 0);

    // Release
    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, p1, 500);
    QTRY_COMPARE(tap->isPressed(), false);
    QCOMPARE(tappedSpy.size(), 0);
}

void tst_TapHandler::negativeZStackingOrder() // QTBUG-83114
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "nested.qml");
    QQuickView *window = windowPtr.data();
    QQuickItem *root = window->rootObject();

    QQuickTapHandler *parentTapHandler = window->rootObject()->findChild<QQuickTapHandler*>("parentTapHandler");
    QVERIFY(parentTapHandler != nullptr);
    QSignalSpy clickSpyParent(parentTapHandler, &QQuickTapHandler::tapped);
    QQuickTapHandler *childTapHandler = window->rootObject()->findChild<QQuickTapHandler*>("childTapHandler");
    QVERIFY(childTapHandler != nullptr);
    QSignalSpy clickSpyChild(childTapHandler, &QQuickTapHandler::tapped);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 100));
    QCOMPARE(clickSpyChild.size(), 1);
    QCOMPARE(clickSpyParent.size(), 1);
    auto order = root->property("taps").toList();
    QVERIFY(order.at(0) == "childTapHandler");
    QVERIFY(order.at(1) == "parentTapHandler");

    // Now change stacking order and try again.
    childTapHandler->parentItem()->setZ(-1);
    root->setProperty("taps", QVariantList());
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 100));
    QCOMPARE(clickSpyChild.size(), 2);
    QCOMPARE(clickSpyParent.size(), 2);
    order = root->property("taps").toList();
    QVERIFY(order.at(0) == "parentTapHandler");
    QVERIFY(order.at(1) == "childTapHandler");
}

void tst_TapHandler::nonTopLevelParentWindow() // QTBUG-91716
{
    QScopedPointer<QQuickWindow> parentWindowPtr(new QQuickWindow);
    auto parentWindow = parentWindowPtr.get();
    parentWindow->setGeometry(400, 400, 250, 250);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "simpleTapHandler.qml", parentWindow);
    auto window = windowPtr.get();
    window->setGeometry(10, 10, 100, 100);

    QQuickItem *root = window->rootObject();

    auto p1 = QPoint(20, 20);
    mouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, p1, window, parentWindow);
    mouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, p1, window, parentWindow);

    QCOMPARE(root->property("tapCount").toInt(), 1);

    QTest::touchEvent(window, touchDevice).press(0, p1, parentWindow).commit();
    QTest::touchEvent(window, touchDevice).release(0, p1, parentWindow).commit();

    QCOMPARE(root->property("tapCount").toInt(), 2);
}

void tst_TapHandler::nestedDoubleTap_data()
{
    QTest::addColumn<QQuickTapHandler::GesturePolicy>("childGesturePolicy");

    QTest::newRow("DragThreshold") << QQuickTapHandler::GesturePolicy::DragThreshold;
    QTest::newRow("WithinBounds") << QQuickTapHandler::GesturePolicy::WithinBounds;
    QTest::newRow("ReleaseWithinBounds") << QQuickTapHandler::GesturePolicy::ReleaseWithinBounds;
    QTest::newRow("DragWithinBounds") << QQuickTapHandler::GesturePolicy::DragWithinBounds;
}

void tst_TapHandler::nestedDoubleTap() // QTBUG-102625
{
    QFETCH(QQuickTapHandler::GesturePolicy, childGesturePolicy);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nested.qml")));
    QQuickItem *root = window.rootObject();
    QQuickTapHandler *parentTapHandler = root->findChild<QQuickTapHandler*>("parentTapHandler");
    QVERIFY(parentTapHandler);
    QSignalSpy parentSpy(parentTapHandler, &QQuickTapHandler::doubleTapped);
    QQuickTapHandler *childTapHandler = root->findChild<QQuickTapHandler*>("childTapHandler");
    QVERIFY(childTapHandler);
    QSignalSpy childSpy(childTapHandler, &QQuickTapHandler::doubleTapped);
    childTapHandler->setGesturePolicy(childGesturePolicy);

    QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 100));

    QCOMPARE(childSpy.size(), 1);
    // If the child gets by with a passive grab, both handlers see tap and double-tap.
    // If the child takes an exclusive grab and stops event propagation, the parent doesn't see them.
    QCOMPARE(parentSpy.size(),
             childGesturePolicy == QQuickTapHandler::GesturePolicy::DragThreshold ? 1 : 0);
    QCOMPARE(root->property("taps").toList().size(),
             childGesturePolicy == QQuickTapHandler::GesturePolicy::DragThreshold ? 4 : 2);
}

void tst_TapHandler::nestedAndSiblingPropagation_data()
{
    QTest::addColumn<const QPointingDevice *>("device");
    QTest::addColumn<QQuickTapHandler::GesturePolicy>("gesturePolicy");
    QTest::addColumn<bool>("expectPropagation");

    const QPointingDevice *constTouchDevice = touchDevice;

    QTest::newRow("primary, DragThreshold") << QPointingDevice::primaryPointingDevice()
            << QQuickTapHandler::GesturePolicy::DragThreshold << true;
    QTest::newRow("primary, WithinBounds") << QPointingDevice::primaryPointingDevice()
            << QQuickTapHandler::GesturePolicy::WithinBounds << false;
    QTest::newRow("primary, ReleaseWithinBounds") << QPointingDevice::primaryPointingDevice()
            << QQuickTapHandler::GesturePolicy::ReleaseWithinBounds << false;
    QTest::newRow("primary, DragWithinBounds") << QPointingDevice::primaryPointingDevice()
            << QQuickTapHandler::GesturePolicy::DragWithinBounds << false;

    QTest::newRow("touch, DragThreshold") << constTouchDevice
            << QQuickTapHandler::GesturePolicy::DragThreshold << true;
    QTest::newRow("touch, WithinBounds") << constTouchDevice
            << QQuickTapHandler::GesturePolicy::WithinBounds << false;
    QTest::newRow("touch, ReleaseWithinBounds") << constTouchDevice
            << QQuickTapHandler::GesturePolicy::ReleaseWithinBounds << false;
    QTest::newRow("touch, DragWithinBounds") << constTouchDevice
            << QQuickTapHandler::GesturePolicy::DragWithinBounds << false;
}

void tst_TapHandler::nestedAndSiblingPropagation() // QTBUG-117387
{
    QFETCH(const QPointingDevice *, device);
    QFETCH(QQuickTapHandler::GesturePolicy, gesturePolicy);
    QFETCH(bool, expectPropagation);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedAndSibling.qml")));
    QQuickItem *root = window.rootObject();
    QQuickTapHandler *th1 = root->findChild<QQuickTapHandler*>("th1");
    QVERIFY(th1);
    th1->setGesturePolicy(gesturePolicy);
    QQuickTapHandler *th2 = root->findChild<QQuickTapHandler*>("th2");
    QVERIFY(th2);
    th2->setGesturePolicy(gesturePolicy);
    QQuickTapHandler *th3 = root->findChild<QQuickTapHandler*>("th3");
    QVERIFY(th3);
    th3->setGesturePolicy(gesturePolicy);

    QPoint middle(180, 140);
    QQuickTest::pointerPress(device, &window, 0, middle);
    QVERIFY(th3->isPressed()); // it's on top
    QCOMPARE(th2->isPressed(), expectPropagation);
    QCOMPARE(th1->isPressed(), expectPropagation);

    QQuickTest::pointerRelease(device, &window, 0, middle);
}

QTEST_MAIN(tst_TapHandler)

#include "tst_qquicktaphandler.moc"

