/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <QtGui/qstylehints.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickmultipointtoucharea_p.h>
#include <QtQuick/private/qquickpincharea_p.h>
#include <QtQuick/private/qquickflickable_p.h>

#include <private/qquickcanvas_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

#include "../../shared/util.h"

struct Event
{
    Event(QEvent::Type t, QPoint mouse, QPoint global)
        :type(t), mousePos(mouse), mousePosGlobal(global)
    {}

    Event(QEvent::Type t, QList<QTouchEvent::TouchPoint> touch)
        :type(t), points(touch)
    {}

    QEvent::Type type;
    QPoint mousePos;
    QPoint mousePosGlobal;
    QList<QTouchEvent::TouchPoint> points;
};

class EventItem : public QQuickItem
{
    Q_OBJECT
public:
    EventItem(QQuickItem *parent = 0)
        : QQuickItem(parent), acceptMouse(false), acceptTouch(false), filterTouch(false)
    {}

    void touchEvent(QTouchEvent *event)
    {
        eventList.append(Event(event->type(), event->touchPoints()));
        event->setAccepted(acceptTouch);
    }
    void mousePressEvent(QMouseEvent *event)
    {
        eventList.append(Event(event->type(), event->pos(), event->globalPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseMoveEvent(QMouseEvent *event)
    {
        eventList.append(Event(event->type(), event->pos(), event->globalPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseReleaseEvent(QMouseEvent *event)
    {
        eventList.append(Event(event->type(), event->pos(), event->globalPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        eventList.append(Event(event->type(), event->pos(), event->globalPos()));
        event->setAccepted(acceptMouse);
    }
    bool event(QEvent *event) {
        if (event->type() == QEvent::UngrabMouse) {
            eventList.append(Event(event->type(), QPoint(0,0), QPoint(0,0)));
        }
        return QQuickItem::event(event);
    }

    QList<Event> eventList;
    bool acceptMouse;
    bool acceptTouch;
    bool filterTouch; // when used as event filter

    bool eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::TouchBegin ||
                event->type() == QEvent::TouchUpdate ||
                event->type() == QEvent::TouchCancel ||
                event->type() == QEvent::TouchEnd) {
            QTouchEvent *touch = static_cast<QTouchEvent*>(event);
            eventList.append(Event(event->type(), touch->touchPoints()));
            if (filterTouch)
                event->accept();
            return true;
        }
        return false;
    }
};

class tst_TouchMouse : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_TouchMouse()
        :device(0)
    {}

private slots:
    void initTestCase();

    void simpleTouchEvent();
    void eventFilter();
    void mouse();
    void touchOverMouse();
    void mouseOverTouch();

    void buttonOnFlickable();
    void buttonOnTouch();

    void pinchOnFlickable();
    void flickableOnPinch();
    void mouseOnFlickableOnPinch();

private:
    QQuickView *createView();
    QTouchDevice *device;
};

QQuickView *tst_TouchMouse::createView()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    return canvas;
}

void tst_TouchMouse::initTestCase()
{
    // This test assumes that we don't get synthesized mouse events from QGuiApplication
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    QQmlDataTest::initTestCase();
    qmlRegisterType<EventItem>("Qt.test", 1, 0, "EventItem");
    if (!device) {
        device = new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }
}

void tst_TouchMouse::simpleTouchEvent()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("singleitem.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->rootObject() != 0);

    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    // Do not accept touch or mouse
    QPoint p1;
    p1 = QPoint(20, 20);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 1);
    eventItem1->eventList.clear();

    // Accept touch
    eventItem1->acceptTouch = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 1);
    p1 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 3);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);
    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(canvas);
    QCOMPARE(canvasPriv->mouseGrabberItem, eventItem1);

