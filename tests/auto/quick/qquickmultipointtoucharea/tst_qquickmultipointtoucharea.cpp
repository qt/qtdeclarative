// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <private/qquickitem_p.h>
#include <private/qquickmultipointtoucharea_p.h>
#include <private/qquickflickable_p.h>
#include <private/qquickmousearea_p.h>
#include <private/qquickwindow_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtQuick/qquickview.h>
#include <QtGui/QScreen>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

using namespace Qt::StringLiterals;

class tst_QQuickMultiPointTouchArea : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickMultiPointTouchArea() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

private slots:
    void cleanupTestCase() {}

    void properties();
    void signalTest();
    void release();
    void reuse();
    void nonOverlapping();
    void nested();
    void nestedTouchPosCheck();
    void inFlickable();
    void inFlickable2();
    void inFlickableWithPressDelay();
    void inMouseArea();
    void mouseAsTouchpoint();
    void invisible();
    void transformedTouchArea_data();
    void transformedTouchArea();
    void mouseInteraction();
    void mouseInteraction_data();
    void mouseGestureStarted_data();
    void mouseGestureStarted();
    void cancel();
    void stationaryTouchWithChangingPressure();
    void touchFiltering();
    void nestedPinchAreaMouse();
    void disabledIgnoresHover();

private:
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
};

void tst_QQuickMultiPointTouchArea::properties()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("properties.qml")));

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QVERIFY(area != nullptr);

    QCOMPARE(area->minimumTouchPoints(), 2);
    QCOMPARE(area->maximumTouchPoints(), 4);

    QQmlListReference ref(area, "touchPoints");
    QCOMPARE(ref.count(), 4);
}

void tst_QQuickMultiPointTouchArea::signalTest()
{
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QTest::ignoreMessage(QtWarningMsg, QString(testFileUrl("signalTest.qml").toString() +
        u":30:5 Parameter \"touchPoints\" is not declared. Injection of parameters into signal handlers "
          "is deprecated. Use JavaScript functions with formal parameters instead."_s).toLatin1().constData());
#endif

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("signalTest.qml")));

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QVERIFY(area != nullptr);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,100);
    QPoint p4(80,100);
    QPoint p5(100,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).press(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(area->property("touchPointPressCount").toInt(), 2);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 2);
    QMetaObject::invokeMethod(area, "clearCounts");

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 2);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    sequence.move(0, p1).move(1, p2).stationary(2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 3);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    p3 += QPoint(10,10);
    sequence.release(0, p1).release(1, p2)
            .move(2, p3).press(3, p4).press(4, p5).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(area->property("touchPointPressCount").toInt(), 2);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 1);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 2);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    sequence.release(2, p3).release(3, p4).release(4, p5).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 3);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QCOMPARE(area->property("touchUpdatedHandled").toBool(), true);
    QMetaObject::invokeMethod(area, "clearCounts");
}

void tst_QQuickMultiPointTouchArea::release()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("basic.qml")));

    QQuickTouchPoint *point1 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");

    QCOMPARE(point1->pressed(), false);

    QPoint p1(20,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), true);

    p1 += QPoint(0,10);

    sequence.move(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point1->x(), qreal(20)); QCOMPARE(point1->y(), qreal(110));

    p1 += QPoint(4,10);

    sequence.release(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    //test that a release without a prior move to the release position successfully updates the point's position
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point1->x(), qreal(24)); QCOMPARE(point1->y(), qreal(120));
}

void tst_QQuickMultiPointTouchArea::reuse()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("basic.qml")));

    QQuickTouchPoint *point1 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QQuickTouchPoint *point2 = window.rootObject()->findChild<QQuickTouchPoint*>("point2");
    QQuickTouchPoint *point3 = window.rootObject()->findChild<QQuickTouchPoint*>("point3");

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,100);
    QPoint p4(80,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).press(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    sequence.release(0, p1).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    //we shouldn't reuse point 1 yet
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), true);

    //back to base state (no touches)
    sequence.release(1, p2).release(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), false);
    QCOMPARE(point3->pressed(), false);

    sequence.press(0, p1).press(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    sequence.release(0, p1).stationary(1).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    sequence.press(4, p4).stationary(1).commit();
    QQuickTouchUtils::flush(&window);

    //the new touch point should reuse point 1
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), true);
    QCOMPARE(point3->pressed(), false);

    QCOMPARE(point1->x(), qreal(80)); QCOMPARE(point1->y(), qreal(100));
}

