/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QDebug>
#include <QTouchEvent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/private/qquickrectangle_p.h>
#include "../../shared/util.h"
#include "../shared/visualtestutil.h"
#include <QSignalSpy>
#include <qpa/qwindowsysteminterface.h>
#include <private/qquickwindow_p.h>
#include <private/qguiapplication_p.h>

struct TouchEventData {
    QEvent::Type type;
    QWidget *widget;
    QWindow *window;
    Qt::TouchPointStates states;
    QList<QTouchEvent::TouchPoint> touchPoints;
};

static QTouchEvent::TouchPoint makeTouchPoint(QQuickItem *item, const QPointF &p, const QPointF &lastPoint = QPointF())
{
    QPointF last = lastPoint.isNull() ? p : lastPoint;

    QTouchEvent::TouchPoint tp;

    tp.setPos(p);
    tp.setLastPos(last);
    tp.setScenePos(item->mapToScene(p));
    tp.setLastScenePos(item->mapToScene(last));
    tp.setScreenPos(item->window()->mapToGlobal(tp.scenePos().toPoint()));
    tp.setLastScreenPos(item->window()->mapToGlobal(tp.lastScenePos().toPoint()));
    return tp;
}

static TouchEventData makeTouchData(QEvent::Type type, QWindow *w, Qt::TouchPointStates states = 0,
                                    const QList<QTouchEvent::TouchPoint>& touchPoints = QList<QTouchEvent::TouchPoint>())
{
    TouchEventData d = { type, 0, w, states, touchPoints };
    return d;
}
static TouchEventData makeTouchData(QEvent::Type type, QWindow *w, Qt::TouchPointStates states, const QTouchEvent::TouchPoint &touchPoint)
{
    QList<QTouchEvent::TouchPoint> points;
    points << touchPoint;
    return makeTouchData(type, w, states, points);
}

#define COMPARE_TOUCH_POINTS(tp1, tp2) \
{ \
    QCOMPARE(tp1.pos(), tp2.pos()); \
    QCOMPARE(tp1.lastPos(), tp2.lastPos()); \
    QCOMPARE(tp1.scenePos(), tp2.scenePos()); \
    QCOMPARE(tp1.lastScenePos(), tp2.lastScenePos()); \
    QCOMPARE(tp1.screenPos(), tp2.screenPos()); \
    QCOMPARE(tp1.lastScreenPos(), tp2.lastScreenPos()); \
}

#define COMPARE_TOUCH_DATA(d1, d2) \
{ \
    QCOMPARE((int)d1.type, (int)d2.type); \
    QCOMPARE(d1.widget, d2.widget); \
    QCOMPARE((int)d1.states, (int)d2.states); \
    QCOMPARE(d1.touchPoints.count(), d2.touchPoints.count()); \
    for (int i=0; i<d1.touchPoints.count(); i++) { \
        COMPARE_TOUCH_POINTS(d1.touchPoints[i], d2.touchPoints[i]); \
    } \
}


class RootItemAccessor : public QQuickItem
{
    Q_OBJECT
public:
    RootItemAccessor()
        : m_rootItemDestroyed(false)
        , m_rootItem(0)
    {
    }
    Q_INVOKABLE QQuickItem *contentItem()
    {
        if (!m_rootItem) {
            QQuickWindowPrivate *c = QQuickWindowPrivate::get(window());
            m_rootItem = c->contentItem;
            QObject::connect(m_rootItem, SIGNAL(destroyed()), this, SLOT(rootItemDestroyed()));
        }
        return m_rootItem;
    }
    bool isRootItemDestroyed() {return m_rootItemDestroyed;}
public slots:
    void rootItemDestroyed() {
        m_rootItemDestroyed = true;
    }

private:
    bool m_rootItemDestroyed;
    QQuickItem *m_rootItem;
};

class TestTouchItem : public QQuickRectangle
{
    Q_OBJECT
public:
    TestTouchItem(QQuickItem *parent = 0)
        : QQuickRectangle(parent), acceptTouchEvents(true), acceptMouseEvents(true),
          mousePressId(0),
          spinLoopWhenPressed(false), touchEventCount(0)
    {
        border()->setWidth(1);
        setAcceptedMouseButtons(Qt::LeftButton);
        setFiltersChildMouseEvents(true);
    }

    void reset() {
        acceptTouchEvents = acceptMouseEvents = true;
        setEnabled(true);
        setVisible(true);

        lastEvent = makeTouchData(QEvent::None, window(), 0, QList<QTouchEvent::TouchPoint>());//CHECK_VALID

        lastVelocity = lastVelocityFromMouseMove = QVector2D();
        lastMousePos = QPointF();
        lastMouseCapabilityFlags = 0;
    }

    static void clearMousePressCounter()
    {
        mousePressNum = mouseMoveNum = mouseReleaseNum = 0;
    }

    void clearTouchEventCounter()
    {
        touchEventCount = 0;
    }

    bool acceptTouchEvents;
    bool acceptMouseEvents;
    TouchEventData lastEvent;
    int mousePressId;
    bool spinLoopWhenPressed;
    int touchEventCount;
    QVector2D lastVelocity;
    QVector2D lastVelocityFromMouseMove;
    QPointF lastMousePos;
    int lastMouseCapabilityFlags;

