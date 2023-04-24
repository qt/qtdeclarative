// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QtTest>

#include <QtGui/qstylehints.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickitemview_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickpinchhandler_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/private/qquicktableview_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qquickwindow_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_FlickableInterop : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_FlickableInterop()
         : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void touchTapButton_data();
    void touchTapButton();
    void touchDragFlickableBehindButton_data();
    void touchDragFlickableBehindButton();
    void mouseClickButton_data();
    void mouseClickButton();
    void mouseDragFlickableBehindButton_data();
    void mouseDragFlickableBehindButton();
    void touchDragSlider();
    void touchDragFlickableBehindSlider();
    void mouseDragSlider_data();
    void mouseDragSlider();
    void mouseDragFlickableBehindSlider();
    void touchDragFlickableBehindItemWithHandlers_data();
    void touchDragFlickableBehindItemWithHandlers();
    void mouseDragFlickableBehindItemWithHandlers_data();
    void mouseDragFlickableBehindItemWithHandlers();
    void touchDragSliderAndFlickable();
    void touchAndDragHandlerOnFlickable_data();
    void touchAndDragHandlerOnFlickable();
    void pinchHandlerOnFlickable();
    void nativeGesturePinchOnFlickableWithParentTapHandler_data();
    void nativeGesturePinchOnFlickableWithParentTapHandler();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QScopedPointer<QPointingDevice> touchDevice = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
    QScopedPointer<QPointingDevice> touchpad = QScopedPointer<QPointingDevice>(
                QTest::createTouchDevice(QInputDevice::DeviceType::TouchPad));
};

void tst_FlickableInterop::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtils::centerOnScreen(window.data());
    QQuickViewTestUtils::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

void tst_FlickableInterop::touchTapButton_data()
{
    QTest::addColumn<QString>("buttonName");
    QTest::newRow("DragThreshold") << QStringLiteral("DragThreshold");
    QTest::newRow("WithinBounds") << QStringLiteral("WithinBounds");
    QTest::newRow("ReleaseWithinBounds") << QStringLiteral("ReleaseWithinBounds");
}

void tst_FlickableInterop::touchTapButton()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QFETCH(QString, buttonName);

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>(buttonName);
    QVERIFY(button);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));

    // Button changes pressed state and emits tapped on release
    QPoint p1 = button->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 1);

    // We can drag <= dragThreshold and the button still acts normal, Flickable doesn't grab
    p1 = button->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 2);
}

void tst_FlickableInterop::touchDragFlickableBehindButton_data()
{
    QTest::addColumn<QString>("buttonName");
    QTest::newRow("DragThreshold") << QStringLiteral("DragThreshold");
    QTest::newRow("WithinBounds") << QStringLiteral("WithinBounds");
    QTest::newRow("ReleaseWithinBounds") << QStringLiteral("ReleaseWithinBounds");
}

void tst_FlickableInterop::touchDragFlickableBehindButton()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QFETCH(QString, buttonName);

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>(buttonName);
    QVERIFY(button);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));

    tappedSpy.clear();
    QPoint p1 = button->mapToScene(QPointF(20, 20)).toPoint();
    QTest::touchEvent(window, touchDevice.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(button->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(button->property("pressed").toBool());
    int i = 0;
    // Start dragging; eventually when the touchpoint goes beyond dragThreshold,
    // Button is no longer pressed because Flickable steals the grab
    for (; i < 100 && !flickable->isMoving(); ++i) {
        p1 += QPoint(1, 0);
        QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
        QQuickTouchUtils::flush(window);
    }
    qCDebug(lcPointerTests) << "flickable started moving after" << i << "moves, when we got to" << p1;
    QVERIFY(flickable->isMoving());
    QCOMPARE(i, 2);
    QVERIFY(!button->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 0);
}

void tst_FlickableInterop::mouseClickButton_data()
{
    QTest::addColumn<QString>("buttonName");
    QTest::newRow("DragThreshold") << QStringLiteral("DragThreshold");
    QTest::newRow("WithinBounds") << QStringLiteral("WithinBounds");
    QTest::newRow("ReleaseWithinBounds") << QStringLiteral("ReleaseWithinBounds");
}

void tst_FlickableInterop::mouseClickButton()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QFETCH(QString, buttonName);

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>(buttonName);
    QVERIFY(button);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));

    // Button changes pressed state and emits tapped on release
    QPoint p1 = button->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(button->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 1);

    // We can drag <= dragThreshold and the button still acts normal, Flickable doesn't grab
    p1 = button->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1, qApp->styleHints()->mouseDoubleClickInterval() + 10);
    QTRY_VERIFY(button->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(button->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 2);
}

void tst_FlickableInterop::mouseDragFlickableBehindButton_data()
{
    QTest::addColumn<QString>("buttonName");
    QTest::newRow("DragThreshold") << QStringLiteral("DragThreshold");
    QTest::newRow("WithinBounds") << QStringLiteral("WithinBounds");
    QTest::newRow("ReleaseWithinBounds") << QStringLiteral("ReleaseWithinBounds");
}

