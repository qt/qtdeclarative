// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QDebug>

#include <QtGui/qstylehints.h>
#include <private/qdebug_p.h>
#include <QtGui/private/qpointingdevice_p.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickmultipointtoucharea_p.h>
#include <QtQuick/private/qquickpincharea_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickwindow_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

struct Event
{
    Event(QEvent::Type t, QPoint mouse, QPoint global)
        :type(t), mousePos(mouse), mousePosGlobal(global)
    {}

    Event(QEvent::Type t, const QList<QEventPoint> &touch)
        :type(t)
    {
        for (auto &tp : touch)
            points << tp;
    }

    QEvent::Type type;
    QPoint mousePos;
    QPoint mousePosGlobal;
    QList<QEventPoint> points;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const struct Event &event) {
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "Event(";
    QtDebugUtils::formatQEnum(dbg, event.type);
    if (event.points.isEmpty())
        dbg << " @ " << event.mousePos << " global " << event.mousePosGlobal;
    else
        dbg << ", " << event.points.size() << " touchpoints: " << event.points;
    dbg << ')';
    return dbg;
}
#endif

class EventItem : public QQuickItem
{
    Q_OBJECT

Q_SIGNALS:
    void onTouchEvent(QQuickItem *receiver);

public:
    EventItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    void touchEvent(QTouchEvent *event) override
    {
        qCDebug(lcTests) << event << "accepting?" << acceptTouch;
        eventList.append(Event(event->type(), event->points()));
        Q_ASSERT(event->pointCount() > 0);
        point0 = event->point(0).id();
        event->setAccepted(acceptTouch);
        emit onTouchEvent(this);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event << "accepting?" << acceptMouse;
        eventList.append(Event(event->type(), event->position().toPoint(), event->globalPosition().toPoint()));
        mouseGrabber = event->exclusiveGrabber(event->points().first());
        event->setAccepted(acceptMouse);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event << "accepting?" << acceptMouse;
        eventList.append(Event(event->type(), event->position().toPoint(), event->globalPosition().toPoint()));
        mouseGrabber = event->exclusiveGrabber(event->points().first());
        event->setAccepted(acceptMouse);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event << "accepting?" << acceptMouse;
        eventList.append(Event(event->type(), event->position().toPoint(), event->globalPosition().toPoint()));
        mouseGrabber = event->exclusiveGrabber(event->points().first());
        event->setAccepted(acceptMouse);
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event << "accepting?" << acceptMouse;
        eventList.append(Event(event->type(), event->position().toPoint(), event->globalPosition().toPoint()));
        mouseGrabber = event->exclusiveGrabber(event->points().first());
        event->setAccepted(acceptMouse);
    }

    void mouseUngrabEvent() override
    {
        qCDebug(lcTests);
        eventList.append(Event(QEvent::UngrabMouse, QPoint(0,0), QPoint(0,0)));
        mouseGrabber = nullptr;
    }

    void touchUngrabEvent() override
    {
        qCDebug(lcTests);
        ++touchUngrabCount;
    }

    void dumpEventList()
    {
        for (const auto &event : eventList)
            qDebug() << event;
    }

    bool event(QEvent *event) override {
        return QQuickItem::event(event);
    }

    QList<Event> eventList;
    QObject *mouseGrabber = nullptr;
    int touchUngrabCount = 0;
    bool acceptMouse = false;
    bool acceptTouch = false;
    bool filterTouch = false; // when used as event filter

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::TouchBegin ||
                event->type() == QEvent::TouchUpdate ||
                event->type() == QEvent::TouchCancel ||
                event->type() == QEvent::TouchEnd) {
            qCDebug(lcTests) << event;
            QTouchEvent *touch = static_cast<QTouchEvent*>(event);
            eventList.append(Event(event->type(), touch->points()));
            Q_ASSERT(touch->pointCount() > 0);
            point0 = touch->point(0).id();
            if (filterTouch)
                event->accept();
            return true;
        }
        return false;
    }
    int point0 = -1;
};

class GrabMonitor : public QObject
{
public:
    QObject *exclusiveGrabber = nullptr;
    int transitionCount = 0;
    bool fromMouseEvent = false;
    bool canceled = false;

    void reset()
    {
        exclusiveGrabber = nullptr;
        transitionCount = 0;
        fromMouseEvent = false;
        canceled = false;
    }

    void onGrabChanged(QObject *grabber, QPointingDevice::GrabTransition transition, const QPointerEvent *event, const QEventPoint &point)
    {
        qCDebug(lcTests) << grabber << transition << event << point << point.device();
        ++transitionCount;
        switch (transition) {
        case QPointingDevice::GrabTransition::GrabExclusive:
            exclusiveGrabber = grabber;
            fromMouseEvent = event && QQuickDeliveryAgentPrivate::isMouseEvent(event);
            canceled = false;
            break;
        case QPointingDevice::GrabTransition::UngrabExclusive:
            exclusiveGrabber = nullptr;
            canceled = false;
            break;
        case QPointingDevice::GrabTransition::CancelGrabExclusive:
            exclusiveGrabber = nullptr;
            canceled = true;
            break;
        default:
            // ignore the passive grabs since this test doesn't involve pointer handlers
            break;
        }
    }
};

class tst_TouchMouse : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_TouchMouse()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void initTestCase() override;

    void simpleTouchEvent_data();
    void simpleTouchEvent();
    void testEventFilter();
    void mouse();
    void touchOverMouse();
    void mouseOverTouch();

    void buttonOnFlickable();
    void touchButtonOnFlickable();
    void buttonOnDelayedPressFlickable_data();
    void buttonOnDelayedPressFlickable();
    void buttonOnTouch();

    void pinchOnFlickable();
    void flickableOnPinch();
    void mouseOnFlickableOnPinch();

    void tapOnDismissiveTopMouseAreaClicksBottomOne();

    void touchGrabCausesMouseUngrab();
    void touchPointDeliveryOrder();

    void hoverEnabled();
    void implicitUngrab();
    void touchCancelWillCancelMousePress();

    void oneTouchInsideAndOneOutside();

    void strayTouchDoesntAutograb();

    void noDoubleClickWithInterveningTouch();

protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->isPointerEvent()) {
            qCDebug(lcTests) << "window filtering" << event;
            QPointerEvent *pe = static_cast<QPointerEvent*>(event);
            filteredEventList.append(Event(pe->type(),
                                           pe->points().first().position().toPoint(),
                                           pe->points().first().globalPosition().toPoint()));
        }
        return false;
    }

private:
    QQuickView *createView();
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
    QList<Event> filteredEventList;
    GrabMonitor grabMonitor;
};

QQuickView *tst_TouchMouse::createView()
{
    QQuickView *window = new QQuickView(nullptr);
    return window;
}

void tst_TouchMouse::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<EventItem>("Qt.test", 1, 0, "EventItem");
    connect(touchscreen.get(), &QPointingDevice::grabChanged, &grabMonitor, &GrabMonitor::onGrabChanged);
}

void tst_TouchMouse::simpleTouchEvent_data()
{
    QTest::addColumn<bool>("synthMouse"); // AA_SynthesizeMouseForUnhandledTouchEvents
    QTest::newRow("no synth") << false;
    QTest::newRow("synth") << true;
}

void tst_TouchMouse::simpleTouchEvent()
{
    QFETCH(bool, synthMouse);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, synthMouse);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("singleitem.qml")));

    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());

    // Do not accept touch or mouse
    eventItem1->setAcceptTouchEvents(false);
    QPoint p1;
    p1 = QPoint(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    // Get a synth-mouse event if allowed
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    if (synthMouse)
        QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    // Not accepted, no updates
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    eventItem1->eventList.clear();

    // Accept touch
    eventItem1->setAcceptTouchEvents(true);
    eventItem1->acceptTouch = true;
    p1 = QPoint(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 1);
    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptTouchEvents(false);
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    if (synthMouse)
        QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(devPriv->firstPointExclusiveGrabber(), synthMouse ? eventItem1 : nullptr);

    QPoint localPos = eventItem1->mapFromScene(p1).toPoint();
    QPoint globalPos = window.mapToGlobal(p1);
    if (synthMouse) {
        QCOMPARE(eventItem1->eventList.at(0).mousePos, localPos);
        QCOMPARE(eventItem1->eventList.at(0).mousePosGlobal, globalPos);
    }

    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 2 : 0);
    if (synthMouse)
        QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseMove);
    // else, if there was no synth-mouse and we didn't accept the touch,
    // TouchUpdate was not sent to eventItem1 either.
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 4 : 0);
    if (synthMouse) {
        QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseButtonRelease);
        QCOMPARE(eventItem1->eventList.at(3).type, QEvent::UngrabMouse);
    }
    // else, if there was no synth-mouse and we didn't accept the touch,
    // TouchEnd was not sent to eventItem1 either.
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept mouse buttons but not the event
    eventItem1->acceptTouch = false;
    eventItem1->acceptMouse = false;
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    if (synthMouse)
        QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), synthMouse ? 1 : 0);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Accept touch and mouse
    eventItem1->acceptTouch = true;
    eventItem1->setAcceptTouchEvents(true);
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    p1 = QPoint(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::TouchUpdate);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::TouchEnd);
    eventItem1->eventList.clear();
}

void tst_TouchMouse::testEventFilter()
{
//    // install event filter on item and see that it can grab events
//    QScopedPointer<QQuickView> window(createView());
//    window.setSource(testFileUrl("singleitem.qml"));
//    window.show();
//    QQuickVisualTestUtils::centerOnScreen(&window);
//    QVERIFY(QTest::qWaitForWindowActive(&window));
//    QVERIFY(window->rootObject() != 0);

//    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
//    QVERIFY(eventItem1);
//    eventItem1->acceptTouch = true;

//    EventItem *filter = new EventItem;
//    filter->filterTouch = true;
//    eventItem1->installEventFilter(filter);

//    QPoint p1 = QPoint(20, 20);
//    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
//    // QEXPECT_FAIL("", "We do not implement event filters correctly", Abort);
//    QCOMPARE(eventItem1->eventList.size(), 0);
//    QCOMPARE(filter->eventList.size(), 1);
//    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
//    QCOMPARE(eventItem1->eventList.size(), 0);
//    QCOMPARE(filter->eventList.size(), 2);

//    delete filter;
}

void tst_TouchMouse::mouse()
{
    // eventItem1
    //   - eventItem2

    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoitems.qml")));

    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    // bottom item likes mouse, top likes touch
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->setAcceptTouchEvents(false);
    eventItem1->acceptMouse = true;
    // item 2 doesn't accept anything, thus it sees a touch pass by
    eventItem2->setAcceptTouchEvents(false);
    QPoint p1 = QPoint(30, 30);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);

    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
}

void tst_TouchMouse::touchOverMouse()
{
    // eventItem1
    //   - eventItem2

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoitems.qml")));

    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    // bottom item likes mouse, top likes touch
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->acceptMouse = true;
    eventItem2->acceptTouch = false; // let it fall through
    eventItem2->setAcceptTouchEvents(true);

    QCOMPARE(eventItem1->eventList.size(), 0);
    QPoint p1 = QPoint(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "expected delivered events: press(mouse)" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 1);
    qCDebug(lcTests) << "expected delivered events: press(touch)" << eventItem2->eventList;
    QCOMPARE(eventItem2->eventList.size(), 1);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::TouchBegin);
    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "expected delivered events: press(mouse) move(mouse)" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 2);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseMove);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "expected delivered events: press(mouse) move(mouse) release(mouse) ungrab(mouse)" << eventItem1->eventList;
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseButtonRelease);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::UngrabMouse);
}

void tst_TouchMouse::mouseOverTouch()
{
    // eventItem1
    //   - eventItem2

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoitems.qml")));

    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    // bottom item likes mouse, top likes touch
    eventItem1->acceptTouch = true;
    eventItem2->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem2->acceptMouse = true;
    eventItem2->setAcceptTouchEvents(false);

    QPoint p1 = QPoint(20, 20);
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 0);
    QCOMPARE(eventItem2->eventList.size(), 1);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::MouseButtonPress);

    p1 += QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QCOMPARE(eventItem2->eventList.size(), 1);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    qCDebug(lcTests) << "expected delivered events: press(mouse) move(mouse) release(mouse) ungrab(mouse)" << eventItem2->eventList;
    QCOMPARE(eventItem2->eventList.size(), 4);
    eventItem2->eventList.clear();
}