void tst_QQuickMultiPointTouchArea::nonOverlapping()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nonOverlapping.qml")));

    QQuickTouchPoint *point11 = window.rootObject()->findChild<QQuickTouchPoint*>("point11");
    QQuickTouchPoint *point12 = window.rootObject()->findChild<QQuickTouchPoint*>("point12");
    QQuickTouchPoint *point21 = window.rootObject()->findChild<QQuickTouchPoint*>("point21");
    QQuickTouchPoint *point22 = window.rootObject()->findChild<QQuickTouchPoint*>("point22");
    QQuickTouchPoint *point23 = window.rootObject()->findChild<QQuickTouchPoint*>("point23");

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,180);
    QPoint p4(80,180);
    QPoint p5(100,180);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.stationary(0).press(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(100));
    QCOMPARE(point12->x(), qreal(40)); QCOMPARE(point12->y(), qreal(100));

    p1 += QPoint(0,10);
    p2 += QPoint(5,0);
    sequence.move(0, p1).move(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.stationary(0).stationary(1).stationary(2).press(3, p4).press(4, p5).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(60)); QCOMPARE(point21->y(), qreal(20));
    QCOMPARE(point22->x(), qreal(80)); QCOMPARE(point22->y(), qreal(20));
    QCOMPARE(point23->x(), qreal(100)); QCOMPARE(point23->y(), qreal(20));

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    p4 += QPoint(1,-1);
    p5 += QPoint(-7,10);
    sequence.move(0, p1).move(1, p2).move(2, p3).move(3, p4).move(4, p5).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point11->x(), qreal(24)); QCOMPARE(point11->y(), qreal(120));
    QCOMPARE(point12->x(), qreal(62)); QCOMPARE(point12->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(63)); QCOMPARE(point21->y(), qreal(20));
    QCOMPARE(point22->x(), qreal(81)); QCOMPARE(point22->y(), qreal(19));
    QCOMPARE(point23->x(), qreal(93)); QCOMPARE(point23->y(), qreal(30));

    sequence.release(0, p1).release(1, p2).release(2, p3).release(3, p4).release(4, p5).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);
}

void tst_QQuickMultiPointTouchArea::nested()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nested.qml")));

    QQuickTouchPoint *point11 = window.rootObject()->findChild<QQuickTouchPoint*>("point11");
    QQuickTouchPoint *point12 = window.rootObject()->findChild<QQuickTouchPoint*>("point12");
    QQuickTouchPoint *point21 = window.rootObject()->findChild<QQuickTouchPoint*>("point21");
    QQuickTouchPoint *point22 = window.rootObject()->findChild<QQuickTouchPoint*>("point22");
    QQuickTouchPoint *point23 = window.rootObject()->findChild<QQuickTouchPoint*>("point23");

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,180);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.stationary(0).press(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(100));
    QCOMPARE(point12->x(), qreal(40)); QCOMPARE(point12->y(), qreal(100));

    p1 += QPoint(0,10);
    p2 += QPoint(5,0);
    sequence.move(0, p1).move(1, p2).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //point11 should be same as point21, point12 same as point22
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.stationary(0).stationary(1).stationary(2).press(3, QPoint(80,180)).press(4, QPoint(100,180)).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //new touch points should be ignored (have no impact on our existing touch points)
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.stationary(0).stationary(1).stationary(2).release(3, QPoint(80,180)).release(4, QPoint(100,180)).commit();

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point23->x(), qreal(63)); QCOMPARE(point23->y(), qreal(180));

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //first two remain the same (touches now grabbed by inner touch area)
    QCOMPARE(point11->x(), qreal(24)); QCOMPARE(point11->y(), qreal(120));
    QCOMPARE(point12->x(), qreal(62)); QCOMPARE(point12->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(28)); QCOMPARE(point21->y(), qreal(130));
    QCOMPARE(point22->x(), qreal(79)); QCOMPARE(point22->y(), qreal(134));
    QCOMPARE(point23->x(), qreal(66)); QCOMPARE(point23->y(), qreal(180));

    sequence.release(0, p1).release(1, p2).release(2, p3).commit();

    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);
    QCOMPARE(point21->pressed(), false);
    QCOMPARE(point22->pressed(), false);
    QCOMPARE(point23->pressed(), false);

    sequence.release(0, p1).commit();
    QQuickTouchUtils::flush(&window);

    //test with grabbing turned off
    window.rootObject()->setProperty("grabInnerArea", false);

    sequence.press(0, p1).press(1, p2).press(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    p1 -= QPoint(4,10);
    p2 -= QPoint(17,17);
    p3 -= QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point23->x(), qreal(63)); QCOMPARE(point23->y(), qreal(180));

    p1 -= QPoint(4,10);
    p2 -= QPoint(17,17);
    p3 -= QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(point21->pressed(), true);
    QCOMPARE(point22->pressed(), true);
    QCOMPARE(point23->pressed(), true);

    //all change (touches not grabbed by inner touch area)
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.release(0, p1).release(1, p2).release(2, p3).commit();
    QQuickTouchUtils::flush(&window);
}


