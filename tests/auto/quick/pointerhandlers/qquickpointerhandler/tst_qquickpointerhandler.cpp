// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>

#include <private/qdebug_p.h>
#include <QtGui/qstylehints.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/private/qqmlglobal_p.h> // qmlobject_cast

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QQmlComponent>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class Event
{
    Q_GADGET
public:
    enum Destination {
        FilterDestination,
        MouseDestination,
        TouchDestination,
        HandlerDestination
    };
    Q_ENUM(Destination)

    Event(Destination d, QEvent::Type t, QEventPoint::State s, int grabTransition, QPointF item, QPointF scene)
        : destination(d), type(t), state(s), grabTransition(grabTransition), posWrtItem(item), posWrtScene(scene)
    {}

    Destination destination;
    QEvent::Type type;                      // if this represents a QEvent that was received
    QEventPoint::State state;              // if this represents an event (pointer, touch or mouse)
    int grabTransition;                     // if this represents an onGrabChanged() notification (QPointingDevice::GrabTransition)
    QPointF posWrtItem;
    QPointF posWrtScene;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const class Event &event) {
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "Event(";
    QtDebugUtils::formatQEnum(dbg, event.destination);
    dbg << ' ';
    QtDebugUtils::formatQEnum(dbg, event.type);
    dbg << ' ';
    QtDebugUtils::formatQEnum(dbg, event.state);
    if (event.grabTransition) {
        dbg << ' ';
        QtDebugUtils::formatQEnum(dbg, QPointingDevice::GrabTransition(event.grabTransition));
    }
    dbg << " @ ";
    QtDebugUtils::formatQPoint(dbg, event.posWrtItem);
    dbg << " S ";
    QtDebugUtils::formatQPoint(dbg, event.posWrtScene);
    dbg << ')';
    return dbg;
}
#endif

enum {
    NoGrab = 0,
};

class EventItem : public QQuickItem
{
    Q_OBJECT
public:
    EventItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent), acceptPointer(false), grabPointer(false), acceptMouse(false), acceptTouch(false), filterTouch(false)
    {}

    inline int grabTransition(bool accept, QEventPoint::State state) {
        return (accept && (state != QEventPoint::State::Released)) ? (int)QPointingDevice::GrabExclusive : (int)NoGrab;
    }

    void touchEvent(QTouchEvent *event) override
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptTouch;
        for (auto &tp : event->points())
            eventList.append(Event(Event::TouchDestination, event->type(), tp.state(), grabTransition(acceptTouch, tp.state()), tp.position(), tp.scenePosition()));
        event->setAccepted(acceptTouch);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptMouse;
        eventList.append(Event(Event::MouseDestination, event->type(), QEventPoint::State::Pressed, grabTransition(acceptMouse, QEventPoint::State::Pressed), event->position().toPoint(), event->scenePosition()));
        event->setAccepted(acceptMouse);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptMouse;
        eventList.append(Event(Event::MouseDestination, event->type(), QEventPoint::State::Updated, grabTransition(acceptMouse, QEventPoint::State::Updated), event->position().toPoint(), event->scenePosition()));
        event->setAccepted(acceptMouse);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptMouse;
        eventList.append(Event(Event::MouseDestination, event->type(), QEventPoint::State::Released, grabTransition(acceptMouse, QEventPoint::State::Released), event->position().toPoint(), event->scenePosition()));
        event->setAccepted(acceptMouse);
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        qCDebug(lcPointerTests) << event << "will accept?" << acceptMouse;
        eventList.append(Event(Event::MouseDestination, event->type(), QEventPoint::State::Pressed, grabTransition(acceptMouse, QEventPoint::State::Pressed), event->position().toPoint(), event->scenePosition()));
        event->setAccepted(acceptMouse);
    }

    void mouseUngrabEvent() override
    {
        qCDebug(lcPointerTests);
        eventList.append(Event(Event::MouseDestination, QEvent::UngrabMouse, QEventPoint::State::Released, QPointingDevice::UngrabExclusive, QPoint(0,0), QPoint(0,0)));
    }

    bool event(QEvent *event) override
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

    bool eventFilter(QObject *o, QEvent *event) override
    {
        qCDebug(lcPointerTests) << event << o;
        if (event->type() == QEvent::TouchBegin ||
                event->type() == QEvent::TouchUpdate ||
                event->type() == QEvent::TouchCancel ||
                event->type() == QEvent::TouchEnd) {
            QTouchEvent *touch = static_cast<QTouchEvent*>(event);
            for (auto &tp : touch->points())
                eventList.append(Event(Event::FilterDestination, event->type(), tp.state(),
                                       QPointingDevice::GrabExclusive, tp.position(), tp.scenePosition()));
            if (filterTouch)
                event->accept();
            return true;
        }
        return false;
    }
};

