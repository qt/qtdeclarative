// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QDebug>
#include <QEvent>
#include <QMimeData>
#include <QTouchEvent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickloader_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QSignalSpy>
#include <private/qquickwindow_p.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QRunnable>
#include <QSGRendererInterface>
#include <QQuickRenderControl>
#include <QOperatingSystemVersion>
#include <functional>
#include <QtGui/private/qeventpoint_p.h>
#include <rhi/qrhi.h>
#if QT_CONFIG(opengl)
#include <QOpenGLContext>
#endif
#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#endif

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

struct TouchEventData {
    QEvent::Type type;
    QWidget *widget;
    QWindow *window;
    QEventPoint::States states;
    QList<QEventPoint> touchPoints;
};

static QEventPoint makeTouchPoint(QQuickItem *item, const QPointF &p, const QPointF &lastPoint = QPointF())
{
    QPointF last = lastPoint.isNull() ? p : lastPoint;

    QEventPoint tp;

    QMutableEventPoint::setPosition(tp, p);
    QMutableEventPoint::setScenePosition(tp, item->mapToScene(p));
    QMutableEventPoint::setGlobalPosition(tp, item->mapToGlobal(p));
    QMutableEventPoint::setGlobalLastPosition(tp, item->mapToGlobal(last));
    return tp;
}

static TouchEventData makeTouchData(QEvent::Type type, QWindow *w, QEventPoint::States states = {},
                                    const QList<QEventPoint>& touchPoints = QList<QEventPoint>())
{
    TouchEventData d = { type, nullptr, w, states, touchPoints };
    for (auto &pt : d.touchPoints)
        QMutableEventPoint::detach(pt);
    return d;
}
static TouchEventData makeTouchData(QEvent::Type type, QWindow *w, QEventPoint::States states,
                                    const QList<QEventPoint*>& touchPoints)
{
    QList <QEventPoint> pts;
    for (auto pt : touchPoints)
        pts << *pt;
    TouchEventData d = { type, nullptr, w, states, pts };
    return d;
}
static TouchEventData makeTouchData(QEvent::Type type, QWindow *w, QEventPoint::States states, const QEventPoint &touchPoint)
{
    QList<QEventPoint*> points;
    points << const_cast<QEventPoint *>(&touchPoint);
    return makeTouchData(type, w, states, points);
}

#define COMPARE_TOUCH_POINTS(tp1, tp2) \
{ \
    QCOMPARE(tp1.position(), tp2.position()); \
    QCOMPARE(tp1.lastPosition(), tp2.lastPosition()); \
    QCOMPARE(tp1.scenePosition(), tp2.scenePosition()); \
    QCOMPARE(tp1.sceneLastPosition(), tp2.sceneLastPosition()); \
    QCOMPARE(tp1.globalPosition(), tp2.globalPosition()); \
    QCOMPARE(tp1.globalLastPosition(), tp2.globalLastPosition()); \
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
        , m_rootItem(nullptr)
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
    TestTouchItem(QQuickItem *parent = nullptr)
        : QQuickRectangle(parent), acceptTouchEvents(true), acceptMouseEvents(true),
          mousePressCount(0), mouseMoveCount(0),
          spinLoopWhenPressed(false), touchEventCount(0),
          mouseUngrabEventCount(0)
    {
        border()->setWidth(1);
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptTouchEvents(true);
        setFiltersChildMouseEvents(true);
    }

    void reset() {
        acceptTouchEvents = acceptMouseEvents = true;
        setEnabled(true);
        setVisible(true);

        lastEvent = makeTouchData(QEvent::None, window());//CHECK_VALID

        lastVelocity = lastVelocityFromMouseMove = QVector2D();
        lastMousePos = QPointF();
        lastMouseCapabilityFlags = {};
        touchEventCount = 0;
        mouseMoveCount = 0;
        mouseUngrabEventCount = 0;
    }

    static void clearMouseEventCounters()
    {
        mousePressNum = mouseMoveNum = mouseReleaseNum = 0;
    }

    void clearTouchEventCounter()
    {
        touchEventCount = 0;
    }

    bool acceptTouchEvents;
    bool acceptMouseEvents;
    bool grabOnRelease = false;
    TouchEventData lastEvent;
    int mousePressCount;
    int mouseMoveCount;
    bool spinLoopWhenPressed;
    int touchEventCount;
    int mouseUngrabEventCount;
    QVector2D lastVelocity;
    QVector2D lastVelocityFromMouseMove;
    QPointF lastMousePos;
    QInputDevice::Capabilities lastMouseCapabilityFlags;

    void touchEvent(QTouchEvent *event)  override
    {
        if (!acceptTouchEvents) {
            event->ignore();
            return;
        }
        qCDebug(lcTests) << objectName() << event;
        ++touchEventCount;
        lastEvent = makeTouchData(event->type(), nullptr, event->touchPointStates(), event->points());
        if (event->device()->capabilities().testFlag(QPointingDevice::Capability::Velocity) && !event->points().isEmpty()) {
            lastVelocity = event->points().first().velocity();
        } else {
            lastVelocity = QVector2D();
        }
        if (spinLoopWhenPressed && event->touchPointStates().testFlag(QEventPoint::State::Pressed)) {
            QCoreApplication::processEvents();
        }
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (!acceptMouseEvents) {
            e->ignore();
            return;
        }
        mousePressCount = ++mousePressNum;
        lastMousePos = e->position().toPoint();
        lastMouseCapabilityFlags = e->device()->capabilities();
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!acceptMouseEvents) {
            e->ignore();
            return;
        }
        mouseMoveCount = ++mouseMoveNum;
        lastVelocityFromMouseMove = e->points().first().velocity();
        lastMouseCapabilityFlags = e->device()->capabilities();
        lastMousePos = e->position().toPoint();
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (!acceptMouseEvents) {
            e->ignore();
            return;
        }
        ++mouseReleaseNum;
        lastMousePos = e->position().toPoint();
        lastMouseCapabilityFlags = e->device()->capabilities();
    }

    void mouseUngrabEvent() override
    {
        qCDebug(lcTests) << objectName();
        ++mouseUngrabEventCount;
    }

    bool childMouseEventFilter(QQuickItem *item, QEvent *e) override
    {
        qCDebug(lcTests) << objectName() << "filtering" << e << "ahead of delivery to" << item->metaObject()->className() << item->objectName();
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            mousePressCount = ++mousePressNum;
            break;
        case QEvent::MouseButtonRelease:
            if (grabOnRelease)
                grabMouse();
            break;
        case QEvent::MouseMove:
            mouseMoveCount = ++mouseMoveNum;
            break;
        default:
            break;
        }

        return false;
    }

    static int mousePressNum, mouseMoveNum, mouseReleaseNum;
};

int TestTouchItem::mousePressNum = 0;
int TestTouchItem::mouseMoveNum = 0;
int TestTouchItem::mouseReleaseNum = 0;

class EventFilter : public QObject
{
public:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        Q_UNUSED(watched);
        events.append(event->type());
        return false;
    }

    QList<int> events;
};

class ConstantUpdateItem : public QQuickItem
{
Q_OBJECT
public:
    ConstantUpdateItem(QQuickItem *parent = nullptr) : QQuickItem(parent), iterations(0) {setFlag(ItemHasContents);}

    int iterations;
protected:
    QSGNode* updatePaintNode(QSGNode *, UpdatePaintNodeData *) override
    {
        iterations++;
        update();
        return nullptr;
    }
};

class PointerRecordingWindow : public QQuickWindow
{
public:
    explicit PointerRecordingWindow(QWindow *parent = nullptr) : QQuickWindow(parent) { }

protected:
    bool event(QEvent *event) override {
        if (event->isPointerEvent()) {
            qCDebug(lcTests) << event;
            m_events << PointerEvent { event->type(), static_cast<QPointerEvent *>(event)->pointingDevice() };
        }
        return QQuickWindow::event(event);
    }

    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << PointerEvent { event->type(), event->pointingDevice() };
        QQuickWindow::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << PointerEvent { event->type(), event->pointingDevice() };
        QQuickWindow::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << PointerEvent { event->type(), event->pointingDevice() };
        QQuickWindow::mouseReleaseEvent(event);
    }

    void touchEvent(QTouchEvent * event) override {
        qCDebug(lcTests) << event;
        m_touchEvents << PointerEvent { event->type(), event->pointingDevice() };
        QQuickWindow::touchEvent(event);
    }

#if QT_CONFIG(tabletevent)
    void tabletEvent(QTabletEvent * event) override {
        qCDebug(lcTests) << event;
        m_tabletEvents << PointerEvent { event->type(), event->pointingDevice() };
        QQuickWindow::tabletEvent(event);
    }
#endif

#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent * event) override {
        qCDebug(lcTests) << event;
        m_tabletEvents << PointerEvent { event->type(), event->pointingDevice() };
        QQuickWindow::wheelEvent(event);
    }
#endif

public:
    struct PointerEvent
    {
        QEvent::Type type;
        const QPointingDevice *device;
    };
    QList<PointerEvent> m_events;
    QList<PointerEvent> m_mouseEvents;
    QList<PointerEvent> m_touchEvents;
    QList<PointerEvent> m_tabletEvents;
};

class MouseRecordingItem : public QQuickItem
{
public:
    MouseRecordingItem(bool acceptTouch, QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
        setSize(QSizeF(300, 300));
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptTouchEvents(acceptTouch);
    }

protected:
    void touchEvent(QTouchEvent* event) override {
        m_touchEvents << event->type();
        qCDebug(lcTests) << "accepted?" << event->isAccepted() << event;
    }
    void mousePressEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << MouseEvent{event->type(), event->source()};
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << MouseEvent{event->type(), event->source()};
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << MouseEvent{event->type(), event->source()};
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override {
        qCDebug(lcTests) << event;
        m_mouseEvents << MouseEvent{event->type(), event->source()};
    }

public:
    struct MouseEvent
    {
        QEvent::Type type;
        Qt::MouseEventSource source;
    };
    QList<MouseEvent> m_mouseEvents;
    QList<QEvent::Type> m_touchEvents;
};

class tst_qquickwindow : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickwindow()
      : QQmlDataTest(QT_QMLTEST_DATADIR)
      , touchDevice(QTest::createTouchDevice())
      , touchDeviceWithVelocity(QTest::createTouchDevice(QInputDevice::DeviceType::TouchScreen,
            QInputDevice::Capability::Position | QPointingDevice::Capability::Velocity))
      , tabletStylusDevice(QPointingDevicePrivate::tabletDevice(QInputDevice::DeviceType::Stylus,
                                                                QPointingDevice::PointerType::Pen,
                                                                QPointingDeviceUniqueId::fromNumericId(1234567890)))
    {
        QQuickWindow::setDefaultAlphaBuffer(true);
    }

private slots:
    void initTestCase() override;
    void cleanup();
    void aboutToStopSignal();

    void constantUpdates();
    void constantUpdatesOnWindow_data();
    void constantUpdatesOnWindow();
    void mouseFiltering();
    void headless();
    void destroyShowWithoutHide();

    void touchEvent_basic();
    void touchEvent_propagation();
    void touchEvent_propagation_data();
    void touchEvent_cancel();
    void touchEvent_cancelClearsMouseGrab();
    void touchEvent_reentrant();
    void touchEvent_velocity();

    void mergeTouchPointLists_data();
    void mergeTouchPointLists();

    void mouseFromTouch_basic();
    void synthMouseFromTouch_data();
    void synthMouseFromTouch();
    void synthMouseDoubleClickFromTouch_data();
    void synthMouseDoubleClickFromTouch();

    void clearWindow();

    void qmlCreation();
    void qmlCreationWithScreen();
    void clearColor();
    void defaultState();

    void grab_data();
    void grab();
    void earlyGrab();
    void multipleWindows();

    void animationsWhileHidden();

    void focusObject();
    void focusReason();

    void ignoreUnhandledMouseEvents();

    void ownershipRootItem();

    void hideThenDelete_data();
    void hideThenDelete();

    void showHideAnimate();

    void testExpose();

    void requestActivate();

    void testWindowVisibilityOrder();

    void blockClosing();
    void blockCloseMethod();

    void crashWhenHoverItemDeleted();

    void unloadSubWindow();
    void changeVisibilityInCompleted();

    void qobjectEventFilter_touch();
    void qobjectEventFilter_key();
    void qobjectEventFilter_mouse();

#if QT_CONFIG(cursor)
    void cursor();
#endif

    void animatingSignal();
    void frameSignals();

    void contentItemSize();

    void defaultSurfaceFormat();

    void attachedProperty();

    void testRenderJob();

    void testHoverChildMouseEventFilter();
    void testHoverTimestamp();
    void test_circleMapItem();

    void grabContentItemToImage();

    void testDragEventPropertyPropagation();

    void findChild();

    void testChildMouseEventFilter();
    void testChildMouseEventFilter_data();
    void cleanupGrabsOnRelease();

    void subclassWithPointerEventVirtualOverrides_data();
    void subclassWithPointerEventVirtualOverrides();

#if QT_CONFIG(shortcut)
    void testShortCut();
    void shortcutOverride_data();
    void shortcutOverride();
#endif

    void rendererInterface();

    void rendererInterfaceWithRenderControl_data();
    void rendererInterfaceWithRenderControl();

    void graphicsConfiguration();

    void visibleVsVisibility_data();
    void visibleVsVisibility();

private:
    QPointingDevice *touchDevice; // TODO make const after fixing QTBUG-107864
    const QPointingDevice *touchDeviceWithVelocity;
    const QPointingDevice *tabletStylusDevice;
};

#if QT_CONFIG(opengl)
Q_DECLARE_METATYPE(QOpenGLContext *);
#endif

void tst_qquickwindow::initTestCase()
{
    // for the graphicsConfiguration test
    qunsetenv("QSG_NO_DEPTH_BUFFER");
    qunsetenv("QSG_RHI_DEBUG_LAYER");
    qunsetenv("QSG_RHI_PROFILE");
    qunsetenv("QSG_RHI_PREFER_SOFTWARE_RENDERER");
    qunsetenv("QT_DISABLE_SHADER_DISK_CACHE");
    qunsetenv("QSG_RHI_DISABLE_DISK_CACHE");
    qunsetenv("QSG_RHI_PIPELINE_CACHE_SAVE");
    qunsetenv("QSG_RHI_PIPELINE_CACHE_LOAD");

    QQmlDataTest::initTestCase();
}

void tst_qquickwindow::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

void tst_qquickwindow::aboutToStopSignal()
{
    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QSignalSpy spy(&window, SIGNAL(sceneGraphAboutToStop()));

    window.hide();

    QTRY_VERIFY(spy.size() > 0);
}

//If the item calls update inside updatePaintNode, it should schedule another sync pass
void tst_qquickwindow::constantUpdates()
{
    QQuickWindow window;
    window.resize(250, 250);
    ConstantUpdateItem item(window.contentItem());
    window.setTitle(QTest::currentTestFunction());
    window.show();

    QSignalSpy beforeSpy(&window, SIGNAL(beforeSynchronizing()));
    QSignalSpy afterSpy(&window, SIGNAL(afterSynchronizing()));

    QTRY_VERIFY(item.iterations > 10);
    QTRY_VERIFY(beforeSpy.size() > 10);
    QTRY_VERIFY(afterSpy.size() > 10);
}