void tst_QQuickMultiPointTouchArea::nestedTouchPosCheck()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedTouchPosCheck.qml")));

    auto *bottomMPTA = window.rootObject()->findChild<QQuickMultiPointTouchArea *>("bottomMPTA");
    QVERIFY(bottomMPTA != nullptr);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, QPoint(10, 110)).commit();
    QQuickTouchUtils::flush(&window);

    sequence.release(0, QPoint(10, 110)).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(bottomMPTA->property("xPressed").toInt(), 10);
    QCOMPARE(bottomMPTA->property("yPressed").toInt(), 10);
    QCOMPARE(bottomMPTA->property("xReleased").toInt(), 10);
    QCOMPARE(bottomMPTA->property("yReleased").toInt(), 10);

}

void tst_QQuickMultiPointTouchArea::inFlickable()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("inFlickable.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable != nullptr);

    QQuickMultiPointTouchArea *mpta = window.rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta != nullptr);

    QQuickTouchPoint *point11 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QQuickTouchPoint *point12 = window.rootObject()->findChild<QQuickTouchPoint*>("point2");

    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);

    // moving one point vertically: flickable gets the grab
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QQuickTouchUtils::flush(&window);

    QPoint delta(0, 15);
    for (int i = 0; i < 4; ++i) {
        if (lcTests().isDebugEnabled())
            QTest::qWait(250);
        p1 += delta;
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1);
        QQuickTouchUtils::flush(&window);
        qCDebug(lcTests, "after drag %d to %d,%d contentY is %lf",
                i, p1.x(), p1.y(), flickable->contentY());
    }

    QVERIFY(flickable->contentY() < 0);
    QCOMPARE(point11->pressed(), false);
    QCOMPARE(point12->pressed(), false);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);
    QQuickTouchUtils::flush(&window);

    QTRY_VERIFY(!flickable->isMoving());

    // moving two points vertically: MPTAs handle them, Flickable ignores multi-touch.
    // The stray mouse events simulate OS-level synth-from-touch, and should not interfere.
    p1 = QPoint(20,100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1).press(1, p2);
    QWindowSystemInterface::handleMouseEvent(&window, touchscreen.get(), p1, window.mapToGlobal(p1),
                                             Qt::LeftButton, Qt::LeftButton, QEvent::MouseButtonPress,
                                             Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    qApp->processEvents();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(window.rootObject()->property("cancelCount").toInt(), 0);
    QCOMPARE(window.rootObject()->property("touchCount").toInt(), 2);

    for (int i = 0; i < 4; ++i) {
        if (lcTests().isDebugEnabled())
            QTest::qWait(250);
        p1 += delta; p2 += delta;
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1).move(1, p2);
        QWindowSystemInterface::handleMouseEvent(&window, touchscreen.get(), p1, window.mapToGlobal(p1),
                                                 Qt::LeftButton, Qt::NoButton, QEvent::MouseMove,
                                                 Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
        qApp->processEvents();
        QQuickTouchUtils::flush(&window);
        qCDebug(lcTests, "after drags %d to %d,%d and %d,%d contentY is %lf",
                i, p1.x(), p1.y(), p2.x(), p2.y(), flickable->contentY());
    }

    QCOMPARE(flickable->contentY(), 0);
    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    QCOMPARE(window.rootObject()->property("cancelCount").toInt(), 0);
    QCOMPARE(window.rootObject()->property("touchCount").toInt(), 2);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1).release(1, p2);
    QWindowSystemInterface::handleMouseEvent(&window, touchscreen.get(), p1, window.mapToGlobal(p1),
                                             Qt::NoButton, Qt::LeftButton, QEvent::MouseButtonRelease,
                                             Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    qApp->processEvents();
    QQuickTouchUtils::flush(&window);

    QTRY_VERIFY(!flickable->isMoving());

    // moving two points horizontally, then two points vertically
    p1 = QPoint(20,100);
    p2 = QPoint(40,100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1).press(1, p2);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);
    // ensure that mouse events do not fall through to the Flickable
    mpta->setMaximumTouchPoints(3);
    mpta->setAcceptedMouseButtons(Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, touchscreen.get(), p1, window.mapToGlobal(p1),
                                             Qt::LeftButton, Qt::LeftButton, QEvent::MouseButtonPress,
                                             Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    qApp->processEvents();

    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);

    delta = QPoint(15, 0);
    for (int i = 0; i < 8; ++i) {
        if (lcTests().isDebugEnabled())
            QTest::qWait(250);
        if (i == 4)
            delta = QPoint(0, 15);
        p1 += delta; p2 += delta;
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1).move(1, p2);
        QWindowSystemInterface::handleMouseEvent(&window, touchscreen.get(), p1, window.mapToGlobal(p1),
                                                 Qt::LeftButton, Qt::NoButton, QEvent::MouseMove,
                                                 Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
        qApp->processEvents();
        QQuickTouchUtils::flush(&window);
        qCDebug(lcTests, "after drags %d to %d,%d and %d,%d contentY is %lf",
                i, p1.x(), p1.y(), p2.x(), p2.y(), flickable->contentY());
    }

    QCOMPARE(flickable->contentY(), qreal(0));
    QCOMPARE(point11->pressed(), true);
    QCOMPARE(point12->pressed(), true);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1).release(1, p2);
    QWindowSystemInterface::handleMouseEvent(&window, touchscreen.get(), p1, window.mapToGlobal(p1),
                                             Qt::NoButton, Qt::LeftButton, QEvent::MouseButtonRelease,
                                             Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    qApp->processEvents();
    QQuickTouchUtils::flush(&window);
}

