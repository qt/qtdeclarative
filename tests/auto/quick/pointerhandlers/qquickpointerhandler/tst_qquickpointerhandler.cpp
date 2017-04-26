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

#include "../../../shared/util.h"
#include "../../shared/viewtestutil.h"

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

struct Event
{
    enum Destination {
        FilterDestination,
        MouseDestination,
        TouchDestination,
        HandlerDestination
    };

    Event(Destination d, QEvent::Type t, Qt::TouchPointState s, QPointF item, QPointF scene)
        : destination(d), type(t), state(s), posWrtItem(item), posWrtScene(scene)
    {}

    Destination destination;
    QEvent::Type type;
    Qt::TouchPointState state;

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
        : QQuickItem(parent), acceptPointer(false), grabPointer(false), acceptMouse(false), acceptTouch(false), filterTouch(false)
    {}

    void touchEvent(QTouchEvent *event)
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptTouch;
        for (const QTouchEvent::TouchPoint &tp : event->touchPoints())
            eventList.append(Event(Event::TouchDestination, event->type(), tp.state(), tp.pos(), tp.scenePos()));
        event->setAccepted(acceptTouch);
    }
    void mousePressEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(Event::MouseDestination, event->type(), Qt::TouchPointPressed, event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseMoveEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(Event::MouseDestination, event->type(), Qt::TouchPointMoved, event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseReleaseEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(Event::MouseDestination, event->type(), Qt::TouchPointReleased, event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        eventList.append(Event(Event::MouseDestination, event->type(), Qt::TouchPointPressed, event->pos(), event->windowPos()));
        event->setAccepted(acceptMouse);
    }

    void mouseUngrabEvent()
    {
        qCDebug(lcPointerTests);
        eventList.append(Event(Event::MouseDestination, QEvent::UngrabMouse, Qt::TouchPointReleased, QPoint(0,0), QPoint(0,0)));
    }

    bool event(QEvent *event)
    {
        qCDebug(lcPointerTests) << event;
        return QQuickItem::event(event);
    }

    QList<Event> eventList;
    bool acceptPointer;
    bool grabPointer;
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
                eventList.append(Event(Event::FilterDestination, event->type(), tp.state(), tp.pos(), tp.scenePos()));
            if (filterTouch)
                event->accept();
            return true;
        }
        return false;
    }
};

#define QCOMPARE_EVENT(i, d, s, t) \
    {\
        const Event &event = eventItem1->eventList.at(i);\
        QCOMPARE(event.destination, d);\
        QCOMPARE(event.type, t);\
        QCOMPARE(event.state, s);\
    }\

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
            if (item->acceptPointer)
                point->setAccepted(item->acceptPointer); // does NOT imply a grab
            if (item->grabPointer)
                setExclusiveGrab(point, true);
            qCDebug(lcPointerTests) << "        " << i << ":" << point << "accepted?" << item->acceptPointer << "grabbed?" << (point->exclusiveGrabber() == this);
            item->eventList.append(Event(Event::HandlerDestination, QEvent::Pointer,
                static_cast<Qt::TouchPointState>(point->state()), eventPos(point), point->scenePos()));
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
        Qt::TouchPointState tpState;
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            tpState = Qt::TouchPointPressed;
            break;
        case QEvent::MouseMove:
            tpState = Qt::TouchPointMoved;
            break;
        case QEvent::MouseButtonRelease:
            tpState = Qt::TouchPointReleased;
            break;
        default:
            // So far we aren't recording filtered touch events here - they would be quite numerous in some cases
            return false;
        }
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        filteredEventList.append(Event(Event::FilterDestination, event->type(), tpState, me->pos(), me->globalPos()));
        return false;
    }

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QTouchDevice *touchDevice;
    QList<Event> filteredEventList;
};

