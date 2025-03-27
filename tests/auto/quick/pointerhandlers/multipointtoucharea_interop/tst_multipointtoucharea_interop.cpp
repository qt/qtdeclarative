// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickmultipointtoucharea_p.h>
#include <QtQuick/private/qquickpinchhandler_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/private/qpointingdevice_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

#include <QtCore/qpointer.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_MptaInterop : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_MptaInterop()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void initTestCase() override;

    void touchDrag();
    void touchesThenPinch();
    void unloadHandlerWithPassiveGrab();
    void dragHandlerInParentStealingGrabFromItem();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
};

void tst_MptaInterop::createView(QScopedPointer<QQuickView> &window, const char *fileName)
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

void tst_MptaInterop::initTestCase()
{
    // This test assumes that we don't get synthesized mouse events from QGuiApplication
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    QQmlDataTest::initTestCase();
}

void tst_MptaInterop::touchDrag()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "pinchDragMPTA.qml");
    QQuickView * window = windowPtr.data();

    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta);
    QQuickPinchHandler *pinch = window->rootObject()->findChild<QQuickPinchHandler*>();
    QVERIFY(pinch);
    QQuickDragHandler *drag = window->rootObject()->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQmlListReference tp(mpta, "touchPoints");
    QVERIFY(tp.at(3)); // the QML declares four touchpoints
    QSignalSpy mptaPressedSpy(mpta, SIGNAL(pressed(QList<QObject*>)));
    QSignalSpy mptaReleasedSpy(mpta, SIGNAL(released(QList<QObject*>)));
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchscreen.get());

    // Press one touchpoint:
    // DragHandler gets a passive grab
    // PinchHandler declines, because it wants 3 touchpoints
    // MPTA grabs because DragHandler doesn't accept the single EventPoint
    QPoint p1 = mpta->mapToScene(QPointF(20, 20)).toPoint();
    touch.press(1, p1).commit();
    QQuickTouchUtils::flush(window);
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    QCOMPARE(tp.at(0)->property("pressed").toBool(), true);
    QTRY_VERIFY(devPriv->pointById(1)->passiveGrabbers.contains(drag));

    // Start moving
    // DragHandler keeps monitoring, due to its passive grab,
    // and eventually steals the exclusive grab from MPTA
    int dragStoleGrab = 0;
    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(dragThreshold / 2, 0);
        touch.move(1, p1).commit();
        QQuickTouchUtils::flush(window);
        if (!dragStoleGrab && devPriv->pointById(1)->exclusiveGrabber == drag)
            dragStoleGrab = i;
    }
    if (dragStoleGrab)
        qCDebug(lcPointerTests, "DragHandler stole the grab after %d events", dragStoleGrab);
    QCOMPARE_GT(dragStoleGrab, 1);

    touch.release(1, p1).commit();
    QQuickTouchUtils::flush(window);
}