void tst_TouchMouse::buttonOnFlickable()
{
    // flickable - height 500 / 1000
    //   - eventItem1 y: 100, height 100
    //   - eventItem2 y: 300, height 100

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("buttononflickable.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    // should a mouse area button be clickable on top of flickable? yes :)
    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->acceptMouse = true;
    eventItem1->setAcceptTouchEvents(false);

    // should a touch button be touchable on top of flickable? yes :)
    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);
    QCOMPARE(eventItem2->eventList.size(), 0);
    eventItem2->acceptTouch = true;
    eventItem2->setAcceptTouchEvents(true);

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // check that buttons are clickable
    // mouse button
    QCOMPARE(eventItem1->eventList.size(), 0);
    QPoint p1 = QPoint(20, 130);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonRelease);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::UngrabMouse);
    eventItem1->eventList.clear();

    // touch button
    p1 = QPoint(10, 310);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem2->eventList.size(), 1);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::TouchBegin);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem2->eventList.size(), 2);
    QCOMPARE(eventItem2->eventList.at(1).type, QEvent::TouchEnd);
    QCOMPARE(eventItem1->eventList.size(), 0);
    eventItem2->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // click above button, no events please
    p1 = QPoint(10, 90);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 0);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 0);
    eventItem1->eventList.clear();

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // check that flickable moves - mouse button
    QCOMPARE(eventItem1->eventList.size(), 0);
    p1 = QPoint(10, 110);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);

    QQuickWindowPrivate *windowPriv = QQuickWindowPrivate::get(&window);
    QVERIFY(windowPriv->deliveryAgentPrivate()->touchMouseId != -1);
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, eventItem1);
    QCOMPARE(grabMonitor.exclusiveGrabber, eventItem1);

    int dragDelta = -qApp->styleHints()->startDragDistance();

    // Start dragging; eventually, Flickable steals the grab
    int i = 0;
    for (; i < 10 && !flickable->isMovingVertically(); ++i) {
        p1 += QPoint(0, dragDelta);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
        QQuickTouchUtils::flush(&window);
    }
    QVERIFY(flickable->isMovingVertically());
    qCDebug(lcTests) << "flickable started moving after" << i << "moves, when we got to" << p1;
    QCOMPARE(eventItem1->eventList.size(), 4);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::MouseMove);
    QCOMPARE(eventItem1->eventList.at(3).type, QEvent::UngrabMouse);

    QCOMPARE(grabMonitor.exclusiveGrabber, flickable);
    QVERIFY(windowPriv->deliveryAgentPrivate()->touchMouseId != -1);
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, flickable);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
}

void tst_TouchMouse::touchButtonOnFlickable()
{
    // flickable - height 500 / 1000
    //   - eventItem1 y: 100, height 100
    //   - eventItem2 y: 300, height 100

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("buttononflickable.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);
    QCOMPARE(eventItem2->eventList.size(), 0);
    eventItem2->acceptTouch = true;
    eventItem2->setAcceptTouchEvents(true);

    // press via touch, then drag: check that flickable moves and that the button gets ungrabbed
    QCOMPARE(eventItem2->eventList.size(), 0);
    QPoint p1 = QPoint(10, 310);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem2->eventList.size(), 1);
    QCOMPARE(eventItem2->eventList.at(0).type, QEvent::TouchBegin);

    QQuickWindowPrivate *windowPriv = QQuickWindowPrivate::get(&window);
    QVERIFY(windowPriv->deliveryAgentPrivate()->touchMouseId == -1);
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, eventItem2);
    QCOMPARE(grabMonitor.exclusiveGrabber, eventItem2);

    int dragDelta = qApp->styleHints()->startDragDistance() * -0.7;
    p1 += QPoint(0, dragDelta);
    QPoint p2 = p1 + QPoint(0, dragDelta);
    QPoint p3 = p2 + QPoint(0, dragDelta);

    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p2, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p3, &window);
    QQuickTouchUtils::flush(&window);

    QTRY_COMPARE(eventItem2->touchUngrabCount, 1);
    qCDebug(lcTests) << "expected delivered events: press(touch) move(touch)" << eventItem2->eventList;
    QCOMPARE(eventItem2->eventList.size(), 3);
    QCOMPARE(eventItem2->eventList.at(1).type, QEvent::TouchUpdate);
    QCOMPARE(grabMonitor.exclusiveGrabber, flickable);
    // both EventItem and Flickable handled the actual touch, so synth-mouse doesn't happen
    QCOMPARE(windowPriv->deliveryAgentPrivate()->touchMouseId, -1);
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, flickable);
    QVERIFY(flickable->isMovingVertically());

    QTest::touchEvent(&window, touchscreen.get()).release(0, p3, &window);
    QQuickTouchUtils::flush(&window);
}

void tst_TouchMouse::buttonOnDelayedPressFlickable_data()
{
    QTest::addColumn<bool>("scrollBeforeDelayIsOver");
    QTest::addColumn<bool>("releaseBeforeDelayIsOver");

    // the item should never see the event,
    // due to the pressDelay which never delivers if we start moving
    QTest::newRow("scroll before press delay is over") << true << false;

    // after release, the item should see the press and release via event replay (QTBUG-61144)
    QTest::newRow("release before press delay is over") << false << true;

    // wait until the "button" sees the press but then
    // start moving: the button gets a press and cancel event
    QTest::newRow("scroll after press delay is over") << false << false;
}

