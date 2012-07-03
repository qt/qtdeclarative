/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QDebug>
#include <QTouchEvent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickCanvas>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/private/qquickrectangle_p.h>
#include <qpa/qwindowsysteminterface.h>
#include "../../shared/util.h"
#include <QSignalSpy>
#include <private/qquickcanvas_p.h>
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
    tp.setScreenPos(item->canvas()->mapToGlobal(tp.scenePos().toPoint()));
    tp.setLastScreenPos(item->canvas()->mapToGlobal(tp.lastScenePos().toPoint()));
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
    Q_INVOKABLE QQuickItem *rootItem()
    {
        if (!m_rootItem) {
            QQuickCanvasPrivate *c = QQuickCanvasPrivate::get(canvas());
            m_rootItem = c->rootItem;
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
        setOpacity(1.0);

        lastEvent = makeTouchData(QEvent::None, canvas(), 0, QList<QTouchEvent::TouchPoint>());//CHECK_VALID

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

class tst_qquickcanvas : public QQmlDataTest
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
    void mouseFiltering();
    void headless();

    void touchEvent_basic();
    void touchEvent_propagation();
    void touchEvent_propagation_data();
    void touchEvent_cancel();
    void touchEvent_reentrant();
    void touchEvent_velocity();

    void mouseFromTouch_basic();

    void clearCanvas();

    void qmlCreation();
    void clearColor();

    void grab();
    void multipleWindows();

    void animationsWhileHidden();

    void focusObject();

    void ignoreUnhandledMouseEvents();

    void ownershipRootItem();
private:
    QTouchDevice *touchDevice;
    QTouchDevice *touchDeviceWithVelocity;
};

//If the item calls update inside updatePaintNode, it should schedule another update
void tst_qquickcanvas::constantUpdates()
{
    QQuickCanvas canvas;
    canvas.resize(250, 250);
    ConstantUpdateItem item(canvas.rootItem());
    canvas.show();
    QTRY_VERIFY(item.iterations > 60);
}

void tst_qquickcanvas::touchEvent_basic()
{
    TestTouchItem::clearMousePressCounter();

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    TestTouchItem *bottomItem = new TestTouchItem(canvas->rootItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPos(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPos(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPointF pos(10, 10);

    // press single point
    QTest::touchEvent(canvas, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),canvas);
    QTest::qWait(50);

    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);

    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    // At one point this was failing with kwin (KDE window manager) because canvas->setPos(100, 100)
    // would put the decorated window at that position rather than the canvas itself.
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    topItem->reset();

    // press multiple points
    QTest::touchEvent(canvas, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),canvas)
            .press(1, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();

    // touch point on top item moves to bottom item, but top item should still receive the event
    QTest::touchEvent(canvas, touchDevice).press(0, topItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, touchDevice).move(0, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchUpdate, canvas, Qt::TouchPointMoved,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // touch point on bottom item moves to top item, but bottom item should still receive the event
    QTest::touchEvent(canvas, touchDevice).press(0, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, touchDevice).move(0, topItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchUpdate, canvas, Qt::TouchPointMoved,
            makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos), pos)));
    bottomItem->reset();

    // a single stationary press on an item shouldn't cause an event
    QTest::touchEvent(canvas, touchDevice).press(0, topItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, touchDevice).stationary(0)
            .press(1, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);    // received press only, not stationary
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();
    // cleanup: what is pressed must be released
    // Otherwise you will get an assertion failure:
    // ASSERT: "itemForTouchPointId.isEmpty()" in file items/qquickcanvas.cpp
    QTest::touchEvent(canvas, touchDevice).release(0, pos.toPoint(), canvas).release(1, pos.toPoint(), canvas);

    // move touch point from top item to bottom, and release
    QTest::touchEvent(canvas, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, touchDevice).release(0, bottomItem->mapToScene(pos).toPoint(),canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, canvas, Qt::TouchPointReleased,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // release while another point is pressed
    QTest::touchEvent(canvas, touchDevice).press(0, topItem->mapToScene(pos).toPoint(),canvas)
            .press(1, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, touchDevice).move(0, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, touchDevice).release(0, bottomItem->mapToScene(pos).toPoint(), canvas)
                             .stationary(1);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, canvas, Qt::TouchPointReleased,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos))));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();

    delete topItem;
    delete middleItem;
    delete bottomItem;
    delete canvas;
}

