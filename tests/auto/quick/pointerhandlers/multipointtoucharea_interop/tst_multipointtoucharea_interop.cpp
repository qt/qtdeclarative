/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickmultipointtoucharea_p.h>
#include <QtQuick/private/qquickpinchhandler_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include "../../../shared/util.h"
#include "../../shared/viewtestutil.h"

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_MptaInterop : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_MptaInterop()
      : touchDevice(QTest::createTouchDevice())
      , touchPointerDevice(QQuickPointerDevice::touchDevice(touchDevice))
    {}

private slots:
    void initTestCase();

    void touchDrag();
    void touchesThenPinch();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QTouchDevice *touchDevice;
    QQuickPointerDevice *touchPointerDevice;
};

void tst_MptaInterop::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtil::centerOnScreen(window.data());
    QQuickViewTestUtil::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != 0);
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
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchDevice);

    // Press one touchpoint:
    // DragHandler gets a passive grab
    // PinchHandler declines, because it wants 3 touchpoints
    // MPTA doesn't get a chance, because DragHandler accepted the single EventPoint
    QPoint p1 = mpta->mapToScene(QPointF(20, 20)).toPoint();
    touch.press(1, p1).commit();
    QQuickTouchUtils::flush(window);
    auto pointerEvent = touchPointerDevice->pointerEvent();
    QCOMPARE(tp.at(0)->property("pressed").toBool(), false);
//    QCOMPARE(pointerEvent->point(0)->exclusiveGrabber(), mpta);

    // Start moving
    // DragHandler gets keeps monitoring, due to its passive grab,
    // and eventually steals the exclusive grab from MPTA
    int dragStoleGrab = 0;
    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(dragThreshold / 2, 0);
        touch.move(1, p1).commit();
        QQuickTouchUtils::flush(window);
        if (!dragStoleGrab && pointerEvent->point(0)->exclusiveGrabber() == drag)
            dragStoleGrab = i;
//        QCOMPARE(tp.at(0)->property("pressed").toBool(), !dragStoleGrab);
    }
    qCDebug(lcPointerTests, "DragHandler stole the grab after %d events", dragStoleGrab);
    QVERIFY(dragStoleGrab > 1);

    touch.release(1, p1).commit();
    QQuickTouchUtils::flush(window);
}

// TODO touchesThenPinch_data with press/release sequences somehow: vectors of touchpoint IDs? or a string representation?
void tst_MptaInterop::touchesThenPinch()
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
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchDevice);
    auto pointerEvent = touchPointerDevice->pointerEvent();
    // TODO after merge of e0c30279ec1fad88346ed3fb483bc3c672fdd01b
//    auto pointerEvent = QQuickWindowPrivate::pointerEventInstance(touchPointerDevice);

    // Press one touchpoint:
    // DragHandler gets a passive grab
    // PinchHandler declines, because it wants 3 touchpoints
    // MPTA doesn't get a chance, because DragHandler accepted the single EventPoint
    QPoint p1 = mpta->mapToScene(QPointF(20, 20)).toPoint();
    touch.press(1, p1).commit();
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(pointerEvent->point(0)->exclusiveGrabber(), nullptr);
    QTRY_COMPARE(pointerEvent->point(0)->passiveGrabbers().first(), drag);