void tst_TouchMouse::buttonOnDelayedPressFlickable()
{
    // flickable - height 500 / 1000
    //   - eventItem1 y: 100, height 100
    //   - eventItem2 y: 300, height 100
    QFETCH(bool, scrollBeforeDelayIsOver);
    QFETCH(bool, releaseBeforeDelayIsOver);
    const int threshold = qApp->styleHints()->startDragDistance();

    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
    filteredEventList.clear();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("buttononflickable.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    window.installEventFilter(this);

    // wait 600 ms before letting the child see the press event
    flickable->setPressDelay(600);

    // should a mouse area button be clickable on top of flickable? yes :)
    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->acceptMouse = true;

    // should a touch button be touchable on top of flickable? yes :)
    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);
    QCOMPARE(eventItem2->eventList.size(), 0);
    eventItem2->acceptTouch = true;

    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);
    QQuickWindowPrivate *windowPriv = QQuickWindowPrivate::get(&window);
    QCOMPARE(windowPriv->deliveryAgentPrivate()->touchMouseId, -1); // no grabber

    // touch press
    QPoint p1 = QPoint(10, 110);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);

    if (scrollBeforeDelayIsOver || releaseBeforeDelayIsOver) {
        // no events yet: press is delayed
        QCOMPARE(eventItem1->eventList.size(), 0);
    } else {
        // wait until the button sees the press
        qCDebug(lcTests) << "expected delivered events: press(mouse)" << eventItem1->eventList;
        qCDebug(lcTests) << "expected filtered events: actual TouchBegin and replayed TouchBegin" << filteredEventList;
        QTRY_COMPARE(eventItem1->eventList.size(), 1);
        QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
        QCOMPARE(filteredEventList.size(), 2); // actual touch begin and replayed touch begin
    }

    if (!releaseBeforeDelayIsOver) {
        // move the touchpoint: try to flick
        for (int i = 0; i < 3; ++i) {
            p1 += QPoint(0, -threshold);
            QTest::touchEvent(&window, touchscreen.get()).move(0, p1, &window);
            QQuickTouchUtils::flush(&window);
        }
        QTRY_VERIFY(flickable->isMovingVertically());

        if (scrollBeforeDelayIsOver) {
            QCOMPARE(eventItem1->eventList.size(), 0);
            qCDebug(lcTests) << "expected filtered events: 1 TouchBegin and 3 TouchUpdate" << filteredEventList;
            QCOMPARE(filteredEventList.size(), 4);
        } else {
            qCDebug(lcTests) << "expected delivered events: press(mouse), move(mouse), move(mouse), ungrab(mouse)" << eventItem1->eventList;
            QCOMPARE(eventItem1->eventList.size(), 4);
            QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
            QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseMove);
            QCOMPARE(eventItem1->eventList.last().type, QEvent::UngrabMouse);
            qCDebug(lcTests) << "expected filtered events: 2 TouchBegin and 3 TouchUpdate" << filteredEventList;
            QCOMPARE(filteredEventList.size(), 5);
        }

        // flickable should have the touchpoint grab: it no longer relies on synth-mouse
        QCOMPARE(grabMonitor.exclusiveGrabber, flickable);
        auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
        QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, flickable);
    }

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);

    if (releaseBeforeDelayIsOver) {
        // when the touchpoint was released, the child saw the delayed press and the release in sequence
        qCDebug(lcTests) << "expected delivered events: press(mouse), release(mouse), ungrab" << eventItem1->eventList;
        qCDebug(lcTests) << "expected filtered events: press(touch), release(touch), delayed press(touch), release (touch)" << filteredEventList;
        QTRY_COMPARE(eventItem1->eventList.size(), 3);
        QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
        QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonRelease);
        QCOMPARE(eventItem1->eventList.last().type, QEvent::UngrabMouse);
        // QQuickWindow filters the delayed press and release
        QCOMPARE(filteredEventList.size(), 4);
        QCOMPARE(filteredEventList.at(filteredEventList.size() - 2).type, QEvent::TouchBegin);
        QCOMPARE(filteredEventList.last().type, QEvent::TouchEnd);
    } else {
        // QQuickWindow filters the delayed press if there was one
        if (scrollBeforeDelayIsOver) {
            qCDebug(lcTests) << "expected filtered events: 1 TouchBegin, 3 TouchUpdate, 1 TouchEnd" << filteredEventList;
            QCOMPARE(filteredEventList.size(), 5);
        } else {
            qCDebug(lcTests) << "expected filtered events: 2 TouchBegin, 3 TouchUpdate, 1 TouchEnd" << filteredEventList;
            QCOMPARE(filteredEventList.size(), 6);
            QCOMPARE(filteredEventList.at(0).type, QEvent::TouchBegin);
            QCOMPARE(filteredEventList.last().type, QEvent::TouchEnd);
        }
    }
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

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("buttonontouch.qml")));

    QQuickPinchArea *pinchArea = window.rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickItem *button1 = window.rootObject()->findChild<QQuickItem*>("button1");
    QVERIFY(button1);
    EventItem *eventItem1 = window.rootObject()->findChild<EventItem*>("eventItem1");
    QVERIFY(eventItem1);
    EventItem *eventItem2 = window.rootObject()->findChild<EventItem*>("eventItem2");
    QVERIFY(eventItem2);

    QQuickMultiPointTouchArea *touchArea = window.rootObject()->findChild<QQuickMultiPointTouchArea*>("toucharea");
    QVERIFY(touchArea);
    EventItem *eventItem3 = window.rootObject()->findChild<EventItem*>("eventItem3");
    QVERIFY(eventItem3);
    EventItem *eventItem4 = window.rootObject()->findChild<EventItem*>("eventItem4");
    QVERIFY(eventItem4);

    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(&window, touchscreen.get(), false);

    // Test the common case of a mouse area on top of pinch
    eventItem1->setAcceptedMouseButtons(Qt::LeftButton);
    eventItem1->setAcceptTouchEvents(false);
    eventItem1->acceptMouse = true;


    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Normal touch click
    QPoint p1 = QPoint(10, 110);
    touchSeq.press(0, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    touchSeq.release(0, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonRelease);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::UngrabMouse);
    eventItem1->eventList.clear();

    // Normal mouse click
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(eventItem1->eventList.size(), 3);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(eventItem1->eventList.at(1).type, QEvent::MouseButtonRelease);
    QCOMPARE(eventItem1->eventList.at(2).type, QEvent::UngrabMouse);
    eventItem1->eventList.clear();

    // Pinch starting on the PinchArea should work
    p1 = QPoint(40, 10);
    QPoint p2 = QPoint(60, 10);

    // Start the events after each other
    touchSeq.press(0, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    touchSeq.stationary(0).press(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);

    QCOMPARE(button1->scale(), 1);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    touchSeq.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    touchSeq.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(button1->scale(), 1.5);
    qCDebug(lcTests) << "Button scale: " << button1->scale();

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    touchSeq.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(button1->scale(), 2);
    qCDebug(lcTests) << "Button scale: " << button1->scale();

    touchSeq.release(0, p1, &window).release(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QVERIFY(eventItem1->eventList.isEmpty());
    QCOMPARE(button1->scale(), 2);
    qCDebug(lcTests) << "Button scale: " << button1->scale();


    // wait to avoid getting a double click event
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    // Start pinching while on the button
    button1->setScale(1.0);
    p1 = QPoint(40, 110);
    p2 = QPoint(60, 110);
    touchSeq.press(0, p1, &window).press(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(button1->scale(), 1);
    QCOMPARE(eventItem1->eventList.size(), 1);
    QCOMPARE(eventItem1->eventList.at(0).type, QEvent::MouseButtonPress);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    touchSeq.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    touchSeq.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "Button scale: " << button1->scale();
    QEXPECT_FAIL("", "No pinch: eventItem1 grabbed both touchpoints", Continue);
    QCOMPARE(button1->scale(), 1.5);

    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    touchSeq.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "Button scale: " << button1->scale();
    QEXPECT_FAIL("", "No pinch: eventItem1 grabbed both touchpoints", Continue);
    QCOMPARE(button1->scale(), 2);

    touchSeq.release(0, p1, &window).release(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(eventItem1->eventList.size(), 6);
    qCDebug(lcTests) << "Button scale: " << button1->scale();
    QEXPECT_FAIL("", "No pinch: eventItem1 grabbed both touchpoints", Continue);
    QCOMPARE(button1->scale(), 2);
}

void tst_TouchMouse::pinchOnFlickable()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pinchonflickable.qml")));
    const int threshold = qApp->styleHints()->startDragDistance();

    QQuickPinchArea *pinchArea = window.rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);
    QQuickItem *rect = window.rootObject()->findChild<QQuickItem*>("rect");
    QVERIFY(rect);

    // flick the flickable with one touchpoint
    QCOMPARE(flickable->contentX(), 0);
    QPoint p = QPoint(100, 100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->position(), QPointF(200, 200));
    for (int i = 0; i < 4; ++i) {
        p -= QPoint(threshold, 0);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
        QQuickTouchUtils::flush(&window);
        if (!flickable->isAtXBeginning()) // currently happens when i == 3
            qCDebug(lcTests, "flicking after %d moves: %lf", i + 1, flickable->contentX());
    }
    QTest::touchEvent(&window, touchscreen.get()).release(0, p, &window);
    QTRY_COMPARE(flickable->isAtXBeginning(), false);
    // wait until flicking is done
    QTRY_COMPARE(flickable->isFlicking(), false);

    // pinch with two touchpoints
    QPoint p1 = QPoint(40, 20);
    QPoint p2 = QPoint(60, 20);
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&window, touchscreen.get());
    QQuickTouchUtils::flush(&window);
    pinchSequence.press(0, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and PinchArea will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->scale(), 1);
    for (int i = 0; i < 3; ++i) {
        p1 -= QPoint(threshold, 0);
        p2 += QPoint(threshold, 0);
        pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
        QQuickTouchUtils::flush(&window);
        qCDebug(lcTests, "pinch scale after %d moves: %lf", i + 1, rect->scale());
    }
    QVERIFY(!flickable->isDragging());
    QQuickTouchUtils::flush(&window);
    pinchSequence.release(0, p1, &window).release(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QVERIFY(rect->scale() > 1); // depends on threshold
}

void tst_TouchMouse::flickableOnPinch()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickableonpinch.qml")));

    QQuickPinchArea *pinchArea = window.rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);
    QQuickItem *rect = window.rootObject()->findChild<QQuickItem*>("rect");
    QVERIFY(rect);

    // flickable - single touch point
    QCOMPARE(flickable->contentX(), 0.0);
    QPoint p = QPoint(100, 100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->position(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    p -= QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
    QQuickTouchUtils::flush(&window);

    QTest::qWait(1000);

    p -= QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p, &window);
    QQuickTouchUtils::flush(&window);

    QTest::qWait(1000);

    //QVERIFY(flickable->isMovingHorizontally());
    qCDebug(lcTests) << "Pos: " << rect->position();
    // wait until flicking is done
    QTRY_VERIFY(!flickable->isFlicking());

    // pinch
    QPoint p1 = QPoint(40, 20);
    QPoint p2 = QPoint(60, 20);
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&window, touchscreen.get());
    pinchSequence.press(0, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and PinchArea will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->scale(), 1.0);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    pinchSequence.release(0, p1, &window).release(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QVERIFY(rect->scale() > 1.0);
}