    QPoint localPos = eventItem1->mapFromScene(p1).toPoint();
    QPoint globalPos = canvas->mapToGlobal(p1);
    QPoint scenePos = p1; // item is at 0,0
    QCOMPARE(eventItem1->eventList.at(0).points.at(0).pos().toPoint(), localPos);
    QCOMPARE(eventItem1->eventList.at(0).points.at(0).scenePos().toPoint(), scenePos);
    QCOMPARE(eventItem1->eventList.at(0).points.at(0).screenPos().toPoint(), globalPos);
    QCOMPARE(eventItem1->eventList.at(1).mousePos, localPos);
    QCOMPARE(eventItem1->eventList.at(1).mousePosGlobal, globalPos);

    p1 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::TouchUpdate);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::MouseMove);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE(eventItem1->eventList.at(4).type, QEvent::TouchEnd);
    QCOMPARE(eventItem1->eventList.at(5).type, QEvent::MouseButtonRelease);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse buttons but not the event
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = false;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);
    p1 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept touch and mouse
    eventItem1->acceptTouch = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchUpdate);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::TouchEnd);
    eventItem1->eventList.clear();

    delete canvas;
}

void tst_TouchMouse::eventFilter()
{
//    // install event filter on item and see that it can grab events
//    QQuickView *canvas = createView();

//    canvas->setSource(testFileUrl("singleitem.qml"));
//    canvas->show();
//    canvas->requestActivateWindow();
//    QVERIFY(canvas->rootObject() != 0);

//    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
//    QVERIFY(eventItem1);
//    eventItem1->acceptTouch = true;

//    EventItem *filter = new EventItem;
//    filter->filterTouch = true;
//    eventItem1->installEventFilter(filter);

//    QPoint p1 = QPoint(20, 20);
//    QTest::touchEvent(canvas, device).press(0, p1, canvas);
//    // QEXPECT_FAIL("", "We do not implement event filters correctly", Abort);
//    QCOMPARE(eventItem1->eventList.size(), 0);
//    QCOMPARE(filter->eventList.size(), 1);
//    QTest::touchEvent(canvas, device).release(0, p1, canvas);
//    QCOMPARE(eventItem1->eventList.size(), 0);
//    QCOMPARE(filter->eventList.size(), 2);

//    delete filter;
//    delete canvas;
}

void tst_TouchMouse::mouse()
{
    // eventItem1
    //   - eventItem2

    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("twoitems.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = canvas->rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);
    QTest::qWaitForWindowShown(canvas);

    // bottom item likes mouse, top likes touch
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->acceptMouse = true;
    // item 2 doesn't accept anything, thus it sees a touch pass by
    QPoint p1 = QPoint(30, 30);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);

    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);

    delete canvas;
}

void tst_TouchMouse::touchOverMouse()
{
    // eventItem1
    //   - eventItem2

    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("twoitems.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = canvas->rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    // bottom item likes mouse, top likes touch
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem2->acceptTouch = true;

    QTest::qWaitForWindowShown(canvas);

    QCOMPARE(eventItem1->eventList.size(), 0);
    QPoint p1 = QPoint(20, 20);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 0);
    QCOMPARE(eventItem2->eventList.size(), 1);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QCOMPARE(eventItem2->eventList.size(), 2);
    QCOMPARE(eventItem2->eventList.at(1).type, QEvent::TouchUpdate);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem2->eventList.size(), 3);
    QCOMPARE(eventItem2->eventList.at(2).type, QEvent::TouchEnd);
    eventItem2->eventList.clear();

    delete canvas;
}

void tst_TouchMouse::mouseOverTouch()
{
    // eventItem1
    //   - eventItem2

    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("twoitems.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = canvas->rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    // bottom item likes mouse, top likes touch
    eventItem1->acceptTouch = true;
    eventItem2->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem2->acceptMouse = true;

    QTest::qWaitForWindowShown(canvas);

    QPoint p1 = QPoint(20, 20);
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 0);
    QCOMPARE(eventItem2->eventList.size(), 2);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem2->eventList.at(1).type, QEvent::MouseButtonPress);