    void touchEvent(QTouchEvent *event) {
        if (!acceptTouchEvents) {
            event->ignore();
            return;
        }
        ++touchEventCount;
        lastEvent = makeTouchData(event->type(), event->window(), event->touchPointStates(), event->touchPoints());
        if (event->device()->capabilities().testFlag(QTouchDevice::Velocity) && !event->touchPoints().isEmpty()) {
            lastVelocity = event->touchPoints().first().velocity();
        } else {
            lastVelocity = QVector2D();
        }
        if (spinLoopWhenPressed && event->touchPointStates().testFlag(Qt::TouchPointPressed)) {
            QCoreApplication::processEvents();
        }
    }

    void mousePressEvent(QMouseEvent *e) {
        if (!acceptMouseEvents) {
            e->ignore();
            return;
        }
        mousePressId = ++mousePressNum;
        lastMousePos = e->pos();
        lastMouseCapabilityFlags = QGuiApplicationPrivate::mouseEventCaps(e);
    }

    void mouseMoveEvent(QMouseEvent *e) {
        if (!acceptMouseEvents) {
            e->ignore();
            return;
        }
        ++mouseMoveNum;
        lastVelocityFromMouseMove = QGuiApplicationPrivate::mouseEventVelocity(e);
        lastMouseCapabilityFlags = QGuiApplicationPrivate::mouseEventCaps(e);
        lastMousePos = e->pos();
    }

    void mouseReleaseEvent(QMouseEvent *e) {
        if (!acceptMouseEvents) {
            e->ignore();
            return;
        }
        ++mouseReleaseNum;
        lastMousePos = e->pos();
        lastMouseCapabilityFlags = QGuiApplicationPrivate::mouseEventCaps(e);
    }

    bool childMouseEventFilter(QQuickItem *, QEvent *event) {
        // TODO Is it a bug if a QTouchEvent comes here?
        if (event->type() == QEvent::MouseButtonPress)
            mousePressId = ++mousePressNum;
        return false;
    }

    static int mousePressNum, mouseMoveNum, mouseReleaseNum;
};

int TestTouchItem::mousePressNum = 0;
int TestTouchItem::mouseMoveNum = 0;
int TestTouchItem::mouseReleaseNum = 0;

class ConstantUpdateItem : public QQuickItem
{
Q_OBJECT
public:
    ConstantUpdateItem(QQuickItem *parent = 0) : QQuickItem(parent), iterations(0) {setFlag(ItemHasContents);}

    int iterations;
protected:
    QSGNode* updatePaintNode(QSGNode *, UpdatePaintNodeData *){
        iterations++;
        update();
        return 0;
    }
};

class tst_qquickwindow : public QQmlDataTest
{
    Q_OBJECT
public:

private slots:
    void initTestCase()
    {
        QQmlDataTest::initTestCase();
        touchDevice = new QTouchDevice;
        touchDevice->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(touchDevice);
        touchDeviceWithVelocity = new QTouchDevice;
        touchDeviceWithVelocity->setType(QTouchDevice::TouchScreen);
        touchDeviceWithVelocity->setCapabilities(QTouchDevice::Position | QTouchDevice::Velocity);
        QWindowSystemInterface::registerTouchDevice(touchDeviceWithVelocity);
    }


    void constantUpdates();
    void constantUpdatesOnWindow_data();
    void constantUpdatesOnWindow();
    void mouseFiltering();
    void headless();
    void noUpdateWhenNothingChanges();

    void touchEvent_basic();
    void touchEvent_propagation();
    void touchEvent_propagation_data();
    void touchEvent_cancel();
    void touchEvent_reentrant();
    void touchEvent_velocity();

    void mouseFromTouch_basic();

    void clearWindow();

    void qmlCreation();
    void clearColor();

    void grab_data();
    void grab();
    void multipleWindows();

    void animationsWhileHidden();

    void focusObject();

    void ignoreUnhandledMouseEvents();

    void ownershipRootItem();

    void hideThenDelete_data();
    void hideThenDelete();

    void showHideAnimate();

    void testExpose();

    void requestActivate();

    void blockClosing();

#ifndef QT_NO_CURSOR
    void cursor();
#endif

private:
    QTouchDevice *touchDevice;
    QTouchDevice *touchDeviceWithVelocity;
};

//If the item calls update inside updatePaintNode, it should schedule another update
void tst_qquickwindow::constantUpdates()
{
    QQuickWindow window;
    window.resize(250, 250);
    ConstantUpdateItem item(window.contentItem());
    window.show();
    QTRY_VERIFY(item.iterations > 60);
}