void tst_qquickcanvas::touchEvent_propagation()
{
    TestTouchItem::clearMousePressCounter();

    QFETCH(bool, acceptTouchEvents);
    QFETCH(bool, acceptMouseEvents);
    QFETCH(bool, enableItem);
    QFETCH(qreal, itemOpacity);

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    TestTouchItem *bottomItem = new TestTouchItem(canvas->rootItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPos(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPos(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPointF pos(10, 10);
    QPoint pointInBottomItem = bottomItem->mapToScene(pos).toPoint();  // (10, 10)
    QPoint pointInMiddleItem = middleItem->mapToScene(pos).toPoint();  // (60, 60) overlaps with bottomItem
    QPoint pointInTopItem = topItem->mapToScene(pos).toPoint();  // (110, 110) overlaps with bottom & top items

    // disable topItem
    topItem->acceptTouchEvents = acceptTouchEvents;
    topItem->acceptMouseEvents = acceptMouseEvents;
    topItem->setEnabled(enableItem);
    topItem->setOpacity(itemOpacity);

    // single touch to top item, should be received by middle item
    QTest::touchEvent(canvas, touchDevice).press(0, pointInTopItem, canvas);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(middleItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed,
            makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))));

    // touch top and middle items, middle item should get both events
    QTest::touchEvent(canvas, touchDevice).press(0, pointInTopItem, canvas)
            .press(1, pointInMiddleItem, canvas);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(middleItem->lastEvent.touchPoints.count(), 2);
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed,
           (QList<QTouchEvent::TouchPoint>() << makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))
                                              << makeTouchPoint(middleItem, pos) )));
    middleItem->reset();

    // disable middleItem as well
    middleItem->acceptTouchEvents = acceptTouchEvents;
    middleItem->acceptMouseEvents = acceptMouseEvents;
    middleItem->setEnabled(enableItem);
    middleItem->setOpacity(itemOpacity);

    // touch top and middle items, bottom item should get all events
    QTest::touchEvent(canvas, touchDevice).press(0, pointInTopItem, canvas)
            .press(1, pointInMiddleItem, canvas);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 2);
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed,
            (QList<QTouchEvent::TouchPoint>() << makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos))
                                              << makeTouchPoint(bottomItem, bottomItem->mapFromItem(middleItem, pos)) )));
    bottomItem->reset();

    // disable bottom item as well
    bottomItem->acceptTouchEvents = acceptTouchEvents;
    bottomItem->setEnabled(enableItem);
    bottomItem->setOpacity(itemOpacity);

    // no events should be received
    QTest::touchEvent(canvas, touchDevice).press(0, pointInTopItem, canvas)
            .press(1, pointInMiddleItem, canvas)
            .press(2, pointInBottomItem, canvas);
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
    middleItem->setOpacity(itemOpacity);
    QTest::touchEvent(canvas, touchDevice).press(0, pointInTopItem, canvas);
    QTest::qWait(50);
    if (!enableItem || itemOpacity == 0) {
        // middle item is disabled or has 0 opacity, bottom item receives the event
        QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
        QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
        QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
        COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed,
                makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos))));
    } else {
        // middle item ignores event, sends it to the top item (top-most child)
        QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
        QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
        QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
        COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed,
                makeTouchPoint(topItem, pos)));
    }

    delete topItem;
    delete middleItem;
    delete bottomItem;
    delete canvas;
}

void tst_qquickcanvas::touchEvent_propagation_data()
{
    QTest::addColumn<bool>("acceptTouchEvents");
    QTest::addColumn<bool>("acceptMouseEvents");
    QTest::addColumn<bool>("enableItem");
    QTest::addColumn<qreal>("itemOpacity");

    QTest::newRow("disable events") << false << false << true << 1.0;
    QTest::newRow("disable item") << true << true << false << 1.0;
    QTest::newRow("opacity of 0") << true << true << true << 0.0;
}