#define QCOMPARE_EVENT(i, d, t, s, g) \
    {\
        const Event &event = eventItem1->eventList.at(i);\
        QCOMPARE(event.destination, d);\
        QCOMPARE(event.type, t);\
        QCOMPARE(event.state, s);\
        QCOMPARE(event.grabTransition, g);\
    }\

class EventHandler : public QQuickPointerHandler
{
public:
    EventHandler(QQuickItem *parent = nullptr) :
        QQuickPointerHandler(parent) {}

    void handlePointerEventImpl(QPointerEvent *event) override
    {
        QQuickPointerHandler::handlePointerEventImpl(event);
        if (!enabled())
            return;
        if (event->isBeginEvent())
            ++pressEventCount;
        if (event->isEndEvent())
            ++releaseEventCount;
        EventItem *item = qmlobject_cast<EventItem *>(target());
        if (!item) {
            event->setExclusiveGrabber(event->point(0), this);
            return;
        }
        qCDebug(lcPointerTests) << item->objectName() << event;
        for (auto point : event->points()) {
            if (item->acceptPointer)
                point.setAccepted(item->acceptPointer); // does NOT imply a grab
            if (item->grabPointer)
                setExclusiveGrab(event, point, true);
            qCDebug(lcPointerTests) << "        " << point << "accepted?" << item->acceptPointer
                                    << "grabbed?" << (event->exclusiveGrabber(point) == this);
            item->eventList.append(Event(Event::HandlerDestination, QEvent::Pointer,
                static_cast<QEventPoint::State>(point.state()),
                item->grabPointer ? (int)QPointingDevice::GrabExclusive : (int)NoGrab,
                eventPos(point), point.scenePosition()));
        }
    }

    void onGrabChanged(QQuickPointerHandler *, QPointingDevice::GrabTransition stateChange,
                       QPointerEvent *ev, QEventPoint &point) override
    {
        Q_UNUSED(ev);
        EventItem *item = qmlobject_cast<EventItem *>(target());
        if (item)
            item->eventList.append(Event(Event::HandlerDestination, QEvent::None,
                static_cast<QEventPoint::State>(point.state()), stateChange, eventPos(point), point.scenePosition()));
    }

    int pressEventCount = 0;
    int releaseEventCount = 0;
};

class tst_PointerHandlers : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_PointerHandlers()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
        , touchDevice(QTest::createTouchDevice())
    {}

private slots:
    void initTestCase() override;

    void touchEventDelivery_data();
    void touchEventDelivery();
    void mouseEventDelivery();
    void touchReleaseOutside_data();
    void touchReleaseOutside();
    void dynamicCreation();
    void handlerInWindow();
    void dynamicCreationInWindow();
    void cppConstruction();
    void reparenting();
    void grabberSceneChange_data();
    void grabberSceneChange();
    void clip();

protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        QEventPoint::State tpState;
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            tpState = QEventPoint::State::Pressed;
            break;
        case QEvent::MouseMove:
            tpState = QEventPoint::State::Updated;
            break;
        case QEvent::MouseButtonRelease:
            tpState = QEventPoint::State::Released;
            break;
        default:
            // So far we aren't recording filtered touch events here - they would be quite numerous in some cases
            return false;
        }
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        filteredEventList.append(Event(Event::FilterDestination, event->type(), tpState,
                                       0, me->position().toPoint(), me->globalPosition().toPoint()));
        return false;
    }

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QPointingDevice *touchDevice;
    QList<Event> filteredEventList;
};

void tst_PointerHandlers::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
//    window->setGeometry(0,0,240,320);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtils::centerOnScreen(window.data());
    QQuickViewTestUtils::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

void tst_PointerHandlers::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<EventItem>("Qt.test", 1, 0, "EventItem");
    qmlRegisterType<EventHandler>("Qt.test", 1, 0, "EventHandler");
}

void tst_PointerHandlers::touchEventDelivery_data()
{
    QTest::addColumn<bool>("synthMouse"); // AA_SynthesizeMouseForUnhandledTouchEvents
    QTest::newRow("no synth") << false;
    QTest::newRow("synth") << true;
}