void tst_qquickwindow::constantUpdatesOnWindow_data()
{
    QTest::addColumn<bool>("blockedGui");
    QTest::addColumn<QByteArray>("signal");

    QQuickWindow window;
    window.setGeometry(100, 100, 300, 200);
    window.show();
    QTest::qWaitForWindowExposed(&window);
    bool threaded = window.openglContext()->thread() != QGuiApplication::instance()->thread();

    if (threaded) {
        QTest::newRow("blocked, beforeSync") << true << QByteArray(SIGNAL(beforeSynchronizing()));
        QTest::newRow("blocked, beforeRender") << true << QByteArray(SIGNAL(beforeRendering()));
        QTest::newRow("blocked, afterRender") << true << QByteArray(SIGNAL(afterRendering()));
        QTest::newRow("blocked, swapped") << true << QByteArray(SIGNAL(frameSwapped()));
    }
    QTest::newRow("unblocked, beforeSync") << false << QByteArray(SIGNAL(beforeSynchronizing()));
    QTest::newRow("unblocked, beforeRender") << false << QByteArray(SIGNAL(beforeRendering()));
    QTest::newRow("unblocked, afterRender") << false << QByteArray(SIGNAL(afterRendering()));
    QTest::newRow("unblocked, swapped") << false << QByteArray(SIGNAL(frameSwapped()));
}

void tst_qquickwindow::constantUpdatesOnWindow()
{
    QSKIP("This test fails frequently on the present overworked CI mac machines");
    QFETCH(bool, blockedGui);
    QFETCH(QByteArray, signal);

    QQuickWindow window;
    window.setGeometry(100, 100, 300, 200);

    connect(&window, signal.constData(), &window, SLOT(update()), Qt::DirectConnection);
    window.show();
    QTRY_VERIFY(window.isExposed());

    QSignalSpy catcher(&window, SIGNAL(frameSwapped()));
    if (blockedGui)
        QTest::qSleep(1000);
    else {
        window.update();
        QTest::qWait(1000);
    }
    window.hide();

    // We should expect 60, but under loaded conditions we could be skipping
    // frames, so don't expect too much.
    QVERIFY(catcher.size() > 10);
}

void tst_qquickwindow::touchEvent_basic()
{
    TestTouchItem::clearMousePressCounter();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    TestTouchItem *bottomItem = new TestTouchItem(window->contentItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPosition(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPosition(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPointF pos(10, 10);

    // press single point
    QTest::touchEvent(window, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),window);
    QTest::qWait(50);

    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);

    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    // At one point this was failing with kwin (KDE window manager) because window->setPosition(100, 100)
    // would put the decorated window at that position rather than the window itself.
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    topItem->reset();

    // press multiple points
    QTest::touchEvent(window, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),window)
            .press(1, bottomItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();

    // touch point on top item moves to bottom item, but top item should still receive the event
    QTest::touchEvent(window, touchDevice).press(0, topItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QTest::touchEvent(window, touchDevice).move(0, bottomItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchUpdate, window, Qt::TouchPointMoved,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // touch point on bottom item moves to top item, but bottom item should still receive the event
    QTest::touchEvent(window, touchDevice).press(0, bottomItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QTest::touchEvent(window, touchDevice).move(0, topItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchUpdate, window, Qt::TouchPointMoved,
            makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos), pos)));
    bottomItem->reset();

    // a single stationary press on an item shouldn't cause an event
    QTest::touchEvent(window, touchDevice).press(0, topItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QTest::touchEvent(window, touchDevice).stationary(0)
            .press(1, bottomItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);    // received press only, not stationary
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();
    // cleanup: what is pressed must be released
    // Otherwise you will get an assertion failure:
    // ASSERT: "itemForTouchPointId.isEmpty()" in file items/qquickwindow.cpp
    QTest::touchEvent(window, touchDevice).release(0, pos.toPoint(), window).release(1, pos.toPoint(), window);

    // move touch point from top item to bottom, and release
    QTest::touchEvent(window, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),window);
    QTest::qWait(50);
    QTest::touchEvent(window, touchDevice).release(0, bottomItem->mapToScene(pos).toPoint(),window);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, window, Qt::TouchPointReleased,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // release while another point is pressed
    QTest::touchEvent(window, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),window)
            .press(1, bottomItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QTest::touchEvent(window, touchDevice).move(0, bottomItem->mapToScene(pos).toPoint(), window);
    QTest::qWait(50);
    QTest::touchEvent(window, touchDevice).release(0, bottomItem->mapToScene(pos).toPoint(), window)
                             .stationary(1);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, window, Qt::TouchPointReleased,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos))));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();

    delete topItem;
    delete middleItem;
    delete bottomItem;
}