void tst_TouchMouse::mouseOnFlickableOnPinch()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("mouseonflickableonpinch.qml")));

    QQuickPinchArea *pinchArea = window.rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QVERIFY(pinchArea);
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);
    QQuickItem *rect = window.rootObject()->findChild<QQuickItem*>("rect");
    QVERIFY(rect);

    // flickable - single touch point
    QCOMPARE(flickable->contentX(), 0.0);
    QPoint p = QPoint(100, 100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->position(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    p -= QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    p -= QPoint(10, 0);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p, &window);
    QQuickTouchUtils::flush(&window);

    QVERIFY(flickable->isMovingHorizontally());

    // Wait for flick to end
    QTRY_VERIFY(!flickable->isMoving());
    qCDebug(lcTests) << "Pos: " << rect->position();

    // pinch
    QPoint p1 = QPoint(40, 20);
    QPoint p2 = QPoint(60, 20);
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&window, touchscreen.get());
    pinchSequence.press(0, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and PinchArea will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->scale(), 1.0);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(10, 0);
    p2 += QPoint(10, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    pinchSequence.release(0, p1, &window).release(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QVERIFY(rect->scale() > 1.0);
    // first pinch is done

    // Press one point and drag left: Flickable flicks
    rect->setScale(1.0);
    flickable->setContentX(0.0);
    p = QPoint(100, 100);
    pinchSequence.press(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->position(), QPointF(200.0, 200.0));
    p -= QPoint(10, 0);
    pinchSequence.move(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);
    p -= QPoint(10, 0);
    pinchSequence.move(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);
    QGuiApplication::processEvents();
    p -= QPoint(10, 0);
    pinchSequence.move(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);

    // Add a second finger: PinchArea should grab one touchpoint and steal the other, even though flicking is ongoing
    p1 = QPoint(40, 100);
    p2 = QPoint(60, 100);
    pinchSequence.stationary(0).press(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(rect->scale(), 1.0);

    p1 -= QPoint(5, 0);
    p2 += QPoint(5, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(5, 0);
    p2 += QPoint(5, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    p1 -= QPoint(5, 0);
    p2 += QPoint(5, 0);
    pinchSequence.move(0, p1, &window).move(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    pinchSequence.release(0, p1, &window).release(1, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "pinch scaled to" << rect->scale();
    QVERIFY(rect->scale() > 1.0);
    pinchSequence.release(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);
}

/*
   Regression test for the following use case:
   You have two mouse areas, on on top of the other.
   1 - You tap the top one.
   2 - That top mouse area receives a mouse press event but doesn't accept it
   Expected outcome:
     3 - the bottom mouse area gets clicked (besides press and release mouse events)
   Bogus outcome:
     3 - the bottom mouse area gets double clicked.
 */
void tst_TouchMouse::tapOnDismissiveTopMouseAreaClicksBottomOne()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoMouseAreas.qml")));

    QQuickMouseArea *bottomMouseArea =
        window.rootObject()->findChild<QQuickMouseArea*>("rear mouseArea");

    QSignalSpy bottomClickedSpy(bottomMouseArea, SIGNAL(clicked(QQuickMouseEvent*)));
    QSignalSpy bottomDoubleClickedSpy(bottomMouseArea,
                                      SIGNAL(doubleClicked(QQuickMouseEvent*)));

    // tap the front mouse area (see qml file)
    QPoint p1(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);

    QCOMPARE(bottomClickedSpy.size(), 1);
    QCOMPARE(bottomDoubleClickedSpy.size(), 0);

    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);

    QCOMPARE(bottomClickedSpy.size(), 1);
    QCOMPARE(bottomDoubleClickedSpy.size(), 1);
}

/*
    If an item grabs a touch that is currently being used for mouse pointer emulation,
    the current mouse grabber should lose the mouse as mouse events will no longer
    be generated from that touch point.
 */
void tst_TouchMouse::touchGrabCausesMouseUngrab()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twosiblingitems.qml")));

    EventItem *leftItem = window.rootObject()->findChild<EventItem*>("leftItem");
    QVERIFY(leftItem);

    EventItem *rightItem = window.rootObject()->findChild<EventItem*>("rightItem");
    QVERIFY(rightItem);

    // Send a touch to the leftItem. But leftItem accepts only mouse events, thus
    // a mouse event will be synthesized out of this touch and will get accepted by
    // leftItem.
    leftItem->acceptMouse = true;
    leftItem->setAcceptedMouseButtons(Qt::LeftButton);
    QPoint p1;
    p1 = QPoint(leftItem->width() / 2, leftItem->height() / 2);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "expected delivered events: press(mouse)" << leftItem->eventList;
    QCOMPARE(leftItem->eventList.size(), 1);
    QCOMPARE(leftItem->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(grabMonitor.exclusiveGrabber, leftItem);
    leftItem->eventList.clear();

    rightItem->acceptTouch = true;
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    auto epd = devPriv->queryPointById(0);
    QVERIFY(epd);
    devPriv->setExclusiveGrabber(nullptr, epd->eventPoint, rightItem);

    // leftItem should have lost the mouse as the touch point that was being used to emulate it
    // has been grabbed by another item.
    QCOMPARE(leftItem->eventList.size(), 1);
    QCOMPARE(leftItem->eventList.at(0).type, QEvent::UngrabMouse);
    QCOMPARE(grabMonitor.exclusiveGrabber, rightItem);
}

void tst_TouchMouse::touchPointDeliveryOrder()
{
    // Touch points should be first delivered to the item under the primary finger
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("touchpointdeliveryorder.qml")));

    /*
    The items are positioned from left to right:
    |      background     |
    |   left   |
    |          |   right  |
          |  middle |
    0   150   300  450  600
    */
    QPoint pLeft = QPoint(100, 100);
    QPoint pRight = QPoint(500, 100);
    QPoint pLeftMiddle = QPoint(200, 100);
    QPoint pRightMiddle = QPoint(350, 100);

    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(&window, touchscreen.get(), false);

    QVector<QQuickItem*> events;
    EventItem *background = window.rootObject()->findChild<EventItem*>("background");
    EventItem *left = window.rootObject()->findChild<EventItem*>("left");
    EventItem *middle = window.rootObject()->findChild<EventItem*>("middle");
    EventItem *right = window.rootObject()->findChild<EventItem*>("right");
    QVERIFY(background);
    QVERIFY(left);
    QVERIFY(middle);
    QVERIFY(right);
    background->setAcceptTouchEvents(true);
    left->setAcceptTouchEvents(true);
    middle->setAcceptTouchEvents(true);
    right->setAcceptTouchEvents(true);
    connect(background, &EventItem::onTouchEvent, [&events](QQuickItem* receiver){ events.append(receiver); });
    connect(left, &EventItem::onTouchEvent, [&events](QQuickItem* receiver){ events.append(receiver); });
    connect(middle, &EventItem::onTouchEvent, [&events](QQuickItem* receiver){ events.append(receiver); });
    connect(right, &EventItem::onTouchEvent, [&events](QQuickItem* receiver){ events.append(receiver); });

    touchSeq.press(0, pLeft, &window).commit();
    QQuickTouchUtils::flush(&window);

    // Touch on left, then background
    QCOMPARE(events.size(), 2);
    QCOMPARE(events.at(0), left);
    QCOMPARE(events.at(1), background);
    events.clear();

    // New press events are deliverd first, the stationary point was not accepted, thus it doesn't get delivered
    touchSeq.stationary(0).press(1, pRightMiddle, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(events.size(), 3);
    QCOMPARE(events.at(0), middle);
    QCOMPARE(events.at(1), right);
    QCOMPARE(events.at(2), background);
    events.clear();

    touchSeq.release(0, pLeft, &window).release(1, pRightMiddle, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(events.size(), 0); // no accepted events

    // Two presses, the first point should come first
    touchSeq.press(0, pLeft, &window).press(1, pRight, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(events.size(), 3);
    QCOMPARE(events.at(0), left);
    QCOMPARE(events.at(1), right);
    QCOMPARE(events.at(2), background);
    touchSeq.release(0, pLeft, &window).release(1, pRight, &window).commit();
    events.clear();

    // Again, pressing right first
    touchSeq.press(0, pRight, &window).press(1, pLeft, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(events.size(), 3);
    QCOMPARE(events.at(0), right);
    QCOMPARE(events.at(1), left);
    QCOMPARE(events.at(2), background);
    touchSeq.release(0, pRight, &window).release(1, pLeft, &window).commit();
    events.clear();

    // Two presses, both hitting the middle item on top, then branching left and right, then bottom
    // Each target should be offered the events exactly once, middle first, left must come before right (id 0)
    touchSeq.press(0, pLeftMiddle, &window).press(1, pRightMiddle, &window).commit();
    QCOMPARE(events.size(), 4);
    QCOMPARE(events.at(0), middle);
    QCOMPARE(events.at(1), left);
    QCOMPARE(events.at(2), right);
    QCOMPARE(events.at(3), background);
    touchSeq.release(0, pLeftMiddle, &window).release(1, pRightMiddle, &window).commit();
    events.clear();

    touchSeq.press(0, pRightMiddle, &window).press(1, pLeftMiddle, &window).commit();
    qCDebug(lcTests) << events;
    QCOMPARE(events.size(), 4);
    QCOMPARE(events.at(0), middle);
    QCOMPARE(events.at(1), right);
    QCOMPARE(events.at(2), left);
    QCOMPARE(events.at(3), background);
    touchSeq.release(0, pRightMiddle, &window).release(1, pLeftMiddle, &window).commit();
}

void tst_TouchMouse::hoverEnabled() // QTBUG-40856
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverMouseAreas.qml")));
    QQuickItem *root = window.rootObject();
    auto deliveryAgent = QQuickWindowPrivate::get(&window)->deliveryAgentPrivate();

    QQuickMouseArea *mouseArea1 = root->findChild<QQuickMouseArea*>("mouseArea1");
    QVERIFY(mouseArea1 != nullptr);

    QQuickMouseArea *mouseArea2 = root->findChild<QQuickMouseArea*>("mouseArea2");
    QVERIFY(mouseArea2 != nullptr);

    QSignalSpy enterSpy1(mouseArea1, SIGNAL(entered()));
    QSignalSpy exitSpy1(mouseArea1, SIGNAL(exited()));
    QSignalSpy clickSpy1(mouseArea1, SIGNAL(clicked(QQuickMouseEvent*)));

    QSignalSpy enterSpy2(mouseArea2, SIGNAL(entered()));
    QSignalSpy exitSpy2(mouseArea2, SIGNAL(exited()));
    QSignalSpy clickSpy2(mouseArea2, SIGNAL(clicked(QQuickMouseEvent*)));

    QPoint p1(150, 150);
    QPoint p2(150, 250);

    // ------------------------- Mouse move to mouseArea1
    QTest::mouseMove(&window, p1);

    QVERIFY(enterSpy1.size() == 1);
    QVERIFY(mouseArea1->hovered());
    QVERIFY(!mouseArea2->hovered());

    // ------------------------- Touch click on mouseArea1
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    deliveryAgent->flushFrameSynchronousEvents(&window);

    QCOMPARE(enterSpy1.size(), 1);
    QCOMPARE(enterSpy2.size(), 0);
    QVERIFY(mouseArea1->isPressed());
    QVERIFY(mouseArea1->hovered());
    QVERIFY(!mouseArea2->hovered());

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    deliveryAgent->flushFrameSynchronousEvents(&window);
    QVERIFY(clickSpy1.size() == 1);
    QVERIFY(mouseArea1->hovered());
    QVERIFY(!mouseArea2->hovered());

    // ------------------------- Touch click on mouseArea2
    QTest::touchEvent(&window, touchscreen.get()).press(0, p2, &window);
    deliveryAgent->flushFrameSynchronousEvents(&window);

    QVERIFY(!mouseArea1->hovered());
    QVERIFY(mouseArea2->hovered());
    QVERIFY(mouseArea2->isPressed());
    QCOMPARE(enterSpy1.size(), 1);
    QCOMPARE(enterSpy2.size(), 1);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p2, &window);
    deliveryAgent->flushFrameSynchronousEvents(&window);

    QVERIFY(clickSpy2.size() == 1);
    QVERIFY(!mouseArea1->hovered());
    QVERIFY(!mouseArea2->hovered());
    QCOMPARE(exitSpy1.size(), 1);
    QCOMPARE(exitSpy2.size(), 1);

    // ------------------------- Another touch click on mouseArea1
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    deliveryAgent->flushFrameSynchronousEvents(&window);

    QCOMPARE(enterSpy1.size(), 2);
    QCOMPARE(enterSpy2.size(), 1);
    QVERIFY(mouseArea1->isPressed());
    QVERIFY(mouseArea1->hovered());
    QVERIFY(!mouseArea2->hovered());

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1, &window);
    deliveryAgent->flushFrameSynchronousEvents(&window);
    QCOMPARE(clickSpy1.size(), 2);
    QVERIFY(mouseArea1->hovered());
    QVERIFY(!mouseArea1->isPressed());
    QVERIFY(!mouseArea2->hovered());
}

void tst_TouchMouse::implicitUngrab()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("singleitem.qml")));
    QQuickWindowPrivate *windowPriv = QQuickWindowPrivate::get(&window);

    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);
    EventItem *eventItem = root->findChild<EventItem*>("eventItem1");
    eventItem->acceptMouse = true;
    eventItem->setAcceptTouchEvents(false);
    QPoint p1(20, 20);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);

    QCOMPARE(grabMonitor.exclusiveGrabber, eventItem);
    eventItem->eventList.clear();
    eventItem->setEnabled(false);
    QVERIFY(!eventItem->eventList.isEmpty());
    QCOMPARE(eventItem->eventList.at(0).type, QEvent::UngrabMouse);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);   // clean up potential state
    QCOMPARE(windowPriv->deliveryAgentPrivate()->touchMouseId, -1);

    eventItem->setEnabled(true);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    eventItem->eventList.clear();
    eventItem->setVisible(false);
    QVERIFY(!eventItem->eventList.isEmpty());
    QCOMPARE(eventItem->eventList.at(0).type, QEvent::UngrabMouse);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);   // clean up potential state
}

