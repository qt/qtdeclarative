/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <private/qquickmultipointtoucharea_p.h>
#include <private/qquickflickable_p.h>
#include <QtQuick/qquickview.h>

class tst_QQuickMultiPointTouchArea: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void properties();
    void signalTest();
    void nonOverlapping();
    void nested();
    void inFlickable();

private:
    QQuickView *createAndShowView(const QString &file);
};

void tst_QQuickMultiPointTouchArea::properties()
{
    QQuickView *canvas = createAndShowView("properties.qml");
    QVERIFY(canvas->rootObject() != 0);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(canvas->rootObject());
    QVERIFY(area != 0);

    QCOMPARE(area->minimumTouchPoints(), 2);
    QCOMPARE(area->maximumTouchPoints(), 4);

    QDeclarativeListReference ref(area, "touchPoints");
    QCOMPARE(ref.count(), 4);

    delete canvas;
}

void tst_QQuickMultiPointTouchArea::signalTest()
{
    QQuickView *canvas = createAndShowView("signalTest.qml");
    QVERIFY(canvas->rootObject() != 0);

    QQuickMultiPointTouchArea *area = qobject_cast<QQuickMultiPointTouchArea *>(canvas->rootObject());
    QVERIFY(area != 0);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,100);
    QPoint p4(80,100);
    QPoint p5(100,100);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(canvas);

    sequence.press(0, p1).press(1, p2).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 2);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 2);
    QMetaObject::invokeMethod(area, "clearCounts");

    sequence.stationary(0).stationary(1).press(2, p3).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 1);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    sequence.move(0, p1).move(1, p2).stationary(2).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 2);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 0);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    p3 += QPoint(10,10);
    sequence.release(0, p1).release(1, p2)
            .move(2, p3).press(3, p4).press(4, p5).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 2);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 1);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 2);
    QCOMPARE(area->property("touchCount").toInt(), 3);
    QMetaObject::invokeMethod(area, "clearCounts");

    sequence.release(2, p3).release(3, p4).release(4, p5).commit();

    QCOMPARE(area->property("touchPointPressCount").toInt(), 0);
    QCOMPARE(area->property("touchPointUpdateCount").toInt(), 0);
    QCOMPARE(area->property("touchPointReleaseCount").toInt(), 3);
    QCOMPARE(area->property("touchCount").toInt(), 0);
    QMetaObject::invokeMethod(area, "clearCounts");

    delete canvas;
}

void tst_QQuickMultiPointTouchArea::nonOverlapping()
{
    QQuickView *canvas = createAndShowView("nonOverlapping.qml");
    QVERIFY(canvas->rootObject() != 0);

    QQuickTouchPoint *point11 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point11");
    QQuickTouchPoint *point12 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point12");
    QQuickTouchPoint *point21 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point21");
    QQuickTouchPoint *point22 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point22");
    QQuickTouchPoint *point23 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point23");

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,180);
    QPoint p4(80,180);
    QPoint p5(100,180);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(canvas);

    sequence.press(0, p1).commit();

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    sequence.stationary(0).press(1, p2).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(100));
    QCOMPARE(point12->x(), qreal(40)); QCOMPARE(point12->y(), qreal(100));

    p1 += QPoint(0,10);
    p2 += QPoint(5,0);
    sequence.move(0, p1).move(1, p2).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));

    sequence.stationary(0).stationary(1).press(2, p3).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    sequence.stationary(0).stationary(1).stationary(2).press(3, p4).press(4, p5).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

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

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    QCOMPARE(point11->x(), qreal(24)); QCOMPARE(point11->y(), qreal(120));
    QCOMPARE(point12->x(), qreal(62)); QCOMPARE(point12->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(63)); QCOMPARE(point21->y(), qreal(20));
    QCOMPARE(point22->x(), qreal(81)); QCOMPARE(point22->y(), qreal(19));
    QCOMPARE(point23->x(), qreal(93)); QCOMPARE(point23->y(), qreal(30));

    sequence.release(0, p1).release(1, p2).release(2, p3).release(3, p4).release(4, p5).commit();

    //points remain valid immediately after release
    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    delete canvas;
}