// test that dragging out of a Flickable containing a MPTA doesn't harm Flickable's state.
void tst_QQuickMultiPointTouchArea::inFlickable2()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("inFlickable2.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable != nullptr);

    QQuickMultiPointTouchArea *mpta = window.rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta);

    QQuickTouchPoint *point11 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QVERIFY(point11);

    QCOMPARE(point11->pressed(), false);

    QPoint p1(50,100);

    // move point horizontally, out of Flickable area
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QQuickTouchUtils::flush(&window);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);

    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(dragThreshold, 0);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1);
        QQuickTouchUtils::flush(&window);
        QTest::mouseMove(&window, p1);
    }

    QVERIFY(!flickable->isMoving());
    QVERIFY(point11->pressed());

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);
    QQuickTouchUtils::flush(&window);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QTest::qWait(50);

    QTRY_VERIFY(!flickable->isMoving());

    // Check that we can still move the Flickable
    QSignalSpy gestureStartedSpy(mpta, &QQuickMultiPointTouchArea::gestureStarted);
    p1 = QPoint(50,100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point11->pressed(), true);

    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(0, dragThreshold);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1);
        QQuickTouchUtils::flush(&window);
        // QTBUG-113653: gestureStarted is emitted when touch delta exceeds drag threshold,
        // regardless of the filtering Flickable parent
        QCOMPARE(gestureStartedSpy.size(), i > 0 ? 1 : 0);
    }

    QVERIFY(flickable->contentY() < 0);
    QVERIFY(flickable->isMoving());
    QCOMPARE(point11->pressed(), false);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);
    QQuickTouchUtils::flush(&window);
    QTest::qWait(50);

    QTRY_VERIFY(!flickable->isMoving());
}

void tst_QQuickMultiPointTouchArea::inFlickableWithPressDelay() // QTBUG-78818
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("inFlickable.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable != nullptr);
    flickable->setPressDelay(50);

    QQuickMultiPointTouchArea *mpta = window.rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta != nullptr);
    mpta->setMouseEnabled(false); // don't depend on synth-mouse
    mpta->setMinimumTouchPoints(1);
    QQuickTouchPoint *point11 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QPoint p1(20,100);

    // press: Flickable prevents delivery of TouchBegin, but sends mouse press instead, after the delay.
    // MPTA handles the mouse press, and its first declared touchpoint is pressed.
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(point11->pressed(), true);
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, mpta);

    // release: MPTA receives TouchEnd (which is asymmetric with mouse press); does NOT emit canceled.
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(flickable->property("cancelCount").toInt(), 0);

    // press again
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(point11->pressed(), true); // wait until pressDelay exceeded
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, mpta);

    // drag past the threshold: Flickable takes over the grab,
    // MPTA gets touchUngrab and is no longer pressed,
    // and Flickable starts moving
    int i = 0;
    int grabbedAfter = -1;
    int movedAfter = -1;
    int contentYChangedAfter = -1;
    for (; i < 10 && contentYChangedAfter < 0; ++i) {
        p1 += QPoint(0,dragThreshold);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1);
        QQuickTouchUtils::flush(&window);
        if (devPriv->firstPointExclusiveGrabber() == flickable)
            grabbedAfter = i;
        if (flickable->isMoving())
            movedAfter = i;
        if (!qFuzzyIsNull(flickable->contentY()))
            contentYChangedAfter = i;
    }
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), flickable);
    int cancelCount = window.rootObject()->property("cancelCount").toInt();
    qCDebug(lcTests, "Flickable stole grab from MPTA after %d moves, started moving after %d, and moved to contentY %lf after %d moves; got %d cancel(s)",
            grabbedAfter, movedAfter, flickable->contentY(), contentYChangedAfter, cancelCount);
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, flickable);
    QCOMPARE(point11->pressed(), false);
    QVERIFY(cancelCount > 0); // 2 touchPoints are declared but only one was really cancelled
    QVERIFY(flickable->contentY() < 0);
    QVERIFY(flickable->isMoving());

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);
    QQuickTouchUtils::flush(&window);

    QTRY_VERIFY(!flickable->isMoving());
}