void tst_TouchMouse::touchCancelWillCancelMousePress()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("singleitem.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    EventItem *eventItem = root->findChild<EventItem*>("eventItem1");
    eventItem->acceptMouse = true;
    eventItem->setAcceptTouchEvents(false);
    QPoint p1(20, 20);

    // Begin a new touch, that gets converted to a mouse press
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QCOMPARE(eventItem->eventList.size(), 1);
    QCOMPARE(eventItem->eventList.at(0).type, QEvent::MouseButtonPress);

    // Cancel it...
    QTouchEvent cancelEvent(QEvent::TouchCancel, touchscreen.get());
    QCoreApplication::sendEvent(&window, &cancelEvent);
    QCOMPARE(eventItem->eventList.size(), 3);
    QCOMPARE(eventItem->eventList.at(1).type, QEvent::TouchCancel);
    QCOMPARE(eventItem->eventList.at(2).type, QEvent::UngrabMouse);

    // Begin a second touch. Since the last one was cancelled, this
    // should end up as a new mouse press on the target item.
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QVERIFY(eventItem->eventList.size() >= 5);
    QCOMPARE(eventItem->eventList.at(3).type, QEvent::MouseButtonPress);

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1);   // clean up potential state
}

void tst_TouchMouse::oneTouchInsideAndOneOutside() // QTBUG-102996
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("oneMouseArea.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);
    QQuickMouseArea *ma = root->findChild<QQuickMouseArea*>();
    QVERIFY(ma);

    // Press the MouseArea
    QPoint p1 = ma->mapToScene(ma->boundingRect().center()).toPoint();
    QTest::touchEvent(&window, touchscreen.get()).press(1, p1);
    QQuickTouchUtils::flush(&window);
    QVERIFY(ma->isPressed());

    // Tap outside the MouseArea with a second finger
    QPoint p2(100, 100);
    QTest::touchEvent(&window, touchscreen.get()).stationary(1).press(2, p2);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).stationary(1).release(2, p2);
    QQuickTouchUtils::flush(&window);
    QVERIFY(ma->isPressed());

    // Press again outside the MouseArea with a second finger
    QTest::touchEvent(&window, touchscreen.get()).stationary(1).press(2, p2);

    // Release the first finger: MouseArea should be released
    QTest::touchEvent(&window, touchscreen.get()).release(1, p1).stationary(2);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(ma->isPressed(), false);

    // Release the second finger
    QTest::touchEvent(&window, touchscreen.get()).release(2, p2);
    QQuickTouchUtils::flush(&window);
}