void tst_qquickcanvas::touchEvent_cancel()
{
    TestTouchItem::clearMousePressCounter();

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    TestTouchItem *item = new TestTouchItem(canvas->rootItem());
    item->setPos(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));

    QPointF pos(10, 10);
    QTest::touchEvent(canvas, touchDevice).press(0, item->mapToScene(pos).toPoint(),canvas);
    QCoreApplication::processEvents();

    QTRY_COMPARE(item->lastEvent.touchPoints.count(), 1);
    TouchEventData d = makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(item,pos));
    COMPARE_TOUCH_DATA(item->lastEvent, d);
    item->reset();

    QWindowSystemInterface::handleTouchCancelEvent(0, touchDevice);
    QCoreApplication::processEvents();
    d = makeTouchData(QEvent::TouchCancel, canvas);
    COMPARE_TOUCH_DATA(item->lastEvent, d);

    delete item;
    delete canvas;
}

void tst_qquickcanvas::touchEvent_reentrant()
{
    TestTouchItem::clearMousePressCounter();

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    TestTouchItem *item = new TestTouchItem(canvas->rootItem());

    item->spinLoopWhenPressed = true; // will call processEvents() from the touch handler

    item->setPos(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));
    QPointF pos(60, 60);

    // None of these should commit from the dtor.
    QTest::QTouchEventSequence press = QTest::touchEvent(canvas, touchDevice, false).press(0, pos.toPoint(), canvas);
    pos += QPointF(2, 2);
    QTest::QTouchEventSequence move = QTest::touchEvent(canvas, touchDevice, false).move(0, pos.toPoint(), canvas);
    QTest::QTouchEventSequence release = QTest::touchEvent(canvas, touchDevice, false).release(0, pos.toPoint(), canvas);

    // Now commit (i.e. call QWindowSystemInterface::handleTouchEvent), but do not process the events yet.
    press.commit(false);
    move.commit(false);
    release.commit(false);

    QCoreApplication::processEvents();

    QTRY_COMPARE(item->touchEventCount, 3);

    delete item;
    delete canvas;
}

void tst_qquickcanvas::touchEvent_velocity()
{
    TestTouchItem::clearMousePressCounter();

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);
    QTest::qWait(10);

    TestTouchItem *item = new TestTouchItem(canvas->rootItem());
    item->setPos(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 1;
    tp.state = Qt::TouchPointPressed;
    QPoint pos = canvas->mapToGlobal(item->mapToScene(QPointF(10, 10)).toPoint());
    tp.area = QRectF(pos, QSizeF(4, 4));
    points << tp;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area.adjust(5, 5, 5, 5);
    QVector2D velocity(1.5, 2.5);
    points[0].velocity = velocity;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
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
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    QCOMPARE(item->lastVelocity, transformedVelocity);
    QPoint itemLocalPos = item->mapFromScene(canvas->mapFromGlobal(points[0].area.center().toPoint())).toPoint();
    QPoint itemLocalPosFromEvent = item->lastEvent.touchPoints[0].pos().toPoint();
    QCOMPARE(itemLocalPos, itemLocalPosFromEvent);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    delete item;
    delete canvas;
}