// QTBUG-31047
void tst_QQuickMultiPointTouchArea::inMouseArea()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("inMouseArea.qml")));

    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(window.rootObject());
    QVERIFY(mouseArea != nullptr);

    QQuickMultiPointTouchArea *mpta = window.rootObject()->findChild<QQuickMultiPointTouchArea*>("mpta");
    QVERIFY(mpta != nullptr);

    QPoint innerPoint(40,100);
    QPoint outerPoint(10,100);

    QTest::touchEvent(&window, touchscreen.get()).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QTest::touchEvent(&window, touchscreen.get()).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::touchEvent(&window, touchscreen.get()).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QTest::touchEvent(&window, touchscreen.get()).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());

    QTest::touchEvent(&window, touchscreen.get()).press(0, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
    QTest::touchEvent(&window, touchscreen.get()).release(0, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());

    // Right click should pass through
    QTest::mousePress(&window, Qt::RightButton, Qt::NoModifier, innerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
    QTest::mouseRelease(&window, Qt::RightButton, Qt::NoModifier, innerPoint);

    mpta->setProperty("mouseEnabled", false);

    QTest::touchEvent(&window, touchscreen.get()).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());
    QTest::touchEvent(&window, touchscreen.get()).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(mouseArea->property("pressed").toBool());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());
    QVERIFY(!mouseArea->property("pressed").toBool());

    QTest::touchEvent(&window, touchscreen.get()).press(0, innerPoint);
    QVERIFY(mpta->property("pressed").toBool());
    QTest::touchEvent(&window, touchscreen.get()).release(0, innerPoint);
    QVERIFY(!mpta->property("pressed").toBool());

    QTest::touchEvent(&window, touchscreen.get()).press(0, outerPoint);
    QVERIFY(mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
    QTest::touchEvent(&window, touchscreen.get()).release(0, outerPoint);
    QVERIFY(!mouseArea->property("pressed").toBool());
    QVERIFY(!mpta->property("pressed").toBool());
}

void tst_QQuickMultiPointTouchArea::mouseAsTouchpoint()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dualGestures.qml")));

    QQuickMultiPointTouchArea *dualmpta = window.rootObject()->findChild<QQuickMultiPointTouchArea*>("dualTouchArea");
    QVERIFY(dualmpta != nullptr);

    QQuickItem *touch1rect = window.rootObject()->findChild<QQuickItem*>("touch1rect");
    QQuickItem *touch2rect = window.rootObject()->findChild<QQuickItem*>("touch2rect");
    QQuickItem *touch3rect = window.rootObject()->findChild<QQuickItem*>("touch3rect");
    QQuickItem *touch4rect = window.rootObject()->findChild<QQuickItem*>("touch4rect");
    QQuickItem *touch5rect = window.rootObject()->findChild<QQuickItem*>("touch5rect");

    {
        QPoint touch1(40,10);
        QPoint touch2(40,100);
        QPoint touch3(10,10);

        // Touch both, release one, manipulate other touchpoint with mouse
        QTest::touchEvent(&window, touchscreen.get()).press(1, touch1);
        QQuickTouchUtils::flush(&window);
        QTest::touchEvent(&window, touchscreen.get()).move(1, touch1).press(2, touch2);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
        QTest::touchEvent(&window, touchscreen.get()).release(1, touch1).move(2, touch2);
        touch1.setY(20);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, touch1);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
        QTest::touchEvent(&window, touchscreen.get()).release(2, touch2);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, touch1);
        QQuickTouchUtils::flush(&window);

        // Start with mouse, move it, touch second point, move it
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, touch1);
        touch1.setX(60);
        QTest::mouseMove(&window, touch1);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        touch2.setX(60);
        QTest::touchEvent(&window, touchscreen.get()).press(3, touch2);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
        touch2.setY(150);
        QTest::touchEvent(&window, touchscreen.get()).move(3, touch2);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());

        // Touch third point - nothing happens
        QTest::touchEvent(&window, touchscreen.get()).press(4, touch3);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());

        // Release all
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, touch1);
        QTest::touchEvent(&window, touchscreen.get()).release(3, touch2);
        QQuickTouchUtils::flush(&window);
        QTest::touchEvent(&window, touchscreen.get()).release(4, touch3);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
    }

    // Mouse and touch on left, touch 3 points on right
    {
        QPoint mouse1(40,10);
        QPoint touch1(10,10);
        QPoint touch2(340,10);
        QPoint touch3(340,100);
        QPoint touch4(540,10);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, mouse1);
        QCOMPARE(touch1rect->property("x").toInt(), mouse1.x());
        QCOMPARE(touch1rect->property("y").toInt(), mouse1.y());
        QTest::touchEvent(&window, touchscreen.get()).press(1, touch1);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), mouse1.x());
        QCOMPARE(touch1rect->property("y").toInt(), mouse1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch1.y());

        QTest::touchEvent(&window, touchscreen.get()).press(2, touch2).press(3, touch3).press(4, touch4);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), mouse1.x());
        QCOMPARE(touch1rect->property("y").toInt(), mouse1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch3rect->property("x").toInt(), touch2.x() - 320);
        QCOMPARE(touch3rect->property("y").toInt(), touch2.y());
        QCOMPARE(touch4rect->property("x").toInt(), touch3.x() - 320);
        QCOMPARE(touch4rect->property("y").toInt(), touch3.y());
        QCOMPARE(touch5rect->property("x").toInt(), touch4.x() - 320);
        QCOMPARE(touch5rect->property("y").toInt(), touch4.y());

        // Release all
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, mouse1);
        QTest::touchEvent(&window, touchscreen.get()).release(1, touch1).release(2, touch2).release(3, touch3).release(4, touch4);
        QQuickTouchUtils::flush(&window);
    }

    dualmpta->setProperty("mouseEnabled", false);
    {
        QPoint mouse1(40,10);
        QPoint touch1(10,10);
        QPoint touch2(100,10);

        // do not break the QML bindings
        auto t1priv = QQuickItemPrivate::get(touch1rect);
        auto t2priv = QQuickItemPrivate::get(touch2rect);
        t1priv->x.setValueBypassingBindings(10);
        t1priv->y.setValueBypassingBindings(10);
        t2priv->x.setValueBypassingBindings(20);
        t2priv->y.setValueBypassingBindings(10);

        // Start with mouse, move it, touch a point, move it, touch another.
        // Mouse is ignored, both touch points are heeded.
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, mouse1);
        mouse1.setX(60);
        QTest::mouseMove(&window, mouse1);
        QCOMPARE(touch1rect->property("x").toInt(), 10);
        QCOMPARE(touch1rect->property("y").toInt(), 10);

        QTest::touchEvent(&window, touchscreen.get()).press(1, touch1);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        touch1.setY(150);
        QTest::touchEvent(&window, touchscreen.get()).move(1, touch1);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QTest::touchEvent(&window, touchscreen.get()).press(2, touch2);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());

        // Release all
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, mouse1);
        QTest::touchEvent(&window, touchscreen.get()).release(1, touch1);
        QQuickTouchUtils::flush(&window);
        QTest::touchEvent(&window, touchscreen.get()).release(2, touch2);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(touch1rect->property("x").toInt(), touch1.x());
        QCOMPARE(touch1rect->property("y").toInt(), touch1.y());
        QCOMPARE(touch2rect->property("x").toInt(), touch2.x());
        QCOMPARE(touch2rect->property("y").toInt(), touch2.y());
    }
}