void tst_FlickableInterop::mouseDragFlickableBehindButton()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QFETCH(QString, buttonName);

    QQuickItem *button = window->rootObject()->findChild<QQuickItem*>(buttonName);
    QVERIFY(button);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QSignalSpy tappedSpy(button, SIGNAL(tapped()));

    // Button is no longer pressed if touchpoint goes beyond dragThreshold,
    // because Flickable steals the grab
    tappedSpy.clear();
    QPoint p1 = button->mapToScene(QPointF(20, 20)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(button->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QVERIFY(button->property("pressed").toBool());
    int i = 0;
    for (; i < 100 && !flickable->isMoving(); ++i) {
        p1 += QPoint(1, 0);
        QTest::mouseMove(window, p1);
    }
    qCDebug(lcPointerTests) << "flickable started moving after" << i << "moves, when we got to" << p1;
    QVERIFY(flickable->isMoving());
    QCOMPARE(i, 2);
    QVERIFY(!button->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!button->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 0);
}

void tst_FlickableInterop::touchDragSlider()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *slider = window->rootObject()->findChild<QQuickItem*>("knobSlider");
    QVERIFY(slider);
    QQuickDragHandler *drag = slider->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQuickItem *knob = slider->findChild<QQuickItem*>("Slider Knob");
    QVERIFY(knob);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QSignalSpy tappedSpy(knob->parent(), SIGNAL(tapped()));
    QSignalSpy translationChangedSpy(drag, &QQuickDragHandler::translationChanged);

    // Drag the slider in the allowed (vertical) direction
    tappedSpy.clear();
    QPoint p1 = knob->mapToScene(knob->clipRect().center()).toPoint() - QPoint(0, 8);
    QTest::touchEvent(window, touchDevice.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(slider->property("pressed").toBool());
    p1 += QPoint(0, dragThreshold);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(slider->property("pressed").toBool());
    QCOMPARE(slider->property("value").toInt(), 49);
    p1 += QPoint(0, 1);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    p1 += QPoint(0, 10);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(slider->property("value").toInt() < 49);
    QVERIFY(!flickable->isMoving());
    QVERIFY(!slider->property("pressed").toBool());

    // Now that the DragHandler is active, the Flickable will not steal the grab
    // even if we move a large distance horizontally
    p1 += QPoint(dragThreshold * 2, 0);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!flickable->isMoving());

    // Release, and do not expect the tapped signal
    QTest::touchEvent(window, touchDevice.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(tappedSpy.size(), 0);
    QCOMPARE(translationChangedSpy.size(), 1);
}

void tst_FlickableInterop::mouseDragSlider_data()
{
    QTest::addColumn<QString>("nameOfSliderToDrag");
    QTest::addColumn<QPoint>("pressPositionRelativeToKnob");
    QTest::addColumn<QPoint>("dragDirection"); // a unit vector
    QTest::addColumn<bool>("expectedTapHandlerPressed");
    QTest::addColumn<bool>("expectedDragHandlerActive");
    QTest::addColumn<bool>("expectedFlickableMoving");

    QTest::newRow("drag down on knob of knobSlider") <<         "knobSlider" << QPoint(0, -8) << QPoint(0, 1) << true << true << false;
    QTest::newRow("drag sideways on knob of knobSlider") <<     "knobSlider" << QPoint(0, 0) << QPoint(1, 0) << true << false << true;
    QTest::newRow("drag down on groove of knobSlider") <<       "knobSlider" << QPoint(0, 20) << QPoint(0, 1) << false << false << true;
    QTest::newRow("drag sideways on groove of knobSlider") <<   "knobSlider" << QPoint(0, 20) << QPoint(1, 0) << false << false << true;

    QTest::newRow("drag down on knob of grooveSlider") <<       "grooveSlider" << QPoint(0, -8) << QPoint(0, 1) << true << true << false;
    QTest::newRow("drag sideways on knob of grooveSlider") <<   "grooveSlider" << QPoint(0, 0) << QPoint(1, 0) << true << false << true;
    QTest::newRow("drag down on groove of grooveSlider") <<     "grooveSlider" << QPoint(0, 20) << QPoint(0, 1) << false << true << false;
    QTest::newRow("drag sideways on groove of grooveSlider") << "grooveSlider" << QPoint(0, 20) << QPoint(1, 0) << false << false << true;
}

void tst_FlickableInterop::mouseDragSlider()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QFETCH(QString, nameOfSliderToDrag);
    QFETCH(QPoint, pressPositionRelativeToKnob);
    QFETCH(QPoint, dragDirection); // a unit vector
    QFETCH(bool, expectedTapHandlerPressed);
    QFETCH(bool, expectedDragHandlerActive);
    QFETCH(bool, expectedFlickableMoving);

    QQuickItem *slider = window->rootObject()->findChild<QQuickItem*>(nameOfSliderToDrag);
    QVERIFY(slider);
    QQuickDragHandler *drag = slider->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQuickItem *knob = slider->findChild<QQuickItem*>("Slider Knob");
    QVERIFY(knob);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QSignalSpy tappedSpy(knob->parent(), SIGNAL(tapped()));
    QSignalSpy translationChangedSpy(drag, &QQuickDragHandler::translationChanged);

    // Drag the slider
    tappedSpy.clear();
    QPoint p1 = knob->mapToScene(knob->clipRect().center()).toPoint() + pressPositionRelativeToKnob;
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(slider->property("pressed").toBool(), expectedTapHandlerPressed);
    p1 += QPoint(dragThreshold * dragDirection.x(), dragThreshold * dragDirection.y());
    QTest::mouseMove(window, p1);
    QCOMPARE(drag->active(), false);
    QCOMPARE(slider->property("pressed").toBool(), expectedTapHandlerPressed);
    QCOMPARE(slider->property("value").toInt(), 49);
    p1 += dragDirection; // one more pixel
    QTest::mouseMove(window, p1);
    // After moving by the drag threshold, the point should still be inside the knob.
    // However, QQuickTapHandler::wantsEventPoint() returns false because the drag threshold is exceeded.
    // Therefore QQuickTapHandler::setPressed(false, true, point) is called: the active state is canceled.
    QCOMPARE(slider->property("pressed").toBool(), false);
    QCOMPARE(drag->active(), expectedDragHandlerActive);
    // drag farther, to make sure the knob gets adjusted significantly
    p1 += QPoint(10 * dragDirection.x(), 10 * dragDirection.y());
    QTest::mouseMove(window, p1);
    if (expectedDragHandlerActive && dragDirection.y() > 0)
        QVERIFY(slider->property("value").toInt() < 49);
    // by now, Flickable will have stolen the grab, if it decided that it wanted to during filtering of the last event
    QCOMPARE(flickable->isMoving(), expectedFlickableMoving);
    QCOMPARE(slider->property("pressed").toBool(), false);

    // If the DragHandler is active, the Flickable will not steal the grab
    // even if we move a large distance horizontally
    if (expectedDragHandlerActive) {
        p1 += QPoint(dragThreshold * 2, 0);
        QTest::mouseMove(window, p1);
        QCOMPARE(flickable->isMoving(), false);
    }

    // Release, and do not expect the tapped signal
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(tappedSpy.size(), 0);
    QCOMPARE(translationChangedSpy.size(), expectedDragHandlerActive ? 1 : 0);
}