void tst_qquickcanvas::mouseFromTouch_basic()
{
    // Turn off accepting touch events with acceptTouchEvents. This
    // should result in sending mouse events generated from the touch
    // with the new event propagation system.

    TestTouchItem::clearMousePressCounter();
    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);
    QTest::qWait(10);

    TestTouchItem *item = new TestTouchItem(canvas->rootItem());
    item->setPos(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));
    item->acceptTouchEvents = false;

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 1;
    tp.state = Qt::TouchPointPressed;
    QPoint pos = canvas->mapToGlobal(item->mapToScene(QPointF(10, 10)).toPoint());
    tp.area = QRectF(pos, QSizeF(4, 4));
    points << tp;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area.adjust(5, 5, 5, 5);
    QVector2D velocity(1.5, 2.5);
    points[0].velocity = velocity;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();

    // The item should have received a mouse press, move, and release.
    QCOMPARE(item->mousePressNum, 1);
    QCOMPARE(item->mouseMoveNum, 1);
    QCOMPARE(item->mouseReleaseNum, 1);
    QCOMPARE(item->lastMousePos.toPoint(), item->mapFromScene(canvas->mapFromGlobal(points[0].area.center().toPoint())).toPoint());
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
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    points[0].state = Qt::TouchPointMoved;
    points[0].area.adjust(5, 5, 5, 5);
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    QCOMPARE(item->lastMousePos.toPoint(), item->mapFromScene(canvas->mapFromGlobal(points[0].area.center().toPoint())).toPoint());
    QCOMPARE(item->lastVelocityFromMouseMove, transformedVelocity);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(canvas, touchDeviceWithVelocity, points);
    QCoreApplication::processEvents();
    delete item;
    delete canvas;
}

void tst_qquickcanvas::clearCanvas()
{
    QQuickCanvas *canvas = new QQuickCanvas;
    QQuickItem *item = new QQuickItem;
    item->setParentItem(canvas->rootItem());

    QVERIFY(item->canvas() == canvas);

    delete canvas;

    QVERIFY(item->canvas() == 0);

    delete item;
}

void tst_qquickcanvas::mouseFiltering()
{
    TestTouchItem::clearMousePressCounter();

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    TestTouchItem *bottomItem = new TestTouchItem(canvas->rootItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPos(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPos(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPoint pos(100, 100);

    QTest::mousePress(canvas, Qt::LeftButton, 0, pos);

    // Mouse filtering propagates down the stack, so the
    // correct order is
    // 1. middleItem filters event
    // 2. bottomItem filters event
    // 3. topItem receives event
    QTRY_COMPARE(middleItem->mousePressId, 1);
    QTRY_COMPARE(bottomItem->mousePressId, 2);
    QTRY_COMPARE(topItem->mousePressId, 3);

    delete canvas;
}

void tst_qquickcanvas::qmlCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("window.qml"));
    QObject* created = component.create();
    QVERIFY(created);

    QQuickCanvas* canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);
    QCOMPARE(canvas->clearColor(), QColor(Qt::green));

    QQuickItem* item = canvas->findChild<QQuickItem*>("item");
    QVERIFY(item);
    QCOMPARE(item->canvas(), canvas);

    delete canvas;
}

void tst_qquickcanvas::clearColor()
{
    //### Can we examine rendering to make sure it is really blue?
    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->setPos(100, 100);
    canvas->setClearColor(Qt::blue);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);
    QCOMPARE(canvas->clearColor(), QColor(Qt::blue));
    delete canvas;
}

void tst_qquickcanvas::grab()
{
    QQuickCanvas canvas;
    canvas.setClearColor(Qt::red);

    canvas.resize(250, 250);
    canvas.show();

    QTest::qWaitForWindowShown(&canvas);

    QImage content = canvas.grabFrameBuffer();
    QCOMPARE(content.width(), canvas.width());
    QCOMPARE(content.height(), canvas.height());
    QCOMPARE((uint) content.convertToFormat(QImage::Format_RGB32).pixel(0, 0), (uint) 0xffff0000);
}

void tst_qquickcanvas::multipleWindows()
{
    QList<QQuickCanvas *> windows;
    for (int i=0; i<6; ++i) {
        QQuickCanvas *c = new QQuickCanvas();
        c->setClearColor(Qt::GlobalColor(Qt::red + i));
        c->resize(300, 200);
        c->setPos(100 + i * 30, 100 + i * 20);
        c->show();
        windows << c;
        QVERIFY(c->isVisible());
    }

    // move them
    for (int i=0; i<windows.size(); ++i) {
        QQuickCanvas *c = windows.at(i);
        c->setPos(c->x() - 10, c->y() - 10);
    }

    // resize them
    for (int i=0; i<windows.size(); ++i) {
        QQuickCanvas *c = windows.at(i);
        c->resize(200, 150);
    }

    qDeleteAll(windows);
}