void tst_PointerHandlers::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
//    window->setGeometry(0,0,240,320);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtil::centerOnScreen(window.data());
    QQuickViewTestUtil::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != 0);
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
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "singleitem.qml");
    QQuickView * window = windowPtr.data();

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    // Do not accept anything
    QPoint p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    QCOMPARE_EVENT(1, Event::TouchDestination, Qt::TouchPointPressed, QEvent::TouchBegin);
    QCOMPARE_EVENT(2, Event::MouseDestination, Qt::TouchPointPressed, QEvent::MouseButtonPress);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(3, Event::HandlerDestination, Qt::TouchPointMoved, QEvent::Pointer);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 5);
    QCOMPARE_EVENT(4, Event::HandlerDestination, Qt::TouchPointReleased, QEvent::Pointer);
    eventItem1->eventList.clear();

    // Accept touch
    eventItem1->acceptTouch = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    QCOMPARE_EVENT(1, Event::TouchDestination, Qt::TouchPointPressed, QEvent::TouchBegin);
    auto pointerEvent = QQuickPointerDevice::touchDevices().at(0)->pointerEvent();
    QCOMPARE(pointerEvent->point(0)->exclusiveGrabber(), eventItem1);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(2, Event::HandlerDestination, Qt::TouchPointMoved, QEvent::Pointer);
    QCOMPARE_EVENT(3, Event::TouchDestination, Qt::TouchPointMoved, QEvent::TouchUpdate);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE_EVENT(4, Event::HandlerDestination, Qt::TouchPointReleased, QEvent::Pointer);
    QCOMPARE_EVENT(5, Event::TouchDestination, Qt::TouchPointReleased, QEvent::TouchEnd);
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
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    QCOMPARE_EVENT(1, Event::TouchDestination, Qt::TouchPointPressed, QEvent::TouchBegin);
    QCOMPARE_EVENT(2, Event::MouseDestination, Qt::TouchPointPressed, QEvent::MouseButtonPress);
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
    QCOMPARE_EVENT(3, Event::HandlerDestination, Qt::TouchPointMoved, QEvent::Pointer);
    QCOMPARE_EVENT(4, Event::TouchDestination,Qt::TouchPointMoved,  QEvent::TouchUpdate);
    QCOMPARE_EVENT(5, Event::MouseDestination, Qt::TouchPointMoved, QEvent::MouseMove);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 10);
    QCOMPARE_EVENT(6, Event::HandlerDestination, Qt::TouchPointReleased, QEvent::Pointer);
    QCOMPARE_EVENT(7, Event::TouchDestination, Qt::TouchPointReleased, QEvent::TouchEnd);
    QCOMPARE_EVENT(8, Event::MouseDestination, Qt::TouchPointReleased, QEvent::MouseButtonRelease);
    QCOMPARE_EVENT(9, Event::MouseDestination, Qt::TouchPointReleased, QEvent::UngrabMouse);
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
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    QCOMPARE_EVENT(1, Event::TouchDestination, Qt::TouchPointPressed, QEvent::TouchBegin);
    QCOMPARE_EVENT(2, Event::MouseDestination, Qt::TouchPointPressed, QEvent::MouseButtonPress);
    QCOMPARE(pointerEvent->point(0)->exclusiveGrabber(), nullptr);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 5);
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
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    QCOMPARE_EVENT(1, Event::TouchDestination, Qt::TouchPointPressed, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(2, Event::HandlerDestination, Qt::TouchPointMoved, QEvent::Pointer);
    QCOMPARE_EVENT(3, Event::TouchDestination, Qt::TouchPointMoved, QEvent::TouchUpdate);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE_EVENT(4, Event::HandlerDestination, Qt::TouchPointReleased, QEvent::Pointer);
    QCOMPARE_EVENT(5, Event::TouchDestination, Qt::TouchPointReleased, QEvent::TouchEnd);
    eventItem1->eventList.clear();

    // Accept pointer events
    eventItem1->acceptPointer = true;
    eventItem1->grabPointer = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(1, Event::HandlerDestination, Qt::TouchPointMoved, QEvent::Pointer);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(2, Event::HandlerDestination, Qt::TouchPointReleased, QEvent::Pointer);
    eventItem1->eventList.clear();
}

void tst_PointerHandlers::mouseEventDelivery()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "singleitem.qml");
    QQuickView * window = windowPtr.data();

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    // Do not accept anything
    QPoint p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QTest::mouseRelease(window, Qt::LeftButton);
    QCOMPARE(eventItem1->eventList.size(), 3);
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
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    QCOMPARE_EVENT(1, Event::MouseDestination, Qt::TouchPointPressed, QEvent::MouseButtonPress);
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
    QCOMPARE_EVENT(2, Event::MouseDestination, Qt::TouchPointMoved, QEvent::MouseMove);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 5);
    QCOMPARE_EVENT(3, Event::MouseDestination, Qt::TouchPointReleased, QEvent::MouseButtonRelease);
    QCOMPARE_EVENT(4, Event::MouseDestination, Qt::TouchPointReleased, QEvent::UngrabMouse);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Grab pointer events
    eventItem1->acceptMouse = false;
    eventItem1->acceptPointer = true;
    eventItem1->grabPointer = true;
    p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE_EVENT(0, Event::HandlerDestination, Qt::TouchPointPressed, QEvent::Pointer);
    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(1, Event::HandlerDestination, Qt::TouchPointMoved, QEvent::Pointer);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(2, Event::HandlerDestination, Qt::TouchPointReleased, QEvent::Pointer);
    eventItem1->eventList.clear();
}

QTEST_MAIN(tst_PointerHandlers)

#include "tst_qquickpointerhandler.moc"