void tst_FlickableInterop::touchDragFlickableBehindSlider()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *slider = window->rootObject()->findChild<QQuickItem*>("knobSlider");
    QVERIFY(slider);
    QQuickDragHandler *drag = slider->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQuickItem *knob = slider->findChild<QQuickItem*>("Slider Knob");
    QVERIFY(knob);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QSignalSpy tappedSpy(knob->parent(), SIGNAL(tapped()));
    QSignalSpy translationChangedSpy(drag, &QQuickDragHandler::translationChanged);

    // Button is no longer pressed if touchpoint goes beyond dragThreshold,
    // because Flickable steals the grab
    tappedSpy.clear();
    QPoint p1 = knob->mapToScene(knob->clipRect().center()).toPoint();
    QTest::touchEvent(window, touchDevice.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(slider->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(slider->property("pressed").toBool());
    int i = 0;
    for (; i < 100 && !flickable->isMoving(); ++i) {
        p1 += QPoint(1, 0);
        QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
        QQuickTouchUtils::flush(window);
    }
    qCDebug(lcPointerTests) << "flickable started moving after" << i << "moves, when we got to" << p1;
    QVERIFY(flickable->isMoving());
    QCOMPARE(i, 2);
    QVERIFY(!slider->property("pressed").toBool());
    QTest::touchEvent(window, touchDevice.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!slider->property("pressed").toBool());
    QCOMPARE(tappedSpy.size(), 0);
    QCOMPARE(translationChangedSpy.size(), 0);
}

void tst_FlickableInterop::mouseDragFlickableBehindSlider()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *slider = window->rootObject()->findChild<QQuickItem*>("knobSlider");
    QVERIFY(slider);
    QQuickDragHandler *drag = slider->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQuickItem *knob = slider->findChild<QQuickItem*>("Slider Knob");
    QVERIFY(knob);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QSignalSpy tappedSpy(knob->parent(), SIGNAL(tapped()));
    QSignalSpy translationChangedSpy(drag, &QQuickDragHandler::translationChanged);

    // Button is no longer pressed if touchpoint goes beyond dragThreshold,
    // because Flickable steals the grab
    tappedSpy.clear();
    QPoint p1 = knob->mapToScene(knob->clipRect().center()).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(slider->property("pressed").toBool());
    p1 += QPoint(dragThreshold, 0);
    QTest::mouseMove(window, p1);
    QQuickTouchUtils::flush(window);
    QVERIFY(slider->property("pressed").toBool());
    int i = 0;
    for (; i < 100 && !flickable->isMoving(); ++i) {
        p1 += QPoint(1, 0);
        QTest::mouseMove(window, p1);
    }
    qCDebug(lcPointerTests) << "flickable started moving after" << i << "moves, when we got to" << p1;
    QVERIFY(flickable->isMoving());
    QCOMPARE(i, 2);
    QVERIFY(!slider->property("pressed").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(tappedSpy.size(), 0);
    QCOMPARE(translationChangedSpy.size(), 0);
}

void tst_FlickableInterop::touchDragFlickableBehindItemWithHandlers_data()
{
    QTest::addColumn<QByteArray>("nameOfRectangleToDrag");
    QTest::addColumn<bool>("expectedFlickableMoving");
    QTest::newRow("drag") << QByteArray("drag") << false;
    QTest::newRow("tap") << QByteArray("tap") << true;
    QTest::newRow("dragAndTap") << QByteArray("dragAndTap") << false;
    QTest::newRow("tapAndDrag") << QByteArray("tapAndDrag") << false;
}

void tst_FlickableInterop::touchDragFlickableBehindItemWithHandlers()
{
    QFETCH(bool, expectedFlickableMoving);
    QFETCH(QByteArray, nameOfRectangleToDrag);
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem *rect = window->rootObject()->findChild<QQuickItem*>(nameOfRectangleToDrag);
    QVERIFY(rect);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QPoint p1 = rect->mapToScene(rect->clipRect().center()).toPoint();
    QPoint originP1 = p1;

    QTest::touchEvent(window, touchDevice.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    for (int i = 0; i < dragThreshold * 3; ++i) {
        p1 = originP1;
        p1.rx() += i;
        QTest::touchEvent(window, touchDevice.get()).move(1, p1, window);
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(flickable->isMoving(), expectedFlickableMoving);
    if (!expectedFlickableMoving) {
        QVERIFY(rect->mapToScene(rect->clipRect().center()).toPoint().x() > originP1.x());
    }
    QTest::touchEvent(window, touchDevice.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
}

void tst_FlickableInterop::mouseDragFlickableBehindItemWithHandlers_data()
{
    touchDragFlickableBehindItemWithHandlers_data();
}

void tst_FlickableInterop::mouseDragFlickableBehindItemWithHandlers()
{
    QFETCH(bool, expectedFlickableMoving);
    QFETCH(QByteArray, nameOfRectangleToDrag);
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem *rect = window->rootObject()->findChild<QQuickItem*>(nameOfRectangleToDrag);
    QVERIFY(rect);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QPoint p1 = rect->mapToScene(rect->clipRect().center()).toPoint();
    QPoint originP1 = p1;
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    for (int i = 0; i < 3; ++i) {
        p1 += QPoint(dragThreshold, 0);
        QTest::mouseMove(window, p1);
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(flickable->isMoving(), expectedFlickableMoving);
    if (!expectedFlickableMoving) {
        QCOMPARE(originP1 + QPoint(3*dragThreshold, 0), p1);
    }
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    // wait until flickable stops
    QTRY_COMPARE(flickable->isMoving(), false);

    // After the mouse button has been released, move the mouse and ensure that nothing is moving
    // because of that (this tests if all grabs are released when the mouse button is released).
    p1 = rect->mapToScene(rect->clipRect().center()).toPoint();
    originP1 = p1;
    for (int i = 0; i < 3; ++i) {
        p1 += QPoint(dragThreshold, 0);
        QTest::mouseMove(window, p1);
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(flickable->isMoving(), false);
    QCOMPARE(originP1, rect->mapToScene(rect->clipRect().center()).toPoint());
}

void tst_FlickableInterop::touchDragSliderAndFlickable()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "flickableWithHandlers.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *slider = window->rootObject()->findChild<QQuickItem*>("knobSlider");
    QVERIFY(slider);
    QQuickDragHandler *drag = slider->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQuickItem *knob = slider->findChild<QQuickItem*>("Slider Knob");
    QVERIFY(knob);
    QQuickFlickable *flickable = window->rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(window, touchDevice.get(), false);

    // The knob is initially centered over the slider's "groove"
    qreal initialXOffset = qAbs(knob->mapToScene(knob->clipRect().center()).x() - slider->mapToScene
                                (slider->clipRect().center()).x());
    QVERIFY(initialXOffset <= 1);

    // Drag the slider in the allowed (vertical) direction with one finger
    QPoint p1 = knob->mapToScene(knob->clipRect().center()).toPoint();
    touchSeq.press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    p1 += QPoint(0, dragThreshold);
    touchSeq.move(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    p1 += QPoint(0, dragThreshold);
    touchSeq.move(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    p1 += QPoint(0, dragThreshold);
    touchSeq.move(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(slider->property("value").toInt() < 49);
    QVERIFY(!flickable->isMoving());

    // Drag the Flickable with a second finger
    QPoint p2(300,300);
    touchSeq.stationary(1).press(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(-10, -10);
        p2 += QPoint(dragThreshold, 0);
        touchSeq.move(1, p1, window).stationary(2).commit();
        QQuickTouchUtils::flush(window);
        p1 += QPoint(-10, -10);
        p2 += QPoint(dragThreshold, 0);
        touchSeq.stationary(1).move(2, p2, window).commit();
        QQuickTouchUtils::flush(window);
        qCDebug(lcPointerTests) << "step" << i << ": fingers @" << p1 << p2 << "is Flickable moving yet?" << flickable->isMoving();
    }
    QVERIFY(flickable->isMoving());
    qreal knobSliderXOffset = qAbs(knob->mapToScene(knob->clipRect().center()).toPoint().x() -
        slider->mapToScene(slider->clipRect().center()).toPoint().x()) - initialXOffset;
    if (knobSliderXOffset > 1)
        qCDebug(lcPointerTests) << "knob has slipped out of groove by" << knobSliderXOffset << "pixels";
    // See if the knob is still centered over the slider's "groove"
    QVERIFY(qAbs(knobSliderXOffset) <= 1);

    // Release
    touchSeq.release(1, p1, window).release(2, p2, window).commit();
}

void tst_FlickableInterop::touchAndDragHandlerOnFlickable_data()
{
    QTest::addColumn<QByteArray>("qmlFile");
    QTest::addColumn<bool>("pressDelay");
    QTest::addColumn<bool>("targetNull");
    QTest::addColumn<QQuickTapHandler::GesturePolicy>("tapGesturePolicy");
    QTest::newRow("tapOnFlickable") << QByteArray("tapOnFlickable.qml") << false << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapOnFlickable-excl") << QByteArray("tapOnFlickable.qml") << false << false << QQuickTapHandler::ReleaseWithinBounds;
    QTest::newRow("tapOnList") << QByteArray("tapOnList.qml") << false << false << QQuickTapHandler::DragThreshold;
    // QTBUG-75223
    QTest::newRow("tapOnList-excl") << QByteArray("tapOnList.qml") << false << false << QQuickTapHandler::ReleaseWithinBounds;
    QTest::newRow("tapOnTable") << QByteArray("tapOnTable.qml") << false << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragOnFlickable") << QByteArray("dragOnFlickable.qml") << false << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragOnList") << QByteArray("dragOnList.qml") << false << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragOnTable") << QByteArray("dragOnTable.qml") << false << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapDelayOnFlickable") << QByteArray("tapOnFlickable.qml") << true << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapDelayOnFlickable-excl") << QByteArray("tapOnFlickable.qml") << true << false << QQuickTapHandler::ReleaseWithinBounds;
    QTest::newRow("tapDelayOnList") << QByteArray("tapOnList.qml") << true << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapDelayOnList-excl") << QByteArray("tapOnList.qml") << true << false << QQuickTapHandler::ReleaseWithinBounds;
    QTest::newRow("tapDelayOnTable") << QByteArray("tapOnTable.qml") << true << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragDelayOnFlickable") << QByteArray("dragOnFlickable.qml") << true << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragDelayOnList") << QByteArray("dragOnList.qml") << true << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragDelayOnTable") << QByteArray("dragOnTable.qml") << true << false << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapOnFlickableWithNullTargets") << QByteArray("tapOnFlickable.qml") << false << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapOnListWithNullTargets") << QByteArray("tapOnList.qml") << false << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapOnTableWithNullTargets") << QByteArray("tapOnTable.qml") << false << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragOnFlickableWithNullTargets") << QByteArray("dragOnFlickable.qml") << false << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragOnListWithNullTargets") << QByteArray("dragOnList.qml") << false << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragOnTableWithNullTargets") << QByteArray("dragOnTable.qml") << false << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapDelayOnFlickableWithNullTargets") << QByteArray("tapOnFlickable.qml") << true << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapDelayOnListWithNullTargets") << QByteArray("tapOnList.qml") << true << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("tapDelayOnTableWithNullTargets") << QByteArray("tapOnTable.qml") << true << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragDelayOnFlickableWithNullTargets") << QByteArray("dragOnFlickable.qml") << true << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragDelayOnListWithNullTargets") << QByteArray("dragOnList.qml") << true << true << QQuickTapHandler::DragThreshold;
    QTest::newRow("dragDelayOnTableWithNullTargets") << QByteArray("dragOnTable.qml") << true << true << QQuickTapHandler::DragThreshold;
}