void tst_qquickwindow::constantUpdatesOnWindow_data()
{
    QTest::addColumn<bool>("blockedGui");
    QTest::addColumn<QByteArray>("signal");

    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.setGeometry(100, 100, 300, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const bool threaded = QQuickWindowPrivate::get(&window)->context->thread() != QGuiApplication::instance()->thread();
    if (threaded) {
        QTest::newRow("blocked, beforeRender") << true << QByteArray(SIGNAL(beforeRendering()));
        QTest::newRow("blocked, afterRender") << true << QByteArray(SIGNAL(afterRendering()));
        QTest::newRow("blocked, swapped") << true << QByteArray(SIGNAL(frameSwapped()));
    }
    QTest::newRow("unblocked, beforeRender") << false << QByteArray(SIGNAL(beforeRendering()));
    QTest::newRow("unblocked, afterRender") << false << QByteArray(SIGNAL(afterRendering()));
    QTest::newRow("unblocked, swapped") << false << QByteArray(SIGNAL(frameSwapped()));
}

class FrameCounter : public QObject
{
    Q_OBJECT
public slots:
    void incr() { QMutexLocker locker(&m_mutex); ++m_counter; }
public:
    FrameCounter() : m_counter(0) {}
    int count() { QMutexLocker locker(&m_mutex); int x = m_counter; return x; }
private:
    int m_counter;
    QMutex m_mutex;
};

void tst_qquickwindow::constantUpdatesOnWindow()
{
    QFETCH(bool, blockedGui);
    QFETCH(QByteArray, signal);

    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.setGeometry(100, 100, 300, 200);

    bool ok = connect(&window, signal.constData(), &window, SLOT(update()), Qt::DirectConnection);
    Q_ASSERT(ok);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    FrameCounter counter;
    connect(&window, SIGNAL(frameSwapped()), &counter, SLOT(incr()), Qt::DirectConnection);

    int frameCount = 10;
    QElapsedTimer timer;
    timer.start();
    if (blockedGui) {
        while (counter.count() < frameCount)
            QTest::qSleep(100);
        QVERIFY(counter.count() >= frameCount);
    } else {
        window.update();
        QTRY_VERIFY(counter.count() > frameCount);
    }
    window.hide();
}

void tst_qquickwindow::touchEvent_basic()
{
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->setTitle(QTest::currentTestFunction());

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

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
    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(window, touchDevice, false);

    // press single point
    touchSeq.press(0, topItem->mapToScene(pos).toPoint(),window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(topItem->lastEvent.touchPoints.size(), 1);

    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    // At one point this was failing with kwin (KDE window manager) because window->setPosition(100, 100)
    // would put the decorated window at that position rather than the window itself.
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed, makeTouchPoint(topItem, pos)));
    topItem->reset();
    touchSeq.release(0, topItem->mapToScene(pos).toPoint(), window).commit();

    // press multiple points
    touchSeq.press(0, topItem->mapToScene(pos).toPoint(), window)
            .press(1, bottomItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(topItem->lastEvent.touchPoints.size(), 1);
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.size(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();
    touchSeq.release(0, topItem->mapToScene(pos).toPoint(), window).release(1, bottomItem->mapToScene(pos).toPoint(), window).commit();

    // touch point on top item moves to bottom item, but top item should still receive the event
    touchSeq.press(0, topItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    touchSeq.move(0, bottomItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(topItem->lastEvent.touchPoints.size(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchUpdate, window, QEventPoint::State::Updated,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();
    touchSeq.release(0, bottomItem->mapToScene(pos).toPoint(), window).commit();

    // touch point on bottom item moves to top item, but bottom item should still receive the event
    touchSeq.press(0, bottomItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    touchSeq.move(0, topItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(bottomItem->lastEvent.touchPoints.size(), 1);
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchUpdate, window, QEventPoint::State::Updated,
            makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos), pos)));
    bottomItem->reset();
    touchSeq.release(0, bottomItem->mapToScene(pos).toPoint(), window).commit();

    // a single stationary press on an item shouldn't cause an event
    touchSeq.press(0, topItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    touchSeq.stationary(0)
            .press(1, bottomItem->mapToScene(pos).toPoint(), window).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(topItem->lastEvent.touchPoints.size(), 1);    // received press and then stationary
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.size(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchUpdate, window, QEventPoint::State::Stationary, makeTouchPoint(topItem, pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed, makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();
    // cleanup: what is pressed must be released
    // Otherwise you will get an assertion failure:
    // ASSERT: "itemForTouchPointId.isEmpty()" in file items/qquickwindow.cpp
    touchSeq.release(0, pos.toPoint(), window).release(1, pos.toPoint(), window).commit();
    QQuickTouchUtils::flush(window);

    // move touch point from top item to bottom, and release
    touchSeq.press(0, topItem->mapToScene(pos).toPoint(),window).commit();
    QQuickTouchUtils::flush(window);
    touchSeq.release(0, bottomItem->mapToScene(pos).toPoint(),window).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(topItem->lastEvent.touchPoints.size(), 1);
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, window, QEventPoint::State::Released,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    topItem->reset();

    // release while another point is pressed
    touchSeq.press(0, topItem->mapToScene(pos).toPoint(),window)                // seen and grabbed by topItem
            .press(1, bottomItem->mapToScene(pos).toPoint(), window).commit();  // seen and grabbed by bottomItem
    QQuickTouchUtils::flush(window);
    touchSeq.move(0, bottomItem->mapToScene(pos).toPoint(), window).stationary(1).commit();
    QQuickTouchUtils::flush(window);
    touchSeq.release(0, bottomItem->mapToScene(pos).toPoint(), window)
                             .stationary(1).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(topItem->lastEvent.touchPoints.size(), 1);
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QCOMPARE(bottomItem->lastEvent.touchPoints.size(), 1);
    // Since qtbase 2692237bb1b0c0f50b7cc5d920eb8ab065063d47, if the point didn't have a different position on release,
    // then lastPosition is not changed.  So in this case, it still holds the press position.  I.e. on release,
    // it's the last position that was actually different.
    COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchEnd, window, QEventPoint::State::Released,
            makeTouchPoint(topItem, topItem->mapFromItem(bottomItem, pos), pos)));
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchUpdate, window, QEventPoint::State::Stationary,
            makeTouchPoint(bottomItem, pos)));
    topItem->reset();
    bottomItem->reset();

    delete topItem;
    delete middleItem;
    delete bottomItem;
}

void tst_qquickwindow::touchEvent_propagation()
{
    TestTouchItem::clearMouseEventCounters();

    QFETCH(bool, acceptTouchEvents);
    QFETCH(bool, acceptMouseEvents);
    QFETCH(bool, enableItem);
    QFETCH(bool, showItem);

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

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
    QTRY_COMPARE(middleItem->lastEvent.touchPoints.size(), 1);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed,
            makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))));
    QTest::touchEvent(window, touchDevice).release(0, pointInTopItem, window);

    // touch top and middle items, middle item should get both events
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window)
            .press(1, pointInMiddleItem, window);
    QTRY_COMPARE(middleItem->lastEvent.touchPoints.size(), 2);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(middleItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed,
           (QList<QEventPoint>() << makeTouchPoint(middleItem, middleItem->mapFromItem(topItem, pos))
                                              << makeTouchPoint(middleItem, pos) )));
    QTest::touchEvent(window, touchDevice).release(0, pointInTopItem, window)
            .release(1, pointInMiddleItem, window);
    middleItem->reset();

    // disable middleItem as well
    middleItem->acceptTouchEvents = acceptTouchEvents;
    middleItem->acceptMouseEvents = acceptMouseEvents;
    middleItem->setEnabled(enableItem);
    middleItem->setVisible(showItem);

    // touch top and middle items, bottom item should get all events
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window)
            .press(1, pointInMiddleItem, window);
    QTRY_COMPARE(bottomItem->lastEvent.touchPoints.size(), 2);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed,
            (QList<QEventPoint>() << makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos))
                                              << makeTouchPoint(bottomItem, bottomItem->mapFromItem(middleItem, pos)) )));
    bottomItem->reset();

    // disable bottom item as well
    bottomItem->acceptTouchEvents = acceptTouchEvents;
    bottomItem->setEnabled(enableItem);
    bottomItem->setVisible(showItem);
    QTest::touchEvent(window, touchDevice).release(0, pointInTopItem, window)
            .release(1, pointInMiddleItem, window);

    // no events should be received
    QTest::touchEvent(window, touchDevice).press(0, pointInTopItem, window)
            .press(1, pointInMiddleItem, window)
            .press(2, pointInBottomItem, window);
    QTest::qWait(50);
    QVERIFY(topItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
    QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
    QTest::touchEvent(window, touchDevice).release(0, pointInTopItem, window)
            .release(1, pointInMiddleItem, window)
            .release(2, pointInBottomItem, window);
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
        QCOMPARE(bottomItem->lastEvent.touchPoints.size(), 1);
        COMPARE_TOUCH_DATA(bottomItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed,
                makeTouchPoint(bottomItem, bottomItem->mapFromItem(topItem, pos))));
    } else {
        // middle item ignores event, sends it to the top item (top-most child)
        QCOMPARE(topItem->lastEvent.touchPoints.size(), 1);
        QVERIFY(middleItem->lastEvent.touchPoints.isEmpty());
        QVERIFY(bottomItem->lastEvent.touchPoints.isEmpty());
        COMPARE_TOUCH_DATA(topItem->lastEvent, makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed,
                makeTouchPoint(topItem, pos)));
    }
    QTest::touchEvent(window, touchDevice).release(0, pointInTopItem, window);

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
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));

    QPointF pos(50, 50);
    QTest::touchEvent(window, touchDevice).press(0, item->mapToScene(pos).toPoint(), window);
    QCoreApplication::processEvents();

    QTRY_COMPARE(item->lastEvent.touchPoints.size(), 1);
    TouchEventData d = makeTouchData(QEvent::TouchBegin, window, QEventPoint::State::Pressed, makeTouchPoint(item, pos));
    COMPARE_TOUCH_DATA(item->lastEvent, d);
    item->reset();

    QWindowSystemInterface::handleTouchCancelEvent(nullptr, touchDevice);
    QCoreApplication::processEvents();
    d = makeTouchData(QEvent::TouchCancel, window, QEventPoint::State::Pressed, makeTouchPoint(item, pos));
    COMPARE_TOUCH_DATA(item->lastEvent, d);

    delete item;
}

void tst_qquickwindow::touchEvent_cancelClearsMouseGrab()
{
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));
    item->acceptMouseEvents = true;
    item->setAcceptTouchEvents(false);

    QPointF pos(50, 50);
    QTest::touchEvent(window, touchDevice).press(0, item->mapToScene(pos).toPoint(), window);
    QCoreApplication::processEvents();

    QTRY_COMPARE(item->mousePressCount, 1);
    QTRY_COMPARE(item->mouseUngrabEventCount, 0);

    QWindowSystemInterface::handleTouchCancelEvent(nullptr, touchDevice);
    QCoreApplication::processEvents();

    QTRY_COMPARE(item->mouseUngrabEventCount, 1);
}

void tst_qquickwindow::touchEvent_reentrant()
{
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);

    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

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
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(10);

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));

    QList<QEventPoint> points;
    QEventPoint tp(1, QEventPoint::State::Pressed, {}, {});
    const QPointF localPos = item->mapToScene(QPointF(10, 10));
    const QPointF screenPos = window->mapToGlobal(localPos.toPoint());
    QMutableEventPoint::setPosition(tp, localPos);
    QMutableEventPoint::setGlobalPosition(tp, screenPos);
    QMutableEventPoint::setEllipseDiameters(tp, QSizeF(4, 4));
    points << tp;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QCOMPARE(item->touchEventCount, 1);

    QMutableEventPoint::setState(points[0], QEventPoint::State::Updated);
    QMutableEventPoint::setPosition(points[0], localPos + QPointF(5, 5));
    QMutableEventPoint::setGlobalPosition(points[0], screenPos + QPointF(5, 5));
    QVector2D velocity(1.5, 2.5);
    QMutableEventPoint::setVelocity(points[0], velocity);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QCOMPARE(item->touchEventCount, 2);
    QCOMPARE(item->lastEvent.touchPoints.size(), 1);
    QCOMPARE(item->lastVelocity, velocity);

    // Now have a transformation on the item and check if position is transformed accordingly.
    // (In Qt 6, transforming the velocity is an exercise left to the user.  This saves work
    // during delivery.  If we want it to be transformed, we should add QEventPoint::sceneVelocity
    // so that we can keep transforming it repeatedly during Item-localization.)
    item->setRotation(90); // clockwise
    QMutableEventPoint::setPosition(points[0], points[0].position() + QPointF(5, 5));
    QMutableEventPoint::setGlobalPosition(points[0], points[0].globalPosition() + QPointF(5, 5));
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QCOMPARE(item->lastVelocity, velocity);
    QPoint itemLocalPos = item->mapFromScene(points[0].position()).toPoint();
    QPoint itemLocalPosFromEvent = item->lastEvent.touchPoints[0].position().toPoint();
    QCOMPARE(itemLocalPos, itemLocalPosFromEvent);

    QMutableEventPoint::setState(points[0], QEventPoint::State::Released);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    delete item;
}

using ItemVector = QVector<std::shared_ptr<QQuickItem>>;
void tst_qquickwindow::mergeTouchPointLists_data()
{
    QTest::addColumn<ItemVector>("list1");
    QTest::addColumn<ItemVector>("list2");
    QTest::addColumn<QVector<QQuickItem *>>("expected");
    QTest::addColumn<bool>("showItem");

    auto item1 = std::make_shared<QQuickItem>();
    auto item2 = std::make_shared<QQuickItem>();
    auto item3 = std::make_shared<QQuickItem>();
    auto item4 = std::make_shared<QQuickItem>();
    auto item5 = std::make_shared<QQuickItem>();

    QTest::newRow("empty") << ItemVector() << ItemVector() << QVector<QQuickItem *>();
    QTest::newRow("single list left")
            << (ItemVector() << item1 << item2 << item3)
            << ItemVector()
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get());
    QTest::newRow("single list right")
            << ItemVector()
            << (ItemVector() << item1 << item2 << item3)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get());
    QTest::newRow("two lists identical")
            << (ItemVector() << item1 << item2 << item3)
            << (ItemVector() << item1 << item2 << item3)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get());
    QTest::newRow("two lists 1")
            << (ItemVector() << item1 << item2 << item5)
            << (ItemVector() << item3 << item4 << item5)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get() << item4.get() << item5.get());
    QTest::newRow("two lists 2")
            << (ItemVector() << item1 << item2 << item5)
            << (ItemVector() << item3 << item4 << item5)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get() << item4.get() << item5.get());
    QTest::newRow("two lists 3")
            << (ItemVector() << item1 << item2 << item3)
            << (ItemVector() << item1 << item4 << item5)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get() << item4.get() << item5.get());
    QTest::newRow("two lists 4")
            << (ItemVector() << item1 << item3 << item4)
            << (ItemVector() << item2 << item3 << item5)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get() << item4.get() << item5.get());
    QTest::newRow("two lists 5")
            << (ItemVector() << item1 << item2 << item4)
            << (ItemVector() << item1 << item3 << item4)
            << (QVector<QQuickItem *>() << item1.get() << item2.get() << item3.get() << item4.get());
}

void tst_qquickwindow::mergeTouchPointLists()
{
    QFETCH(ItemVector, list1);
    QFETCH(ItemVector, list2);
    QFETCH(QVector<QQuickItem *>, expected);

    QQuickWindow win;
    auto windowPrivate = QQuickWindowPrivate::get(&win);

    QVector<QQuickItem *> a;
    for (const auto &item : list1)
        a.append(item.get());

    QVector<QQuickItem *> b;
    for (const auto &item : list2)
        b.append(item.get());

    auto targetList = windowPrivate->deliveryAgentPrivate()->mergePointerTargets(a, b);
    QCOMPARE(targetList, expected);
}