void tst_qquickwindow::touchEvent_propagation()
{
    TestTouchItem::clearMousePressCounter();

    QFETCH(bool, acceptTouchEvents);
    QFETCH(bool, acceptMouseEvents);
    QFETCH(bool, enableItem);
    QFETCH(bool, showItem);

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    TestTouchItem *bottomItem = new TestTouchItem(window->contentItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPosition(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPosition(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPointF pos(10, 10);
    QPoint pointInBottomItem = bottomItem->mapToScene(pos).toPoint();  // (10, 10)
    QPoint pointInMiddleItem = middleItem->mapToScene(pos).toPoint();  // (60, 60) overlaps with bottomItem
    QPoint pointInTopItem = topItem->mapToScene(pos).toPoint();  // (110, 110) overlaps with bottom & top items

    // disable topItem
    topItem->acceptTouchEvents = acceptTouchEvents;
    topItem->acceptMouseEvents = acceptMouseEvents;
    topItem->setEnabled(enableItem);
    topItem->setVisible(showItem);

    // single touch to top item, should be received by middle item
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(middleItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed,
            makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))));

    // touch top and middle items, middle item should get both events
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window)
            .press(1, pointInMiddleItem, window);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(middleItem->lastEvent.touchPoints.count(), 2);
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed,
           (QList<QTouchEvent::TouchPoint>() << makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))
                                              << makeTouchPoint(middleItem, pos) )));
    middleItem->reset();

    // disable middleItem as well
    middleItem->acceptTouchEvents = acceptTouchEvents;
    middleItem->acceptMouseEvents = acceptMouseEvents;
    middleItem->setEnabled(enableItem);
    middleItem->setVisible(showItem);

    // touch top and middle items, bottom item should get all events
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window)
            .press(1, pointInMiddleItem, window);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 2);
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed,
            (QList<QTouchEvent::TouchPoint>() << makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos))
                                              << makeTouchPoint(bottomItem, bottomItem->mapFromItem(middleItem, pos)) )));
    bottomItem->reset();

    // disable bottom item as well
    bottomItem->acceptTouchEvents = acceptTouchEvents;
    bottomItem->setEnabled(enableItem);
    bottomItem->setVisible(showItem);

    // no events should be received
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window)
            .press(1, pointInMiddleItem, window)
            .press(2, pointInBottomItem, window);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());

    topItem->reset();
    middleItem->reset();
    bottomItem->reset();

    // disable middle item, touch on top item
    middleItem->acceptTouchEvents = acceptTouchEvents;
    middleItem->setEnabled(enableItem);
    middleItem->setVisible(showItem);
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window);
    QTest::qWait(50);
    if (!enableItem || !showItem) {
        // middle item is disabled or has 0 opacity, bottom item receives the event
        QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
        QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
        QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
        COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed,
                makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos))));
    } else {
        // middle item ignores event, sends it to the top item (top-most child)
        QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
        QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
        QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
        COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed,
                makeTouchPoint(topItem, pos)));
    }

    delete topItem;
    delete middleItem;
    delete bottomItem;
}

void tst_qquickwindow::touchEvent_propagation_data()
{
    QTest::addColumn<bool>("acceptTouchEvents");
    QTest::addColumn<bool>("acceptMouseEvents");
    QTest::addColumn<bool>("enableItem");
    QTest::addColumn<bool>("showItem");

    QTest::newRow("disable events") << false << false << true << true;
    QTest::newRow("disable item") << true << true << false << true;
    QTest::newRow("hide item") << true << true << true << false;
}

void tst_qquickwindow::touchEvent_cancel()
{
    TestTouchItem::clearMousePressCounter();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));

    QPointF pos(10, 10);
    QTest::touchEvent(window, touchDevice).press(0, item->mapToScene(pos).toPoint(),window);
    QCoreApplication::processEvents();

    QTRY_COMPARE(item->lastEvent.touchPoints.count(), 1);
    TouchEventData d = makeTouchData(QEvent::TouchBegin, window, Qt::TouchPointPressed, makeTouchPoint(item,pos));
    COMPARE_TOUCH_DATA(item->lastEvent, d);
    item->reset();

    QWindowSystemInterface::handleTouchCancelEvent(0, touchDevice);
    QCoreApplication::processEvents();
    d = makeTouchData(QEvent::TouchCancel, window);
    COMPARE_TOUCH_DATA(item->lastEvent, d);

    delete item;
}

void tst_qquickwindow::touchEvent_reentrant()
{
    TestTouchItem::clearMousePressCounter();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    TestTouchItem *item = new TestTouchItem(window->contentItem());

    item->spinLoopWhenPressed = true; // will call processEvents() from the touch handler

    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));
    QPointF pos(60, 60);

    // None of these should commit from the dtor.
    QTest::QTouchEventSequence press = QTest::touchEvent(window, touchDevice, false).press(0, pos.toPoint(), window);
    pos += QPointF(2, 2);
    QTest::QTouchEventSequence move = QTest::touchEvent(window, touchDevice, false).move(0, pos.toPoint(), window);
    QTest::QTouchEventSequence release = QTest::touchEvent(window, touchDevice, false).release(0, pos.toPoint(), window);

    // Now commit (i.e. call QWindowSystemInterface::handleTouchEvent), but do not process the events yet.
    press.commit(false);
    move.commit(false);
    release.commit(false);

    QCoreApplication::processEvents();

    QTRY_COMPARE(item->touchEventCount, 3);

    delete item;
}