void tst_FlickableInterop::touchAndDragHandlerOnFlickable()
{
    QFETCH(QByteArray, qmlFile);
    QFETCH(bool, pressDelay);
    QFETCH(bool, targetNull);
    QFETCH(QQuickTapHandler::GesturePolicy, tapGesturePolicy);

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, qmlFile.constData());
    QQuickView * window = windowPtr.data();
    QQuickFlickable *flickable = qmlobject_cast<QQuickFlickable*>(window->rootObject());
    QVERIFY(flickable);
    flickable->setPressDelay(pressDelay ? 5000 : 0);
    QQuickItem *delegate = flickable->property("delegateUnderTest").value<QQuickItem*>();
    QQuickItem *button = delegate ? delegate->findChild<QQuickItem*>("button")
                                  : flickable->findChild<QQuickItem*>("button");
    if (!button)
        button = flickable->property("buttonUnderTest").value<QQuickItem*>();
    QVERIFY(button);
    QQuickPointerHandler *buttonHandler = button->findChild<QQuickPointerHandler*>();
    QVERIFY(buttonHandler);
    QQuickTapHandler *buttonTapHandler = qmlobject_cast<QQuickTapHandler *>(buttonHandler);
    if (buttonTapHandler)
        buttonTapHandler->setGesturePolicy(tapGesturePolicy);
    QQuickDragHandler *buttonDragHandler = qmlobject_cast<QQuickDragHandler *>(buttonHandler);
    QQuickPointerHandler *delegateHandler = delegate ? delegate->findChild<QQuickPointerHandler*>() : nullptr;
    QQuickTapHandler *delegateTapHandler = qmlobject_cast<QQuickTapHandler *>(delegateHandler);
    if (delegateTapHandler)
        delegateTapHandler->setGesturePolicy(tapGesturePolicy);
    QQuickPointerHandler *contentItemHandler = flickable->findChild<QQuickPointerHandler*>();
    QVERIFY(contentItemHandler);
    // a handler declared directly in a Flickable (or item view) must actually be a child of the contentItem,
    // just as Items declared inside are (QTBUG-71918 and QTBUG-73035)
    QCOMPARE(contentItemHandler->parentItem(), flickable->contentItem());
    if (targetNull) {
        buttonHandler->setTarget(nullptr);
        if (delegateHandler)
            delegateHandler->setTarget(nullptr);
        contentItemHandler->setTarget(nullptr);
    }

    // Drag one finger on the Flickable (between delegates) and make sure it flicks
    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(window, touchDevice.get(), false);
    QPoint p1(780, 460);
    if (delegate)
        p1 = delegate->mapToScene(delegate->clipRect().bottomRight()).toPoint() + QPoint(-1, 1);
    qCDebug(lcPointerTests) << "drag between delegates starting @" << p1;
    touchSeq.press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    for (int i = 0; i < 4; ++i) {
        p1 -= QPoint(dragThreshold, dragThreshold);
        touchSeq.move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
    }
    if (!(buttonDragHandler && !pressDelay))
        QTRY_VERIFY(flickable->contentY() >= dragThreshold);
    if (buttonTapHandler)
        QCOMPARE(buttonTapHandler->isPressed(), false);
    touchSeq.release(1, p1, window).commit();
    QQuickTouchUtils::flush(window);

    // Drag one finger on the delegate and make sure Flickable flicks
    if (delegate) {
        flickable->setContentY(0);
        QTRY_COMPARE(flickable->isMoving(), false);
        QVERIFY(delegateHandler);
        p1 = delegate->mapToScene(delegate->clipRect().topRight()).toPoint() + QPoint(-2, 2);
        qCDebug(lcPointerTests) << "drag on delegate 2 starting @" << p1;
        touchSeq.press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        if (delegateTapHandler && !pressDelay)
            QCOMPARE(delegateTapHandler->isPressed(), true);
        for (int i = 0; i < 4; ++i) {
            p1 -= QPoint(dragThreshold, dragThreshold);
            touchSeq.move(1, p1, window).commit();
            QQuickTouchUtils::flush(window);
            qCDebug(lcPointerTests) << i << p1 << delegateHandler->objectName()
                                    << "active" << delegateHandler->active() << "flickable moving" << flickable->isMoving();
            if (i > 1)
                QTRY_VERIFY(delegateHandler->active() || flickable->isMoving());
        }
        if (!(buttonDragHandler && !pressDelay))
            QVERIFY(flickable->contentY() > 0);
        if (delegateTapHandler)
            QCOMPARE(delegateTapHandler->isPressed(), false);
        touchSeq.release(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
    }

    // Drag one finger on the button and make sure Flickable flicks
    flickable->setContentY(0);
    QTRY_COMPARE(flickable->isMoving(), false);
    p1 = button->mapToScene(button->clipRect().center()).toPoint();
    touchSeq.press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    if (buttonTapHandler && !pressDelay)
        QTRY_COMPARE(buttonTapHandler->isPressed(), true);
    for (int i = 0; i < 4; ++i) {
        p1 -= QPoint(dragThreshold, dragThreshold);
        touchSeq.move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
    }
    if (!(buttonDragHandler && !pressDelay))
        QVERIFY(flickable->contentY() > 0);
    if (buttonTapHandler)
        QCOMPARE(buttonTapHandler->isPressed(), false);
    touchSeq.release(1, p1, window).commit();
}