void tst_qquickcanvas::animationsWhileHidden()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("AnimationsWhileHidden.qml"));
    QObject* created = component.create();

    QQuickCanvas* canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);
    QVERIFY(canvas->isVisible());

    // Now hide the window and verify that it went off screen
    canvas->hide();
    QTest::qWait(10);
    QVERIFY(!canvas->isVisible());

    // Running animaiton should cause it to become visible again shortly.
    QTRY_VERIFY(canvas->isVisible());

    delete canvas;
}


void tst_qquickcanvas::headless()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("Headless.qml"));
    QObject* created = component.create();

    QQuickCanvas* canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);

    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->isVisible());

    QSignalSpy initialized(canvas, SIGNAL(sceneGraphInitialized()));
    QSignalSpy invalidated(canvas, SIGNAL(sceneGraphInvalidated()));

    // Verify that the canvas is alive and kicking
    QVERIFY(canvas->openglContext() != 0);

    // Store the visual result
    QImage originalContent = canvas->grabFrameBuffer();

    // Hide the canvas and verify signal emittion and GL context deletion
    canvas->hide();
    canvas->releaseResources();

    QTRY_COMPARE(invalidated.size(), 1);
    QVERIFY(canvas->openglContext() == 0);

    // Destroy the native windowing system buffers
    canvas->destroy();
    QVERIFY(canvas->handle() == 0);

    // Show and verify that we are back and running
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QTRY_COMPARE(initialized.size(), 1);
    QVERIFY(canvas->openglContext() != 0);

    // Verify that the visual output is the same
    QImage newContent = canvas->grabFrameBuffer();

    QCOMPARE(originalContent, newContent);

    delete canvas;
}

void tst_qquickcanvas::focusObject()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("focus.qml"));
    QObject *created = component.create();
    QVERIFY(created);

    QQuickCanvas *canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);

    QQuickItem *item1 = canvas->findChild<QQuickItem*>("item1");
    QVERIFY(item1);
    item1->setFocus(true);
    QCOMPARE(item1, canvas->focusObject());

    QQuickItem *item2 = canvas->findChild<QQuickItem*>("item2");
    QVERIFY(item2);
    item2->setFocus(true);
    QCOMPARE(item2, canvas->focusObject());

    delete canvas;
}

void tst_qquickcanvas::ignoreUnhandledMouseEvents()
{
    QQuickCanvas* canvas = new QQuickCanvas;
    canvas->resize(100, 100);
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickItem* item = new QQuickItem;
    item->setSize(QSizeF(100, 100));
    item->setParentItem(canvas->rootItem());

    {
        QMouseEvent me(QEvent::MouseButtonPress, QPointF(50, 50), Qt::LeftButton, Qt::LeftButton,
                       Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(canvas, &me));
        QVERIFY(!me.isAccepted());
    }

    {
        QMouseEvent me(QEvent::MouseMove, QPointF(51, 51), Qt::LeftButton, Qt::LeftButton,
                       Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(canvas, &me));
        QVERIFY(!me.isAccepted());
    }

    {
        QMouseEvent me(QEvent::MouseButtonRelease, QPointF(51, 51), Qt::LeftButton, Qt::LeftButton,
                       Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(canvas, &me));
        QVERIFY(!me.isAccepted());
    }

    delete canvas;
}


void tst_qquickcanvas::ownershipRootItem()
{
    qmlRegisterType<RootItemAccessor>("QtQuick", 2, 0, "RootItemAccessor");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("ownershipRootItem.qml"));
    QObject* created = component.create();

    QQuickCanvas* canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);
    QTest::qWaitForWindowShown(canvas);

    RootItemAccessor* accessor = canvas->findChild<RootItemAccessor*>("accessor");
    QVERIFY(accessor);
    engine.collectGarbage();

    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(!accessor->isRootItemDestroyed());
}
QTEST_MAIN(tst_qquickcanvas)

#include "tst_qquickcanvas.moc"