void tst_qquickwindow::touchEvent_velocity()
{
    TestTouchItem::clearMousePressCounter();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(10);

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 1;
    tp.state = Qt::TouchPointPressed;
    QPoint pos = window->mapToGlobal(item->mapToScene(QPointF(10, 10)).toPoint());
    tp.area = QRectF(pos, QSizeF(4, 4));
    points << tp;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area.adjust(5, 5, 5, 5);
    QVector2D velocity(1.5, 2.5);
    points[0].velocity = velocity;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    QCOMPARE(item->touchEventCount, 2);
    QCOMPARE(item->lastEvent.touchPoints.count(), 1);
    QCOMPARE(item->lastVelocity, velocity);

    // Now have a transformation on the item and check if velocity and position are transformed accordingly.
    item->setRotation(90); // clockwise
    QMatrix4x4 transformMatrix;
    transformMatrix.rotate(-90, 0, 0, 1); // counterclockwise
    QVector2D transformedVelocity = transformMatrix.mapVector(velocity).toVector2D();
    points[0].area.adjust(5, 5, 5, 5);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    QCOMPARE(item->lastVelocity, transformedVelocity);
    QPoint itemLocalPos = item->mapFromScene(window->mapFromGlobal(points[0].area.center().toPoint())).toPoint();
    QPoint itemLocalPosFromEvent = item->lastEvent.touchPoints[0].pos().toPoint();
    QCOMPARE(itemLocalPos, itemLocalPosFromEvent);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    delete item;
}

void tst_qquickwindow::mouseFromTouch_basic()
{
    // Turn off accepting touch events with acceptTouchEvents. This
    // should result in sending mouse events generated from the touch
    // with the new event propagation system.

    TestTouchItem::clearMousePressCounter();
    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(10);

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));
    item->acceptTouchEvents = false;

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 1;
    tp.state = Qt::TouchPointPressed;
    QPoint pos = window->mapToGlobal(item->mapToScene(QPointF(10, 10)).toPoint());
    tp.area = QRectF(pos, QSizeF(4, 4));
    points << tp;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area.adjust(5, 5, 5, 5);
    QVector2D velocity(1.5, 2.5);
    points[0].velocity = velocity;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();

    // The item should have received a mouse press, move, and release.
    QCOMPARE(item->mousePressNum, 1);
    QCOMPARE(item->mouseMoveNum, 1);
    QCOMPARE(item->mouseReleaseNum, 1);
    QCOMPARE(item->lastMousePos.toPoint(), item->mapFromScene(window->mapFromGlobal(points[0].area.center().toPoint())).toPoint());
    QCOMPARE(item->lastVelocityFromMouseMove, velocity);
    QVERIFY((item->lastMouseCapabilityFlags & QTouchDevice::Velocity) != 0);

    // Now the same with a transformation.
    item->setRotation(90); // clockwise
    QMatrix4x4 transformMatrix;
    transformMatrix.rotate(-90, 0, 0, 1); // counterclockwise
    QVector2D transformedVelocity = transformMatrix.mapVector(velocity).toVector2D();
    points[0].state = Qt::TouchPointPressed;
    points[0].velocity = velocity;
    points[0].area = QRectF(pos, QSizeF(4, 4));
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area.adjust(5, 5, 5, 5);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    QCOMPARE(item->lastMousePos.toPoint(), item->mapFromScene(window->mapFromGlobal(points[0].area.center().toPoint())).toPoint());
    QCOMPARE(item->lastVelocityFromMouseMove, transformedVelocity);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    delete item;
}

void tst_qquickwindow::clearWindow()
{
    QQuickWindow *window = new QQuickWindow;
    QQuickItem *item = new QQuickItem;
    item->setParentItem(window->contentItem());

    QVERIFY(item->window() == window);

    delete window;

    QVERIFY(item->window() == 0);

    delete item;
}