void tst_qquickwindow::mouseFromTouch_basic()
{
    TestTouchItem::clearMouseEventCounters();
    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::qWait(10);

    TestTouchItem *item = new TestTouchItem(window->contentItem());
    item->setPosition(QPointF(50, 50));
    item->setSize(QSizeF(150, 150));
    // If it doesn't accept touch, but does accept LeftButton mouse events, then
    // the first point in each touch event should generate a synth-mouse event.
    item->setAcceptTouchEvents(false);

    QList<QEventPoint> points;
    QEventPoint tp(1, QEventPoint::State::Pressed, {}, {});
    const QPointF localPos = item->mapToScene(QPointF(10, 10));
    const QPointF screenPos = window->mapToGlobal(localPos.toPoint());
    QMutableEventPoint::setPosition(tp, localPos);
    QMutableEventPoint::setGlobalPosition(tp, screenPos);
    QMutableEventPoint::setEllipseDiameters(tp, QSizeF(4, 4));
    points << tp;
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QMutableEventPoint::setState(points[0], QEventPoint::State::Updated);
    QMutableEventPoint::setPosition(points[0], localPos + QPointF(5, 5));
    QMutableEventPoint::setGlobalPosition(points[0], screenPos + QPointF(5, 5));
    QVector2D velocity(1.5, 2.5);
    QMutableEventPoint::setVelocity(points[0], velocity);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QMutableEventPoint::setState(points[0], QEventPoint::State::Released);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);

    // The item should have received a mouse press, move, and release.
    QCOMPARE(item->mousePressNum, 1);
    QCOMPARE(item->mouseMoveNum, 1);
    QCOMPARE(item->mouseReleaseNum, 1);
    QCOMPARE(item->lastMousePos.toPoint(), item->mapFromScene(points[0].position()).toPoint());
    QCOMPARE(item->lastVelocityFromMouseMove, velocity);
//    QVERIFY(item->lastMouseCapabilityFlags.testFlag(QInputDevice::Capability::Velocity)); // TODO

    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10); // avoid generating a double-click
    // Now the same with a transformation.
    item->setRotation(90); // clockwise
    QMutableEventPoint::setState(points[0], QEventPoint::State::Pressed);
    QMutableEventPoint::setVelocity(points[0], velocity);
    QMutableEventPoint::setPosition(tp, localPos);
    QMutableEventPoint::setGlobalPosition(tp, screenPos);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QMutableEventPoint::setState(points[0], QEventPoint::State::Updated);
    QMutableEventPoint::setPosition(points[0], localPos + QPointF(5, 5));
    QMutableEventPoint::setGlobalPosition(points[0], screenPos + QPointF(5, 5));
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QGuiApplication::processEvents();
    QQuickTouchUtils::flush(window);
    QCOMPARE(item->lastMousePos.toPoint(), item->mapFromScene(points[0].position()).toPoint());
    QCOMPARE(item->lastVelocityFromMouseMove, velocity); // Velocity is always in scene coords

    QMutableEventPoint::setState(points[0], QEventPoint::State::Released);
    QWindowSystemInterface::handleTouchEvent(window, touchDeviceWithVelocity,
                                             QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window));
    QCoreApplication::processEvents();
    QQuickTouchUtils::flush(window);
    delete item;
}

void tst_qquickwindow::synthMouseFromTouch_data()
{
    QTest::addColumn<bool>("synthMouse"); // AA_SynthesizeMouseForUnhandledTouchEvents
    QTest::addColumn<bool>("acceptTouch"); // QQuickItem::touchEvent: setAccepted()

    QTest::newRow("no synth, accept") << false << true; // suitable for touch-capable UIs
    QTest::newRow("no synth, don't accept") << false << false;
    QTest::newRow("synth and accept") << true << true;
    QTest::newRow("synth, don't accept") << true << false; // the default
}

void tst_qquickwindow::synthMouseFromTouch()
{
    QFETCH(bool, synthMouse);
    QFETCH(bool, acceptTouch);

    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, synthMouse);
    QScopedPointer<PointerRecordingWindow> window(new PointerRecordingWindow);
    QScopedPointer<MouseRecordingItem> item(new MouseRecordingItem(acceptTouch, nullptr));
    item->setParentItem(window->contentItem());
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    QPoint p1 = QPoint(20, 20);
    QPoint p2 = QPoint(30, 30);
    QTest::touchEvent(window.data(), touchDevice).press(0, p1, window.data());
    QQuickTouchUtils::flush(window.data());
    QTest::touchEvent(window.data(), touchDevice).move(0, p2, window.data());
    QQuickTouchUtils::flush(window.data());
    QTest::touchEvent(window.data(), touchDevice).release(0, p2, window.data());
    QQuickTouchUtils::flush(window.data());

    QCOMPARE(item->m_touchEvents.size(), acceptTouch ? 3 : 0);
    QCOMPARE(item->m_mouseEvents.size(), (acceptTouch || !synthMouse) ? 0 : 3);
    QCOMPARE(window->m_mouseEvents.size(), 0);
    for (const auto &ev : item->m_mouseEvents)
        QCOMPARE(ev.source, Qt::MouseEventSynthesizedByQt);
}

void tst_qquickwindow::synthMouseDoubleClickFromTouch_data()
{
    QTest::addColumn<QPoint>("movement");
    QTest::addColumn<QPoint>("distanceBetweenPresses");
    QTest::addColumn<bool>("expectedSynthesizedDoubleClickEvent");

    QTest::newRow("normal") << QPoint(0, 0) << QPoint(0, 0) << true;
    QTest::newRow("with 1 pixel wiggle") << QPoint(1, 1) << QPoint(1, 1) << true;
    QTest::newRow("too much distance to second tap") << QPoint(0, 0) << QPoint(50, 0) << false;
    QTest::newRow("too much drag") << QPoint(50, 0) << QPoint(0, 0) << false;
    QTest::newRow("too much drag and too much distance to second tap") << QPoint(50, 0) << QPoint(50, 0) << false;
}

void tst_qquickwindow::synthMouseDoubleClickFromTouch()
{
    QFETCH(QPoint, movement);
    QFETCH(QPoint, distanceBetweenPresses);
    QFETCH(bool, expectedSynthesizedDoubleClickEvent);

    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
    QScopedPointer<PointerRecordingWindow> window(new PointerRecordingWindow);
    QScopedPointer<MouseRecordingItem> item(new MouseRecordingItem(false, nullptr));
    item->setParentItem(window->contentItem());
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QTest::qWait(100);

    QPoint p1 = item->mapToScene(item->clipRect().center()).toPoint();
    QTest::touchEvent(window.data(), touchDevice).press(0, p1, window.data());
    QTest::touchEvent(window.data(), touchDevice).move(0, p1 + movement, window.data());
    QTest::touchEvent(window.data(), touchDevice).release(0, p1 + movement, window.data());

    QPoint p2 = p1 + distanceBetweenPresses;
    QTest::touchEvent(window.data(), touchDevice).press(1, p2, window.data());
    QTest::touchEvent(window.data(), touchDevice).move(1, p2 + movement, window.data());
    QTest::touchEvent(window.data(), touchDevice).release(1, p2 + movement, window.data());

    const int eventCount = item->m_mouseEvents.size();
    QVERIFY(eventCount >= 2);

    const int nDoubleClicks = std::count_if(item->m_mouseEvents.constBegin(), item->m_mouseEvents.constEnd(),
                                            [](const MouseRecordingItem::MouseEvent &ev) { return (ev.type == QEvent::MouseButtonDblClick); } );
    const bool foundDoubleClick = (nDoubleClicks == 1);
    QCOMPARE(foundDoubleClick, expectedSynthesizedDoubleClickEvent);

}

void tst_qquickwindow::clearWindow()
{
    QQuickWindow *window = new QQuickWindow;
    window->setTitle(QTest::currentTestFunction());
    QQuickItem *item = new QQuickItem;
    item->setParentItem(window->contentItem());

    QCOMPARE(item->window(), window);

    delete window;

    QVERIFY(!item->window());

    delete item;
}

void tst_qquickwindow::mouseFiltering()
{
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    TestTouchItem *bottomItem = new TestTouchItem(window->contentItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));

    TestTouchItem *siblingItem = new TestTouchItem(bottomItem);
    siblingItem->setObjectName("Sibling of Middle Item");
    siblingItem->setPosition(QPointF(90, 25));
    siblingItem->setSize(QSizeF(150, 150));

    TestTouchItem *middleItem = new TestTouchItem(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPosition(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));

    TestTouchItem *topItem = new TestTouchItem(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPosition(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));

    QPoint pos(100, 100);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, pos);

    // Mouse filtering propagates down the stack, so the
    // correct order is
    // 1. middleItem filters event
    // 2. bottomItem filters event
    // 3. topItem receives event
    QTRY_COMPARE(middleItem->mousePressCount, 1);
    QTRY_COMPARE(bottomItem->mousePressCount, 2);
    QTRY_COMPARE(topItem->mousePressCount, 3);
    QCOMPARE(siblingItem->mousePressCount, 0);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, pos);
    topItem->clearMouseEventCounters();
    middleItem->clearMouseEventCounters();
    bottomItem->clearMouseEventCounters();
    siblingItem->clearMouseEventCounters();

    // Repeat, but this time have the top item accept the press
    topItem->acceptMouseEvents = true;

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, pos);

    // Mouse filtering propagates down the stack, so the
    // correct order is
    // 1. middleItem filters event
    // 2. bottomItem filters event
    // 3. topItem receives event
    QTRY_COMPARE(middleItem->mousePressCount, 1);
    QTRY_COMPARE(bottomItem->mousePressCount, 2);
    QTRY_COMPARE(topItem->mousePressCount, 3);
    QCOMPARE(siblingItem->mousePressCount, 0);

    pos += QPoint(50, 50);
    QTest::mouseMove(window, pos);

    // The top item has grabbed, so the move goes there, but again
    // all the ancestors can filter, even when the mouse is outside their bounds
    QTRY_COMPARE(middleItem->mouseMoveCount, 1);
    QTRY_COMPARE(bottomItem->mouseMoveCount, 2);
    QTRY_COMPARE(topItem->mouseMoveCount, 3);
    QCOMPARE(siblingItem->mouseMoveCount, 0);

    // clean up mouse press state for the next tests
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, pos);
}

void tst_qquickwindow::qmlCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("window.qml"));
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QCOMPARE(window->color(), QColor(Qt::green));

    QQuickItem *item = window->findChild<QQuickItem*>("item");
    QVERIFY(item);
    QCOMPARE(item->window(), window);
}

void tst_qquickwindow::qmlCreationWithScreen()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("windowWithScreen.qml"));
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QCOMPARE(window->color(), QColor(Qt::green));

    QQuickItem *item = window->findChild<QQuickItem*>("item");
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
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(window->color(), QColor(Qt::blue));
}

void tst_qquickwindow::defaultState()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; import QtQuick.Window 2.1; Window { }", QUrl());
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow *qmlWindow = qobject_cast<QQuickWindow*>(created);
    QVERIFY(qmlWindow);
    qmlWindow->setTitle(QTest::currentTestFunction());

    QQuickWindow cppWindow;
    cppWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&cppWindow));

    QCOMPARE(qmlWindow->windowState(), cppWindow.windowState());
}

void tst_qquickwindow::grab_data()
{
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("alpha");
    QTest::newRow("visible,opaque") << true << false;
    QTest::newRow("invisible,opaque") << false << false;
    QTest::newRow("visible,transparent") << true << true;
    QTest::newRow("invisible,transparent") << false << true;
}

void tst_qquickwindow::grab()
{
    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QFETCH(bool, visible);
    QFETCH(bool, alpha);

    QQuickWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char(' ') + QLatin1String(QTest::currentDataTag()));
    if (alpha) {
        window.setColor(QColor(0, 0, 0, 0));
    } else {
        window.setColor(Qt::red);
    }

    window.resize(250, 250);

    if (visible) {
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    } else {
        window.create();
    }

    QImage content = window.grabWindow();
    QCOMPARE(content.size(), window.size() * window.devicePixelRatio());

    if (alpha) {
        QCOMPARE((uint) content.convertToFormat(QImage::Format_ARGB32_Premultiplied).pixel(0, 0), (uint) 0x00000000);
    } else {
        QCOMPARE((uint) content.convertToFormat(QImage::Format_RGB32).pixel(0, 0), (uint) 0xffff0000);
    }

    if (visible) {
        // Now hide() and regrab to exercise the case of a window that is
        // renderable and then becomes non-exposed (non-renderable). This is not
        // the same as the visible==false case which starts with a window that
        // never was renderable before grabbing.
        window.hide();
        QImage content = window.grabWindow();
        QCOMPARE(content.size(), window.size() * window.devicePixelRatio());
        if (alpha) {
            QCOMPARE((uint) content.convertToFormat(QImage::Format_ARGB32_Premultiplied).pixel(0, 0), (uint) 0x00000000);
        } else {
            QCOMPARE((uint) content.convertToFormat(QImage::Format_RGB32).pixel(0, 0), (uint) 0xffff0000);
        }

        // now make it visible and exercise the main grab path again to see if it still works
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        content = window.grabWindow();
        QCOMPARE(content.size(), window.size() * window.devicePixelRatio());
        if (alpha) {
            QCOMPARE((uint) content.convertToFormat(QImage::Format_ARGB32_Premultiplied).pixel(0, 0), (uint) 0x00000000);
        } else {
            QCOMPARE((uint) content.convertToFormat(QImage::Format_RGB32).pixel(0, 0), (uint) 0xffff0000);
        }
    }
}

class Grabber : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void grab(QObject *obj) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(obj);
        images.append(window->grabWindow());
    }
    QVector<QImage> images;
};