//    p1 += QPoint(10, 0);
//    QTest::touchEvent(canvas, device).move(0, p1, canvas);
//    QCOMPARE(eventItem2->eventList.size(), 1);
//    QTest::touchEvent(canvas, device).release(0, p1, canvas);
//    QCOMPARE(eventItem2->eventList.size(), 1);
//    eventItem2->eventList.clear();

    delete canvas;
}

void tst_TouchMouse::buttonOnFlickable()
{
    // flickable - height 500 / 1000
    //   - eventItem1 y: 100, height 100
    //   - eventItem2 y: 300, height 100

    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("buttononflickable.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flickable = canvas->rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    // should a mouse area button be clickable on top of flickable? yes :)
    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->acceptMouse = true;

    // should a touch button be touchable on top of flickable? yes :)
    EventItem *eventItem2 = canvas->rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);
    QCOMPARE(eventItem2->eventList.size(), 0);
    eventItem2->acceptTouch = true;

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // check that buttons are clickable
    // mouse button
    QCOMPARE(eventItem1->eventList.size(), 0);
    QPoint p1 = QPoint(20, 130);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::TouchEnd);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::MouseButtonRelease);
    eventItem1->eventList.clear();

    // touch button
    p1 = QPoint(10, 310);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem2->eventList.size(), 1);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::TouchBegin);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem2->eventList.size(), 2);
    QCOMPARE(eventItem2->eventList.at(1).type, QEvent::TouchEnd);
    QCOMPARE(eventItem1->eventList.size(), 0);
    eventItem2->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // click above button, no events please
    p1 = QPoint(10, 90);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 0);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 0);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // check that flickable moves - mouse button
    QCOMPARE(eventItem1->eventList.size(), 0);
    p1 = QPoint(10, 110);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);

    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(canvas);
    QCOMPARE(canvasPriv->touchMouseId, 0);
    QCOMPARE(canvasPriv->itemForTouchPointId[0], eventItem1);
    QCOMPARE(canvasPriv->mouseGrabberItem, eventItem1);

    p1 += QPoint(0, -10);
    QPoint p2 = p1 + QPoint(0, -10);
    QPoint p3 = p2 + QPoint(0, -10);
    QTest::qWait(10);
    QTest::touchEvent(canvas, device).move(0, p1, canvas);
    QTest::qWait(10);
    QTest::touchEvent(canvas, device).move(0, p2, canvas);
    QTest::qWait(10);
    QTest::touchEvent(canvas, device).move(0, p3, canvas);

    // we cannot really know when the events get grabbed away
    QVERIFY(eventItem1->eventList.size() >= 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::TouchUpdate);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::MouseMove);

    QCOMPARE(canvasPriv->mouseGrabberItem, flickable);
    QVERIFY(flickable->isMovingVertically());

    QTest::touchEvent(canvas, device).release(0, p3, canvas);
    delete canvas;
}