void tst_FlickableInterop::pinchHandlerOnFlickable()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pinchOnFlickable.qml")));
    QQuickFlickable *flickable = qmlobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);
    QQuickPointerHandler *pinchHandler = flickable->findChild<QQuickPointerHandler*>();
    QVERIFY(pinchHandler);
    QQuickItem *pinchable = pinchHandler->target();
    QVERIFY(pinchable);

    QSignalSpy flickMoveSpy(flickable, &QQuickFlickable::movementStarted);
    QSignalSpy grabChangedSpy(touchDevice.get(), &QPointingDevice::grabChanged);

    QObject *grabber = nullptr;
    connect(touchDevice.get(), &QPointingDevice::grabChanged,
            [&grabber](QObject *g, QPointingDevice::GrabTransition transition, const QPointerEvent *, const QEventPoint &) {
        if (transition == QPointingDevice::GrabTransition::GrabExclusive)
            grabber = g;
    });

    QPoint p0 = pinchable->mapToScene({50, 100}).toPoint();
    QPoint p1 = pinchable->mapToScene({150, 100}).toPoint();
    QTest::QTouchEventSequence touch = QTest::touchEvent(&window, touchDevice.get());

    touch.press(0, p0, &window).press(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    int activeStep = -1;
    int grabTransitionCount = 0;
    // drag two fingers down: PinchHandler moves the item; Flickable doesn't grab, because there are 2 points
    for (int i = 0; i < 4; ++i) {
        p0 += QPoint(0, dragThreshold);
        p1 += QPoint(0, dragThreshold);
        touch.move(0, p0, &window).move(1, p1, &window).commit();
        QQuickTouchUtils::flush(&window);
        if (pinchHandler->active() && activeStep < 0) {
            qCDebug(lcPointerTests) << "pinch began at step" << i;
            activeStep = i;
            QCOMPARE(grabber, pinchHandler);
            grabTransitionCount = grabChangedSpy.count();
        }
    }
    QVERIFY(pinchHandler->active());
    QCOMPARE(grabChangedSpy.count(), grabTransitionCount);
    QCOMPARE(grabber, pinchHandler);
    qreal scale = pinchable->scale();
    QCOMPARE(scale, 1);
    qreal rot = pinchable->rotation();
    QCOMPARE(rot, 0);
    // start expanding and rotating
    for (int i = 0; i < 4; ++i) {
        p0 += QPoint(-5, 10);
        p1 += QPoint(5, -10);
        touch.move(0, p0, &window).move(1, p1, &window).commit();
        QQuickTouchUtils::flush(&window);
        QVERIFY(pinchHandler->active());
        // PinchHandler keeps grab: no more transitions
        QCOMPARE(grabChangedSpy.count(), grabTransitionCount);
        QCOMPARE(grabber, pinchHandler);
        QTRY_COMPARE_GT(pinchable->scale(), scale);
        scale = pinchable->scale();
        QCOMPARE_LT(pinchable->rotation(), rot);
        rot = pinchable->rotation();
    }
    touch.release(0, p0, &window).release(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(pinchHandler->active(), false);
    QCOMPARE(flickMoveSpy.count(), 0); // Flickable never moved
}

void tst_FlickableInterop::nativeGesturePinchOnFlickableWithParentTapHandler_data()
{
    QTest::addColumn<const QPointingDevice*>("device");
    QTest::addColumn<Qt::MouseButton>("button");
    QTest::addColumn<Qt::NativeGestureType>("gesture");
    QTest::addColumn<qreal>("value");
    QTest::addColumn<qreal>("expectedPropertyValue");

    const QPointingDevice *constTouchPad = touchpad.data();

    QTest::newRow("touchpad: left and rotate") << constTouchPad << Qt::LeftButton << Qt::RotateNativeGesture << 5.0 << 10.0;
    QTest::newRow("touchpad: right and rotate") << constTouchPad << Qt::RightButton << Qt::RotateNativeGesture << 5.0 << 10.0;
    QTest::newRow("touchpad: left and scale") << constTouchPad << Qt::LeftButton << Qt::ZoomNativeGesture << 0.1 << 1.21;
    QTest::newRow("touchpad: right and scale") << constTouchPad << Qt::RightButton << Qt::ZoomNativeGesture << 0.1 << 1.21;

    const auto *mouse = QPointingDevice::primaryPointingDevice();
    if (mouse->type() == QInputDevice::DeviceType::Mouse) {
        QTest::newRow("mouse: left and rotate") << mouse << Qt::LeftButton << Qt::RotateNativeGesture << 5.0 << 10.0;
        QTest::newRow("mouse: right and rotate") << mouse << Qt::RightButton << Qt::RotateNativeGesture << 5.0 << 10.0;
        QTest::newRow("mouse: left and scale") << mouse << Qt::LeftButton << Qt::ZoomNativeGesture << 0.1 << 1.21;
        QTest::newRow("mouse: right and scale") << mouse << Qt::RightButton << Qt::ZoomNativeGesture << 0.1 << 1.21;
    } else {
        qCWarning(lcPointerTests) << "skipping mouse tests: primary device is not a mouse" << mouse;
    }
}

void tst_FlickableInterop::nativeGesturePinchOnFlickableWithParentTapHandler()
{
    QFETCH(const QPointingDevice*, device);
    QFETCH(Qt::MouseButton, button);
    QFETCH(Qt::NativeGestureType, gesture);
    QFETCH(qreal, value);
    QFETCH(qreal, expectedPropertyValue);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pinchOnFlickableWithParentTapHandler.qml")));
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QQuickPointerHandler *pinchHandler = flickable->findChild<QQuickPinchHandler*>();
    QVERIFY(pinchHandler);
    QQuickItem *pinchable = pinchHandler->target();
    QVERIFY(pinchable);
    QQuickTapHandler *tapHandler = window.rootObject()->findChild<QQuickTapHandler*>();
    QVERIFY(tapHandler);
    const bool expectTap = button & tapHandler->acceptedButtons();

    QSignalSpy flickMoveSpy(flickable, &QQuickFlickable::movementStarted);
    QSignalSpy grabChangedSpy(touchDevice.get(), &QPointingDevice::grabChanged);
    QSignalSpy tapActiveSpy(tapHandler, &QQuickTapHandler::activeChanged);
    QSignalSpy tapSpy(tapHandler, &QQuickTapHandler::tapped);

    QObject *grabber = nullptr;
    connect(device, &QPointingDevice::grabChanged,
            [&grabber](QObject *g, QPointingDevice::GrabTransition transition, const QPointerEvent *, const QEventPoint &) {
        if (transition == QPointingDevice::GrabTransition::GrabExclusive)
            grabber = g;
    });

    const QPoint pinchPos(75, 75);
    const QPoint outsidePos(200, 200);

    // move to position
    QTest::mouseMove(&window, pinchPos);

    // pinch via native gesture
    ulong ts = 502; // after the mouse move, which is at time 501 in practice
    QWindowSystemInterface::handleGestureEvent(&window, ts++, touchpad.get(),
                                               Qt::BeginNativeGesture, pinchPos, pinchPos);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    for (int i = 0; i < 2; ++i) {
        QWindowSystemInterface::handleGestureEventWithRealValue(&window, ts++, touchpad.get(),
                                                                gesture, value, pinchPos, pinchPos);
    }
    if (gesture == Qt::RotateNativeGesture)
        QTRY_COMPARE(pinchHandler->parentItem()->rotation(), expectedPropertyValue);
    else if (gesture == Qt::ZoomNativeGesture)
        QTRY_COMPARE(pinchHandler->parentItem()->scale(), expectedPropertyValue);
    QVERIFY(pinchHandler->active());
    QCOMPARE(grabChangedSpy.count(), 0);
    QCOMPARE(grabber, nullptr);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QWindowSystemInterface::handleGestureEvent(&window, ts++, touchpad.get(),
                                               Qt::EndNativeGesture, pinchPos, pinchPos);

    // tap in square: TapHandler detects tap iff acceptedButtons permits
    // TODO delay; unfortunately this also begins at timestamp 502 because we don't have testlib
    // functions to send gesture events, and QQuickTest::pointerPress() doesn't take a delay value
    QQuickTest::pointerPress(device, &window, 0, pinchPos, button);
    QCOMPARE(tapHandler->point().id(), expectTap ? 0 : -1);
    QQuickTest::pointerRelease(device, &window, 0, pinchPos, button);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QCOMPARE(tapSpy.size() != 0, expectTap);
    QCOMPARE(tapActiveSpy.size(), 0);
    QCOMPARE(tapHandler->point().id(), -1); // does not keep tracking after release

    // move outside: nothing should happen;
    // but QTBUG-108896 happened because TapHandler was setting pointInfo to track this moving point
    QQuickTest::pointerMove(device, &window, 0, outsidePos);
    QCOMPARE(tapHandler->point().id(), -1); // does not track after mouse move

    // tap outside: nothing happens
    tapSpy.clear();
    tapActiveSpy.clear();
    QQuickTest::pointerPress(device, &window, 0, outsidePos, button);
    QQuickTest::pointerRelease(device, &window, 0, outsidePos, button);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QCOMPARE(tapSpy.size(), 0);
    QCOMPARE(tapActiveSpy.size(), 0);
}

QTEST_MAIN(tst_FlickableInterop)

#include "tst_flickableinterop.moc"