void tst_qquickwindow::earlyGrab()
{
    if (QGuiApplication::platformName() == QLatin1String("minimal"))
        QSKIP("Skipping due to grabWindow not functional on minimal platforms");

    qmlRegisterType<Grabber>("Test", 1, 0, "Grabber");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("earlyGrab.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setTitle(QTest::currentTestFunction());

    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    Grabber *grabber = qobject_cast<Grabber *>(window->findChild<QObject *>("grabber"));
    QVERIFY(grabber);
    QCOMPARE(grabber->images.size(), 1);
    QVERIFY(!grabber->images[0].isNull());
    QCOMPARE(grabber->images[0].convertToFormat(QImage::Format_RGBX8888).pixel(10, 20), QColor(Qt::red).rgb());
}

void tst_qquickwindow::multipleWindows()
{
    QList<QQuickWindow *> windows;
    QScopedPointer<QQuickWindow> cleanup[6];

    for (int i=0; i<6; ++i) {
        QQuickWindow *c = new QQuickWindow();
        c->setTitle(QLatin1String(QTest::currentTestFunction()) + QString::number(i));
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
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->setTitle(QTest::currentTestFunction());
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
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    window->setPersistentGraphics(false);
    window->setPersistentSceneGraph(false);
    QVERIFY(window);
    window->setTitle(QTest::currentTestFunction());
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->isVisible());
    const bool threaded = QQuickWindowPrivate::get(window)->context->thread() != QThread::currentThread();
    QSignalSpy initialized(window, SIGNAL(sceneGraphInitialized()));
    QSignalSpy invalidated(window, SIGNAL(sceneGraphInvalidated()));

    // Verify that the window is alive and kicking
    QVERIFY(window->isSceneGraphInitialized());

    // Store the visual result
    QImage originalContent = window->grabWindow();

    // Hide the window and verify signal emittion and GL context deletion
    window->hide();
    window->releaseResources();

    if (threaded) {
        QTRY_VERIFY(invalidated.size() >= 1);
    }
    // Destroy the native windowing system buffers
    window->destroy();
    QVERIFY(!window->handle());

    // Show and verify that we are back and running
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    if (threaded)
        QTRY_COMPARE(initialized.size(), 1);

    QVERIFY(window->isSceneGraphInitialized());

    // Verify that the visual output is the same
    QImage newContent = window->grabWindow();
    QString errorMessage;
    QVERIFY2(QQuickVisualTestUtils::compareImages(newContent, originalContent, &errorMessage),
             qPrintable(errorMessage));
}

void tst_qquickwindow::destroyShowWithoutHide()
{
    // this is a variation of the headless case, to test if the more aggressive
    // destroy(); show(); sequence survives.

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("Headless.qml"));
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->setTitle(QTest::currentTestFunction());
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->isSceneGraphInitialized());

    QImage originalContent = window->grabWindow();

    window->destroy();
    QVERIFY(!window->handle());

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QVERIFY(window->isSceneGraphInitialized());

    QImage newContent = window->grabWindow();
    QString errorMessage;
    QVERIFY2(QQuickVisualTestUtils::compareImages(newContent, originalContent, &errorMessage),
             qPrintable(errorMessage));
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
    window->setTitle(QTest::currentTestFunction());

    QSignalSpy focusObjectSpy(window, SIGNAL(focusObjectChanged(QObject*)));

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QCOMPARE(window->contentItem(), window->focusObject());
    QCOMPARE(focusObjectSpy.size(), 1);

    QQuickItem *item1 = window->findChild<QQuickItem*>("item1");
    QVERIFY(item1);
    item1->setFocus(true);
    QCOMPARE(item1, window->focusObject());
    QCOMPARE(focusObjectSpy.size(), 2);

    QQuickItem *item2 = window->findChild<QQuickItem*>("item2");
    QVERIFY(item2);
    item2->setFocus(true);
    QCOMPARE(item2, window->focusObject());
    QCOMPARE(focusObjectSpy.size(), 3);

    // set focus for item in non-focused focus scope and
    // ensure focusObject does not change and signal is not emitted
    QQuickItem *item3 = window->findChild<QQuickItem*>("item3");
    QVERIFY(item3);
    item3->setFocus(true);
    QCOMPARE(item2, window->focusObject());
    QCOMPARE(focusObjectSpy.size(), 3);
}

void tst_qquickwindow::focusReason()
{
    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(200, 200);
    window->show();
    window->setTitle(QTest::currentTestFunction());
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QScopedPointer<QQuickItem> firstItem(new QQuickItem);
    firstItem->setSize(QSizeF(100, 100));
    firstItem->setParentItem(window->contentItem());

    QScopedPointer<QQuickItem> secondItem(new QQuickItem);
    secondItem->setSize(QSizeF(100, 100));
    secondItem->setParentItem(window->contentItem());

    firstItem->forceActiveFocus(Qt::OtherFocusReason);
    QCOMPARE(QQuickWindowPrivate::get(window)->deliveryAgentPrivate()->lastFocusReason, Qt::OtherFocusReason);

    secondItem->forceActiveFocus(Qt::TabFocusReason);
    QCOMPARE(QQuickWindowPrivate::get(window)->deliveryAgentPrivate()->lastFocusReason, Qt::TabFocusReason);

    firstItem->forceActiveFocus(Qt::BacktabFocusReason);
    QCOMPARE(QQuickWindowPrivate::get(window)->deliveryAgentPrivate()->lastFocusReason, Qt::BacktabFocusReason);

}

void tst_qquickwindow::ignoreUnhandledMouseEvents()
{
    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->setTitle(QTest::currentTestFunction());
    window->resize(100, 100);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QScopedPointer<QQuickItem> item(new QQuickItem);
    item->setSize(QSizeF(100, 100));
    item->setParentItem(window->contentItem());

    {
        QMouseEvent me(QEvent::MouseButtonPress, QPointF(50, 50), window->mapToGlobal(QPointF(50, 50)),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(window, &me));
        QVERIFY(!me.isAccepted());
    }

    {
        QMouseEvent me(QEvent::MouseMove, QPointF(51, 51), window->mapToGlobal(QPointF(51, 51)),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        me.setAccepted(true);
        QVERIFY(QCoreApplication::sendEvent(window, &me));
        QVERIFY(!me.isAccepted());
    }

    {
        QMouseEvent me(QEvent::MouseButtonRelease, QPointF(51, 51), window->mapToGlobal(QPointF(51, 51)),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
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
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    RootItemAccessor* accessor = window->findChild<RootItemAccessor*>("accessor");
    QVERIFY(accessor);
    engine.collectGarbage();

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(!accessor->isRootItemDestroyed());
}

#if QT_CONFIG(cursor)
void tst_qquickwindow::cursor()
{
    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.setFramePosition(QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(50, 50));
    window.resize(320, 290);

    QQuickItem parentItem;
    parentItem.setObjectName("parentItem");
    parentItem.setPosition(QPointF(0, 0));
    parentItem.setSize(QSizeF(180, 180));
    parentItem.setParentItem(window.contentItem());

    QQuickItem childItem;
    childItem.setObjectName("childItem");
    childItem.setPosition(QPointF(60, 90));
    childItem.setSize(QSizeF(120, 120));
    childItem.setParentItem(&parentItem);

    QQuickItem clippingItem;
    clippingItem.setObjectName("clippingItem");
    clippingItem.setPosition(QPointF(120, 120));
    clippingItem.setSize(QSizeF(180, 180));
    clippingItem.setClip(true);
    clippingItem.setParentItem(window.contentItem());

    QQuickItem clippedItem;
    clippedItem.setObjectName("clippedItem");
    clippedItem.setPosition(QPointF(-30, -30));
    clippedItem.setSize(QSizeF(120, 120));
    clippedItem.setParentItem(&clippingItem);

    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

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
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
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
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));

    // Remove the cursor item from the scene. Theoretically this should make parentItem the
    // cursorItem, but given the situation will correct itself after the next mouse move it
    // simply unsets the window cursor for now.
    childItem.setParentItem(nullptr);
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    parentItem.setCursor(Qt::SizeAllCursor);
    QCOMPARE(parentItem.cursor().shape(), Qt::SizeAllCursor);
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    // Changing the cursor of an un-parented item doesn't affect the window's cursor.
    childItem.setCursor(Qt::ClosedHandCursor);
    QCOMPARE(childItem.cursor().shape(), Qt::ClosedHandCursor);
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    childItem.unsetCursor();
    QCOMPARE(childItem.cursor().shape(), Qt::ArrowCursor);
    QCOMPARE(window.cursor().shape(), Qt::ArrowCursor);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 101));
    QCOMPARE(window.cursor().shape(), Qt::SizeAllCursor);
}
#endif

void tst_qquickwindow::hideThenDelete_data()
{
    QTest::addColumn<bool>("persistentSG");
    QTest::addColumn<bool>("persistentGraphics");

    QTest::newRow("persistent:SG=false,Graphics=false") << false << false;
    QTest::newRow("persistent:SG=true,Graphics=false") << true << false;
    QTest::newRow("persistent:SG=false,Graphics=true") << false << true;
    QTest::newRow("persistent:SG=true,Graphics=true") << true << true;
}

void tst_qquickwindow::hideThenDelete()
{
    QFETCH(bool, persistentSG);
    QFETCH(bool, persistentGraphics);

    QScopedPointer<QSignalSpy> sgInvalidated;

    {
        QQuickWindow window;
        window.setTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char(' ')
                        + QLatin1String(QTest::currentDataTag()));
        window.setColor(Qt::red);

        window.setPersistentSceneGraph(persistentSG);
        window.setPersistentGraphics(persistentGraphics);

        window.resize(400, 300);
        window.show();

        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const bool threaded = QQuickWindowPrivate::get(&window)->context->thread() != QGuiApplication::instance()->thread();

        sgInvalidated.reset(new QSignalSpy(&window, SIGNAL(sceneGraphInvalidated())));

        window.hide();

        QTRY_VERIFY(!window.isExposed());

        // Only the threaded render loop implements the opt-in full
        // scenegraph and graphics teardown on unexpose.
        if (threaded) {
            if (!QSGRendererInterface::isApiRhiBased(window.rendererInterface()->graphicsApi()))
                QSKIP("Skipping persistency verification due to not running with RHI");

            if (!persistentSG)
                QVERIFY(sgInvalidated->size() > 0);
            else
                QCOMPARE(sgInvalidated->size(), 0);
        }
    }

    QVERIFY(sgInvalidated->size() > 0);
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
    QScopedPointer<QQuickItem> created(qobject_cast<QQuickItem *>(component.create()));

    QVERIFY(created);

    QTRY_VERIFY(created->opacity() > 0.5);
    QTRY_VERIFY(created->opacity() < 0.5);
}

void tst_qquickwindow::testExpose()
{
    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
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
#ifdef Q_OS_ANDROID
    QSKIP("tst_qquickwindow::requestActivate crashes on Android, see QTBUG-103078.");
#endif

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("active.qml"));
    QScopedPointer<QQuickWindow> window1(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window1.isNull());
    window1->setTitle(QTest::currentTestFunction());

    QWindowList windows = QGuiApplication::topLevelWindows();
    QCOMPARE(windows.size(), 2);

    for (int i = 0; i < windows.size(); ++i) {
        if (windows.at(i)->objectName() == window1->objectName()) {
            windows.removeAt(i);
            break;
        }
    }
    QCOMPARE(windows.size(), 1);
    QCOMPARE(windows.at(0)->objectName(), QLatin1String("window2"));

    window1->show();
    QVERIFY(QTest::qWaitForWindowExposed(windows.at(0))); //We wait till window 2 comes up
    window1->requestActivate();                 // and then transfer the focus to window1

    QTRY_COMPARE(QGuiApplication::focusWindow(), window1.data());
    QVERIFY(window1->isActive());

    QQuickItem *item = QQuickVisualTestUtils::findItem<QQuickItem>(window1->contentItem(), "item1");
    QVERIFY(item);

    //copied from src/qmltest/quicktestevent.cpp
    QPoint pos = item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint();

    {
        QMouseEvent me(QEvent::MouseButtonPress, pos, window1->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QSpontaneKeyEvent::setSpontaneous(&me);
        if (!qApp->notify(window1.data(), &me))
            qWarning("Mouse event MousePress not accepted by receiving window");
    }
    {
        QMouseEvent me = QMouseEvent(QEvent::MouseButtonPress, pos, window1->mapToGlobal(pos), Qt::LeftButton, {}, Qt::NoModifier);
        QSpontaneKeyEvent::setSpontaneous(&me);
        if (!qApp->notify(window1.data(), &me))
            qWarning("Mouse event MouseRelease not accepted by receiving window");
    }

    QTRY_COMPARE(QGuiApplication::focusWindow(), windows.at(0));
    QVERIFY(windows.at(0)->isActive());
}

void tst_qquickwindow::testWindowVisibilityOrder()
{
#ifdef Q_OS_ANDROID
    QSKIP("tst_qquickwindow::testWindowVisibilityOrder crashes on Android, see QTBUG-103078.");
#endif

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("windoworder.qml"));
    QScopedPointer<QQuickWindow> window1(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window1.isNull());
    window1->setTitle(QTest::currentTestFunction());
    QQuickWindow *window2 = window1->property("win2").value<QQuickWindow*>();
    QQuickWindow *window3 = window1->property("win3").value<QQuickWindow*>();
    QQuickWindow *window4 = window1->property("win4").value<QQuickWindow*>();
    QQuickWindow *window5 = window1->property("win5").value<QQuickWindow*>();
    QVERIFY(window2);
    QVERIFY(window3);

    QVERIFY(QTest::qWaitForWindowExposed(window3));

    QWindowList windows = QGuiApplication::topLevelWindows();
    QTRY_COMPARE(windows.size(), 5);

    QVERIFY(window1->isActive());
    QVERIFY(window2->isActive());
    QVERIFY(window3->isActive());

    //Test if window4 is shown 2 seconds after the application startup
    //with window4 visible window5 (transient child) should also become visible
    QVERIFY(!window4->isVisible());
    QVERIFY(!window5->isVisible());

    window4->setVisible(true);

    QVERIFY(QTest::qWaitForWindowExposed(window5));
    QVERIFY(window4->isVisible());
    QVERIFY(window5->isVisible());
}

void tst_qquickwindow::blockClosing()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("ucantclosethis.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QVERIFY(window->isVisible());
    QWindowSystemInterface::handleCloseEvent(window.data());
    QVERIFY(window->isVisible());
    QWindowSystemInterface::handleCloseEvent(window.data());
    QVERIFY(window->isVisible());
    window->setProperty("canCloseThis", true);
    QWindowSystemInterface::handleCloseEvent(window.data());
    QTRY_VERIFY(!window->isVisible());
}

void tst_qquickwindow::blockCloseMethod()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("ucantclosethis.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QVERIFY(window->isVisible());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "close", Qt::DirectConnection));
    QVERIFY(window->isVisible());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "close", Qt::DirectConnection));
    QVERIFY(window->isVisible());
    window->setProperty("canCloseThis", true);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "close", Qt::DirectConnection));
    QTRY_VERIFY(!window->isVisible());
}