void tst_PointerHandlers::touchEventDelivery()
{
    QFETCH(bool, synthMouse);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, synthMouse);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "singleitem.qml");
    QQuickView * window = windowPtr.data();

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    auto devPriv = QPointingDevicePrivate::get(touchDevice);
    // In Qt 5, QQItem::acceptTouchEvents(true) was the default, but not in Qt 6.
    // This test is written to expect touch events on eventItem1.
    // acceptTouch = false just tells it to reject the events as they come.
    eventItem1->setAcceptTouchEvents(true);

    // Reject incoming mouse and touch events
    QPoint p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events from touch press" << eventItem1->eventList;
    QTRY_COMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(1, Event::TouchDestination, QEvent::TouchBegin, QEventPoint::State::Pressed, NoGrab);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch move" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 3); // no grabs -> only the handler gets the update
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(eventItem1->eventList.size() - 1, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Released, NoGrab);
    eventItem1->eventList.clear();

    // Accept touch events only
    eventItem1->acceptTouch = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(1, Event::TouchDestination, QEvent::TouchBegin, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, eventItem1);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(2, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Updated, NoGrab);
    QCOMPARE_EVENT(3, Event::TouchDestination, QEvent::TouchUpdate, QEventPoint::State::Updated, QPointingDevice::GrabExclusive);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE_EVENT(4, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Released, NoGrab);
    QCOMPARE_EVENT(5, Event::TouchDestination, QEvent::TouchEnd, QEventPoint::State::Released, NoGrab);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse events only
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch press" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(1, Event::TouchDestination, QEvent::TouchBegin, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, nullptr);

    QPointF localPos = eventItem1->mapFromScene(p1);
    QPointF scenePos = p1; // item is at 0,0
    QCOMPARE(eventItem1->eventList.at(1).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(1).posWrtScene, scenePos);

    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch move" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 3);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(3, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Released, NoGrab);
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
    qCDebug(lcPointerTests) << "events after touch press" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(1, Event::TouchDestination, QEvent::TouchBegin, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, nullptr);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch move" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 3);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 4);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept touch events
    eventItem1->acceptTouch = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(1, Event::TouchDestination, QEvent::TouchBegin, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(2, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Updated, NoGrab);
    QCOMPARE_EVENT(3, Event::TouchDestination, QEvent::TouchUpdate, QEventPoint::State::Updated, QPointingDevice::GrabExclusive);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE_EVENT(4, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Released, NoGrab);
    QCOMPARE_EVENT(5, Event::TouchDestination, QEvent::TouchEnd, QEventPoint::State::Released, NoGrab);
    eventItem1->eventList.clear();

    // Accept pointer events
    // TODO acceptPointer currently does nothing unique, but we could
    // re-enable this test if we define the best way to handle generic QPointerEvents in Item subclasses
    /*
    eventItem1->acceptPointer = true;
    eventItem1->grabPointer = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::None, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    QCOMPARE_EVENT(1, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(2, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Updated, QPointingDevice::GrabExclusive);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(eventItem1->eventList.size(), 5);
    qCDebug(lcPointerTests) << eventItem1->eventList;
    QCOMPARE_EVENT(3, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Released, QPointingDevice::GrabExclusive);
    QCOMPARE_EVENT(4, Event::HandlerDestination, QEvent::None, QEventPoint::State::Released, QPointingDevice::UngrabExclusive);
    eventItem1->eventList.clear();
    */
}