void tst_TouchMouse::buttonOnTouch()
{
    // 400x800
    //   PinchArea - height 400
    //     - eventItem1 y: 100, height 100
    //     - eventItem2 y: 300, height 100
    //   MultiPointTouchArea - height 400
    //     - eventItem1 y: 100, height 100
    //     - eventItem2 y: 300, height 100

    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("buttonontouch.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickItem *button1 = canvas->rootObject()->findChild<QQuickItem*>("button1");
    QVERIFY(button1);
    EventItem *eventItem1 = canvas->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = canvas->rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    QQuickMultiPointTouchArea *touchArea = canvas->rootObject()->findChild<QQuickMultiPointTouchArea*>("toucharea");
    QVERIFY(touchArea);
    EventItem *eventItem3 = canvas->rootObject()->findChild<EventItem*>("eventItem3");
    QVERIFY(eventItem3);
    EventItem *eventItem4 = canvas->rootObject()->findChild<EventItem*>("eventItem4");
    QVERIFY(eventItem4);


    // Test the common case of a mouse area on top of pinch
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->acceptMouse = true;


    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Normal touch click
    QPoint p1 = QPoint(10, 110);
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QTest::touchEvent(canvas, device).release(0, p1, canvas);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::TouchEnd);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::MouseButtonRelease);
    eventItem1->eventList.clear();

    // Normal mouse click
    QTest::mouseClick(canvas, Qt::LeftButton, 0, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonRelease);
    eventItem1->eventList.clear();

    // Pinch starting on the PinchArea should work
    p1 = QPoint(40, 10);
    QPoint p2 = QPoint(60, 10);

    // Start the events after each other
    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QTest::touchEvent(canvas, device).stationary(0).press(1, p2, canvas);

    QCOMPARE(button1->scale(), 1.0);

    // This event seems to be discarded, let's ignore it for now until someone digs into pincharea
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);
//    QCOMPARE(button1->scale(), 1.5);
    qDebug() << "Button scale: " << button1->scale();

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);
//    QCOMPARE(button1->scale(), 2.0);
    qDebug() << "Button scale: " << button1->scale();

    QTest::touchEvent(canvas, device).release(0, p1, canvas).release(1, p2, canvas);
//    QVERIFY(eventItem1->eventList.isEmpty());
//    QCOMPARE(button1->scale(), 2.0);
    qDebug() << "Button scale: " << button1->scale();


    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Start pinching while on the button
    button1->setScale(1.0);
    p1 = QPoint(40, 110);
    p2 = QPoint(60, 110);
    QTest::touchEvent(canvas, device).press(0, p1, canvas).press(1, p2, canvas);
    QCOMPARE(button1->scale(), 1.0);
    QCOMPARE(eventItem1->eventList.count(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);

    // This event seems to be discarded, let's ignore it for now until someone digs into pincharea
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);
    //QCOMPARE(button1->scale(), 1.5);
    qDebug() << button1->scale();

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);
    qDebug() << button1->scale();
    //QCOMPARE(button1->scale(), 2.0);

    QTest::touchEvent(canvas, device).release(0, p1, canvas).release(1, p2, canvas);
//    QCOMPARE(eventItem1->eventList.size(), 99);
    qDebug() << button1->scale();
    //QCOMPARE(button1->scale(), 2.0);

    delete canvas;
}