void tst_qquickwindow::crashWhenHoverItemDeleted()
{
    // QTBUG-32771
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("hoverCrash.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    // Simulate a move from the first rectangle to the second. Crash will happen in here
    // Moving instantaneously from (0, 99) to (0, 102) does not cause the crash
    for (int i = 99; i < 102; ++i) {
        QTest::mouseMove(window.data(), QPoint(0, i));
    }
}

// QTBUG-33436
void tst_qquickwindow::unloadSubWindow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("unloadSubWindow.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QPointer<QQuickWindow> transient;
    QTRY_VERIFY(transient = window->property("transientWindow").value<QQuickWindow*>());
    QVERIFY(QTest::qWaitForWindowExposed(transient));

    // Unload the inner window (in nested Loaders) and make sure it doesn't crash
    QQuickLoader *loader = window->property("loader1").value<QQuickLoader*>();
    loader->setActive(false);
    QTRY_VERIFY(transient.isNull() || !transient->isVisible());
}

// QTBUG-52573
void tst_qquickwindow::changeVisibilityInCompleted()
{
#ifdef Q_OS_ANDROID
    QSKIP("tst_qquickwindow::changeVisibilityInCompleted crashes on Android, see QTBUG-103078.");
#endif

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("changeVisibilityInCompleted.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QPointer<QQuickWindow> winVisible;
    QTRY_VERIFY(winVisible = window->property("winVisible").value<QQuickWindow*>());
    QPointer<QQuickWindow> winVisibility;
    QTRY_VERIFY(winVisibility = window->property("winVisibility").value<QQuickWindow*>());
    QVERIFY(QTest::qWaitForWindowExposed(winVisible));
    QVERIFY(QTest::qWaitForWindowExposed(winVisibility));

    QVERIFY(winVisible->isVisible());
    QCOMPARE(winVisibility->visibility(), QWindow::Windowed);
}

// QTBUG-32004
void tst_qquickwindow::qobjectEventFilter_touch()
{
    QQuickWindow window;

    window.resize(250, 250);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    TestTouchItem *item = new TestTouchItem(window.contentItem());
    item->setSize(QSizeF(150, 150));

    EventFilter eventFilter;
    item->installEventFilter(&eventFilter);

    QPointF pos(10, 10);

    // press single point
    QTest::touchEvent(&window, touchDevice).press(0, item->mapToScene(pos).toPoint(), &window);

    QCOMPARE(eventFilter.events.size(), 1);
    QCOMPARE(eventFilter.events.first(), (int)QEvent::TouchBegin);
}

// QTBUG-32004
void tst_qquickwindow::qobjectEventFilter_key()
{
    QQuickWindow window;

    window.resize(250, 250);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    TestTouchItem *item = new TestTouchItem(window.contentItem());
    item->setSize(QSizeF(150, 150));
    item->setFocus(true);

    EventFilter eventFilter;
    item->installEventFilter(&eventFilter);

    QTest::keyPress(&window, Qt::Key_A);

    // NB: It may also receive some QKeyEvent(ShortcutOverride) which we're not interested in
    QVERIFY(eventFilter.events.contains((int)QEvent::KeyPress));
    eventFilter.events.clear();

    QTest::keyRelease(&window, Qt::Key_A);

    QVERIFY(eventFilter.events.contains((int)QEvent::KeyRelease));
}

// QTBUG-32004
void tst_qquickwindow::qobjectEventFilter_mouse()
{
    QQuickWindow window;

    window.resize(250, 250);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    TestTouchItem *item = new TestTouchItem(window.contentItem());
    item->setSize(QSizeF(150, 150));

    EventFilter eventFilter;
    item->installEventFilter(&eventFilter);

    QPoint point = item->mapToScene(QPointF(10, 10)).toPoint();
    QTest::mouseMove(&window, point);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);

    QVERIFY(eventFilter.events.contains((int)QEvent::MouseButtonPress));

    // clean up mouse press state for the next tests
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
}

void tst_qquickwindow::animatingSignal()
{
    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.setGeometry(100, 100, 300, 200);

    QSignalSpy spy(&window, SIGNAL(afterAnimating()));

    window.show();
    QTRY_VERIFY(window.isExposed());

    QTRY_VERIFY(spy.size() > 1);
}

void tst_qquickwindow::frameSignals()
{
    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.setGeometry(100, 100, 300, 200);

    QSignalSpy beforeSpy(&window, SIGNAL(beforeFrameBegin()));
    QSignalSpy afterSpy(&window, SIGNAL(afterFrameEnd()));

    window.show();
    QTRY_VERIFY(window.isExposed());
    QSGRendererInterface *rif = window.rendererInterface();
    QVERIFY(rif);

    QTRY_VERIFY(beforeSpy.size() > 1);
    QTRY_VERIFY(afterSpy.size() > 1);
    QTRY_COMPARE(beforeSpy.size(), afterSpy.size());
}

// QTBUG-36938
void tst_qquickwindow::contentItemSize()
{
    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    QQuickItem *contentItem = window.contentItem();
    QVERIFY(contentItem);
    QCOMPARE(QSize(contentItem->width(), contentItem->height()), window.size());

    QSizeF size(300, 200);
    window.resize(size.toSize());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QCOMPARE(window.size(), size.toSize());
    QCOMPARE(QSizeF(contentItem->width(), contentItem->height()), size);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQuick 2.1\n Rectangle { anchors.fill: parent }"), QUrl());
    QScopedPointer<QQuickItem> rect(qobject_cast<QQuickItem *>(component.create()));
    QVERIFY(rect);
    rect->setParentItem(window.contentItem());
    QCOMPARE(QSizeF(rect->width(), rect->height()), size);

    size.transpose();
    window.resize(size.toSize());
    QCOMPARE(window.size(), size.toSize());
    // wait for resize event
    QTRY_COMPARE(QSizeF(contentItem->width(), contentItem->height()), size);
    QCOMPARE(QSizeF(rect->width(), rect->height()), size);
}

void tst_qquickwindow::defaultSurfaceFormat()
{
    // It is quite difficult to verify anything for real since the resulting format after
    // surface/context creation can be anything, depending on the platform and drivers,
    // and many options and settings may fail in various configurations, but test at
    // least using some harmless settings to check that the global, static format is
    // taken into account in the requested format.

    QSurfaceFormat savedDefaultFormat = QSurfaceFormat::defaultFormat();

    // Verify that depth and stencil are set, as they should be, unless they are disabled
    // via environment variables.

    QSurfaceFormat format = savedDefaultFormat;
    format.setSwapInterval(0);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    // Will not set depth and stencil. That should be added automatically,
    // unless the are disabled (but they aren't).
    QSurfaceFormat::setDefaultFormat(format);

    QQuickWindow window;
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    if (window.rendererInterface()->graphicsApi() != QSGRendererInterface::OpenGLRhi)
        QSKIP("Skipping OpenGL context test due to not running with OpenGL");

    const QSurfaceFormat reqFmt = window.requestedFormat();
    QCOMPARE(format.swapInterval(), reqFmt.swapInterval());
    QCOMPARE(format.redBufferSize(), reqFmt.redBufferSize());
    QCOMPARE(format.greenBufferSize(), reqFmt.greenBufferSize());
    QCOMPARE(format.blueBufferSize(), reqFmt.blueBufferSize());
    QCOMPARE(format.profile(), reqFmt.profile());
    QCOMPARE(int(format.options()), int(reqFmt.options()));

#if QT_CONFIG(opengl)
    // Depth and stencil should be >= what has been requested. For real. But use
    // the context since the window's surface format is only partially updated
    // on most platforms.
    const QOpenGLContext *ctx = nullptr;
    QTRY_VERIFY((ctx = static_cast<QOpenGLContext *>(window.rendererInterface()->getResource(
                                                         &window, QSGRendererInterface::OpenGLContextResource))) != nullptr);
    QVERIFY(ctx->format().depthBufferSize() >= 16);
    QVERIFY(ctx->format().stencilBufferSize() >= 8);
#endif
    QSurfaceFormat::setDefaultFormat(savedDefaultFormat);
}

void tst_qquickwindow::attachedProperty()
{
    QQuickView view(testFileUrl("windowattached.qml"));
    view.setTitle(QTest::currentTestFunction());
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(view.rootObject()->property("windowActive").toBool());
    QCOMPARE(view.rootObject()->property("contentItem").value<QQuickItem*>(), view.contentItem());
    QCOMPARE(view.rootObject()->property("windowWidth").toInt(), view.width());
    QCOMPARE(view.rootObject()->property("windowHeight").toInt(), view.height());
    QCOMPARE(view.rootObject()->property("window").value<QQuickView*>(), &view);

    QQuickWindow *innerWindow = view.rootObject()->findChild<QQuickWindow*>("extraWindow");
    QVERIFY(innerWindow);
    innerWindow->show();
    innerWindow->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(innerWindow));

    QQuickText *text = view.rootObject()->findChild<QQuickText*>("extraWindowText");
    QVERIFY(text);
    QCOMPARE(text->text(), QLatin1String("active\nvisibility: 2"));
    QCOMPARE(text->property("contentItem").value<QQuickItem*>(), innerWindow->contentItem());
    QCOMPARE(text->property("windowWidth").toInt(), innerWindow->width());
    QCOMPARE(text->property("windowHeight").toInt(), innerWindow->height());
    QCOMPARE(text->property("window").value<QQuickWindow*>(), innerWindow);

    text->setParentItem(nullptr);
    QVERIFY(!text->property("contentItem").value<QQuickItem*>());
    QCOMPARE(text->property("windowWidth").toInt(), 0);
    QCOMPARE(text->property("windowHeight").toInt(), 0);
    QVERIFY(!text->property("window").value<QQuickWindow*>());
}

class RenderJob : public QRunnable
{
public:
    RenderJob(QQuickWindow::RenderStage s, QList<QQuickWindow::RenderStage> *l) : stage(s), list(l) { }
    ~RenderJob() { ++deleted; }
    QQuickWindow::RenderStage stage;
    QList<QQuickWindow::RenderStage> *list;
    void run() override
    {
        list->append(stage);
    }
    static int deleted;
};
int RenderJob::deleted = 0;

void tst_qquickwindow::testRenderJob()
{
    QList<QQuickWindow::RenderStage> completedJobs;

    QQuickWindow::RenderStage stages[] = {
        QQuickWindow::BeforeSynchronizingStage,
        QQuickWindow::AfterSynchronizingStage,
        QQuickWindow::BeforeRenderingStage,
        QQuickWindow::AfterRenderingStage,
        QQuickWindow::AfterSwapStage,
        QQuickWindow::NoStage
    };

    const int numJobs = 6;

    {
        QQuickWindow window;
        window.setTitle(QTest::currentTestFunction());
        RenderJob::deleted = 0;

        // Schedule the jobs
        for (int i = 0; i < numJobs; ++i)
            window.scheduleRenderJob(new RenderJob(stages[i], &completedJobs), stages[i]);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        // All jobs should be deleted
        QTRY_COMPARE(RenderJob::deleted, numJobs);

        // The NoStage job is not completed, if it is issued when there is no context,
        // but the rest will be queued and completed once relevant render stage is hit.
        QCOMPARE(completedJobs.size(), numJobs - 1);

        // Verify jobs were completed in correct order
        for (int i = 0; i < numJobs - 1; ++i)
            QCOMPARE(completedJobs.at(i), stages[i]);


        // Check that NoStage job gets executed if it is scheduled when window is exposed
        completedJobs.clear();
        RenderJob::deleted = 0;
        window.scheduleRenderJob(new RenderJob(QQuickWindow::NoStage, &completedJobs),
                                 QQuickWindow::NoStage);
        QTRY_COMPARE(RenderJob::deleted, 1);
        QCOMPARE(completedJobs.size(), 1);
    }

    // Verify that jobs are deleted when window is not rendered at all
    completedJobs.clear();
    RenderJob::deleted = 0;
    {
        QQuickWindow window2;
        for (int i = 0; i < numJobs; ++i) {
            window2.scheduleRenderJob(new RenderJob(stages[i], &completedJobs), stages[i]);
        }
    }
    QTRY_COMPARE(RenderJob::deleted, numJobs);
    QCOMPARE(completedJobs.size(), 0);
}

class EventCounter : public QQuickRectangle
{
public:
    EventCounter(QQuickItem *parent = nullptr)
        : QQuickRectangle(parent)
    { }

    void addFilterEvent(QEvent::Type type)
    {
        m_returnTrueForType.append(type);
    }

    int childMouseEventFilterEventCount(QEvent::Type type)
    {
        return m_childMouseEventFilterEventCount.value(type, 0);
    }

    int eventCount(QEvent::Type type)
    {
        return m_eventCount.value(type, 0);
    }

    void reset()
    {
        m_eventCount.clear();
        m_childMouseEventFilterEventCount.clear();
    }
protected:
    bool childMouseEventFilter(QQuickItem *, QEvent *event) override
    {
        m_childMouseEventFilterEventCount[event->type()]++;
        return m_returnTrueForType.contains(event->type());
    }

    bool event(QEvent *event) override
    {
        m_eventCount[event->type()]++;
        return QQuickRectangle::event(event);
    }


private:
    QList<QEvent::Type> m_returnTrueForType;
    QMap<QEvent::Type, int> m_childMouseEventFilterEventCount;
    QMap<QEvent::Type, int> m_eventCount;
};

void tst_qquickwindow::testHoverChildMouseEventFilter()
{
    QQuickWindow window;

    window.resize(250, 250);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    EventCounter *bottomItem = new EventCounter(window.contentItem());
    bottomItem->setObjectName("Bottom Item");
    bottomItem->setSize(QSizeF(150, 150));
    bottomItem->setAcceptHoverEvents(true);

    EventCounter *middleItem = new EventCounter(bottomItem);
    middleItem->setObjectName("Middle Item");
    middleItem->setPosition(QPointF(50, 50));
    middleItem->setSize(QSizeF(150, 150));
    middleItem->setAcceptHoverEvents(true);

    EventCounter *topItem = new EventCounter(middleItem);
    topItem->setObjectName("Top Item");
    topItem->setPosition(QPointF(50, 50));
    topItem->setSize(QSizeF(150, 150));
    topItem->setAcceptHoverEvents(true);

    QPoint pos(10, 10);

    QTest::mouseMove(&window, pos);

    QTRY_VERIFY(bottomItem->eventCount(QEvent::HoverEnter) > 0);
    QCOMPARE(bottomItem->childMouseEventFilterEventCount(QEvent::HoverEnter), 0);
    QCOMPARE(middleItem->eventCount(QEvent::HoverEnter), 0);
    QCOMPARE(topItem->eventCount(QEvent::HoverEnter), 0);
    bottomItem->reset();

    pos = QPoint(60, 60);
    QTest::mouseMove(&window, pos);
    QTRY_VERIFY(middleItem->eventCount(QEvent::HoverEnter) > 0);
    QCOMPARE(bottomItem->childMouseEventFilterEventCount(QEvent::HoverEnter), 0);
    middleItem->reset();

    pos = QPoint(70,70);
    bottomItem->setFiltersChildMouseEvents(true);
    QTest::mouseMove(&window, pos);
    QTRY_VERIFY(middleItem->eventCount(QEvent::HoverMove) > 0);
    QVERIFY(bottomItem->childMouseEventFilterEventCount(QEvent::HoverMove) > 0);
    QCOMPARE(topItem->eventCount(QEvent::HoverEnter), 0);
    bottomItem->reset();
    middleItem->reset();

    pos = QPoint(110,110);
    bottomItem->addFilterEvent(QEvent::HoverEnter);
    QTest::mouseMove(&window, pos);
    QTRY_VERIFY(bottomItem->childMouseEventFilterEventCount(QEvent::HoverEnter) > 0);
    QCOMPARE(topItem->eventCount(QEvent::HoverEnter), 0);
    QCOMPARE(middleItem->eventCount(QEvent::HoverEnter), 0);
}

class HoverTimestampConsumer : public QQuickItem
{
    Q_OBJECT
public:
    HoverTimestampConsumer(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
        setAcceptHoverEvents(true);
    }

    void hoverEnterEvent(QHoverEvent *event) override
    { hoverTimestamps << event->timestamp(); }
    void hoverLeaveEvent(QHoverEvent *event) override
    { hoverTimestamps << event->timestamp(); }
    void hoverMoveEvent(QHoverEvent *event) override
    { hoverTimestamps << event->timestamp(); }

    QList<ulong> hoverTimestamps;
};

