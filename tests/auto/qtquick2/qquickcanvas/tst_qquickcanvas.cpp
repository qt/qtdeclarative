/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qtest.h>
#include <QDebug>
#include <QTouchEvent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickCanvas>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtGui/QWindowSystemInterface>
#include "../../shared/util.h"
#include <QSignalSpy>

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

static TouchEventData makeTouchData(QEvent::Type type, QWindow *w, Qt::TouchPointStates states, const QList<QTouchEvent::TouchPoint>& touchPoints)
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

class TestTouchItem : public QQuickRectangle
{
    Q_OBJECT
public:
    TestTouchItem(QQuickItem *parent = 0)
        : QQuickRectangle(parent), acceptEvents(true), mousePressId(0)
    {
        border()->setWidth(1);
        setAcceptedMouseButtons(Qt::LeftButton);
        setFiltersChildMouseEvents(true);
    }

    void reset() {
        acceptEvents = true;
        setEnabled(true);
        setOpacity(1.0);

        lastEvent = makeTouchData(QEvent::None, canvas(), 0, QList<QTouchEvent::TouchPoint>());//CHECK_VALID
    }

    bool acceptEvents;
    TouchEventData lastEvent;
    int mousePressId;

protected:
    virtual void touchEvent(QTouchEvent *event) {
        if (!acceptEvents) {
            event->ignore();
            return;
        }
        lastEvent = makeTouchData(event->type(), event->window(), event->touchPointStates(), event->touchPoints());
        event->accept();
    }

    virtual void mousePressEvent(QMouseEvent *) {
        mousePressId = ++mousePressNum;
    }

    bool childMouseEventFilter(QQuickItem *, QEvent *event) {
        if (event->type() == QEvent::MouseButtonPress)
            mousePressId = ++mousePressNum;
        return false;
    }

    static int mousePressNum;
};

int TestTouchItem::mousePressNum = 0;

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

class tst_qquickcanvas : public QDeclarativeDataTest
{
    Q_OBJECT
public:

private slots:
    void constantUpdates();

    void touchEvent_basic();
    void touchEvent_propagation();
    void touchEvent_propagation_data();

    void clearCanvas();
    void mouseFiltering();

    void qmlCreation();
    void clearColor();

    void grab();
    void multipleWindows();

    void animationsWhileHidden();