// TODO touchesThenPinch_data with press/release sequences somehow: vectors of touchpoint IDs? or a string representation?
void tst_MptaInterop::touchesThenPinch()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "pinchDragMPTA.qml");
    QQuickView * window = windowPtr.data();

    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta);
    QQuickPinchHandler *pinch = window->rootObject()->findChild<QQuickPinchHandler*>();
    QVERIFY(pinch);
    QQuickDragHandler *drag = window->rootObject()->findChild<QQuickDragHandler*>();
    QVERIFY(drag);
    QQmlListReference tp(mpta, "touchPoints");
    QVERIFY(tp.at(3)); // the QML declares four touchpoints
    QSignalSpy mptaPressedSpy(mpta, SIGNAL(pressed(QList<QObject*>)));
    QSignalSpy mptaReleasedSpy(mpta, SIGNAL(released(QList<QObject*>)));
    QSignalSpy mptaCanceledSpy(mpta, SIGNAL(canceled(QList<QObject*>)));
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchscreen.get());
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());

    // Press one touchpoint:
    // DragHandler gets a passive grab
    // PinchHandler declines, because it wants 3 touchpoints
    // MPTA doesn't get a chance, because DragHandler accepted the single EventPoint
    QPoint p1 = mpta->mapToScene(QPointF(20, 20)).toPoint();
    touch.press(1, p1).commit();
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(devPriv->pointById(1)->exclusiveGrabber, mpta);
    QTRY_COMPARE(devPriv->pointById(1)->passiveGrabbers.first(), drag);

    // Press a second touchpoint: MPTA grabs it
    QPoint p2 = mpta->mapToScene(QPointF(200, 30)).toPoint();
    touch.stationary(1).press(2, p2).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(tp.at(0)->property("pressed").toBool());
    QTRY_VERIFY(tp.at(1)->property("pressed").toBool());
    QCOMPARE(mptaPressedSpy.size(), 2);

    // Press a third touchpoint: MPTA grabs it too
    QPoint p3 = mpta->mapToScene(QPointF(110, 200)).toPoint();
    touch.stationary(1).stationary(2).press(3, p3).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(tp.at(0)->property("pressed").toBool(), true);
    QCOMPARE(tp.at(1)->property("pressed").toBool(), true);
    QCOMPARE(tp.at(2)->property("pressed").toBool(), true);
    QCOMPARE(mptaPressedSpy.size(), 3);
    QCOMPARE(mptaCanceledSpy.size(), 0);
    QCOMPARE(devPriv->pointById(1)->exclusiveGrabber, mpta);
    QCOMPARE(devPriv->pointById(2)->exclusiveGrabber, mpta);
    QCOMPARE(devPriv->pointById(3)->exclusiveGrabber, mpta);
    QCOMPARE(pinch->active(), false);

    // Start moving: PinchHandler steals the exclusive grab from MPTA as soon as dragThreshold is exceeded
    int pinchStoleGrab = 0;

    const QPointF c = (p1 + p2 + p3)/3;     // centroid of p1,p2,p3
    QTransform xform;   // transform to rotate around the centroid
    xform.translate(c.x(), c.y()).rotate(1).translate(-c.x(), -c.y());

    for (int i = 0; i < 16; ++i) {
        p1 = xform.map(p1);
        p2 = xform.map(p2);
        p3 = xform.map(p3);
        touch.move(1, p1).move(2, p2).move(3, p3).commit();
        QQuickTouchUtils::flush(window);
        if (!pinchStoleGrab && devPriv->pointById(0)->exclusiveGrabber == pinch) {
            pinchStoleGrab = i;
            QCOMPARE(tp.at(0)->property("pressed").toBool(), false);
            QCOMPARE(tp.at(1)->property("pressed").toBool(), false);
            QCOMPARE(tp.at(2)->property("pressed").toBool(), false);
        }
    }
    qCDebug(lcPointerTests) << "pinch started after" << pinchStoleGrab << "moves; ended with scale" << pinch->activeScale() << "rot" << pinch->rotation();
    QTRY_COMPARE_GT(pinch->rotation(), 4);
    QCOMPARE_GT(pinch->activeScale(), 1);

    // Press one more point (pinkie finger)
    QPoint p4 = mpta->mapToScene(QPointF(300, 200)).toPoint();
    touch.move(1, p1).move(2, p2).move(3, p3).press(4, p4).commit();
    // PinchHandler deactivates, which lets MPTA grab all the points
    QTRY_COMPARE(devPriv->pointById(1)->exclusiveGrabber, mpta);
    QCOMPARE(devPriv->pointById(2)->exclusiveGrabber, mpta);
    QCOMPARE(devPriv->pointById(3)->exclusiveGrabber, mpta);
    QCOMPARE(devPriv->pointById(4)->exclusiveGrabber, mpta);
    // Move some more... MPTA keeps reacting
    for (int i = 0; i < 8; ++i) {
        p1 += QPoint(4, 4);
        p2 += QPoint(4, 4);
        p3 += QPoint(-4, 4);
        p4 += QPoint(-4, -4);
        touch.move(1, p1).move(2, p2).move(3, p3).move(4, p4).commit();
        QCOMPARE(devPriv->pointById(1)->exclusiveGrabber, mpta);
        QCOMPARE(devPriv->pointById(2)->exclusiveGrabber, mpta);
        QCOMPARE(devPriv->pointById(3)->exclusiveGrabber, mpta);
        QCOMPARE(devPriv->pointById(4)->exclusiveGrabber, mpta);
        QCOMPARE(tp.at(0)->property("pressed").toBool(), true);
        QCOMPARE(tp.at(1)->property("pressed").toBool(), true);
        QCOMPARE(tp.at(2)->property("pressed").toBool(), true);
        QCOMPARE(tp.at(3)->property("pressed").toBool(), true);
    }

    // Release the pinkie: PinchHandler acquires passive grabs on the 3 remaining points
    touch.move(1, p1).move(2, p2).move(3, p3).release(4, p4).commit();
    // Move some more: PinchHander grabs again, and reacts
    for (int i = 0; i < 8; ++i) {
        p1 -= QPoint(4, 4);
        p2 += QPoint(4, 4);
        p3 -= QPoint(-4, 4);
        touch.move(1, p1).move(2, p2).move(3, p3).commit();
        QTRY_COMPARE(devPriv->pointById(1)->exclusiveGrabber, pinch);
        QCOMPARE(devPriv->pointById(2)->exclusiveGrabber, pinch);
        QCOMPARE(devPriv->pointById(3)->exclusiveGrabber, pinch);
    }

    // Release the first finger
    touch.stationary(2).stationary(3).release(1, p1).commit();
    // Move some more: PinchHander isn't interested in a mere 2 points.
    // MPTA could maybe react; but QQuickWindowPrivate::deliverTouchEvent() calls
    // deliverPressOrReleaseEvent() in a way which "starts over" with event delivery
    // only for handlers, not for Items; therefore MPTA is not visited at this time.
    for (int i = 0; i < 8; ++i) {
        p2 -= QPoint(4, 4);
        p3 += QPoint(4, 4);
        touch.move(2, p2).move(3, p3).commit();
        QQuickTouchUtils::flush(window);
    }

    // Release another finger
    touch.stationary(2).release(3, p3).commit();
    // Move some more: DragHandler eventually reacts.
    int dragTookGrab = 0;
    for (int i = 0; i < 8; ++i) {
        p2 += QPoint(8, -8);
        touch.move(2, p2).commit();
        QQuickTouchUtils::flush(window);
        QVERIFY(devPriv->pointById(2)->passiveGrabbers.contains(drag));
        if (!dragTookGrab && devPriv->pointById(2)->exclusiveGrabber == drag)
            dragTookGrab = i;
    }
    qCDebug(lcPointerTests) << "drag started after" << dragTookGrab
                            << "moves; ended with translation" << drag->activeTranslation();
    QCOMPARE(devPriv->pointById(2)->exclusiveGrabber, drag);
    QTRY_COMPARE_GT(drag->activeTranslation().x(), 0);

    touch.release(2, p2).commit();
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(mptaReleasedSpy.size(), 1);
}

