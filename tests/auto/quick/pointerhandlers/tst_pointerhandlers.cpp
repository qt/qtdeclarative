/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtGui/qstylehints.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickmultipointtoucharea_p.h>
#include <QtQuick/private/qquickpincharea_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qquickwindow_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

#include "../../shared/util.h"
#include "../shared/viewtestutil.h"

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

struct Event
{
    Event(QEvent::Type t, QPointF item, QPointF scene)
        :type(t), posWrtItem(item), posWrtScene(scene)
    {}

    QEvent::Type type;
    QPointF posWrtItem;
    QPointF posWrtScene;

};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const struct Event &event) {
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "Event(" << event.type << " @" << event.posWrtScene << ")";
    return dbg;
}
#endif

class EventItem : public QQuickItem
{
    Q_OBJECT
public:
    EventItem(QQuickItem *parent = 0)
        : QQuickItem(parent), acceptPointer(false), acceptMouse(false), acceptTouch(false), filterTouch(false)
    {}

    void touchEvent(QTouchEvent *event)
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptTouch;
        for (const QTouchEvent::TouchPoint &tp : event->touchPoints())
            eventList.append(Event(event->type(), tp.pos(), tp.scenePos()));
        event->setAccepted(acceptTouch);
    }
    void mousePressEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(event->type(), event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseMoveEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(event->type(), event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseReleaseEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(event->type(), event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(event->type(), event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }

    void mouseUngrabEvent()
    {
        qCDebug(lcPointerTests);
        eventList.append(Event(QEvent::UngrabMouse, QPoint(0,0), QPoint(0,0)));
    }

    bool event(QEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        return QQuickItem::event(event);
    }

    QList<Event> eventList;
    bool acceptPointer;
    bool acceptMouse;
    bool acceptTouch;
    bool filterTouch; // when used as event filter

    bool eventFilter(QObject *o, QEvent *event)
    {
        qCDebug(lcPointerTests) << event << o;
        if (event->type() == QEvent::TouchBegin ||
                event->type() == QEvent::TouchUpdate ||
                event->type() == QEvent::TouchCancel ||
                event->type() == QEvent::TouchEnd) {
            QTouchEvent *touch = static_cast<QTouchEvent*>(event);
            for (const QTouchEvent::TouchPoint &tp : touch->touchPoints())
                eventList.append(Event(event->type(), tp.pos(), tp.scenePos()));
            if (filterTouch)
                event->accept();
            return true;
        }
        return false;
    }
};

class EventHandler : public QQuickPointerHandler
{
    void handlePointerEventImpl(QQuickPointerEvent *event) override
    {
        QQuickPointerHandler::handlePointerEventImpl(event);
        if (!enabled())
            return;
        EventItem *item = static_cast<EventItem *>(target());
        qCDebug(lcPointerTests) << item->objectName() << event;
        int c = event->pointCount();
        for (int i = 0; i < c; ++i) {
            QQuickEventPoint *point = event->point(i);
            point->setAccepted(item->acceptPointer); // has no effect: grabbing is up to us
            if (item->acceptPointer)
                setGrab(point, true);
            qCDebug(lcPointerTests) << "        " << i << ":" << point << "grabbed?" << item->acceptPointer;
            item->eventList.append(Event(QEvent::Pointer, eventPos(point), point->scenePos()));
        }
    }
};

class tst_PointerHandlers : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_PointerHandlers()
        :touchDevice(QTest::createTouchDevice())
    {}

private slots:
    void initTestCase();

    void touchEventDelivery();
    void mouseEventDelivery();

protected:
    bool eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            filteredEventList.append(Event(me->type(), me->pos(), me->globalPos()));
        }
        return false;
    }

private:
    QQuickView *createView();
    QTouchDevice *touchDevice;
    QList<Event> filteredEventList;
};

QQuickView *tst_PointerHandlers::createView()
{
    QQuickView *window = new QQuickView(0);
    window->setGeometry(0,0,240,320);

    return window;
}

void tst_PointerHandlers::initTestCase()
{
    // This test assumes that we don't get synthesized mouse events from QGuiApplication
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    QQmlDataTest::initTestCase();
    qmlRegisterType<EventItem>("Qt.test", 1, 0, "EventItem");
    qmlRegisterType<EventHandler>("Qt.test", 1, 0, "EventHandler");
}

void tst_PointerHandlers::touchEventDelivery()
{
    QQuickView *window = createView();

    window->setSource(testFileUrl("singleitem.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(window->rootObject() != 0);

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    // Do not accept anything
    QPoint p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseButtonPress);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    eventItem1->eventList.clear();

    // Accept touch
    eventItem1->acceptTouch = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchBegin);
    auto pointerEvent = QQuickPointerDevice::touchDevices().at(0)->pointerEvent();
    QCOMPARE(pointerEvent->point(0)->grabber(), eventItem1);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::TouchUpdate);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE(eventItem1->eventList.at(4).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(5).type, QEvent::TouchEnd);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseButtonPress);
    QCOMPARE(window->mouseGrabberItem(), eventItem1);

    QPointF localPos = eventItem1->mapFromScene(p1);
    QPointF scenePos = p1; // item is at 0,0
    QCOMPARE(eventItem1->eventList.at(1).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(1).posWrtScene, scenePos);
    QCOMPARE(eventItem1->eventList.at(2).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(2).posWrtScene, scenePos);

    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(4).type, QEvent::TouchUpdate);
    QCOMPARE(eventItem1->eventList.at(5).type, QEvent::MouseMove);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 10);
    QCOMPARE(eventItem1->eventList.at(6).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(7).type, QEvent::TouchEnd);
    QCOMPARE(eventItem1->eventList.at(8).type, QEvent::MouseButtonRelease);
    QCOMPARE(eventItem1->eventList.at(9).type, QEvent::UngrabMouse);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse buttons but not the touch event
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = false;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchBegin);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseButtonPress);
    QCOMPARE(pointerEvent->point(0)->grabber(), nullptr);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept touch
    eventItem1->acceptTouch = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::TouchUpdate);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE(eventItem1->eventList.at(4).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(5).type, QEvent::TouchEnd);
    eventItem1->eventList.clear();

    // Accept pointer events
    eventItem1->acceptPointer = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::Pointer);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::Pointer);
    eventItem1->eventList.clear();

    delete window;
}

void tst_PointerHandlers::mouseEventDelivery()
{
    QQuickView *window = createView();

    window->setSource(testFileUrl("singleitem.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(window->rootObject() != 0);

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    // Do not accept anything
    QPoint p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QTest::mouseRelease(window, Qt::LeftButton);
    QCOMPARE(eventItem1->eventList.size(), 2);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonPress);
    QCOMPARE(window->mouseGrabberItem(), eventItem1);

    QPointF localPos = eventItem1->mapFromScene(p1);
    QPointF scenePos = p1; // item is at 0,0
    QCOMPARE(eventItem1->eventList.at(0).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(0).posWrtScene, scenePos);
    QCOMPARE(eventItem1->eventList.at(1).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(1).posWrtScene, scenePos);

    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseMove);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 5);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::MouseButtonRelease);
    QCOMPARE(eventItem1->eventList.at(4).type, QEvent::UngrabMouse);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept pointer events
    eventItem1->acceptMouse = false;
    eventItem1->acceptPointer = true;
    p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::Pointer);
    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::Pointer);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::Pointer);
    eventItem1->eventList.clear();

    delete window;
}

QTEST_MAIN(tst_PointerHandlers)

#include "tst_pointerhandlers.moc"