// Checks that a QHoverEvent carries the timestamp of the QMouseEvent that caused it.
// QTBUG-54600
void tst_qquickwindow::testHoverTimestamp()
{
    QQuickWindow window;

    window.resize(200, 200);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    HoverTimestampConsumer *hoverConsumer = new HoverTimestampConsumer(window.contentItem());
    hoverConsumer->setWidth(100);
    hoverConsumer->setHeight(100);
    hoverConsumer->setX(50);
    hoverConsumer->setY(50);

    // First position, outside
    {
        QMouseEvent mouseEvent(QEvent::MouseMove, QPointF(40, 40), QPointF(40, 40), QPointF(140, 140),
                Qt::NoButton, Qt::NoButton, Qt::NoModifier, Qt::MouseEventNotSynthesized);
        mouseEvent.setTimestamp(10);
        QVERIFY(QCoreApplication::sendEvent(&window, &mouseEvent));
    }

    // Enter
    {
        QMouseEvent mouseEvent(QEvent::MouseMove, QPointF(50, 50), QPointF(50, 50), QPointF(150, 150),
                Qt::NoButton, Qt::NoButton, Qt::NoModifier, Qt::MouseEventNotSynthesized);
        mouseEvent.setTimestamp(20);
        QVERIFY(QCoreApplication::sendEvent(&window, &mouseEvent));
    }
    QCOMPARE(hoverConsumer->hoverTimestamps.size(), 1);
    QCOMPARE(hoverConsumer->hoverTimestamps.last(), 20UL);

    // Move
    {
        QMouseEvent mouseEvent(QEvent::MouseMove, QPointF(60, 60), QPointF(60, 60), QPointF(160, 160),
                Qt::NoButton, Qt::NoButton, Qt::NoModifier, Qt::MouseEventNotSynthesized);
        mouseEvent.setTimestamp(30);
        QVERIFY(QCoreApplication::sendEvent(&window, &mouseEvent));
    }
    QCOMPARE(hoverConsumer->hoverTimestamps.size(), 2);
    QCOMPARE(hoverConsumer->hoverTimestamps.last(), 30UL);

    // Move
    {
        QMouseEvent mouseEvent(QEvent::MouseMove, QPointF(100, 100), QPointF(100, 100), QPointF(200, 200),
                Qt::NoButton, Qt::NoButton, Qt::NoModifier, Qt::MouseEventNotSynthesized);
        mouseEvent.setTimestamp(40);
        QVERIFY(QCoreApplication::sendEvent(&window, &mouseEvent));
    }
    QCOMPARE(hoverConsumer->hoverTimestamps.size(), 3);
    QCOMPARE(hoverConsumer->hoverTimestamps.last(), 40UL);

    // Leave
    {
        QMouseEvent mouseEvent(QEvent::MouseMove, QPointF(160, 160), QPointF(160, 160), QPointF(260, 260),
                Qt::NoButton, Qt::NoButton, Qt::NoModifier, Qt::MouseEventNotSynthesized);
        mouseEvent.setTimestamp(5);
        QVERIFY(QCoreApplication::sendEvent(&window, &mouseEvent));
    }
    QCOMPARE(hoverConsumer->hoverTimestamps.size(), 4);
    QCOMPARE(hoverConsumer->hoverTimestamps.last(), 5UL);
}

class CircleItem : public QQuickRectangle
{
public:
    CircleItem(QQuickItem *parent = nullptr) : QQuickRectangle(parent) { }

    void setRadius(qreal radius) {
        const qreal diameter = radius*2;
        setWidth(diameter);
        setHeight(diameter);
    }

    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override
    {
        Q_UNUSED(item);
        if (event->type() == QEvent::MouseButtonPress && !contains(static_cast<QMouseEvent*>(event)->position().toPoint())) {
            // This is an evil hack: in case of items that are not rectangles, we never accept the event.
            // Instead the events are now delivered to QDeclarativeGeoMapItemBase which doesn't to anything with them.
            // The map below it still works since it filters events and steals the events at some point.
            event->setAccepted(false);
            return true;
        }
        return false;
    }

    bool contains(const QPointF &pos) const override {
        // returns true if the point is inside the the embedded circle inside the (square) rect
        const float radius = (float)width()/2;
        const QVector2D center(radius, radius);
        const QVector2D dx = QVector2D(pos) - center;
        const bool ret = dx.lengthSquared() < radius*radius;
        return ret;
    }
};

void tst_qquickwindow::test_circleMapItem()
{
    QQuickWindow window;

    window.resize(250, 250);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());

    QQuickItem *root = window.contentItem();
    QQuickMouseArea *mab = new QQuickMouseArea(root);
    mab->setObjectName("Bottom MouseArea");
    mab->setSize(QSizeF(100, 100));

    CircleItem *topItem = new CircleItem(root);
    topItem->setFiltersChildMouseEvents(true);
    topItem->setColor(Qt::green);
    topItem->setObjectName("Top Item");
    topItem->setPosition(QPointF(30, 30));
    topItem->setRadius(20);
    QQuickMouseArea *mat = new QQuickMouseArea(topItem);
    mat->setObjectName("Top Item/MouseArea");
    mat->setSize(QSizeF(40, 40));

    QSignalSpy bottomSpy(mab, SIGNAL(clicked(QQuickMouseEvent*)));
    QSignalSpy topSpy(mat, SIGNAL(clicked(QQuickMouseEvent*)));

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTest::qWait(1000);

    QPoint pos(50, 50);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::KeyboardModifiers(), pos);

    QCOMPARE(topSpy.size(), 1);
    QCOMPARE(bottomSpy.size(), 0);

    // Outside the "Circles" "input area", but on top of the bottomItem rectangle
    pos = QPoint(66, 66);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::KeyboardModifiers(), pos);

    QCOMPARE(bottomSpy.size(), 1);
    QCOMPARE(topSpy.size(), 1);
}

void tst_qquickwindow::grabContentItemToImage()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("grabContentItemToImage.qml"));

    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow *window = qobject_cast<QQuickWindow *>(created);
    QVERIFY(QTest::qWaitForWindowActive(window));

    QMetaObject::invokeMethod(window, "grabContentItemToImage");
    QTRY_COMPARE(created->property("success").toInt(), 1);
}

class TestDropTarget : public QQuickItem
{
    Q_OBJECT
public:
    TestDropTarget(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
        , enterDropAction(Qt::CopyAction)
        , moveDropAction(Qt::CopyAction)
        , dropDropAction(Qt::CopyAction)
        , enterAccept(true)
        , moveAccept(true)
        , dropAccept(true)
    {
        setFlags(ItemAcceptsDrops);
    }

    void reset()
    {
        enterDropAction = Qt::CopyAction;
        moveDropAction = Qt::CopyAction;
        dropDropAction = Qt::CopyAction;
        enterAccept = true;
        moveAccept = true;
        dropAccept = true;
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        event->setAccepted(enterAccept);
        event->setDropAction(enterDropAction);
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        event->setAccepted(moveAccept);
        event->setDropAction(moveDropAction);
    }

    void dropEvent(QDropEvent *event) override
    {
        event->setAccepted(dropAccept);
        event->setDropAction(dropDropAction);
    }

    Qt::DropAction enterDropAction;
    Qt::DropAction moveDropAction;
    Qt::DropAction dropDropAction;
    bool enterAccept;
    bool moveAccept;
    bool dropAccept;
};

class DragEventTester {
public:
    DragEventTester()
        : pos(60, 60)
        , actions(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction)
        , buttons(Qt::LeftButton)
        , modifiers(Qt::NoModifier)
    {
    }

    ~DragEventTester() {
        qDeleteAll(events);
        events.clear();
        enterEvent = nullptr;
        moveEvent = nullptr;
        dropEvent = nullptr;
        leaveEvent = nullptr;
    }

    void addEnterEvent()
    {
        enterEvent = new QDragEnterEvent(pos, actions, &data, buttons, modifiers);
        events.append(enterEvent);
    }

    void addMoveEvent()
    {
        moveEvent = new QDragMoveEvent(pos, actions, &data, buttons, modifiers, QEvent::DragMove);
        events.append(moveEvent);
    }

    void addDropEvent()
    {
        dropEvent = new QDropEvent(pos, actions, &data, buttons, modifiers, QEvent::Drop);
        events.append(dropEvent);
    }

    void addLeaveEvent()
    {
        leaveEvent = new QDragLeaveEvent();
        events.append(leaveEvent);
    }

    void sendDragEventSequence(QQuickWindow *window) const {
        for (int i = 0; i < events.size(); ++i) {
            QCoreApplication::sendEvent(window, events[i]);
        }
    }

    // Used for building events.
    QMimeData data;
    QPoint pos;
    Qt::DropActions actions;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;

    // Owns events.
    QList<QEvent *> events;

    // Non-owner pointers for easy acccess.
    QDragEnterEvent *enterEvent;
    QDragMoveEvent *moveEvent;
    QDropEvent *dropEvent;
    QDragLeaveEvent *leaveEvent;
};

void tst_qquickwindow::testDragEventPropertyPropagation()
{
    QQuickWindow window;
    TestDropTarget dropTarget(window.contentItem());

    // Setting the size is important because the QQuickWindow checks if the drag happened inside
    // the drop target.
    dropTarget.setSize(QSizeF(100, 100));

    // Test enter events property propagation.
    // For enter events, only isAccepted gets propagated.
    {
        DragEventTester builder;
        dropTarget.enterAccept = false;
        dropTarget.enterDropAction = Qt::IgnoreAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragEnterEvent* enterEvent = builder.enterEvent;
        QCOMPARE(enterEvent->isAccepted(), dropTarget.enterAccept);
    }
    {
        DragEventTester builder;
        dropTarget.enterAccept = false;
        dropTarget.enterDropAction = Qt::CopyAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragEnterEvent* enterEvent = builder.enterEvent;
        QCOMPARE(enterEvent->isAccepted(), dropTarget.enterAccept);
    }
    {
        DragEventTester builder;
        dropTarget.enterAccept = true;
        dropTarget.enterDropAction = Qt::IgnoreAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragEnterEvent* enterEvent = builder.enterEvent;
        QCOMPARE(enterEvent->isAccepted(), dropTarget.enterAccept);
    }
    {
        DragEventTester builder;
        dropTarget.enterAccept = true;
        dropTarget.enterDropAction = Qt::CopyAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragEnterEvent* enterEvent = builder.enterEvent;
        QCOMPARE(enterEvent->isAccepted(), dropTarget.enterAccept);
    }

    // Test move events property propagation.
    // For move events, both isAccepted and dropAction get propagated.
    dropTarget.reset();
    {
        DragEventTester builder;
        dropTarget.moveAccept = false;
        dropTarget.moveDropAction = Qt::IgnoreAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragMoveEvent* moveEvent = builder.moveEvent;
        QCOMPARE(moveEvent->isAccepted(), dropTarget.moveAccept);
        QCOMPARE(moveEvent->dropAction(), dropTarget.moveDropAction);
    }
    {
        DragEventTester builder;
        dropTarget.moveAccept = false;
        dropTarget.moveDropAction = Qt::CopyAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragMoveEvent* moveEvent = builder.moveEvent;
        QCOMPARE(moveEvent->isAccepted(), dropTarget.moveAccept);
        QCOMPARE(moveEvent->dropAction(), dropTarget.moveDropAction);
    }
    {
        DragEventTester builder;
        dropTarget.moveAccept = true;
        dropTarget.moveDropAction = Qt::IgnoreAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragMoveEvent* moveEvent = builder.moveEvent;
        QCOMPARE(moveEvent->isAccepted(), dropTarget.moveAccept);
        QCOMPARE(moveEvent->dropAction(), dropTarget.moveDropAction);
    }
    {
        DragEventTester builder;
        dropTarget.moveAccept = true;
        dropTarget.moveDropAction = Qt::CopyAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addLeaveEvent();
        builder.sendDragEventSequence(&window);
        QDragMoveEvent* moveEvent = builder.moveEvent;
        QCOMPARE(moveEvent->isAccepted(), dropTarget.moveAccept);
        QCOMPARE(moveEvent->dropAction(), dropTarget.moveDropAction);
    }

    // Test drop events property propagation.
    // For drop events, both isAccepted and dropAction get propagated.
    dropTarget.reset();
    {
        DragEventTester builder;
        dropTarget.dropAccept = false;
        dropTarget.dropDropAction = Qt::IgnoreAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addDropEvent();
        builder.sendDragEventSequence(&window);
        QDropEvent* dropEvent = builder.dropEvent;
        QCOMPARE(dropEvent->isAccepted(), dropTarget.dropAccept);
        QCOMPARE(dropEvent->dropAction(), dropTarget.dropDropAction);
    }
    {
        DragEventTester builder;
        dropTarget.dropAccept = false;
        dropTarget.dropDropAction = Qt::CopyAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addDropEvent();
        builder.sendDragEventSequence(&window);
        QDropEvent* dropEvent = builder.dropEvent;
        QCOMPARE(dropEvent->isAccepted(), dropTarget.dropAccept);
        QCOMPARE(dropEvent->dropAction(), dropTarget.dropDropAction);
    }
    {
        DragEventTester builder;
        dropTarget.dropAccept = true;
        dropTarget.dropDropAction = Qt::IgnoreAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addDropEvent();
        builder.sendDragEventSequence(&window);
        QDropEvent* dropEvent = builder.dropEvent;
        QCOMPARE(dropEvent->isAccepted(), dropTarget.dropAccept);
        QCOMPARE(dropEvent->dropAction(), dropTarget.dropDropAction);
    }
    {
        DragEventTester builder;
        dropTarget.dropAccept = true;
        dropTarget.dropDropAction = Qt::CopyAction;
        builder.addEnterEvent(); builder.addMoveEvent(); builder.addDropEvent();
        builder.sendDragEventSequence(&window);
        QDropEvent* dropEvent = builder.dropEvent;
        QCOMPARE(dropEvent->isAccepted(), dropTarget.dropAccept);
        QCOMPARE(dropEvent->dropAction(), dropTarget.dropDropAction);
    }
}

void tst_qquickwindow::findChild()
{
    QQuickWindow window;

    // QQuickWindow
    // |_ QQuickWindow::contentItem
    // |  |_ QObject("contentItemChild")
    // |_ QObject("viewChild")

    QObject *windowChild = new QObject(&window);
    windowChild->setObjectName("windowChild");

    QObject *contentItemChild = new QObject(window.contentItem());
    contentItemChild->setObjectName("contentItemChild");

    QCOMPARE(window.findChild<QObject *>("windowChild"), windowChild);
    QCOMPARE(window.findChild<QObject *>("contentItemChild"), contentItemChild);

    QVERIFY(!window.contentItem()->findChild<QObject *>("viewChild")); // sibling
    QCOMPARE(window.contentItem()->findChild<QObject *>("contentItemChild"), contentItemChild);
}

class DeliveryRecord : public QPair<QString, QString>
{
public:
    DeliveryRecord(const QString &filter, const QString &receiver) : QPair<QString, QString>(filter, receiver) { }
    DeliveryRecord(const QString &receiver) : QPair<QString, QString>(QString(), receiver) { }
    DeliveryRecord() : QPair<QString, QString>() { }
    QString toString() const {
        if (second.isEmpty())
            return QLatin1String("Delivery(no receiver)");
        else if (first.isEmpty())
            return QString(QLatin1String("Delivery(to '%1')")).arg(second);
        else
            return QString(QLatin1String("Delivery('%1' filtering for '%2')")).arg(first).arg(second);
    }
};

Q_DECLARE_METATYPE(DeliveryRecord)

QDebug operator<<(QDebug dbg, const DeliveryRecord &pair)
{
    dbg << pair.toString();
    return dbg;
}

typedef QVector<DeliveryRecord> DeliveryRecordVector;

class EventItem : public QQuickRectangle
{
    Q_OBJECT
public:
    EventItem(QQuickItem *parent)
        : QQuickRectangle(parent)
        , m_eventAccepts(true)
        , m_filterReturns(true)
        , m_filterAccepts(true)
        , m_filterNotPreAccepted(false)
    {
        QSizeF psize(parent->width(), parent->height());
        psize -= QSizeF(20, 20);
        setWidth(psize.width());
        setHeight(psize.height());
        setPosition(QPointF(10, 10));
    }

    void setFilterReturns(bool filterReturns) { m_filterReturns = filterReturns; }
    void setFilterAccepts(bool accepts) { m_filterAccepts = accepts; }
    void setEventAccepts(bool accepts) { m_eventAccepts = accepts; }