// QTBUG-23327
void tst_QQuickMultiPointTouchArea::invisible()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("signalTest.qml")));

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QVERIFY(area != nullptr);

    area->setVisible(false);

    QPoint p1(20,100);
    QPoint p2(40,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).press(1, p2).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 0);
}

void tst_QQuickMultiPointTouchArea::transformedTouchArea_data()
{
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<QPoint>("p2");
    QTest::addColumn<QPoint>("p3");
    QTest::addColumn<int>("total1");
    QTest::addColumn<int>("total2");
    QTest::addColumn<int>("total3");

    QTest::newRow("1st point inside")
        << QPoint(140, 200) << QPoint(260, 260) << QPoint(0, 140) << 1 << 1 << 1;

    QTest::newRow("2nd point inside")
        << QPoint(260, 260) << QPoint(200, 200) << QPoint(0, 0) << 0 << 1 << 1;

    QTest::newRow("3rd point inside")
        << QPoint(140, 260) << QPoint(260, 140) << QPoint(200, 140) << 0 << 0 << 1;
    QTest::newRow("all points inside")
        << QPoint(200, 140) << QPoint(200, 260) << QPoint(140, 200) << 1 << 2 << 3;

    QTest::newRow("all points outside")
        << QPoint(140, 140) << QPoint(260, 260) << QPoint(260, 140) << 0 << 0 << 0;

    QTest::newRow("1st and 2nd points inside")
        << QPoint(200, 260) << QPoint(200, 140) << QPoint(140, 140) << 1 << 2 << 2;

    QTest::newRow("1st and 3rd points inside")
        << QPoint(200, 200) << QPoint(0, 0) << QPoint(200, 260) << 1 << 1 << 2;
}

void tst_QQuickMultiPointTouchArea::transformedTouchArea()
{
    QFETCH(QPoint, p1);
    QFETCH(QPoint, p2);
    QFETCH(QPoint, p3);
    QFETCH(int, total1);
    QFETCH(int, total2);
    QFETCH(int, total3);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("transformedMultiPointTouchArea.qml")));

    QQuickMultiPointTouchArea *area = window.rootObject()->findChild<QQuickMultiPointTouchArea *>("touchArea");
    QVERIFY(area != nullptr);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());

    sequence.press(0, p1).commit();
    QCOMPARE(area->property("pointCount").toInt(), total1);

    sequence.stationary(0).press(1, p2).commit();
    QCOMPARE(area->property("pointCount").toInt(), total2);

    sequence.stationary(0).stationary(1).press(2, p3).commit();
    QCOMPARE(area->property("pointCount").toInt(), total3);
}