void tst_TouchMouse::pinchOnFlickable()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("pinchonflickable.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickFlickable *flickable = canvas->rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);
    QQuickItem *rect = canvas->rootObject()->findChild<QQuickItem*>("rect");
    QVERIFY(rect);

    // flickable - single touch point
    QVERIFY(flickable->contentX() == 0.0);
    QPoint p = QPoint(100, 100);
    QTest::touchEvent(canvas, device).press(0, p, canvas);
    QCOMPARE(rect->pos(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    QTest::qWait(10);
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    QTest::qWait(10);
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    QTest::touchEvent(canvas, device).release(0, p, canvas);

    QGuiApplication::processEvents();
    QTest::qWait(10);
    QVERIFY(!flickable->isAtXBeginning());
    // wait until flicking is done
    QTRY_VERIFY(!flickable->isFlicking());

    // pinch
    QPoint p1 = QPoint(40, 20);
    QPoint p2 = QPoint(60, 20);

    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(canvas, device);
    pinchSequence.press(0, p1, canvas).commit();
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and PinchArea will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p2, canvas).commit();
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    QCOMPARE(rect->scale(), 1.0);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    pinchSequence.release(0, p1, canvas).release(1, p2, canvas).commit();
    QVERIFY(rect->scale() > 1.0);
}

void tst_TouchMouse::flickableOnPinch()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("flickableonpinch.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickFlickable *flickable = canvas->rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);
    QQuickItem *rect = canvas->rootObject()->findChild<QQuickItem*>("rect");
    QVERIFY(rect);

    // flickable - single touch point
    QVERIFY(flickable->contentX() == 0.0);
    QPoint p = QPoint(100, 100);
    QTest::touchEvent(canvas, device).press(0, p, canvas);
    QCOMPARE(rect->pos(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);

    QTest::qWait(1000);

    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    QTest::touchEvent(canvas, device).release(0, p, canvas);

    QTest::qWait(1000);

    //QVERIFY(flickable->isMovingHorizontally());
    qDebug() << "Pos: " << rect->pos();
    // wait until flicking is done
    QTRY_VERIFY(!flickable->isFlicking());

    // pinch
    QPoint p1 = QPoint(40, 20);
    QPoint p2 = QPoint(60, 20);
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(canvas, device);
    pinchSequence.press(0, p1, canvas).commit();
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and PinchArea will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p2, canvas).commit();
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    QCOMPARE(rect->scale(), 1.0);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    pinchSequence.release(0, p1, canvas).release(1, p2, canvas).commit();
    QVERIFY(rect->scale() > 1.0);
}

void tst_TouchMouse::mouseOnFlickableOnPinch()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("mouseonflickableonpinch.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickFlickable *flickable = canvas->rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);
    QQuickItem *rect = canvas->rootObject()->findChild<QQuickItem*>("rect");
    QVERIFY(rect);

    // flickable - single touch point
    QVERIFY(flickable->contentX() == 0.0);
    QPoint p = QPoint(100, 100);
    QTest::touchEvent(canvas, device).press(0, p, canvas);
    QCOMPARE(rect->pos(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);

    QTest::qWait(1000);

    p -= QPoint(10, 0);
    QTest::touchEvent(canvas, device).move(0, p, canvas);
    QTest::touchEvent(canvas, device).release(0, p, canvas);

    QTest::qWait(1000);

    //QVERIFY(flickable->isMovingHorizontally());
    qDebug() << "Pos: " << rect->pos();

    // pinch
    QPoint p1 = QPoint(40, 20);
    QPoint p2 = QPoint(60, 20);
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(canvas, device);
    pinchSequence.press(0, p1, canvas).commit();
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and PinchArea will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p2, canvas).commit();
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    QCOMPARE(rect->scale(), 1.0);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    pinchSequence.release(0, p1, canvas).release(1, p2, canvas).commit();
    QVERIFY(rect->scale() > 1.0);

    // PinchArea should steal the event after flicking started
    rect->setScale(1.0);
    flickable->setContentX(0.0);
    p = QPoint(100, 100);
    pinchSequence.press(0, p, canvas).commit();
    QCOMPARE(rect->pos(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    pinchSequence.move(0, p, canvas).commit();
    p -= QPoint(10, 0);
    pinchSequence.move(0, p, canvas).commit();
    QTest::qWait(1000);
    p -= QPoint(10, 0);
    pinchSequence.move(0, p, canvas).commit();

    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(canvas);
    QCOMPARE(canvasPriv->mouseGrabberItem, flickable);
    qDebug() << "Mouse Grabber: " << canvasPriv->mouseGrabberItem << " itemForTouchPointId: " << canvasPriv->itemForTouchPointId;

    // Add a second finger, this should lead to stealing
    p1 = QPoint(40, 100);
    p2 = QPoint(60, 100);
    pinchSequence.stationary(0).press(1, p2, canvas).commit();
    QCOMPARE(rect->scale(), 1.0);

    p1 -= QPoint(5, 0);
    p2 += QPoint(5, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(5, 0);
    p2 += QPoint(5, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    p1 -= QPoint(5, 0);
    p2 += QPoint(5, 0);
    pinchSequence.move(0, p1, canvas).move(1, p2, canvas).commit();
    pinchSequence.release(0, p1, canvas).release(1, p2, canvas).commit();
    QVERIFY(rect->scale() > 1.0);
    pinchSequence.release(0, p, canvas).commit();
}

QTEST_MAIN(tst_TouchMouse)

#include "tst_touchmouse.moc"