void tst_MptaInterop::unloadHandlerWithPassiveGrab()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "unloadHandlerOnPress.qml");
    QQuickView * window = windowPtr.data();

    QPointer<QQuickPointerHandler> handler = window->rootObject()->findChild<QQuickPointerHandler*>();
    QVERIFY(handler);
    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta);

    QPoint point(90, 90);
    QTest::mousePress(window, Qt::LeftButton, {}, point);
    QCOMPARE(window->mouseGrabberItem(), mpta);
    QTRY_VERIFY(handler.isNull()); // it got unloaded
    QTest::mouseRelease(window, Qt::LeftButton, {}, point); // QTBUG-73819: don't crash
}

void tst_MptaInterop::dragHandlerInParentStealingGrabFromItem() // QTBUG-75025
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragParentOfMPTA.qml");
    QQuickView * window = windowPtr.data();
    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());

    QPointer<QQuickPointerHandler> handler = window->rootObject()->findChild<QQuickPointerHandler*>();
    QVERIFY(handler);
    QQuickMultiPointTouchArea *mpta = window->rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta);

    // In QTBUG-75025 there is a QQ Controls Button; the MPTA here stands in for that,
    // simply as an Item that grabs the mouse.  The bug has nothing to do with MPTA specifically.

    QPoint point(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(window->mouseGrabberItem(), mpta);
    QCOMPARE(window->rootObject()->property("touchpointPressed").toBool(), true);

    // Start dragging
    // DragHandler keeps monitoring, due to its passive grab,
    // and eventually steals the exclusive grab from MPTA
    int dragStoleGrab = 0;
    for (int i = 0; i < 4; ++i) {
        point += QPoint(dragThreshold / 2, 0);
        QTest::mouseMove(window, point);
        if (!dragStoleGrab && devPriv->firstPointExclusiveGrabber() == handler)
            dragStoleGrab = i;
    }
    if (dragStoleGrab)
        qCDebug(lcPointerTests, "DragHandler stole the grab after %d events", dragStoleGrab);
    QCOMPARE_GT(dragStoleGrab, 1);
    QCOMPARE(handler->active(), true);
    QCOMPARE(window->rootObject()->property("touchpointPressed").toBool(), false);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(handler->active(), false);
    QCOMPARE(window->rootObject()->property("touchpointPressed").toBool(), false);
}

QTEST_MAIN(tst_MptaInterop)

#include "tst_multipointtoucharea_interop.moc"