void tst_QQuickMultiPointTouchArea::mouseInteraction_data()
{
    QTest::addColumn<int>("buttons");
    QTest::addColumn<int>("accept");

    QTest::newRow("left") << (int) Qt::LeftButton << 1;
    QTest::newRow("right") << (int) Qt::RightButton << 0;
    QTest::newRow("middle") << (int) Qt::MiddleButton << 0;
}

void tst_QQuickMultiPointTouchArea::mouseInteraction()
{
    QFETCH(int, buttons);
    QFETCH(int, accept);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("mouse.qml")));

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QVERIFY(area != nullptr);
    QQuickTouchPoint *point1 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);

    QCOMPARE(area->property("touchCount").toInt(), 0);
    QPoint p1 = QPoint(100, 100);
    QTest::mousePress(&window, (Qt::MouseButton) buttons, Qt::NoModifier, p1);
    QCOMPARE(area->property("touchCount").toInt(), accept);
    QCOMPARE(point1->pressed(), accept != 0);
    p1 += QPoint(10, 10);
    QTest::mouseMove(&window, p1);
    QCOMPARE(point1->pressed(), accept != 0);
    QCOMPARE(area->property("touchCount").toInt(), accept);
    p1 += QPoint(10, 10);
    QTest::mouseMove(&window, p1);
    QCOMPARE(point1->pressed(), accept != 0);
    QCOMPARE(area->property("touchCount").toInt(), accept);
    QTest::mouseRelease(&window, (Qt::MouseButton) buttons);
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(area->property("touchCount").toInt(), 0);
}

void tst_QQuickMultiPointTouchArea::mouseGestureStarted_data()
{
    QTest::addColumn<bool>("grabGesture");
    QTest::addColumn<int>("distanceFromOrigin");

    QTest::newRow("near origin, don't grab") << false << 4;
    QTest::newRow("near origin, grab") << true << 4;
    QTest::newRow("away from origin, don't grab") << false << 100;
    QTest::newRow("away from origin, grab") << true << 100;
}

void tst_QQuickMultiPointTouchArea::mouseGestureStarted() // QTBUG-70258
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QFETCH(bool, grabGesture);
    QFETCH(int, distanceFromOrigin);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("mouse.qml")));

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QVERIFY(area);
    area->setProperty("grabGesture", grabGesture);
    QQuickTouchPoint *point1 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);
    QSignalSpy gestureStartedSpy(area, SIGNAL(gestureStarted(QQuickGrabGestureEvent*)));

    QPoint p1 = QPoint(distanceFromOrigin, distanceFromOrigin);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(gestureStartedSpy.size(), 0);

    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::mouseMove(&window, p1);
    QCOMPARE(gestureStartedSpy.size(), 0);

    p1 += QPoint(1, 1);
    QTest::mouseMove(&window, p1);
    QTRY_COMPARE(gestureStartedSpy.size(), 1);
    QTRY_COMPARE(area->property("gestureStartedX").toInt(), distanceFromOrigin);
    QCOMPARE(area->property("gestureStartedY").toInt(), distanceFromOrigin);

    p1 += QPoint(10, 10);
    QTest::mouseMove(&window, p1);
    // if nobody called gesteure->grab(), gestureStarted will keep happening
    QTRY_COMPARE(gestureStartedSpy.size(), grabGesture ? 1 : 2);
    QCOMPARE(area->property("gestureStartedX").toInt(), distanceFromOrigin);
    QCOMPARE(area->property("gestureStartedY").toInt(), distanceFromOrigin);

    QTest::mouseRelease(&window, Qt::LeftButton);
}

void tst_QQuickMultiPointTouchArea::cancel()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("cancel.qml")));

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QTest::QTouchEventSequence sequence = QTest::touchEvent(&window, touchscreen.get());
    QQuickTouchPoint *point1 = area->findChild<QQuickTouchPoint*>("point1");

    QPoint p1(20,100);
    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 1);
    QMetaObject::invokeMethod(area, "clearCounts");

    area->setVisible(false);
    // we should get a onCancel signal
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 1);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QMetaObject::invokeMethod(area, "clearCounts");
    area->setVisible(true);


    sequence.press(0, p1).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 1);
    QMetaObject::invokeMethod(area, "clearCounts");

    area->setEnabled(false);
    // we should get a onCancel signal
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchPointCancelCount").toInt(), 1);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QMetaObject::invokeMethod(area, "clearCounts");

}