void tst_QQuickMultiPointTouchArea::nested()
{
    QQuickView *canvas = createAndShowView("nested.qml");
    QVERIFY(canvas->rootObject() != 0);

    QQuickTouchPoint *point11 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point11");
    QQuickTouchPoint *point12 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point12");
    QQuickTouchPoint *point21 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point21");
    QQuickTouchPoint *point22 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point22");
    QQuickTouchPoint *point23 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point23");

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);
    QPoint p3(60,180);

    QTest::QTouchEventSequence sequence = QTest::touchEvent(canvas);

    sequence.press(0, p1).commit();

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    sequence.stationary(0).press(1, p2).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(100));
    QCOMPARE(point12->x(), qreal(40)); QCOMPARE(point12->y(), qreal(100));

    p1 += QPoint(0,10);
    p2 += QPoint(5,0);
    sequence.move(0, p1).move(1, p2).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));

    sequence.stationary(0).stationary(1).press(2, p3).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    //point11 should be same as point21, point12 same as point22
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.stationary(0).stationary(1).stationary(2).press(3, QPoint(80,180)).press(4, QPoint(100,180)).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

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

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point23->x(), qreal(63)); QCOMPARE(point23->y(), qreal(180));

    p1 += QPoint(4,10);
    p2 += QPoint(17,17);
    p3 += QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    //first two remain the same (touches now grabbed by inner touch area)
    QCOMPARE(point11->x(), qreal(24)); QCOMPARE(point11->y(), qreal(120));
    QCOMPARE(point12->x(), qreal(62)); QCOMPARE(point12->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(28)); QCOMPARE(point21->y(), qreal(130));
    QCOMPARE(point22->x(), qreal(79)); QCOMPARE(point22->y(), qreal(134));
    QCOMPARE(point23->x(), qreal(66)); QCOMPARE(point23->y(), qreal(180));

    sequence.release(0, p1).release(1, p2).release(2, p3).commit();

    sequence.press(0, p1).commit();

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);
    QCOMPARE(point21->isValid(), false);
    QCOMPARE(point22->isValid(), false);
    QCOMPARE(point23->isValid(), false);

    sequence.release(0, p1).commit();

    //test with grabbing turned off
    canvas->rootObject()->setProperty("grabInnerArea", false);

    sequence.press(0, p1).press(1, p2).press(2, p3).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    p1 -= QPoint(4,10);
    p2 -= QPoint(17,17);
    p3 -= QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point21->x(), qreal(24)); QCOMPARE(point21->y(), qreal(120));
    QCOMPARE(point22->x(), qreal(62)); QCOMPARE(point22->y(), qreal(117));
    QCOMPARE(point23->x(), qreal(63)); QCOMPARE(point23->y(), qreal(180));

    p1 -= QPoint(4,10);
    p2 -= QPoint(17,17);
    p3 -= QPoint(3,0);
    sequence.move(0, p1).move(1, p2).move(2, p3).commit();

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);
    QCOMPARE(point21->isValid(), true);
    QCOMPARE(point22->isValid(), true);
    QCOMPARE(point23->isValid(), true);

    //all change (touches not grabbed by inner touch area)
    QCOMPARE(point11->x(), qreal(20)); QCOMPARE(point11->y(), qreal(110));
    QCOMPARE(point12->x(), qreal(45)); QCOMPARE(point12->y(), qreal(100));
    QCOMPARE(point21->x(), qreal(20)); QCOMPARE(point21->y(), qreal(110));
    QCOMPARE(point22->x(), qreal(45)); QCOMPARE(point22->y(), qreal(100));
    QCOMPARE(point23->x(), qreal(60)); QCOMPARE(point23->y(), qreal(180));

    sequence.release(0, p1).release(1, p2).release(2, p3).commit();

    delete canvas;
}

void tst_QQuickMultiPointTouchArea::inFlickable()
{
    QQuickView *canvas = createAndShowView("inFlickable.qml");
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(canvas->rootObject());
    QVERIFY(flickable != 0);

    QQuickTouchPoint *point11 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point1");
    QQuickTouchPoint *point12 = canvas->rootObject()->findChild<QQuickTouchPoint*>("point2");

    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);

    QPoint p1(20,100);
    QPoint p2(40,100);

    //moving one point vertically
    QTest::touchEvent(canvas).press(0, p1);
    QTest::mousePress(canvas, Qt::LeftButton, 0, p1);

    p1 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1);
    QTest::mouseMove(canvas, p1);

    QVERIFY(flickable->contentY() < 0);
    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);

    QTest::touchEvent(canvas).release(0, p1);
    QTest::mouseRelease(canvas,Qt::LeftButton, 0, p1);
    QTest::qWait(50);

    QTRY_VERIFY(!flickable->isMoving());

    //moving two points vertically
    p1 = QPoint(20,100);
    QTest::touchEvent(canvas).press(0, p1).press(1, p2);
    QTest::mousePress(canvas, Qt::LeftButton, 0, p1);

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    QVERIFY(flickable->contentY() < 0);
    QCOMPARE(point11->isValid(), false);
    QCOMPARE(point12->isValid(), false);

    QTest::touchEvent(canvas).release(0, p1).release(1, p2);
    QTest::mouseRelease(canvas,Qt::LeftButton, 0, p1);
    QTest::qWait(50);

    QTRY_VERIFY(!flickable->isMoving());

    //moving two points horizontally, then one point vertically
    p1 = QPoint(20,100);
    p2 = QPoint(40,100);
    QTest::touchEvent(canvas).press(0, p1).press(1, p2);
    QTest::mousePress(canvas, Qt::LeftButton, 0, p1);

    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(15,0); p2 += QPoint(15,0);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    p1 += QPoint(0,15); p2 += QPoint(0,15);
    QTest::touchEvent(canvas).move(0, p1).move(1, p2);
    QTest::mouseMove(canvas, p1);

    QVERIFY(flickable->contentY() == 0);
    QCOMPARE(point11->isValid(), true);
    QCOMPARE(point12->isValid(), true);

    QTest::touchEvent(canvas).release(0, p1).release(1, p2);
    QTest::mouseRelease(canvas,Qt::LeftButton, 0, p1);
    QTest::qWait(50);

    delete canvas;
}

QQuickView *tst_QQuickMultiPointTouchArea::createAndShowView(const QString &file)
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + QLatin1String("/data/") + file));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);

    return canvas;
}

QTEST_MAIN(tst_QQuickMultiPointTouchArea)

#include "tst_qquickmultipointtoucharea.moc"