//    QTRY_VERIFY(tp.at(0)->property("pressed").toBool());

    // Press a second touchpoint: MPTA grabs it
    QPoint p2 = mpta->mapToScene(QPointF(200, 30)).toPoint();
    touch.stationary(1).press(2, p2).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(tp.at(0)->property("pressed").toBool());
    QTRY_VERIFY(tp.at(1)->property("pressed").toBool());
    QCOMPARE(mptaPressedSpy.count(), 1);

    // ATM it's required that when PinchHandler sees the third touchpoint,
    // the pre-existing points must have moved far enough to exceed the drag threshold.
    // If MPTA is allowed to grab that third point, then PinchHandler won't steal.
    // TODO should we change that?  make sure that if PH has a passive grab, it always gets updated even though MPTA has the grab?
    for (int i = 0; i < 2; ++i) {
        p1 += QPoint(dragThreshold, dragThreshold);
        p2 += QPoint(dragThreshold, dragThreshold);
        touch.move(1, p1).move(2, p2).commit();
    }

    // Press a third touchpoint: PinchHandler grabs, MPTA doesn't
    QPoint p3 = mpta->mapToScene(QPointF(110, 200)).toPoint();
    touch.stationary(1).stationary(2).press(3, p3).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(tp.at(0)->property("pressed").toBool(), true);
    QCOMPARE(tp.at(1)->property("pressed").toBool(), true);
    QCOMPARE(tp.at(2)->property("pressed").toBool(), false);
    QCOMPARE(mptaPressedSpy.count(), 1);
    QTRY_COMPARE(pointerEvent->point(2)->exclusiveGrabber(), pinch);
    QVERIFY(pinch->active());

    // Move some more: PinchHandler reacts
    for (int i = 0; i < 8; ++i) {
        p1 += QPoint(4, 4);
        p2 += QPoint(4, 4);
        p3 += QPoint(-4, 4);
        touch.move(1, p1).move(2, p2).move(3, p3).commit();
//        QTRY_COMPARE(tp.at(0)->property("pressed").toBool(), false); // TODO fails; MPTA doesn't know it lost its grabs
//        QCOMPARE(tp.at(1)->property("pressed").toBool(), false);
//        QCOMPARE(tp.at(2)->property("pressed").toBool(), false);
    }
    qCDebug(lcPointerTests) << "scale" << pinch->scale() << "rot" << pinch->rotation();
    QTRY_VERIFY(pinch->rotation() > 10);
    QVERIFY(pinch->scale() > 1);

    // Press one more point (pinkie finger)
    QPoint p4 = mpta->mapToScene(QPointF(300, 200)).toPoint();
    touch.stationary(1).stationary(2).stationary(3).press(4, p4).commit();
    // MPTA grabs p4 (which is at index 3)
//    QTRY_COMPARE(pointerEvent->point(3)->exclusiveGrabber(), mpta);
    // PinchHandler wantsPointerEvent declines, because it wants exactly 3 touchpoints, and there are now 4.
    // Move some more... MPTA reacts, in spite of not grabbing all the points
    for (int i = 0; i < 8; ++i) {
        p1 += QPoint(4, 4);
        p2 += QPoint(4, 4);
        p3 += QPoint(-4, 4);
        p4 += QPoint(-4, -4);
        touch.move(1, p1).move(2, p2).move(3, p3).move(4, p4).commit();
//        QTRY_COMPARE(pointerEvent->point(0)->exclusiveGrabber(), nullptr);
//        QCOMPARE(pointerEvent->point(1)->exclusiveGrabber(), nullptr);
//        QCOMPARE(pointerEvent->point(2)->exclusiveGrabber(), nullptr);
//        QCOMPARE(pointerEvent->point(3)->exclusiveGrabber(), mpta);
        QCOMPARE(tp.at(0)->property("pressed").toBool(), true);
        QCOMPARE(tp.at(1)->property("pressed").toBool(), true);
//        QCOMPARE(tp.at(2)->property("pressed").toBool(), true);
//        QCOMPARE(tp.at(3)->property("pressed").toBool(), true);
    }

    // Release the pinkie
    touch.stationary(1).stationary(2).stationary(3).release(4, p4).commit();
    // Move some more: PinchHander grabs again, and reacts
    for (int i = 0; i < 8; ++i) {
        p1 -= QPoint(4, 4);
        p2 += QPoint(4, 4);
        p3 -= QPoint(-4, 4);
        touch.move(1, p1).move(2, p2).move(3, p3).commit();
        QTRY_COMPARE(pointerEvent->point(0)->exclusiveGrabber(), pinch);
    }

    // Release the first finger
    touch.stationary(2).stationary(3).release(1, p1).commit();
    // Move some more: PinchHander isn't interested in a mere 2 points, and MPTA should react... but it doesn't (TODO?)
    for (int i = 0; i < 8; ++i) {
        p1 -= QPoint(4, 4);
        p2 += QPoint(4, 4);
        touch.move(1, p1).move(2, p2).commit();
        QTest::qWait(100);
    }

    touch.release(1, p1).release(2, p2).release(3, p3).commit();
    QQuickTouchUtils::flush(window);
//    QTRY_COMPARE(mptaReleasedSpy.count(), 1); // all points at once
}

QTEST_MAIN(tst_MptaInterop)

#include "tst_multipointtoucharea_interop.moc"