void tst_PointerHandlers::mouseEventDelivery()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "singleitem.qml");
    QQuickView * window = windowPtr.data();

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    EventHandler *handler = window->rootObject()->findChild<EventHandler*>("eventHandler");
    QVERIFY(handler);
    QCOMPARE(handler->parentItem(), eventItem1);
    QCOMPARE(handler->target(), eventItem1);
    QVERIFY(QQuickItemPrivate::get(eventItem1)->extra->resourcesList.contains(handler));

    // Do not accept anything
    QPoint p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    qCDebug(lcPointerTests) << "events after mouse press" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 3); // handler: hover; handler: press; item: press
    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    qCDebug(lcPointerTests) << "events after mouse move" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 4); // handler: hover
    QTest::mouseRelease(window, Qt::LeftButton);
    qCDebug(lcPointerTests) << "events after mouse release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 4);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    qCDebug(lcPointerTests) << "events after mouse press" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(1, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(2, Event::MouseDestination, QEvent::MouseButtonPress, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    QCOMPARE(window->mouseGrabberItem(), eventItem1);

    QPointF localPos = eventItem1->mapFromScene(p1);
    QPointF scenePos = p1; // item is at 0,0
    QCOMPARE(eventItem1->eventList.at(0).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(0).posWrtScene, scenePos);
    QCOMPARE(eventItem1->eventList.at(1).posWrtItem, localPos);
    QCOMPARE(eventItem1->eventList.at(1).posWrtScene, scenePos);

    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    qCDebug(lcPointerTests) << "events after mouse move" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 5);
    QCOMPARE_EVENT(4, Event::MouseDestination, QEvent::MouseMove, QEventPoint::State::Updated, QPointingDevice::GrabExclusive);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    qCDebug(lcPointerTests) << "events after mouse release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 8);
    QCOMPARE_EVENT(eventItem1->eventList.size() - 2, Event::MouseDestination, QEvent::MouseButtonRelease, QEventPoint::State::Released, NoGrab);
    QCOMPARE_EVENT(eventItem1->eventList.size() - 1, Event::MouseDestination, QEvent::UngrabMouse, QEventPoint::State::Released, QPointingDevice::UngrabExclusive);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Grab pointer events
    // TODO acceptPointer currently does nothing unique, but we could
    // re-enable this test if we define the best way to handle generic QPointerEvents in Item subclasses
    /*
    eventItem1->acceptMouse = false;
    eventItem1->acceptPointer = true;
    eventItem1->grabPointer = true;
    p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::None, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    QCOMPARE_EVENT(1, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, QPointingDevice::GrabExclusive);
    QCOMPARE_EVENT(2, Event::MouseDestination, QEvent::MouseButtonPress, QEventPoint::State::Pressed, 0);
    p1 += QPoint(10, 0);
    QTest::mouseMove(window, p1);
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE_EVENT(3, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Updated, QPointingDevice::GrabExclusive);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 6);
    QCOMPARE_EVENT(4, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Released, QPointingDevice::GrabExclusive);
    QCOMPARE_EVENT(5, Event::HandlerDestination, QEvent::None, QEventPoint::State::Released, QPointingDevice::UngrabExclusive);
    eventItem1->eventList.clear();
    */
}

void tst_PointerHandlers::touchReleaseOutside_data()
{
    QTest::addColumn<bool>("acceptPointer");
    QTest::addColumn<bool>("grabPointer");
    QTest::addColumn<int>("eventCount");
    QTest::addColumn<int>("endIndexToTest");
    QTest::addColumn<int>("endDestination");    // Event::Destination
    QTest::addColumn<int>("endType");           // QEvent::Type
    QTest::addColumn<int>("endState");          // QEventPoint::State
    QTest::addColumn<int>("endGrabState");      // QEventPoint::State

    QTest::newRow("reject and ignore") << false << false << 6 << 5 << (int)Event::TouchDestination
        << (int)QEvent::TouchEnd << (int)QEventPoint::State::Released << (int)NoGrab;
    QTest::newRow("reject and grab") << false << true << 5 << 4 << (int)Event::HandlerDestination
        << (int)QEvent::None << (int)QEventPoint::State::Released << (int)QPointingDevice::UngrabExclusive;
    QTest::newRow("accept and ignore") << true << false << 1 << 0 << (int)Event::HandlerDestination
        << (int)QEvent::Pointer << (int)QEventPoint::State::Pressed << (int)NoGrab;
    QTest::newRow("accept and grab") << true << true << 5 << 4 << (int)Event::HandlerDestination
        << (int)QEvent::None << (int)QEventPoint::State::Released << (int)QPointingDevice::UngrabExclusive;
}