void tst_qquickwindow::mouseFiltering()
{
    TestTouchItem::clearMousePressCounter();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    TestTouchItem *bottomItem = new TestTouchItem(window->contentItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPosition(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPosition(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPoint pos(100, 100);

    QTest::mousePress(window, Qt::LeftButton, 0, pos);

    // Mouse filtering propagates down the stack, so the
    // correct order is
    // 1. middleItem filters event
    // 2. bottomItem filters event
    // 3. topItem receives event
    QTRY_COMPARE(middleItem->mousePressId, 1);
    QTRY_COMPARE(bottomItem->mousePressId, 2);
    QTRY_COMPARE(topItem->mousePressId, 3);
}

void tst_qquickwindow::qmlCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("window.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QCOMPARE(window->color(), QColor(Qt::green));

    QQuickItem* item = window->findChild<QQuickItem*>("item");
    QVERIFY(item);
    QCOMPARE(item->window(), window);
}

void tst_qquickwindow::clearColor()
{
    //::grab examines rendering to make sure it works visually
    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setColor(Qt::blue);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(window->color(), QColor(Qt::blue));
}

void tst_qquickwindow::grab_data()
{
    QTest::addColumn<bool>("visible");
    QTest::newRow("visible") << true;
    QTest::newRow("invisible") << false;
}

void tst_qquickwindow::grab()
{
    QFETCH(bool, visible);

    QQuickWindow window;
    window.setColor(Qt::red);

    window.resize(250, 250);

    if (visible) {
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    } else {
        window.create();
    }

    QImage content = window.grabWindow();
    QCOMPARE(content.width(), window.width());
    QCOMPARE(content.height(), window.height());
    QCOMPARE((uint) content.convertToFormat(QImage::Format_RGB32).pixel(0, 0), (uint) 0xffff0000);
}

void tst_qquickwindow::multipleWindows()
{
    QList<QQuickWindow *> windows;
    QScopedPointer<QQuickWindow> cleanup[6];

    for (int i=0; i<6; ++i) {
        QQuickWindow *c = new QQuickWindow();
        c->setColor(Qt::GlobalColor(Qt::red + i));
        c->resize(300, 200);
        c->setPosition(100 + i * 30, 100 + i * 20);
        c->show();
        windows << c;
        cleanup[i].reset(c);
        QVERIFY(QTest::qWaitForWindowExposed(c));
    }

    // move them
    for (int i=0; i<windows.size(); ++i) {
        QQuickWindow *c = windows.at(i);
        c->setPosition(100 + i * 30, 100 + i * 20 + 100);
    }

    // resize them
    for (int i=0; i<windows.size(); ++i) {
        QQuickWindow *c = windows.at(i);
        c->resize(200, 150);
    }
}

void tst_qquickwindow::animationsWhileHidden()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("AnimationsWhileHidden.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QVERIFY(window->isVisible());

    // Now hide the window and verify that it went off screen
    window->hide();
    QTest::qWait(10);
    QVERIFY(!window->isVisible());

    // Running animaiton should cause it to become visible again shortly.
    QTRY_VERIFY(window->isVisible());
}


void tst_qquickwindow::headless()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("Headless.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    window->setPersistentOpenGLContext(false);
    window->setPersistentSceneGraph(false);
    QVERIFY(window);
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->isVisible());

    QSignalSpy initialized(window, SIGNAL(sceneGraphInitialized()));
    QSignalSpy invalidated(window, SIGNAL(sceneGraphInvalidated()));

    // Verify that the window is alive and kicking
    QVERIFY(window->openglContext() != 0);

    // Store the visual result
    QImage originalContent = window->grabWindow();

    // Hide the window and verify signal emittion and GL context deletion
    window->hide();
    window->releaseResources();

    QTRY_COMPARE(invalidated.size(), 1);
    QVERIFY(window->openglContext() == 0);

    // Destroy the native windowing system buffers
    window->destroy();
    QVERIFY(window->handle() == 0);

    // Show and verify that we are back and running
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QTRY_COMPARE(initialized.size(), 1);
    QVERIFY(window->openglContext() != 0);

    // Verify that the visual output is the same
    QImage newContent = window->grabWindow();

    QCOMPARE(originalContent, newContent);
}

void tst_qquickwindow::noUpdateWhenNothingChanges()
{
    QQuickWindow window;
    window.setGeometry(100, 100, 300, 200);

    QQuickRectangle rect(window.contentItem());

    window.show();
    QTRY_VERIFY(window.isExposed());

    if (window.openglContext()->thread() == QGuiApplication::instance()->thread()) {
        QSKIP("Only threaded renderloop implements this feature");
        return;
    }

    QSignalSpy spy(&window, SIGNAL(frameSwapped()));
    rect.update();
    QTest::qWait(500);

    QCOMPARE(spy.size(), 0);
}

void tst_qquickwindow::focusObject()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("focus.qml"));
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QVERIFY(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickItem *item1 = window->findChild<QQuickItem*>("item1");
    QVERIFY(item1);
    item1->setFocus(true);
    QCOMPARE(item1, window->focusObject());

    QQuickItem *item2 = window->findChild<QQuickItem*>("item2");
    QVERIFY(item2);
    item2->setFocus(true);
    QCOMPARE(item2, window->focusObject());
}

void tst_qquickwindow::ignoreUnhandledMouseEvents()
{
    QQuickWindow* window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickItem* item = new QQuickItem;
    item->setSize(QSizeF(100, 100));
    item->setParentItem(window->contentItem());

    {
        QMouseEvent me(QEvent::MouseButtonPress, QPointF(50, 50), Qt::LeftButton, Qt::LeftButton,
                       Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(window, &me));
        QVERIFY(!me.isAccepted());
    }

    {
        QMouseEvent me(QEvent::MouseMove, QPointF(51, 51), Qt::LeftButton, Qt::LeftButton,
                       Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(window, &me));
        QVERIFY(!me.isAccepted());
    }

    {
        QMouseEvent me(QEvent::MouseButtonRelease, QPointF(51, 51), Qt::LeftButton, Qt::LeftButton,
                       Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(window, &me));
        QVERIFY(!me.isAccepted());
    }
}


void tst_qquickwindow::ownershipRootItem()
{
    qmlRegisterType<RootItemAccessor>("Test", 1, 0, "RootItemAccessor");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("ownershipRootItem.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    RootItemAccessor* accessor = window->findChild<RootItemAccessor*>("accessor");
    QVERIFY(accessor);
    engine.collectGarbage();

    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(!accessor->isRootItemDestroyed());
}

#ifndef QT_NO_CURSOR
void tst_qquickwindow::cursor()
{
    QQuickWindow window;
    window.resize(320, 240);

    QQuickItem parentItem;
    parentItem.setPosition(QPointF(0, 0));
    parentItem.setSize(QSizeF(180, 180));
    parentItem.setParentItem(window.contentItem());

    QQuickItem childItem;
    childItem.setPosition(QPointF(60, 90));
    childItem.setSize(QSizeF(120, 120));
    childItem.setParentItem(&parentItem);

    QQuickItem clippingItem;
    clippingItem.setPosition(QPointF(120, 120));
    clippingItem.setSize(QSizeF(180, 180));
    clippingItem.setClip(true);
    clippingItem.setParentItem(window.contentItem());

    QQuickItem clippedItem;
    clippedItem.setPosition(QPointF(-30, -30));
    clippedItem.setSize(QSizeF(120, 120));
    clippedItem.setParentItem(&clippingItem);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Position the cursor over the parent and child item and the clipped section of clippedItem.
    QTest::mouseMove(&window, QPoint(100, 100));

    // No items cursors, window cursor is the default arrow.
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    // The section of clippedItem under the cursor is clipped, and so doesn't affect the window cursor.
    clippedItem.setCursor(Qt::ForbiddenCursor);
    QCOMPARE(clippedItem.cursor().shape(), Qt::ForbiddenCursor);
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    // parentItem is under the cursor, so the window cursor is changed.
    parentItem.setCursor(Qt::IBeamCursor);
    QCOMPARE(parentItem.cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(window.cursor().shape(), Qt::IBeamCursor);

    // childItem is under the cursor and is in front of its parent, so the window cursor is changed.
    childItem.setCursor(Qt::WaitCursor);
    QCOMPARE(childItem.cursor().shape(), Qt::WaitCursor);
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    childItem.setCursor(Qt::PointingHandCursor);
    QCOMPARE(childItem.cursor().shape(), Qt::PointingHandCursor);
    QCOMPARE(window.cursor().shape(), Qt::PointingHandCursor);

    // childItem is the current cursor item, so this has no effect on the window cursor.
    parentItem.unsetCursor();
    QCOMPARE(parentItem.cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(window.cursor().shape(), Qt::PointingHandCursor);

    parentItem.setCursor(Qt::IBeamCursor);
    QCOMPARE(parentItem.cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(window.cursor().shape(), Qt::PointingHandCursor);

    // With the childItem cursor cleared, parentItem is now foremost.
    childItem.unsetCursor();
    QCOMPARE(childItem.cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(window.cursor().shape(), Qt::IBeamCursor);

    // Setting the childItem cursor to the default still takes precedence over parentItem.
    childItem.setCursor(Qt::ArrowCursor);
    QCOMPARE(childItem.cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    childItem.setCursor(Qt::WaitCursor);
    QCOMPARE(childItem.cursor().shape(), Qt::WaitCursor);
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    // Move the cursor so it is over just parentItem.
    QTest::mouseMove(&window, QPoint(20, 20));
    QCOMPARE(window.cursor().shape(), Qt::IBeamCursor);

    // Move the cursor so that is over all items, clippedItem wins because its a child of
    // clippingItem which is in from of parentItem in painting order.
    QTest::mouseMove(&window, QPoint(125, 125));
    QCOMPARE(window.cursor().shape(), Qt::ForbiddenCursor);

    // Over clippingItem only, so no cursor.
    QTest::mouseMove(&window, QPoint(200, 280));
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    // Over no item, so no cursor.
    QTest::mouseMove(&window, QPoint(10, 280));
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    // back to the start.
    QTest::mouseMove(&window, QPoint(100, 100));
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    // Try with the mouse pressed.
    QTest::mousePress(&window, Qt::LeftButton, 0, QPoint(100, 100));
    QTest::mouseMove(&window, QPoint(20, 20));
    QCOMPARE(window.cursor().shape(), Qt::IBeamCursor);
    QTest::mouseMove(&window, QPoint(125, 125));
    QCOMPARE(window.cursor().shape(), Qt::ForbiddenCursor);
    QTest::mouseMove(&window, QPoint(200, 280));
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);
    QTest::mouseMove(&window, QPoint(10, 280));
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);
    QTest::mouseMove(&window, QPoint(100, 100));
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);
    QTest::mouseRelease(&window, Qt::LeftButton, 0, QPoint(100, 100));

    // Remove the cursor item from the scene. Theoretically this should make parentItem the
    // cursorItem, but given the situation will correct itself after the next mouse move it's
    // probably better left as is to avoid unnecessary work during tear down.
    childItem.setParentItem(0);
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    parentItem.setCursor(Qt::SizeAllCursor);
    QCOMPARE(parentItem.cursor().shape(), Qt::SizeAllCursor);
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    // Changing the cursor of an un-parented item doesn't affect the window's cursor.
    childItem.setCursor(Qt::ClosedHandCursor);
    QCOMPARE(childItem.cursor().shape(), Qt::ClosedHandCursor);
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    childItem.unsetCursor();
    QCOMPARE(childItem.cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(window.cursor().shape(), Qt::WaitCursor);

    QTest::mouseRelease(&window, Qt::LeftButton, 0, QPoint(100, 101));
    QCOMPARE(window.cursor().shape(), Qt::SizeAllCursor);
}
#endif

void tst_qquickwindow::hideThenDelete_data()
{
    QTest::addColumn<bool>("persistentSG");
    QTest::addColumn<bool>("persistentGL");

    QTest::newRow("persistent:SG=false,GL=false") << false << false;
    QTest::newRow("persistent:SG=true,GL=false") << true << false;
    QTest::newRow("persistent:SG=false,GL=true") << false << true;
    QTest::newRow("persistent:SG=true,GL=true") << true << true;
}

void tst_qquickwindow::hideThenDelete()
{
    if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
        QSKIP("For some obscure reason this test fails in CI only");
        return;
    }

    QFETCH(bool, persistentSG);
    QFETCH(bool, persistentGL);

    QSignalSpy *openglDestroyed = 0;
    QSignalSpy *sgInvalidated = 0;

    {
        QQuickWindow window;
        window.setColor(Qt::red);

        window.setPersistentSceneGraph(persistentSG);
        window.setPersistentOpenGLContext(persistentGL);

        window.resize(400, 300);
        window.show();

        QTest::qWaitForWindowExposed(&window);

        openglDestroyed = new QSignalSpy(window.openglContext(), SIGNAL(aboutToBeDestroyed()));
        sgInvalidated = new QSignalSpy(&window, SIGNAL(sceneGraphInvalidated()));

        window.hide();

        QTRY_VERIFY(!window.isExposed());

        if (!persistentSG) {
            QVERIFY(sgInvalidated->size() > 0);
            if (!persistentGL)
                QVERIFY(openglDestroyed->size() > 0);
            else
                QVERIFY(openglDestroyed->size() == 0);
        } else {
            QVERIFY(sgInvalidated->size() == 0);
            QVERIFY(openglDestroyed->size() == 0);
        }
    }

    QVERIFY(sgInvalidated->size() > 0);
    QVERIFY(openglDestroyed->size() > 0);
}

void tst_qquickwindow::showHideAnimate()
{
    // This test tries to mimick a bug triggered in the qquickanimatedimage test
    // A window is shown, then removed again before it is exposed. This left
    // traces in the render loop which prevent other animations from running
    // later on.
    {
        QQuickWindow window;
        window.resize(400, 300);
        window.show();
    }

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("showHideAnimate.qml"));
    QQuickItem* created = qobject_cast<QQuickItem *>(component.create());

    QVERIFY(created);

    QTRY_VERIFY(created->opacity() > 0.5);
    QTRY_VERIFY(created->opacity() < 0.5);
}

void tst_qquickwindow::testExpose()
{
    QQuickWindow window;
    window.setGeometry(100, 100, 300, 200);

    window.show();
    QTRY_VERIFY(window.isExposed());

    QSignalSpy swapSpy(&window, SIGNAL(frameSwapped()));

    // exhaust pending exposes, as some platforms send us plenty
    // while showing the first time
    QTest::qWait(1000);
    while (swapSpy.size() != 0) {
        swapSpy.clear();
        QTest::qWait(100);
    }

    QWindowSystemInterface::handleExposeEvent(&window, QRegion(10, 10, 20, 20));
    QTRY_COMPARE(swapSpy.size(), 1);
}

void tst_qquickwindow::requestActivate()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("active.qml"));
    QQuickWindow* window1 = qobject_cast<QQuickWindow *>(component.create());
    QVERIFY(window1);

    QWindowList windows = QGuiApplication::topLevelWindows();
    QVERIFY(windows.size() == 2);

    for (int i = 0; i < windows.size(); ++i) {
        if (windows.at(i)->objectName() == window1->objectName()) {
            windows.removeAt(i);
            break;
        }
    }
    QVERIFY(windows.size() == 1);
    QVERIFY(windows.at(0)->objectName() == "window2");

    window1->show();
    window1->requestActivate();

    QTRY_VERIFY(QGuiApplication::focusWindow() == window1);
    QVERIFY(window1->isActive() == true);

    QQuickItem *item = QQuickVisualTestUtil::findItem<QQuickItem>(window1->contentItem(), "item1");
    QVERIFY(item);

    //copied from src/qmltest/quicktestevent.cpp
    QPoint pos = item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint();

    QMouseEvent me(QEvent::MouseButtonPress, pos, window1->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QSpontaneKeyEvent::setSpontaneous(&me);
    if (!qApp->notify(window1, &me)) {
        QString warning = QString::fromLatin1("Mouse event MousePress not accepted by receiving window");
        QWARN(warning.toLatin1().data());
    }
    me = QMouseEvent(QEvent::MouseButtonPress, pos, window1->mapToGlobal(pos), Qt::LeftButton, 0, Qt::NoModifier);
    QSpontaneKeyEvent::setSpontaneous(&me);
    if (!qApp->notify(window1, &me)) {
        QString warning = QString::fromLatin1("Mouse event MouseRelease not accepted by receiving window");
        QWARN(warning.toLatin1().data());
    }

    QTRY_VERIFY(QGuiApplication::focusWindow() == windows.at(0));
    QVERIFY(windows.at(0)->isActive());
}

void tst_qquickwindow::blockClosing()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("ucantclosethis.qml"));
    QQuickWindow* window = qobject_cast<QQuickWindow *>(component.create());
    QVERIFY(window);
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->isVisible());
    QWindowSystemInterface::handleCloseEvent(window);
    QVERIFY(window->isVisible());
    QWindowSystemInterface::handleCloseEvent(window);
    QVERIFY(window->isVisible());
    window->setProperty("canCloseThis", true);
    QWindowSystemInterface::handleCloseEvent(window);
    QTRY_VERIFY(!window->isVisible());
}

QTEST_MAIN(tst_qquickwindow)

#include "tst_qquickwindow.moc"