    /*!
     * \internal
     *
     * returns false if any of the calls to childMouseEventFilter had the wrong
     * preconditions. If all calls had the expected precondition, returns true.
     */
    bool testFilterPreConditions() const { return !m_filterNotPreAccepted; }
    static QVector<DeliveryRecord> &deliveryList() { return m_deliveryList; }
    static QSet<QEvent::Type> &includedEventTypes()
    {
        if (m_includedEventTypes.isEmpty())
            m_includedEventTypes << QEvent::MouseButtonPress;
        return m_includedEventTypes;
    }
    static void setExpectedDeliveryList(const QVector<DeliveryRecord> &v) { m_expectedDeliveryList = v; }

protected:
    bool childMouseEventFilter(QQuickItem *i, QEvent *e) override
    {
        appendEvent(this, i, e);
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            if (!e->isAccepted())
                m_filterNotPreAccepted = true;
            e->setAccepted(m_filterAccepts);
            // qCDebug(lcTests) << objectName() << i->objectName();
            return m_filterReturns;
        default:
            break;
        }
        return QQuickRectangle::childMouseEventFilter(i, e);
    }

    bool event(QEvent *e) override
    {
        appendEvent(nullptr, this, e);
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            // qCDebug(lcTests) << objectName();
            e->setAccepted(m_eventAccepts);
            return true;
        default:
            break;
        }
        return QQuickRectangle::event(e);
    }

private:
    static void appendEvent(QQuickItem *filter, QQuickItem *receiver, QEvent *event) {
        if (includedEventTypes().contains(event->type())) {
            auto record = DeliveryRecord(filter ? filter->objectName() : QString(), receiver ? receiver->objectName() : QString());
            int i = m_deliveryList.size();
            if (m_expectedDeliveryList.size() > i && m_expectedDeliveryList[i] == record)
                qCDebug(lcTests).noquote().nospace() << i << ": " << record;
            else
                qCDebug(lcTests).noquote().nospace() << i << ": " << record
                     << ", expected " << (m_expectedDeliveryList.size() > i ? m_expectedDeliveryList[i].toString() : QLatin1String("nothing")) << " <---";
            m_deliveryList << record;
        }
    }
    bool m_eventAccepts;
    bool m_filterReturns;
    bool m_filterAccepts;
    bool m_filterNotPreAccepted;

    // list of (filtering-parent . receiver) pairs
    static DeliveryRecordVector m_expectedDeliveryList;
    static DeliveryRecordVector m_deliveryList;
    static QSet<QEvent::Type> m_includedEventTypes;
};

DeliveryRecordVector EventItem::m_expectedDeliveryList;
DeliveryRecordVector EventItem::m_deliveryList;
QSet<QEvent::Type> EventItem::m_includedEventTypes;

typedef QVector<const char*> CharStarVector;

Q_DECLARE_METATYPE(CharStarVector)

struct InputState {
    struct {
        // event() behavior
        bool eventAccepts;
        // filterChildMouse behavior
        bool returns;
        bool accepts;
        bool filtersChildMouseEvent;
    } r[4];
};

Q_DECLARE_METATYPE(InputState)

void tst_qquickwindow::testChildMouseEventFilter_data()
{
    // HIERARCHY:
    // r0->r1->r2->r3
    //
    QTest::addColumn<QPoint>("mousePos");
    QTest::addColumn<InputState>("inputState");
    QTest::addColumn<DeliveryRecordVector>("expectedDeliveryOrder");

    QTest::newRow("if filtered and rejected, do not deliver it to the item that filtered it")
        << QPoint(100, 100)
        << InputState({
              //  | event() |   child mouse filter
              //  +---------+---------+---------+---------
            { //  | accepts | returns | accepts | filtersChildMouseEvent
                  { false,    false,    false,    false},
                  { true,     false,    false,    false},
                  { false,    true,     false,    true},
                  { false,    false,    false,    false}
            }
        })
        << (DeliveryRecordVector()
            << DeliveryRecord("r2", "r3")
            //<< DeliveryRecord("r3")       // it got filtered -> do not deliver
            // DeliveryRecord("r2")         // r2 filtered it -> do not deliver
            << DeliveryRecord("r1")
            );

    QTest::newRow("no filtering, no accepting")
        << QPoint(100, 100)
        << InputState({
              //  | event() |   child mouse filter
              //  +---------+---------+---------+---------
            { //  | accepts | returns | accepts | filtersChildMouseEvent
                  { false,    false,    false,    false},
                  { false ,   false,    false,    false},
                  { false,    false,    false,    false},
                  { false,    false,    false,    false}
            }
        })
        << (DeliveryRecordVector()
            << DeliveryRecord("r3")
            << DeliveryRecord("r2")
            << DeliveryRecord("r1")
            << DeliveryRecord("r0")
            << DeliveryRecord("root")
            );

    QTest::newRow("all filtering, no accepting")
        << QPoint(100, 100)
        << InputState({
              //  | event() |   child mouse filter
              //  +---------+---------+---------+---------
            { //  | accepts | returns | accepts | filtersChildMouseEvent
                  { false,    false,    false,    true},
                  { false,    false,    false,    true},
                  { false,    false,    false,    true},
                  { false,    false,    false,    true}
            }
        })
        << (DeliveryRecordVector()
            << DeliveryRecord("r2", "r3")
            << DeliveryRecord("r1", "r3")
            << DeliveryRecord("r0", "r3")
            << DeliveryRecord("r3")
            << DeliveryRecord("r1", "r2")
            << DeliveryRecord("r0", "r2")
            << DeliveryRecord("r2")
            << DeliveryRecord("r0", "r1")
            << DeliveryRecord("r1")
            << DeliveryRecord("r0")
            << DeliveryRecord("root")
            );


    QTest::newRow("some filtering, no accepting")
        << QPoint(100, 100)
        << InputState({
              //  | event() |   child mouse filter
              //  +---------+---------+---------+---------
            { //  | accepts | returns | accepts | filtersChildMouseEvent
                  { false,    false,    false,    true},
                  { false,    false,    false,    true},
                  { false,    false,    false,    false},
                  { false,    false,    false,    false}
            }
        })
        << (DeliveryRecordVector()
            << DeliveryRecord("r1", "r3")
            << DeliveryRecord("r0", "r3")
            << DeliveryRecord("r3")
            << DeliveryRecord("r1", "r2")
            << DeliveryRecord("r0", "r2")
            << DeliveryRecord("r2")
            << DeliveryRecord("r0", "r1")
            << DeliveryRecord("r1")
            << DeliveryRecord("r0")
            << DeliveryRecord("root")
            );

    QTest::newRow("r1 accepts")
        << QPoint(100, 100)
        << InputState({
              //  | event() |   child mouse filter
              //  +---------+---------+---------+---------
            { //  | accepts | returns | accepts | filtersChildMouseEvent
                  { false,    false,    false,    true},
                  { true ,    false,    false,    true},
                  { false,    false,    false,    false},
                  { false,    false,    false,    false}
            }
        })
        << (DeliveryRecordVector()
            << DeliveryRecord("r1", "r3")
            << DeliveryRecord("r0", "r3")
            << DeliveryRecord("r3")
            << DeliveryRecord("r1", "r2")
            << DeliveryRecord("r0", "r2")
            << DeliveryRecord("r2")
            << DeliveryRecord("r0", "r1")
            << DeliveryRecord("r1")
            );

    QTest::newRow("r1 rejects and filters")
        << QPoint(100, 100)
        << InputState({
              //  | event() |   child mouse filter
              //  +---------+---------+---------+---------
            { //  | accepts | returns | accepts | filtersChildMouseEvent
                  { false,    false,    false,    true},
                  { false ,    true,    false,    true},
                  { false,    false,    false,    false},
                  { false,    false,    false,    false}
            }
        })
        << (DeliveryRecordVector()
            << DeliveryRecord("r1", "r3")
            << DeliveryRecord("r0", "r3")
//            << DeliveryRecord("r3")   // since it got filtered we don't deliver to r3
            << DeliveryRecord("r1", "r2")
            << DeliveryRecord("r0", "r2")
//            << DeliveryRecord("r2"   // since it got filtered we don't deliver to r2
            << DeliveryRecord("r0", "r1")
//            << DeliveryRecord("r1")  // since it acted as a filter and returned true, we don't deliver to r1
            << DeliveryRecord("r0")
            << DeliveryRecord("root")
            );

}

void tst_qquickwindow::testChildMouseEventFilter()
{
    QFETCH(QPoint, mousePos);
    QFETCH(InputState, inputState);
    QFETCH(DeliveryRecordVector, expectedDeliveryOrder);

    EventItem::setExpectedDeliveryList(expectedDeliveryOrder);

    QQuickWindow window;
    window.resize(500, 809);
    QQuickItem *root = window.contentItem();
    root->setAcceptedMouseButtons(Qt::LeftButton);

    root->setObjectName("root");
    QScopedPointer<EventFilter> rootFilter(new EventFilter);
    root->installEventFilter(rootFilter.data());

    // Create 4 items; each item a child of the previous item.
    EventItem *r[4];
    r[0] = new EventItem(root);
    r[0]->setColor(QColor(0x404040));
    r[0]->setWidth(200);
    r[0]->setHeight(200);

    r[1] = new EventItem(r[0]);
    r[1]->setColor(QColor(0x606060));

    r[2] = new EventItem(r[1]);
    r[2]->setColor(Qt::red);

    r[3] = new EventItem(r[2]);
    r[3]->setColor(Qt::green);

    for (uint i = 0; i < sizeof(r)/sizeof(EventItem*); ++i) {
        r[i]->setEventAccepts(inputState.r[i].eventAccepts);
        r[i]->setFilterReturns(inputState.r[i].returns);
        r[i]->setFilterAccepts(inputState.r[i].accepts);
        r[i]->setFiltersChildMouseEvents(inputState.r[i].filtersChildMouseEvent);
        r[i]->setObjectName(QString::fromLatin1("r%1").arg(i));
        r[i]->setAcceptedMouseButtons(Qt::LeftButton);
    }

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    DeliveryRecordVector &actualDeliveryOrder = EventItem::deliveryList();
    actualDeliveryOrder.clear();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, mousePos);

    // Check if event got delivered to the root item. If so, append it to the list of items the event got delivered to
    if (rootFilter->events.contains(QEvent::MouseButtonPress))
        actualDeliveryOrder.append(DeliveryRecord("root"));

    for (int i = 0; i < qMax(actualDeliveryOrder.size(), expectedDeliveryOrder.size()); ++i) {
        const DeliveryRecord expectedNames = expectedDeliveryOrder.value(i);
        const DeliveryRecord actualNames = actualDeliveryOrder.value(i);
        QCOMPARE(actualNames.toString(), expectedNames.toString());
    }

    for (EventItem *item : r) {
        QVERIFY(item->testFilterPreConditions());
    }

    // "restore" mouse state
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, mousePos);
}

void tst_qquickwindow::cleanupGrabsOnRelease()
{
    TestTouchItem::clearMouseEventCounters();

    QQuickWindow *window = new QQuickWindow;
    QScopedPointer<QQuickWindow> cleanup(window);
    window->resize(250, 250);
    window->setPosition(100, 100);
    window->setTitle(QTest::currentTestFunction());
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    TestTouchItem *parent = new TestTouchItem(window->contentItem());
    parent->setObjectName("parent");
    parent->setSize(QSizeF(150, 150));
    parent->acceptMouseEvents = true;
    parent->grabOnRelease = true;

    TestTouchItem *child = new TestTouchItem(parent);
    child->setObjectName("child");
    child->setSize(QSizeF(100, 100));
    child->acceptMouseEvents = true;

    QPoint pos(80, 80);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, pos);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, pos);
    // There is an explicit parent->grabMouse on release(!). This means grab changes from child
    // to parent:
    // This will emit two ungrab events:
    // 1. One for the child (due to the explicit call to parent->grabMouse())
    // 2. One for the parent (since the mouse button was finally released)
    QCOMPARE(child->mouseUngrabEventCount, 1);
    QCOMPARE(parent->mouseUngrabEventCount, 1);
}

void tst_qquickwindow::subclassWithPointerEventVirtualOverrides_data()
{
    QTest::addColumn<const QPointingDevice *>("device");

    QTest::newRow("mouse click") << QPointingDevice::primaryPointingDevice();
    QTest::newRow("touch tap") << static_cast<const QPointingDevice*>(touchDevice); // TODO QTBUG-107864
    QTest::newRow("stylus tap") << tabletStylusDevice;
}

void tst_qquickwindow::subclassWithPointerEventVirtualOverrides() // QTBUG-97859
{
    QFETCH(const QPointingDevice *, device);

    PointerRecordingWindow window;
    window.resize(250, 250);
    window.setPosition(100, 100);
    window.setTitle(QTest::currentTestFunction());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    const QPoint pos(120, 120);

    QQuickTest::pointerPress(device, &window, 0, pos);
    QQuickTest::pointerRelease(device, &window, 0, pos);

    switch (device->type()) {
    case QPointingDevice::DeviceType::Mouse:
        QTRY_COMPARE(window.m_mouseEvents.size(), 3); // separate move before press
        QCOMPARE(window.m_events.size(), 3);
        break;
    case QPointingDevice::DeviceType::TouchScreen:
        QTRY_COMPARE(window.m_touchEvents.size(), 2);
        QCOMPARE(window.m_events.size(), 2);
        break;
    case QPointingDevice::DeviceType::Stylus:
        QTRY_COMPARE(window.m_tabletEvents.size(), 2);
        QVERIFY(window.m_events.size() >= window.m_tabletEvents.size()); // tablet + synth-mouse events
        break;
    default:
        break;
    }
}

#if QT_CONFIG(shortcut)
void tst_qquickwindow::testShortCut()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("shortcut.qml"));

    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow *window = qobject_cast<QQuickWindow *>(created);
    QVERIFY(QTest::qWaitForWindowActive(window));

    EventFilter eventFilter;
    window->activeFocusItem()->installEventFilter(&eventFilter);
    //Send non-spontaneous key press event
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &keyEvent);
    QVERIFY(eventFilter.events.contains(int(QEvent::ShortcutOverride)));
    QVERIFY(window->property("received").value<bool>());
}

void tst_qquickwindow::shortcutOverride_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<bool>("overridden");
    QTest::addColumn<bool>("receivedA");
    QTest::addColumn<bool>("receivedB");

    QTest::addRow("Space") << Qt::Key_Space << false << false << false;
    QTest::addRow("A") << Qt::Key_A << true << false << false;
    QTest::addRow("B") << Qt::Key_B << false << false << true;
}

void tst_qquickwindow::shortcutOverride()
{
    QFETCH(Qt::Key, key);
    QFETCH(bool, overridden);
    QFETCH(bool, receivedA);
    QFETCH(bool, receivedB);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("shortcutOverride.qml"));

    QScopedPointer<QWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowActive(window.get()));

    QTest::keyPress(window.get(), key);
    QCOMPARE(window->property("overridden").value<bool>(), overridden);
    QCOMPARE(window->property("receivedA").value<bool>(), receivedA);
    QCOMPARE(window->property("receivedB").value<bool>(), receivedB);
}
#endif