void tst_PointerHandlers::touchReleaseOutside()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "singleitem.qml");
    QQuickView * window = windowPtr.data();

    QFETCH(bool, acceptPointer);
    QFETCH(bool, grabPointer);
    QFETCH(int, eventCount);
    QFETCH(int, endIndexToTest);
    QFETCH(int, endDestination);
    QFETCH(int, endType);
    QFETCH(int, endState);
    QFETCH(int, endGrabState);

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);

    eventItem1->acceptTouch = true;
    eventItem1->setAcceptTouchEvents(true);
    eventItem1->acceptPointer = acceptPointer;
    eventItem1->grabPointer = grabPointer;

    QPoint p1 = QPoint(20, 20);
    QTest::touchEvent(window, touchDevice).press(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch press" << eventItem1->eventList;
    p1.setX(eventItem1->mapToScene(eventItem1->clipRect().bottomRight()).x() + 10);
    QTest::touchEvent(window, touchDevice).move(0, p1, window);
    QTest::touchEvent(window, touchDevice).release(0, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "events after touch release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), eventCount);
    QCOMPARE_EVENT(endIndexToTest, endDestination, endType, endState, endGrabState);
}

void tst_PointerHandlers::dynamicCreation()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dynamicallyCreated.qml");
    QQuickView * window = windowPtr.data();

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventHandler *handler = window->rootObject()->findChild<EventHandler*>("eventHandler");
    QVERIFY(handler);

    QCOMPARE(handler->parentItem(), eventItem1);
    QCOMPARE(handler->target(), eventItem1);
    QVERIFY(QQuickItemPrivate::get(eventItem1)->extra->resourcesList.contains(handler));

    QPoint p1(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE_EVENT(0, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Updated, NoGrab);
    QCOMPARE_EVENT(1, Event::HandlerDestination, QEvent::Pointer, QEventPoint::State::Pressed, NoGrab);
    QCOMPARE_EVENT(2, Event::MouseDestination, QEvent::MouseButtonPress, QEventPoint::State::Pressed, NoGrab);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
}

void tst_PointerHandlers::handlerInWindow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("handlerInWindow.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow*>(component.create());
    QScopedPointer<QQuickWindow> cleanup(window);
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    EventHandler *handler = window->contentItem()->findChild<EventHandler*>("eventHandler");
    QVERIFY(handler);

    QCOMPARE(handler->parentItem(), window->contentItem());
    QCOMPARE(handler->target(), window->contentItem());

    QPoint p1(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(handler->pressEventCount, 1);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(handler->releaseEventCount, 1);
}

void tst_PointerHandlers::dynamicCreationInWindow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("dynamicallyCreatedInWindow.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow*>(component.create());
    QScopedPointer<QQuickWindow> cleanup(window);
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    EventHandler *handler = window->contentItem()->findChild<EventHandler*>("eventHandler");
    QVERIFY(handler);

    QCOMPARE(handler->parentItem(), window->contentItem());
    QCOMPARE(handler->target(), window->contentItem());

    QPoint p1(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(handler->pressEventCount, 1);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(handler->releaseEventCount, 1);
}

void tst_PointerHandlers::cppConstruction()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "itemOnly.qml");
    QQuickView * window = windowPtr.data();

    EventItem *eventItem1 = window->rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    QQuickItemPrivate *eventItemPriv = QQuickItemPrivate::get(eventItem1);
    // tell the handler to grab on press
    eventItem1->grabPointer = true;

    EventHandler handler(eventItem1);
    QCOMPARE(handler.parentItem(), eventItem1);
    QCOMPARE(handler.target(), eventItem1);
    QCOMPARE(eventItemPriv->extra->pointerHandlers.first(), &handler);
    QVERIFY(eventItemPriv->extra->resourcesList.contains(&handler));

    // the handler and then eventItem1 sees each event
    QPoint p1 = QPoint(20, 20);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(handler.pressEventCount, 1);
    qCDebug(lcPointerTests) << "events after mouse press" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 3);
    QTest::mouseRelease(window, Qt::LeftButton);
    QCOMPARE(handler.releaseEventCount, 1);
    qCDebug(lcPointerTests) << "events after mouse release" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 7);

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);
}

void tst_PointerHandlers::reparenting()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "reparenting.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *topItem = window->rootObject()->findChild<QQuickItem*>("top");
    QVERIFY(topItem);
    QQuickItem *bottomItem = window->rootObject()->findChild<QQuickItem*>("bottom");
    QVERIFY(bottomItem);
    EventHandler *handler = window->rootObject()->findChild<EventHandler*>("eventHandler");
    QVERIFY(handler);
    topItem->setAcceptedMouseButtons(Qt::RightButton);

    for (int i = 1; i < 5; ++i) {
        QQuickItem *expectedParentItem = (i % 2 ? topItem : bottomItem);
        QQuickItem *unexpectedParentItem = (i % 2 ? bottomItem : topItem);
        qCDebug(lcPointerTests) << "initial parent" << handler->parentItem() << "waiting for" << expectedParentItem;
        QTRY_COMPARE(handler->parentItem(), expectedParentItem);
        QCOMPARE(handler->target(), expectedParentItem);
        QVERIFY(QQuickItemPrivate::get(expectedParentItem)->extra.isAllocated());
        QVERIFY(QQuickItemPrivate::get(expectedParentItem)->extra->resourcesList.contains(handler));
        if (QQuickItemPrivate::get(unexpectedParentItem)->extra.isAllocated())
            QVERIFY(!QQuickItemPrivate::get(unexpectedParentItem)->extra->resourcesList.contains(handler));
        QCOMPARE(expectedParentItem->acceptedMouseButtons(), Qt::AllButtons);
        QCOMPARE(unexpectedParentItem->acceptedMouseButtons(), unexpectedParentItem == topItem ? Qt::RightButton : Qt::NoButton);

        QPoint pt = expectedParentItem->mapToScene(expectedParentItem->boundingRect().center()).toPoint();
        qCDebug(lcPointerTests) << "click @" << pt;
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, pt);
        QTRY_COMPARE(handler->pressEventCount, i);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, pt);
        QTRY_COMPARE(handler->releaseEventCount, i);
    }
}