void tst_QQuickMultiPointTouchArea::stationaryTouchWithChangingPressure() // QTBUG-77142
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("basic.qml")));

    QQuickTouchPoint *point1 = window.rootObject()->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);

    QPoint p1(20,100);
    QEventPoint tp1(1);

    QMutableEventPoint::setGlobalPosition(tp1, window.mapToGlobal(p1));
    QMutableEventPoint::setState(tp1, QEventPoint::State::Pressed);
    QMutableEventPoint::setPressure(tp1, 0.5);
    qt_handleTouchEvent(&window, touchscreen.get(), {tp1});
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point1->pressure(), 0.5);

    QMutableEventPoint::setState(tp1, QEventPoint::State::Stationary);
    QMutableEventPoint::setPressure(tp1, 0.6);
    qt_handleTouchEvent(&window, touchscreen.get(), {tp1});
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressure(), 0.6);

    QMutableEventPoint::setState(tp1, QEventPoint::State::Released);
    QMutableEventPoint::setPressure(tp1, 0);
    qt_handleTouchEvent(&window, touchscreen.get(), {tp1});
    QQuickTouchUtils::flush(&window);

    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point1->pressure(), 0);
}

void tst_QQuickMultiPointTouchArea::touchFiltering() // QTBUG-74028
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedMouseArea.qml")));
    QQuickMultiPointTouchArea *mpta = window.rootObject()->findChild<QQuickMultiPointTouchArea*>();
    QVERIFY(mpta);
    QQuickMouseArea *ma = window.rootObject()->findChild<QQuickMouseArea*>();
    QVERIFY(ma);

    QSignalSpy mptaSpy(mpta, &QQuickMultiPointTouchArea::pressed);
    const QPoint pt = window.rootObject()->boundingRect().center().toPoint();
    QTest::touchEvent(&window, touchscreen.get()).press(1, pt);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(mpta->parentItem()->property("mptaPoint").toPoint(), pt);
    QCOMPARE(mpta->parentItem()->property("maPoint").toPoint(), ma->boundingRect().center().toPoint());
    QCOMPARE(mptaSpy.size(), 1);
}

void tst_QQuickMultiPointTouchArea::nestedPinchAreaMouse() // QTBUG-83662
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedPinchArea.qml")));
    QQuickMultiPointTouchArea *mpta = qobject_cast<QQuickMultiPointTouchArea *>(window.rootObject());
    QVERIFY(mpta);

    QQuickTouchPoint *point1 = mpta->findChild<QQuickTouchPoint*>("point1");
    QCOMPARE(point1->pressed(), false);
    QQuickTouchPoint *point2 = mpta->findChild<QQuickTouchPoint*>("point2");
    QCOMPARE(point2->pressed(), false);
    QSignalSpy pressedSpy(mpta, &QQuickMultiPointTouchArea::pressed);
    QSignalSpy updatedSpy(mpta, &QQuickMultiPointTouchArea::updated);
    QSignalSpy releasedSpy(mpta, &QQuickMultiPointTouchArea::released);

    QPoint p1(20, 20);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), false);
    QCOMPARE(pressedSpy.size(), 1);
    QCOMPARE(mpta->property("pressedCount").toInt(), 1);
    QCOMPARE(updatedSpy.size(), 0);
    QCOMPARE(mpta->property("updatedCount").toInt(), 0);
    QCOMPARE(releasedSpy.size(), 0);
    QCOMPARE(mpta->property("releasedCount").toInt(), 0);

    p1 += QPoint(0, 15);
    QTest::mouseMove(&window, p1);
    QCOMPARE(point1->pressed(), true);
    QCOMPARE(point2->pressed(), false);
    QCOMPARE(pressedSpy.size(), 1);
    QCOMPARE(mpta->property("pressedCount").toInt(), 1);
    QCOMPARE(updatedSpy.size(), 1);
    QCOMPARE(mpta->property("updatedCount").toInt(), 1);
    QCOMPARE(releasedSpy.size(), 0);
    QCOMPARE(mpta->property("releasedCount").toInt(), 0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(point1->pressed(), false);
    QCOMPARE(point2->pressed(), false);
    QCOMPARE(pressedSpy.size(), 1);
    QCOMPARE(mpta->property("pressedCount").toInt(), 1);
    QCOMPARE(updatedSpy.size(), 1);
    QCOMPARE(mpta->property("updatedCount").toInt(), 1);
    QCOMPARE(releasedSpy.size(), 1);
    QCOMPARE(mpta->property("releasedCount").toInt(), 1);
}

/*
    A disabled MultiPointTouchArea should not interfere with hover event
    propagation to siblings underneath.
*/
void tst_QQuickMultiPointTouchArea::disabledIgnoresHover()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("touchOverMouseArea.qml")));
    QQuickItem *root = qobject_cast<QQuickItem *>(window.rootObject());
    QVERIFY(root);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea *>();

    QTest::mouseMove(&window, QPoint(40, 40));
    QTest::mouseMove(&window, QPoint(50, 50));
    QVERIFY(mouseArea->hovered());
}

QTEST_MAIN(tst_QQuickMultiPointTouchArea)

#include "tst_qquickmultipointtoucharea.moc"