void tst_qquickwindow::rendererInterface()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("Headless.qml"));
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->setTitle(QTest::currentTestFunction());
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->isSceneGraphInitialized());
    QVERIFY(window->rendererInterface());

    QSGRendererInterface *rif = window->rendererInterface();
    QVERIFY(rif->graphicsApi() != QSGRendererInterface::Unknown);

    // Verify the essential integration points used by Quick3D.
    if (QSGRendererInterface::isApiRhiBased(rif->graphicsApi())) {
        QVERIFY(rif->getResource(window, QSGRendererInterface::RhiResource));
        QVERIFY(rif->getResource(window, QSGRendererInterface::RhiSwapchainResource));
        // the rendercontrol specific objects should not be present for an on-screen window
        QVERIFY(!rif->getResource(window, QSGRendererInterface::RhiRedirectCommandBuffer));
        QVERIFY(!rif->getResource(window, QSGRendererInterface::RhiRedirectRenderTarget));
    }

    // Now, depending on the graphics API, verify the native objects that are
    // most common in applications that integrate native rendering code. Check
    // only the globally available ones (that are available whenever the
    // scenegraph is initialized, not just during recording a frame)
    switch (rif->graphicsApi()) {
    case QSGRendererInterface::OpenGLRhi:
        QVERIFY(rif->getResource(window, QSGRendererInterface::OpenGLContextResource));
#if QT_CONFIG(opengl)
        {
            QOpenGLContext *ctx = static_cast<QOpenGLContext *>(rif->getResource(window, QSGRendererInterface::OpenGLContextResource));
            QVERIFY(ctx->isValid());
        }
#endif
        break;
    case QSGRendererInterface::Direct3D11Rhi:
        QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceResource));
        QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceContextResource));
        break;
    case QSGRendererInterface::VulkanRhi:
        QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceResource));
        QVERIFY(rif->getResource(window, QSGRendererInterface::PhysicalDeviceResource));
        QVERIFY(rif->getResource(window, QSGRendererInterface::VulkanInstanceResource));
#if QT_CONFIG(vulkan)
        QCOMPARE(rif->getResource(window, QSGRendererInterface::VulkanInstanceResource), window->vulkanInstance());
#endif
        QVERIFY(rif->getResource(window, QSGRendererInterface::CommandQueueResource));
        break;
    case QSGRendererInterface::MetalRhi:
        QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceResource));
        QVERIFY(rif->getResource(window, QSGRendererInterface::CommandQueueResource));
        break;
    default:
        break;
    }

    // Now the objects that are available only when preparing a frame.
    if (QSGRendererInterface::isApiRhiBased(rif->graphicsApi())) {
        bool ok[4] = { false, false, false, false };
        auto f = [&ok, window](int idx) {
            QSGRendererInterface *rif = window->rendererInterface();
            if (rif) {
                ok[idx] = true;
                switch (rif->graphicsApi()) {
                case QSGRendererInterface::VulkanRhi:
                    if (!rif->getResource(window, QSGRendererInterface::CommandListResource))
                        ok[idx] = false;
                    if (!rif->getResource(window, QSGRendererInterface::RenderPassResource))
                        ok[idx] = false;
                    break;
                case QSGRendererInterface::MetalRhi:
                    if (!rif->getResource(window, QSGRendererInterface::CommandListResource))
                        ok[idx] = false;
                    if (idx == 1 || idx == 2) { // must be recording a render pass to query the command encoder
                        if (!rif->getResource(window, QSGRendererInterface::CommandEncoderResource))
                            ok[idx] = false;
                    }
                    break;
                default:
                    break;
                }
            }
        };
        // Also tests if all 4 signals are emitted as expected.
        QObject::connect(window, &QQuickWindow::beforeRendering, window, std::bind(f, 0), Qt::DirectConnection);
        QObject::connect(window, &QQuickWindow::beforeRenderPassRecording, window, std::bind(f, 1), Qt::DirectConnection);
        QObject::connect(window, &QQuickWindow::afterRenderPassRecording, window, std::bind(f, 2), Qt::DirectConnection);
        QObject::connect(window, &QQuickWindow::afterRendering, window, std::bind(f, 3), Qt::DirectConnection);
        window->grabWindow();
        QVERIFY(ok[0]);
        QVERIFY(ok[1]);
        QVERIFY(ok[2]);
        QVERIFY(ok[3]);
    }
}

void tst_qquickwindow::rendererInterfaceWithRenderControl_data()
{
    QTest::addColumn<QSGRendererInterface::GraphicsApi>("api");

#if QT_CONFIG(opengl)
    QTest::newRow("OpenGL") << QSGRendererInterface::OpenGLRhi;
#endif
#if QT_CONFIG(vulkan)
    QTest::newRow("Vulkan") << QSGRendererInterface::VulkanRhi;
#endif
#ifdef Q_OS_WIN
    QTest::newRow("D3D11") << QSGRendererInterface::Direct3D11Rhi;
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    QTest::newRow("Metal") << QSGRendererInterface::MetalRhi;
#endif
}

void tst_qquickwindow::rendererInterfaceWithRenderControl()
{
    QFETCH(QSGRendererInterface::GraphicsApi, api);

    // no automatic QVulkanInstance when used with rendercontrol, must create our own
#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (api == QSGRendererInterface::VulkanRhi) {
        if (!inst.create())
            QSKIP("Skipping Vulkan-based test due to failing to create Vulkan instance");
    }
#endif

    {
        // Changing the graphics api is not possible once a QQuickWindow et al is
        // created, however we do support changing it once all QQuickWindow,
        // QQuickRenderControl, etc. instances are destroyed, before creating new
        // ones. That's why it is possible to have this test run with multiple QRhi
        // backends.
        QQuickWindow::setGraphicsApi(api);

        QScopedPointer<QQuickRenderControl> renderControl(new QQuickRenderControl);
        QScopedPointer<QQuickWindow> quickWindow(new QQuickWindow(renderControl.data()));
#if QT_CONFIG(vulkan)
        if (api == QSGRendererInterface::VulkanRhi)
            quickWindow->setVulkanInstance(&inst);
#endif

        QScopedPointer<QQmlEngine> qmlEngine(new QQmlEngine);
        // Pick a scene that does not have a Window, as having a Window in there is
        // incompatible with QQuickRenderControl-based redirection by definition.
        QScopedPointer<QQmlComponent> qmlComponent(new QQmlComponent(qmlEngine.data(),
                                                                     testFileUrl(QLatin1String("showHideAnimate.qml"))));
        QVERIFY(!qmlComponent->isLoading());
        if (qmlComponent->isError()) {
            for (const QQmlError &error : qmlComponent->errors())
                qWarning() << error.url() << error.line() << error;
        }
        QVERIFY(!qmlComponent->isError());

        QObject *rootObject = qmlComponent->create();
        if (qmlComponent->isError()) {
            for (const QQmlError &error : qmlComponent->errors())
                qWarning() << error.url() << error.line() << error;
        }
        QVERIFY(!qmlComponent->isError());

        QQuickItem *rootItem = qobject_cast<QQuickItem *>(rootObject);
        QVERIFY(rootItem);
        rootItem->setSize(QSize(200, 200));

        quickWindow->contentItem()->setSize(rootItem->size());
        quickWindow->setGeometry(0, 0, rootItem->width(), rootItem->height());

        rootItem->setParentItem(quickWindow->contentItem());

        const bool initSuccess = renderControl->initialize();

        // We cannot just test for initSuccess; it is highly likely that a
        // number of configurations will simply fail in a CI environment
        // (Vulkan, Metal, ...) So the only reasonable choice is to skip if
        // initialize() failed. The exception for now is OpenGL - that should
        // usually work (except software backend etc.).
        if (!initSuccess) {
#if QT_CONFIG(opengl)
            if (api != QSGRendererInterface::OpenGLRhi
                    || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
#endif
            {
                QSKIP("Could not initialize graphics, perhaps unsupported graphics API, skipping");
            }
        }

        // Not strictly required for the CI (because platforms like offscreen
        // skip already above), but play nice with running with
        // QT_QUICK_BACKEND=software on a normal desktop platform.
        if (QQuickWindow::sceneGraphBackend() == QLatin1String("software"))
            QSKIP("Software backend was forced via env.var, skipping this test");

        QVERIFY(initSuccess);

        QQuickWindow *window = quickWindow.data();
        QSGRendererInterface *rif = window->rendererInterface();
        QVERIFY(rif);
        QCOMPARE(rif->graphicsApi(), api);

        QRhi *rhi = static_cast<QRhi *>(rif->getResource(window, QSGRendererInterface::RhiResource));
        QVERIFY(rhi);

        // not an on-screen window so no swapchain
        QVERIFY(!rif->getResource(window, QSGRendererInterface::RhiSwapchainResource));

        switch (rif->graphicsApi()) {
        case QSGRendererInterface::OpenGLRhi:
            QVERIFY(rif->getResource(window, QSGRendererInterface::OpenGLContextResource));
#if QT_CONFIG(opengl)
            {
                QOpenGLContext *ctx = static_cast<QOpenGLContext *>(rif->getResource(window, QSGRendererInterface::OpenGLContextResource));
                QVERIFY(ctx->isValid());
            }
#endif
            break;
        case QSGRendererInterface::Direct3D11Rhi:
            QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceResource));
            QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceContextResource));
            break;
        case QSGRendererInterface::VulkanRhi:
            QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceResource));
            QVERIFY(rif->getResource(window, QSGRendererInterface::PhysicalDeviceResource));
            QVERIFY(rif->getResource(window, QSGRendererInterface::VulkanInstanceResource));
#if QT_CONFIG(vulkan)
            QCOMPARE(rif->getResource(window, QSGRendererInterface::VulkanInstanceResource), window->vulkanInstance());
            QCOMPARE(rif->getResource(window, QSGRendererInterface::VulkanInstanceResource), &inst);
#endif
            QVERIFY(rif->getResource(window, QSGRendererInterface::CommandQueueResource));
            break;
        case QSGRendererInterface::MetalRhi:
            QVERIFY(rif->getResource(window, QSGRendererInterface::DeviceResource));
            QVERIFY(rif->getResource(window, QSGRendererInterface::CommandQueueResource));
            break;
        default:
            break;
        }

        const QSize size(1280, 720);
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        QVERIFY(tex->create());
        QScopedPointer<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1));
        QVERIFY(ds->create());
        QRhiTextureRenderTargetDescription rtDesc(QRhiColorAttachment(tex.data()));
        rtDesc.setDepthStencilBuffer(ds.data());
        QScopedPointer<QRhiTextureRenderTarget> texRt(rhi->newTextureRenderTarget(rtDesc));
        QScopedPointer<QRhiRenderPassDescriptor> rp(texRt->newCompatibleRenderPassDescriptor());
        texRt->setRenderPassDescriptor(rp.data());
        QVERIFY(texRt->create());

        // redirect Qt Quick rendering into our texture
        quickWindow->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(texRt.data()));

        bool ok[4] = { false, false, false, false };
        auto f = [&ok, window](int idx) {
            QSGRendererInterface *rif = window->rendererInterface();
            if (rif) {
                ok[idx] = true;
                switch (rif->graphicsApi()) {
                case QSGRendererInterface::VulkanRhi:
                    if (!rif->getResource(window, QSGRendererInterface::CommandListResource))
                        ok[idx] = false;
                    if (!rif->getResource(window, QSGRendererInterface::RenderPassResource))
                        ok[idx] = false;
                    break;
                case QSGRendererInterface::MetalRhi:
                    if (!rif->getResource(window, QSGRendererInterface::CommandListResource))
                        ok[idx] = false;
                    if (idx == 1 || idx == 2) { // must be recording a render pass to query the command encoder
                        if (!rif->getResource(window, QSGRendererInterface::CommandEncoderResource))
                            ok[idx] = false;
                    }
                    break;
                default:
                    break;
                }
            }
        };
        // we could just check this below like the RhiRedirect* ones, but this way we test the signals as well
        QObject::connect(window, &QQuickWindow::beforeRendering, window, std::bind(f, 0), Qt::DirectConnection);
        QObject::connect(window, &QQuickWindow::beforeRenderPassRecording, window, std::bind(f, 1), Qt::DirectConnection);
        QObject::connect(window, &QQuickWindow::afterRenderPassRecording, window, std::bind(f, 2), Qt::DirectConnection);
        QObject::connect(window, &QQuickWindow::afterRendering, window, std::bind(f, 3), Qt::DirectConnection);

        renderControl->polishItems();
        renderControl->beginFrame();

        renderControl->sync(); // this is when the custom rendertarget request gets processed
        // now check for the queries that are used by Quick3D f.ex.
        QVERIFY(rif->getResource(window, QSGRendererInterface::RhiRedirectCommandBuffer));
        QVERIFY(rif->getResource(window, QSGRendererInterface::RhiRedirectRenderTarget));
        QCOMPARE(rif->getResource(window, QSGRendererInterface::RhiRedirectRenderTarget), texRt.data());

        renderControl->render();
        renderControl->endFrame();

        QVERIFY(ok[0]);
        QVERIFY(ok[1]);
        QVERIFY(ok[2]);
        QVERIFY(ok[3]);
    }

    // Now that everything is torn down, go back to the default unspecified-api
    // state, to prevent conflicting with tests that come afterwards.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Unknown);
}

void tst_qquickwindow::graphicsConfiguration()
{
    QQuickGraphicsConfiguration config;
    qDebug() << config;
    QVERIFY(config.isDepthBufferEnabledFor2D());
    QVERIFY(!config.isDebugLayerEnabled());
    QVERIFY(!config.isDebugMarkersEnabled());
    QVERIFY(!config.prefersSoftwareDevice());
    QVERIFY(config.isAutomaticPipelineCacheEnabled());
    QVERIFY(config.pipelineCacheSaveFile().isEmpty());
    QVERIFY(config.pipelineCacheLoadFile().isEmpty());

    QQuickGraphicsConfiguration config2 = config;
    config.setDebugLayer(true);
    config.setDepthBufferFor2D(false);
    QVERIFY(config.isDebugLayerEnabled());
    QVERIFY(!config2.isDebugLayerEnabled());

    config2 = config;
    QVERIFY(config2.isDebugLayerEnabled());
    QVERIFY(!config2.isDepthBufferEnabledFor2D());

    config.setAutomaticPipelineCache(false);
    config.setPipelineCacheSaveFile(QLatin1String("save"));
    config.setPipelineCacheLoadFile(QLatin1String("load"));
    config2 = config;
    QVERIFY(!config2.isAutomaticPipelineCacheEnabled());
    QCOMPARE(config2.pipelineCacheSaveFile(), QLatin1String("save"));
    QCOMPARE(config2.pipelineCacheLoadFile(), QLatin1String("load"));

#if QT_CONFIG(vulkan)
    QCOMPARE(QQuickGraphicsConfiguration::preferredInstanceExtensions(), QRhiVulkanInitParams::preferredInstanceExtensions());
#endif
}

void tst_qquickwindow::visibleVsVisibility_data()
{
    QTest::addColumn<QUrl>("qmlfile");
    QTest::addColumn<bool>("expectVisible");
    QTest::addColumn<bool>("expectConflictingPropertyWarning");

    QTest::newRow("default invisible") << testFileUrl("window.qml") << false << false;
    QTest::newRow("just visibility") << testFileUrl("maximized.qml") << true << false;
    // In these conflicting cases, the 'visibility' property "wins" (see QQuickWindowQmlImpl::setWindowVisibility())
    QTest::newRow("conflicting invisible") << testFileUrl("conflictingVisibleFalse.qml") << true << true;
    QTest::newRow("conflicting visible") << testFileUrl("conflictingVisibleTrue.qml") << false << true;
}

void tst_qquickwindow::visibleVsVisibility()
{
    QFETCH(QUrl, qmlfile);
    QFETCH(bool, expectVisible);
    QFETCH(bool, expectConflictingPropertyWarning);

    const QString warningMsg = qmlfile.toString() + ": Conflicting properties 'visible' and 'visibility'";

    QTest::failOnWarning(QRegularExpression(".*"));
    if (expectConflictingPropertyWarning)
        QTest::ignoreMessage(QtWarningMsg, warningMsg.toUtf8().data());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(qmlfile);
    QObject *created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow *window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QCOMPARE(window->isVisible(), expectVisible);
}

QTEST_MAIN(tst_qquickwindow)

#include "tst_qquickwindow.moc"