    void headless();
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
    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->move(100, 100);
    canvas->show();

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

    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);

    // press single point
    QTest::touchEvent(canvas, device).press(0, topItem->mapToScene(pos).toPoint(),canvas);
    QTest::qWait(50);

    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);

    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    TouchEventData d = makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(topItem,pos));
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    topItem->reset();

    // press multiple points
    QTest::touchEvent(canvas, device).press(0, topItem->mapToScene(pos).toPoint(),canvas)
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
    QTest::touchEvent(canvas, device).press(0, topItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, device).move(0, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchUpdate, canvas, Qt::TouchPointMoved,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // touch point on bottom item moves to top item, but bottom item should still receive the event
    QTest::touchEvent(canvas, device).press(0, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, device).move(0, topItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchUpdate, canvas, Qt::TouchPointMoved,
            makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos), pos)));
    bottomItem->reset();

    // a single stationary press on an item shouldn't cause an event
    QTest::touchEvent(canvas, device).press(0, topItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, device).stationary(0)
            .press(1, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);    // received press only, not stationary
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();

    // move touch point from top item to bottom, and release
    QTest::touchEvent(canvas, device).press(0, topItem->mapToScene(pos).toPoint(),canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, device).release(0, bottomItem->mapToScene(pos).toPoint(),canvas);
    QTest::qWait(50);
    QCOMPARE(topItem->lastEvent.touchPoints.count(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, canvas, Qt::TouchPointReleased,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // release while another point is pressed
    QTest::touchEvent(canvas, device).press(0, topItem->mapToScene(pos).toPoint(),canvas)
            .press(1, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, device).move(0, bottomItem->mapToScene(pos).toPoint(), canvas);
    QTest::qWait(50);
    QTest::touchEvent(canvas, device).release(0, bottomItem->mapToScene(pos).toPoint(), canvas)
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
    QFETCH(bool, acceptEvents);
    QFETCH(bool, enableItem);
    QFETCH(qreal, itemOpacity);

    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);

    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->move(100, 100);
    canvas->show();

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
    topItem->acceptEvents = acceptEvents;
    topItem->setEnabled(enableItem);
    topItem->setOpacity(itemOpacity);

    // single touch to top item, should be received by middle item
    QTest::touchEvent(canvas, device).press(0, pointInTopItem, canvas);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(middleItem->lastEvent.touchPoints.count(), 1);
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, canvas, Qt::TouchPointPressed,
            makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))));

    // touch top and middle items, middle item should get both events
    QTest::touchEvent(canvas, device).press(0, pointInTopItem, canvas)
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
    middleItem->acceptEvents = acceptEvents;
    middleItem->setEnabled(enableItem);
    middleItem->setOpacity(itemOpacity);

    // touch top and middle items, bottom item should get all events
    QTest::touchEvent(canvas, device).press(0, pointInTopItem, canvas)
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
    bottomItem->acceptEvents = acceptEvents;
    bottomItem->setEnabled(enableItem);
    bottomItem->setOpacity(itemOpacity);

    // no events should be received
    QTest::touchEvent(canvas, device).press(0, pointInTopItem, canvas)
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
    middleItem->acceptEvents = acceptEvents;
    middleItem->setEnabled(enableItem);
    middleItem->setOpacity(itemOpacity);
    QTest::touchEvent(canvas, device).press(0, pointInTopItem, canvas);
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
    QTest::addColumn<bool>("acceptEvents");
    QTest::addColumn<bool>("enableItem");
    QTest::addColumn<qreal>("itemOpacity");

    QTest::newRow("disable events") << false << true << 1.0;
    QTest::newRow("disable item") << true << false << 1.0;
    QTest::newRow("opacity of 0") << true << true << 0.0;
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
    QQuickCanvas *canvas = new QQuickCanvas;
    canvas->resize(250, 250);
    canvas->move(100, 100);
    canvas->show();

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
    QTest::qWait(50);

    // Mouse filtering propagates down the stack, so the
    // correct order is
    // 1. middleItem filters event
    // 2. bottomItem filters event
    // 3. topItem receives event
    QCOMPARE(middleItem->mousePressId, 1);
    QCOMPARE(bottomItem->mousePressId, 2);
    QCOMPARE(topItem->mousePressId, 3);

    delete canvas;
}

void tst_qquickcanvas::qmlCreation()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
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
    canvas->move(100, 100);
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
        QVERIFY(c->visible());
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
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.loadUrl(testFileUrl("AnimationsWhileHidden.qml"));
    QObject* created = component.create();

    QQuickCanvas* canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);
    QVERIFY(canvas->visible());

    // Now hide the window and verify that it went off screen
    canvas->hide();
    QTest::qWait(10);
    QVERIFY(!canvas->visible());

    // Running animaiton should cause it to become visible again shortly.
    QTRY_VERIFY(canvas->visible());

    delete canvas;
}


void tst_qquickcanvas::headless()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.loadUrl(testFileUrl("Headless.qml"));
    QObject* created = component.create();

    QQuickCanvas* canvas = qobject_cast<QQuickCanvas*>(created);
    QVERIFY(canvas);

    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->visible());

    QSignalSpy initialized(canvas, SIGNAL(sceneGraphInitialized()));
    QSignalSpy invalidated(canvas, SIGNAL(sceneGraphInvalidated()));

    // Verify that the canvas is alive and kicking
    QVERIFY(canvas->openglContext() != 0);

    // Store the visual result
    QImage originalContent = canvas->grabFrameBuffer();

    // Hide the canvas and verify signal emittion and GL context deletion
    canvas->hide();
    QCOMPARE(invalidated.size(), 1);
    QVERIFY(canvas->openglContext() == 0);

    // Destroy the native windowing system buffers
    canvas->destroy();
    QVERIFY(canvas->handle() == 0);

    // Show and verify that we are back and running
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QCOMPARE(initialized.size(), 1);
    QVERIFY(canvas->openglContext() != 0);

    // Verify that the visual output is the same
    QImage newContent = canvas->grabFrameBuffer();

    QCOMPARE(originalContent, newContent);


}

QTEST_MAIN(tst_qquickcanvas)

#include "tst_qquickcanvas.moc"