void tst_TouchMouse::strayTouchDoesntAutograb() // QTBUG-107867
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("singleitem.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);
    EventItem *eventItem = root->findChild<EventItem*>();
    QVERIFY(eventItem);
    // This item accepts (synth-)mouse events but NOT touch
    eventItem->acceptMouse = true;
    QCOMPARE(eventItem->acceptTouchEvents(), false); // the default in Qt 6
    QPoint p1(6, 6);
    grabMonitor.reset();

    // Begin a new touch, that gets converted to a mouse press
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "after touch press:" << eventItem->eventList;
    QCOMPARE(eventItem->eventList.size(), 1);
    QCOMPARE(eventItem->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(grabMonitor.exclusiveGrabber, eventItem);

    // Drag
    for (int i = 0; i < 3; ++i) {
        QTest::touchEvent(&window, touchscreen.get()).move(0, p1 + QPoint(i * 5, i * 5), &window);
        QQuickTouchUtils::flush(&window);
        QCOMPARE(grabMonitor.transitionCount, 1); // no new grab
        QCOMPARE(eventItem->eventList.size(), i + 2);
        QCOMPARE(eventItem->eventList.last().type, QEvent::MouseMove);
    }

    // Press an extra point: EventItem should see nothing
    QTest::touchEvent(&window, touchscreen.get()).stationary(0).press(1, p1);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "after press of second touchpoint:" << eventItem->eventList;
    QCOMPARE(eventItem->eventList.size(), 4);
    QCOMPARE(grabMonitor.transitionCount, 1); // no new grab

    QTest::touchEvent(&window, touchscreen.get()).release(0, p1).release(1, p1);
}

void tst_TouchMouse::noDoubleClickWithInterveningTouch() // QTBUG-116442
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twosiblingitems.qml")));

    EventItem *leftItem = window.rootObject()->findChild<EventItem*>("leftItem");
    QVERIFY(leftItem);
    // simulate a MouseArea: don't accept touch
    leftItem->setAcceptedMouseButtons(Qt::LeftButton);
    leftItem->acceptMouse = true;

    EventItem *rightItem = window.rootObject()->findChild<EventItem*>("rightItem");
    QVERIFY(rightItem);
    // simulate an item that reacts to either touch or mouse
    rightItem->setAcceptedMouseButtons(Qt::LeftButton);
    rightItem->acceptMouse = true;
    rightItem->setAcceptTouchEvents(true);
    rightItem->acceptTouch = true;

    const QPoint pLeft(80, 200);
    const QPoint pRight(240, 200);

    // tap left
    QTest::touchEvent(&window, touchscreen.get()).press(1, pLeft, &window);
    QTest::touchEvent(&window, touchscreen.get()).release(1, pLeft, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "left tap" << leftItem->eventList;
    QCOMPARE(leftItem->eventList.size(), 3);
    QCOMPARE(leftItem->eventList.at(0).type, QEvent::MouseButtonPress);
    leftItem->eventList.clear();

    // tap right
    QTest::touchEvent(&window, touchscreen.get()).press(1, pRight, &window);
    QTest::touchEvent(&window, touchscreen.get()).release(1, pRight, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "right tap" << rightItem->eventList;
    QCOMPARE(rightItem->eventList.size(), 2);
    QCOMPARE(rightItem->eventList.at(0).type, QEvent::TouchBegin);
    rightItem->eventList.clear();

    // tap left again: this is NOT a double-click, even though it's within time and space limits
    QTest::touchEvent(&window, touchscreen.get()).press(3, pLeft, &window);
    QTest::touchEvent(&window, touchscreen.get()).release(3, pLeft, &window);
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "left tap again" << leftItem->eventList;
    QCOMPARE(leftItem->eventList.size(), 3);
    QCOMPARE(leftItem->eventList.at(0).type, QEvent::MouseButtonPress);
    QCOMPARE(leftItem->eventList.at(1).type, QEvent::MouseButtonRelease);
    QCOMPARE(leftItem->eventList.at(2).type, QEvent::UngrabMouse);
    leftItem->eventList.clear();
}

QTEST_MAIN(tst_TouchMouse)

#include "tst_touchmouse.moc"