/*!
    Verify that removing an item that has a grabbing handler from the scene
    does not result in crashes in our event dispatching code. The item's window()
    pointer will be nullptr, so the handler must have released the grab, or never
    gotten the grab, depending on when the item gets removed.

    See QTBUG-114475.
*/
void tst_PointerHandlers::grabberSceneChange_data()
{
    QTest::addColumn<bool>("useTimer");
    QTest::addColumn<int>("grabChangedCount");

    QTest::addRow("Immediately") << false << 0;
    QTest::addRow("Delayed") << true << 2;
}

void tst_PointerHandlers::grabberSceneChange()
{
    QFETCH(const bool, useTimer);
    QFETCH(const int, grabChangedCount);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("grabberSceneChange.qml"));
    QQuickWindow *window = qobject_cast<QQuickWindow*>(component.create());
    QScopedPointer<QQuickWindow> cleanup(window);
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    window->setProperty("useTimer", useTimer);

    QQuickItem *container = window->findChild<QQuickItem *>("container");

    QPoint p1 = QPoint(window->width() / 2, window->height() / 2);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    // The container gets removed from this window, either immediately on
    // press, or through a timer.
    QTRY_COMPARE(container->parentItem(), nullptr);

    QEXPECT_FAIL("Delayed",
                 "PointerHandlers don't release their grab when item is removed", Continue);
    QCOMPARE(window->property("grabChangedCounter").toInt(), grabChangedCount);

    // this should not crash
    QTest::mouseMove(window, p1 + QPoint(5, 5));
}

void tst_PointerHandlers::clip()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "clip.qml");
    QQuickView * window = windowPtr.data();
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    EventHandler *handler = window->contentItem()->findChild<EventHandler*>("eventHandler");
    EventHandler *circleHandler = window->contentItem()->findChild<EventHandler*>("circle eventHandler");

    QCOMPARE(handler->pressEventCount, 0);
    QCOMPARE(circleHandler->pressEventCount, 0);
    QCOMPARE(handler->releaseEventCount, 0);
    QCOMPARE(circleHandler->releaseEventCount, 0);

    const QPoint rectPt = QPoint(1, 1);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, rectPt);
    QCOMPARE(handler->pressEventCount, 1);
    QCOMPARE(circleHandler->pressEventCount, 0);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, rectPt);
    QCOMPARE(handler->releaseEventCount, 1);
    QCOMPARE(circleHandler->releaseEventCount, 0);


    handler->pressEventCount = 0;
    circleHandler->pressEventCount = 0;
    handler->releaseEventCount = 0;
    circleHandler->releaseEventCount = 0;

    const QPoint rectAndCirclePt = QPoint(49 ,49);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, rectAndCirclePt);
    QCOMPARE(handler->pressEventCount, 1);
    QCOMPARE(circleHandler->pressEventCount, 1);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, rectAndCirclePt);
    QCOMPARE(handler->releaseEventCount, 1);
    QCOMPARE(circleHandler->releaseEventCount, 1);


    handler->pressEventCount = 0;
    circleHandler->pressEventCount = 0;
    handler->releaseEventCount = 0;
    circleHandler->releaseEventCount = 0;

    const QPoint circlePt = QPoint(51 ,51);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, circlePt);
    QCOMPARE(handler->pressEventCount, 0);
    QCOMPARE(circleHandler->pressEventCount, 1);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, circlePt);
    QCOMPARE(handler->releaseEventCount, 0);
    QCOMPARE(circleHandler->releaseEventCount, 1);
}

QTEST_MAIN(tst_PointerHandlers)

#include "tst_qquickpointerhandler.moc"

