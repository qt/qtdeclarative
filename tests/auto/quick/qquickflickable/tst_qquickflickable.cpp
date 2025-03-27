// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/QtQuickTest>
#include <QtGui/QStyleHints>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <private/qguiapplication_p.h>
#include <private/qquickflickable_p.h>
#include <private/qquickflickable_p_p.h>
#include <private/qquickmousearea_p.h>
#include <private/qquicktransition_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qquicktaphandler_p.h>
#include <math.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/geometrytestutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <qpa/qwindowsysteminterface.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

// an abstract Slider which only handles touch events
class TouchDragArea : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos NOTIFY posChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool keepMouseGrab READ keepMouseGrab WRITE setKeepMouseGrab NOTIFY keepMouseGrabChanged)
    Q_PROPERTY(bool keepTouchGrab READ keepTouchGrab WRITE setKeepTouchGrab NOTIFY keepTouchGrabChanged)

public:
    TouchDragArea(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
        , touchEvents(0)
        , touchUpdates(0)
        , touchReleases(0)
        , ungrabs(0)
        , m_active(false)
    {
        setAcceptTouchEvents(true);
    }

    QPointF pos() const { return m_pos; }

    bool active() const  { return m_active; }

    void setKeepMouseGrab(bool keepMouseGrab)
    {
        QQuickItem::setKeepMouseGrab(keepMouseGrab);
        emit keepMouseGrabChanged();
    }

    void setKeepTouchGrab(bool keepTouchGrab)
    {
        QQuickItem::setKeepTouchGrab(keepTouchGrab);
        emit keepTouchGrabChanged();
    }

    int touchEvents;
    int touchUpdates;
    int touchReleases;
    int ungrabs;
    QVector<QEventPoint::State> touchPointStates;

protected:
    void touchEvent(QTouchEvent *ev) override
    {
        QCOMPARE(ev->points().size(), 1);
        auto touchpoint = ev->points().first();
        switch (touchpoint.state()) {
        case QEventPoint::State::Pressed:
            QVERIFY(!m_active);
            m_active = true;
            emit activeChanged();
            grabTouchPoints(QList<int>() << touchpoint.id());
            break;
        case QEventPoint::State::Updated:
            ++touchUpdates;
            break;
        case QEventPoint::State::Released:
            QVERIFY(m_active);
            m_active = false;
            ++touchReleases;
            emit activeChanged();
        case QEventPoint::State::Stationary:
        case QEventPoint::State::Unknown:
            break;
        }
        touchPointStates << touchpoint.state();
        ++touchEvents;
        m_pos = touchpoint.position();
        emit posChanged();
    }

    void touchUngrabEvent() override
    {
        ++ungrabs;
        emit ungrabbed();
        m_active = false;
        emit activeChanged();
    }

signals:
    void ungrabbed();
    void posChanged();
    void keepMouseGrabChanged();
    void keepTouchGrabChanged();
    void activeChanged();

private:
    QPointF m_pos;
    bool m_active;
};

class FlickableWithExtents : public QQuickFlickable
{
public:
    qreal extent = 10;

    qreal minXExtent() const override
    {
        return QQuickFlickable::minXExtent() + extent;
    }

    qreal maxXExtent() const override
    {
        return QQuickFlickable::maxXExtent() + extent;
    }

    qreal minYExtent() const override
    {
        return QQuickFlickable::minYExtent() + extent;
    }

    qreal maxYExtent() const override
    {
        return QQuickFlickable::maxYExtent() + extent;
    }
};

class tst_qquickflickable : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickflickable()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void initTestCase() override;
    void create();
    void horizontalViewportSize();
    void verticalViewportSize();
    void visibleAreaRatiosUpdate();
    void properties();
    void boundsBehavior();
    void rebound();
    void maximumFlickVelocity();
    void flickDeceleration();
    void pressDelay_data();
    void pressDelay();
    void nestedPressDelay();
    void filterReplayedPress();
    void nestedClickThenFlick();
    void flickableDirection();
    void resizeContent();
    void returnToBounds();
    void returnToBounds_data();
    void wheel();
    void wheelBackwards();
    void trackpad();
    void nestedTrackpad();
    void nestedSameDirectionTrackpad();
    void movingAndFlicking();
    void movingAndFlicking_data();
    void movingAndDragging();
    void dragAndSwitchDirection_data();
    void dragAndSwitchDirection();
    void movingAndDragging_data();
    void flickOnRelease();
    void pressWhileFlicking();
    void dragWhileFlicking();
    void disabled();
    void flickVelocity();
    void margins();
    void cancelOnHide();
    void cancelOnMouseGrab();
    void clickAndDragWhenTransformed();
    void flickTwiceUsingTouches();
    void nestedStopAtBounds();
    void nestedDraggingFlickingMoving_data();
    void nestedDraggingFlickingMoving();
    void nestedStopAtBounds_data();
    void stopAtBounds();
    void stopAtBounds_data();
    void nestedMouseAreaUsingTouch();
    void nestedMouseAreaPropagateComposedEvents();
    void nestedSliderUsingTouch();
    void nestedSliderUsingTouch_data();
    void pressDelayWithLoader();
    void movementFromProgrammaticFlick();
    void cleanup();
    void contentSize();
    void ratios_smallContent();
    void contentXYNotTruncatedToInt();
    void keepGrab();
    void overshoot();
    void overshoot_data();
    void overshoot_reentrant();
    void synchronousDrag_data();
    void synchronousDrag();
    void visibleAreaBinding();
    void parallelTouch();
    void ignoreNonLeftMouseButtons();
    void ignoreNonLeftMouseButtons_data();
    void receiveTapOutsideContentItem();
    void flickWhenRotated_data();
    void flickWhenRotated();
    void flickAndReleaseOutsideBounds();
    void scrollingWithFractionalExtentSize_data();
    void scrollingWithFractionalExtentSize();
    void setContentPositionWhileDragging_data();
    void setContentPositionWhileDragging();
    void coalescedMove();
    void onlyOneMove();
    void proportionalWheelScrolling();
    void touchCancel();
    void pixelAlignedEndPoints();
    void nestedWheelEventPropagation_data();
    void nestedWheelEventPropagation();

private:
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
    std::unique_ptr<QPointingDevice> mouseDevice{new QPointingDevice(
            "test mouse", 1000, QInputDevice::DeviceType::Mouse, QPointingDevice::PointerType::Generic,
            QInputDevice::Capability::Position | QInputDevice::Capability::Hover | QInputDevice::Capability::Scroll,
            1, 5, QString(), QPointingDeviceUniqueId(), this)};
    std::unique_ptr<QPointingDevice> touchpad{
            QTest::createTouchDevice(QInputDevice::DeviceType::TouchPad,
                                     QInputDevice::Capability::Position | QInputDevice::Capability::PixelScroll)};
};

void tst_qquickflickable::initTestCase()
{
#ifdef Q_OS_ANDROID
    QSKIP("test crashes at unknown location in Android");
#endif
    QQmlDataTest::initTestCase();
    qmlRegisterType<TouchDragArea>("Test",1,0,"TouchDragArea");
    QWindowSystemInterface::registerInputDevice(mouseDevice.get());
}

void tst_qquickflickable::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

void tst_qquickflickable::create()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("flickable01.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.createWithInitialProperties({{"setRebound", false}}));

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);

    QCOMPARE(obj->horizontalVelocity(), 0.);
    QCOMPARE(obj->verticalVelocity(), 0.);

    QCOMPARE(obj->isInteractive(), true);
    QCOMPARE(obj->boundsBehavior(), QQuickFlickable::DragAndOvershootBounds);
    QCOMPARE(obj->pressDelay(), 0);
    QCOMPARE(obj->maximumFlickVelocity(), 2500.);

    delete obj;
}

void tst_qquickflickable::horizontalViewportSize()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("flickable02.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->contentWidth(), 800.);
    QCOMPARE(obj->contentHeight(), 300.);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);

    delete obj;
}

void tst_qquickflickable::verticalViewportSize()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("flickable03.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->contentWidth(), 200.);
    QCOMPARE(obj->contentHeight(), 6000.);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);

    delete obj;
}

void tst_qquickflickable::visibleAreaRatiosUpdate()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("ratios.qml"));
    QQuickItem *obj = qobject_cast<QQuickItem*>(c.create());

    QVERIFY(obj != nullptr);
    // check initial ratio values
    QCOMPARE(obj->property("heightRatioIs").toDouble(), obj->property("heightRatioShould").toDouble());
    QCOMPARE(obj->property("widthRatioIs").toDouble(), obj->property("widthRatioShould").toDouble());
    // change flickable geometry so that flicking is enabled (content size > flickable size)
    obj->setProperty("forceNoFlicking", false);
    QCOMPARE(obj->property("heightRatioIs").toDouble(), obj->property("heightRatioShould").toDouble());
    QCOMPARE(obj->property("widthRatioIs").toDouble(), obj->property("widthRatioShould").toDouble());
    // change flickable geometry so that flicking is disabled (content size == flickable size)
    obj->setProperty("forceNoFlicking", true);
    QCOMPARE(obj->property("heightRatioIs").toDouble(), obj->property("heightRatioShould").toDouble());
    QCOMPARE(obj->property("widthRatioIs").toDouble(), obj->property("widthRatioShould").toDouble());

    delete obj;
}

void tst_qquickflickable::properties()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("flickable04.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->isInteractive(), false);
    QCOMPARE(obj->boundsBehavior(), QQuickFlickable::StopAtBounds);
    QCOMPARE(obj->pressDelay(), 200);
    QCOMPARE(obj->maximumFlickVelocity(), 2000.);

    QVERIFY(!obj->property("ok").toBool());
    QMetaObject::invokeMethod(obj, "check");
    QVERIFY(obj->property("ok").toBool());

    delete obj;
}

void tst_qquickflickable::boundsBehavior()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { boundsBehavior: Flickable.StopAtBounds }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(boundsBehaviorChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->boundsBehavior(), QQuickFlickable::StopAtBounds);

    flickable->setBoundsBehavior(QQuickFlickable::DragAndOvershootBounds);
    QCOMPARE(flickable->boundsBehavior(), QQuickFlickable::DragAndOvershootBounds);
    QCOMPARE(spy.size(),1);
    flickable->setBoundsBehavior(QQuickFlickable::DragAndOvershootBounds);
    QCOMPARE(spy.size(),1);

    flickable->setBoundsBehavior(QQuickFlickable::DragOverBounds);
    QCOMPARE(flickable->boundsBehavior(), QQuickFlickable::DragOverBounds);
    QCOMPARE(spy.size(),2);
    flickable->setBoundsBehavior(QQuickFlickable::DragOverBounds);
    QCOMPARE(spy.size(),2);

    flickable->setBoundsBehavior(QQuickFlickable::StopAtBounds);
    QCOMPARE(flickable->boundsBehavior(), QQuickFlickable::StopAtBounds);
    QCOMPARE(spy.size(),3);
    flickable->setBoundsBehavior(QQuickFlickable::StopAtBounds);
    QCOMPARE(spy.size(),3);

    flickable->setBoundsBehavior(QQuickFlickable::OvershootBounds);
    QCOMPARE(flickable->boundsBehavior(), QQuickFlickable::OvershootBounds);
    QCOMPARE(spy.size(),4);
    flickable->setBoundsBehavior(QQuickFlickable::OvershootBounds);
    QCOMPARE(spy.size(),4);

    delete flickable;
}

void tst_qquickflickable::rebound()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("rebound.qml")));
    QVERIFY(window.rootObject() != nullptr);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QQuickTransition *rebound = window.rootObject()->findChild<QQuickTransition*>("rebound");
    QVERIFY(rebound);
    QSignalSpy reboundSpy(rebound, SIGNAL(runningChanged()));

    QSignalSpy movementStartedSpy(flickable, SIGNAL(movementStarted()));
    QSignalSpy movementEndedSpy(flickable, SIGNAL(movementEnded()));
    QSignalSpy vMoveSpy(flickable, SIGNAL(movingVerticallyChanged()));
    QSignalSpy hMoveSpy(flickable, SIGNAL(movingHorizontallyChanged()));

    // flick and test the transition is run
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,20), QPoint(120,120), 200);

    QTRY_COMPARE(window.rootObject()->property("transitionsStarted").toInt(), 2);
    QCOMPARE(hMoveSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), 1);
    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 0);
    QVERIFY(rebound->running());

    QTRY_VERIFY(!flickable->isMoving());
    QCOMPARE(flickable->contentX(), 0.0);
    QCOMPARE(flickable->contentY(), 0.0);

    QCOMPARE(hMoveSpy.size(), 2);
    QCOMPARE(vMoveSpy.size(), 2);
    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 1);
    QCOMPARE(window.rootObject()->property("transitionsStarted").toInt(), 2);
    QVERIFY(!rebound->running());
    QCOMPARE(reboundSpy.size(), 2);

    hMoveSpy.clear();
    vMoveSpy.clear();
    movementStartedSpy.clear();
    movementEndedSpy.clear();
    window.rootObject()->setProperty("transitionsStarted", 0);
    window.rootObject()->setProperty("transitionsFinished", 0);

    // flick and trigger the transition multiple times
    // (moving signals are emitted as soon as the first transition starts)
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,20), QPoint(120,120), 50);     // both x and y will bounce back
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,120), QPoint(120,20), 50);     // only x will bounce back

    QVERIFY(flickable->isMoving());
    QTRY_VERIFY(window.rootObject()->property("transitionsStarted").toInt() >= 1);
    QCOMPARE(hMoveSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), 1);
    QCOMPARE(movementStartedSpy.size(), 1);

    QTRY_VERIFY(!flickable->isMoving());
    QCOMPARE(flickable->contentX(), 0.0);

    // moving started/stopped signals should only have been emitted once,
    // and when they are, all transitions should have finished
    QCOMPARE(hMoveSpy.size(), 2);
    QCOMPARE(vMoveSpy.size(), 2);
    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 1);

    hMoveSpy.clear();
    vMoveSpy.clear();
    movementStartedSpy.clear();
    movementEndedSpy.clear();
    window.rootObject()->setProperty("transitionsStarted", 0);
    window.rootObject()->setProperty("transitionsFinished", 0);

    // disable and the default transition should run
    // (i.e. moving but transition->running = false)
    window.rootObject()->setProperty("transitionEnabled", false);

    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,20), QPoint(120,120), 200);
    QCOMPARE(window.rootObject()->property("transitionsStarted").toInt(), 0);
    QCOMPARE(hMoveSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), 1);
    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 0);

    QTRY_VERIFY(!flickable->isMoving());
    QCOMPARE(hMoveSpy.size(), 2);
    QCOMPARE(vMoveSpy.size(), 2);
    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 1);
    QCOMPARE(window.rootObject()->property("transitionsStarted").toInt(), 0);
}

void tst_qquickflickable::maximumFlickVelocity()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { maximumFlickVelocity: 1.0; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(maximumFlickVelocityChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->maximumFlickVelocity(), 1.0);

    flickable->setMaximumFlickVelocity(2.0);
    QCOMPARE(flickable->maximumFlickVelocity(), 2.0);
    QCOMPARE(spy.size(),1);
    flickable->setMaximumFlickVelocity(2.0);
    QCOMPARE(spy.size(),1);

    delete flickable;
}

void tst_qquickflickable::flickDeceleration()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { flickDeceleration: 1.0; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(flickDecelerationChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->flickDeceleration(), 1.0);

    flickable->setFlickDeceleration(2.0);
    QCOMPARE(flickable->flickDeceleration(), 2.0);
    QCOMPARE(spy.size(),1);
    flickable->setFlickDeceleration(2.0);
    QCOMPARE(spy.size(),1);

    delete flickable;
}

void tst_qquickflickable::pressDelay_data()
{
    QTest::addColumn<const QPointingDevice *>("device");
    const QPointingDevice *constMouse = mouseDevice.get();
    const QPointingDevice *constTouchscreen = touchscreen.get();

    QTest::newRow("mouse") << constMouse;
    QTest::newRow("touch") << constTouchscreen;
}

void tst_qquickflickable::pressDelay()
{
    QFETCH(const QPointingDevice *, device);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pressDelay.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);
    QQuickMouseArea *mouseArea = flickable->findChild<QQuickMouseArea*>();
    QSignalSpy clickedSpy(mouseArea, &QQuickMouseArea::clicked);

    // Test the pressDelay property itself
    QSignalSpy pressDelayChangedSpy(flickable, &QQuickFlickable::pressDelayChanged);
    QCOMPARE(flickable->pressDelay(), 100);
    flickable->setPressDelay(200);
    QCOMPARE(flickable->pressDelay(), 200);
    QCOMPARE(pressDelayChangedSpy.size(), 1);
    flickable->setPressDelay(200);
    QCOMPARE(pressDelayChangedSpy.size(), 1);


    // Test the press delay
    QPoint p(150, 150);
    if (device->type() == QInputDevice::DeviceType::Mouse)
        QQuickTest::pointerMove(device, &window, 0, p);
    QQuickTest::pointerPress(device, &window, 0, p);

    // The press should not occur immediately
    QCOMPARE(mouseArea->isPressed(), false);

    // But, it should occur eventually
    QTRY_VERIFY(mouseArea->isPressed());

    QCOMPARE(clickedSpy.size(), 0);

    // On release the clicked signal should be emitted
    QQuickTest::pointerRelease(device, &window, 0, p);
    QCOMPARE(clickedSpy.size(), 1);

    // Press and release position should match
    QCOMPARE(flickable->property("pressX").toReal(), flickable->property("releaseX").toReal());
    QCOMPARE(flickable->property("pressY").toReal(), flickable->property("releaseY").toReal());


    // Test a quick tap within the pressDelay timeout
    p = QPoint(180, 180);
    clickedSpy.clear();
    if (device->type() == QInputDevice::DeviceType::Mouse)
        QQuickTest::pointerMove(device, &window, 0, p);
    QQuickTest::pointerPress(device, &window, 0, p);

    // The press should not occur immediately
    QCOMPARE(mouseArea->isPressed(), false);
    QCOMPARE(clickedSpy.size(), 0);

    // On release, the press, release and clicked signal should be emitted
    QQuickTest::pointerRelease(device, &window, 0, p);
    QCOMPARE(clickedSpy.size(), 1);

    // Press and release position should match
    QCOMPARE(flickable->property("pressX").toReal(), flickable->property("releaseX").toReal());
    QCOMPARE(flickable->property("pressY").toReal(), flickable->property("releaseY").toReal());


    // Test flick after press (QTBUG-31168)
    QPoint startPosition(150, 110);
    p = QPoint(150, 190);
    clickedSpy.clear();
    if (device->type() == QInputDevice::DeviceType::Mouse)
        QQuickTest::pointerMove(device, &window, 0, startPosition);
    QQuickTest::pointerPress(device, &window, 0, startPosition);

    // The press should not occur immediately
    QCOMPARE(mouseArea->isPressed(), false);
    QQuickTest::pointerMove(device, &window, 0, p);

    // Since we moved past the drag threshold, we should never receive the press
    QCOMPARE(mouseArea->isPressed(), false);
    QTRY_COMPARE(mouseArea->isPressed(), false);

    // On release, the clicked signal should *not* be emitted
    QQuickTest::pointerRelease(device, &window, 0, p);
    QCOMPARE(clickedSpy.size(), 0);
}

// QTBUG-17361
void tst_qquickflickable::nestedPressDelay()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedPressDelay.qml")));

    QQuickFlickable *outer = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(outer != nullptr);

    QQuickFlickable *inner = window.rootObject()->findChild<QQuickFlickable*>("innerFlickable");
    QVERIFY(inner != nullptr);

    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(150, 150));
    // the MouseArea is not pressed immediately
    QVERIFY(!outer->property("pressed").toBool());
    QVERIFY(!inner->property("pressed").toBool());

    // The inner pressDelay will prevail (50ms, vs. 10sec)
    // QTRY_VERIFY() has 5sec timeout, so will timeout well within 10sec.
    QTRY_VERIFY(outer->property("pressed").toBool());

    QTest::mouseMove(&window, QPoint(130, 150));
    QTest::mouseMove(&window, QPoint(110, 150));
    QTest::mouseMove(&window, QPoint(90, 150));

    QVERIFY(!outer->property("moving").toBool());
    QVERIFY(!outer->property("dragging").toBool());
    QVERIFY(inner->property("moving").toBool());
    QVERIFY(inner->property("dragging").toBool());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));

    QVERIFY(!inner->property("dragging").toBool());
    QTRY_VERIFY(!inner->property("moving").toBool());

    // Dragging inner Flickable should work
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(80, 150));
    // the MouseArea is not pressed immediately
    QVERIFY(!outer->property("pressed").toBool());
    QVERIFY(!inner->property("pressed").toBool());

    QTest::mouseMove(&window, QPoint(60, 150));
    QTest::mouseMove(&window, QPoint(40, 150));
    QTest::mouseMove(&window, QPoint(20, 150));

    QVERIFY(inner->property("moving").toBool());
    QVERIFY(inner->property("dragging").toBool());
    QVERIFY(!outer->property("moving").toBool());
    QVERIFY(!outer->property("dragging").toBool());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(20, 150));

    QVERIFY(!inner->property("dragging").toBool());
    QTRY_VERIFY(!inner->property("moving").toBool());

    // Dragging the MouseArea in the inner Flickable should move the inner Flickable
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(150, 150));
    // the MouseArea is not pressed immediately
    QVERIFY(!outer->property("pressed").toBool());

    QTest::mouseMove(&window, QPoint(130, 150));
    QTest::mouseMove(&window, QPoint(110, 150));
    QTest::mouseMove(&window, QPoint(90, 150));

    QVERIFY(!outer->property("moving").toBool());
    QVERIFY(!outer->property("dragging").toBool());
    QVERIFY(inner->property("moving").toBool());
    QVERIFY(inner->property("dragging").toBool());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(90, 150));

    QVERIFY(!inner->property("dragging").toBool());
    QTRY_VERIFY(!inner->property("moving").toBool());
}

void tst_qquickflickable::filterReplayedPress()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedPressDelay.qml")));

    QQuickFlickable *outer = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(outer != nullptr);

    QQuickFlickable *inner = window.rootObject()->findChild<QQuickFlickable*>("innerFlickable");
    QVERIFY(inner != nullptr);

    QQuickItem *filteringMouseArea = outer->findChild<QQuickItem *>("filteringMouseArea");
    QVERIFY(filteringMouseArea);

    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(150, 150));
    // the MouseArea filtering the Flickable is pressed immediately.
    QCOMPARE(filteringMouseArea->property("pressed").toBool(), true);

    // Some event causes the mouse area to set keepMouseGrab.
    filteringMouseArea->setKeepMouseGrab(true);
    QCOMPARE(filteringMouseArea->keepMouseGrab(), true);

    // The inner pressDelay will prevail (50ms, vs. 10sec)
    // QTRY_VERIFY() has 5sec timeout, so will timeout well within 10sec.
    QTRY_VERIFY(outer->property("pressed").toBool());

    // The replayed press event isn't delivered to parent items of the
    // flickable with the press delay, and the state of the parent mouse
    // area is therefore unaffected.
    QCOMPARE(filteringMouseArea->property("pressed").toBool(), true);
    QCOMPARE(filteringMouseArea->keepMouseGrab(), true);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));
}


// QTBUG-37316
void tst_qquickflickable::nestedClickThenFlick()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedClickThenFlick.qml")));

    QQuickFlickable *outer = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(outer != nullptr);

    QQuickFlickable *inner = window.rootObject()->findChild<QQuickFlickable*>("innerFlickable");
    QVERIFY(inner != nullptr);

    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(150, 150));

    // the MouseArea is not pressed immediately
    QVERIFY(!outer->property("pressed").toBool());
    QTRY_VERIFY(outer->property("pressed").toBool());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));

    QVERIFY(!outer->property("pressed").toBool());

    // Dragging inner Flickable should work
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(80, 150));
    // the MouseArea is not pressed immediately

    QVERIFY(!outer->property("pressed").toBool());

    QTest::mouseMove(&window, QPoint(80, 148));
    QTest::mouseMove(&window, QPoint(80, 140));
    QTest::mouseMove(&window, QPoint(80, 120));
    QTest::mouseMove(&window, QPoint(80, 100));

    QVERIFY(!outer->property("moving").toBool());
    QVERIFY(inner->property("moving").toBool());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(80, 100));
}

void tst_qquickflickable::flickableDirection()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { flickableDirection: Flickable.VerticalFlick; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(flickableDirectionChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::VerticalFlick);

    flickable->setFlickableDirection(QQuickFlickable::HorizontalAndVerticalFlick);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::HorizontalAndVerticalFlick);
    QCOMPARE(spy.size(),1);

    flickable->setFlickableDirection(QQuickFlickable::AutoFlickDirection);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::AutoFlickDirection);
    QCOMPARE(spy.size(),2);

    flickable->setFlickableDirection(QQuickFlickable::HorizontalFlick);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::HorizontalFlick);
    QCOMPARE(spy.size(),3);

    flickable->setFlickableDirection(QQuickFlickable::HorizontalFlick);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::HorizontalFlick);
    QCOMPARE(spy.size(),3);

    delete flickable;
}

// QtQuick 1.1
void tst_qquickflickable::resizeContent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("resize.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(c.createWithInitialProperties({{"setRebound", false}}));
    QQuickFlickable *obj = findItem<QQuickFlickable>(root, "flick");

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);
    QCOMPARE(obj->contentWidth(), 300.);
    QCOMPARE(obj->contentHeight(), 300.);

    QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(obj);
    QSizeChangeListener sizeListener(fp->contentItem);

    QMetaObject::invokeMethod(root, "resizeContent");
    for (const QSize &sizeOnGeometryChanged : sizeListener) {
        // Check that we have the correct size on all signals
        QCOMPARE(sizeOnGeometryChanged, QSize(600, 600));
    }

    QCOMPARE(obj->contentX(), 100.);
    QCOMPARE(obj->contentY(), 100.);
    QCOMPARE(obj->contentWidth(), 600.);
    QCOMPARE(obj->contentHeight(), 600.);

    delete root;
}

void tst_qquickflickable::returnToBounds()
{
    QFETCH(bool, setRebound);

    QScopedPointer<QQuickView> window(new QQuickView);

    window->setInitialProperties({{"setRebound", setRebound}});
    window->setSource(testFileUrl("resize.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
    QQuickFlickable *obj = findItem<QQuickFlickable>(window->rootObject(), "flick");

    QQuickTransition *rebound = window->rootObject()->findChild<QQuickTransition*>("rebound");
    QVERIFY(rebound);
    QSignalSpy reboundSpy(rebound, SIGNAL(runningChanged()));

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);
    QCOMPARE(obj->contentWidth(), 300.);
    QCOMPARE(obj->contentHeight(), 300.);

    obj->setContentX(100);
    obj->setContentY(400);
    QTRY_COMPARE(obj->contentX(), 100.);
    QTRY_COMPARE(obj->contentY(), 400.);

    QMetaObject::invokeMethod(window->rootObject(), "returnToBounds");

    if (setRebound)
        QTRY_VERIFY(rebound->running());

    QTRY_COMPARE(obj->contentX(), 0.);
    QTRY_COMPARE(obj->contentY(), 0.);

    QVERIFY(!rebound->running());
    QCOMPARE(reboundSpy.size(), setRebound ? 2 : 0);
}

void tst_qquickflickable::returnToBounds_data()
{
    QTest::addColumn<bool>("setRebound");

    QTest::newRow("with bounds transition") << true;
    QTest::newRow("with bounds transition") << false;
}

void tst_qquickflickable::wheel()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("wheel.qml")));

    QQuickFlickable *flick = window.rootObject()->findChild<QQuickFlickable*>("flick");
    QVERIFY(flick != nullptr);
    QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(flick);
    QSignalSpy moveEndSpy(flick, SIGNAL(movementEnded()));
    quint64 timestamp = 10;

    // test a vertical flick
    {
        QPoint pos(200, 200);
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(), QPoint(0,-120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp);
        QGuiApplication::sendEvent(&window, &event);
    }

    QTRY_VERIFY(flick->contentY() > 0);
    QCOMPARE(flick->contentX(), qreal(0));

    QTRY_COMPARE(moveEndSpy.size(), 1);
    QCOMPARE(fp->velocityTimeline.isActive(), false);
    QCOMPARE(fp->timeline.isActive(), false);
    QTest::qWait(50); // make sure that onContentYChanged won't sneak in again
    timestamp += 50;
    QCOMPARE(flick->property("movementsAfterEnd").value<int>(), 0); // QTBUG-55886

    // get ready to test horizontal flick
    flick->setContentY(0); // which triggers movementEnded again
    flick->setProperty("movementsAfterEnd", 0);
    flick->setProperty("ended", false);
    QCOMPARE(flick->contentY(), qreal(0));

    // test a horizontal flick
    {
        QPoint pos(200, 200);
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(), QPoint(-120,0),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp);
        QGuiApplication::sendEvent(&window, &event);
    }

    QTRY_VERIFY(flick->contentX() > 0);
    QCOMPARE(flick->contentY(), qreal(0));
    QTRY_COMPARE(moveEndSpy.size(), 2);
    QCOMPARE(fp->velocityTimeline.isActive(), false);
    QCOMPARE(fp->timeline.isActive(), false);
    QTest::qWait(50); // make sure that onContentXChanged won't sneak in again
    QCOMPARE(flick->property("movementsAfterEnd").value<int>(), 0); // QTBUG-55886
}

void tst_qquickflickable::wheelBackwards() // (QTBUG-121349)
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("wheel.qml")));
    QQuickFlickable *flick = window.rootObject()->findChild<QQuickFlickable*>("flick");
    QVERIFY(flick);
    QSignalSpy moveEndSpy(flick, SIGNAL(movementEnded()));
    quint64 timestamp = 10;
    const QPoint pos(200, 200);

    // attempting to scroll vertically "backwards" beyond extents does not initiate overshoot
    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }
    QCOMPARE(flick->contentY(), qreal(0));
    QCOMPARE(flick->isMoving(), false);
    QCOMPARE(moveEndSpy.size(), 0);

    // get ready to test horizontal wheel
    flick->setContentY(0); // which triggers movementEnded again
    flick->setProperty("movementsAfterEnd", 0);
    flick->setProperty("ended", false);
    QCOMPARE(flick->contentY(), qreal(0));

    // attempting to scroll horizontally "backwards" beyond extents does not initiate overshoot
    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(), QPoint(120, 0),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp);
        QGuiApplication::sendEvent(&window, &event);
    }
    QCOMPARE(flick->contentX(), qreal(0));
    QCOMPARE(flick->isMoving(), false);
    QCOMPARE(moveEndSpy.size(), 0);
}

void tst_qquickflickable::trackpad()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("wheel.qml")));

    QQuickFlickable *flick = window.rootObject()->findChild<QQuickFlickable*>("flick");
    QVERIFY(flick != nullptr);
    QSignalSpy moveEndSpy(flick, SIGNAL(movementEnded()));
    QPoint pos(200, 200);
    quint64 timestamp = 10;

    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(0,-100), QPoint(0,-120),
                          Qt::NoButton, Qt::NoModifier, Qt::ScrollBegin, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }

    QTRY_VERIFY(flick->contentY() > 0);
    QCOMPARE(flick->contentX(), qreal(0));

    flick->setContentY(0);
    QCOMPARE(flick->contentY(), qreal(0));

    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(-100,0), QPoint(-120,0),
                          Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }

    QTRY_VERIFY(flick->contentX() > 0);
    QCOMPARE(flick->contentY(), qreal(0));

    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(0,0), QPoint(0,0),
                          Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }

    QTRY_COMPARE(moveEndSpy.size(), 1); // QTBUG-55871
    QCOMPARE(flick->property("movementsAfterEnd").value<int>(), 0); // QTBUG-55886
}

void tst_qquickflickable::nestedTrackpad()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nested.qml")));

    QQuickFlickable *rootFlickable = qmlobject_cast<QQuickFlickable *>(window.rootObject());
    QVERIFY(rootFlickable);
    QQuickFlickable *innerFlickable = rootFlickable->findChild<QQuickFlickable*>();
    QVERIFY(innerFlickable);
    QSignalSpy outerMoveEndSpy(rootFlickable, SIGNAL(movementEnded()));
    QSignalSpy innerMoveEndSpy(innerFlickable, SIGNAL(movementEnded()));
    QPoint pos = innerFlickable->mapToScene(QPoint(50, 50)).toPoint();
    quint64 timestamp = 10;

    // Scroll horizontally
    for (int i = 0; i < 10 && qFuzzyIsNull(innerFlickable->contentX()); ++i) {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(-10,4), QPoint(-20,8),
                          Qt::NoButton, Qt::NoModifier, i ? Qt::ScrollUpdate : Qt::ScrollBegin, false,
                          Qt::MouseEventSynthesizedBySystem, touchpad.get());
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }
    QVERIFY(innerFlickable->contentX() > 0);
    QCOMPARE(innerFlickable->contentY(), qreal(0));
    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(0,0), QPoint(0,0),
                          Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false,
                          Qt::MouseEventSynthesizedBySystem, touchpad.get());
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }
    QTRY_COMPARE(innerMoveEndSpy.size(), 1);

    innerFlickable->setContentX(0);
    QCOMPARE(innerFlickable->contentX(), qreal(0));

    // Scroll vertically
    for (int i = 0; i < 10 && qFuzzyIsNull(innerFlickable->contentY()); ++i) {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(4,-10), QPoint(8,-20),
                          Qt::NoButton, Qt::NoModifier, i ? Qt::ScrollUpdate : Qt::ScrollBegin, false,
                          Qt::MouseEventSynthesizedBySystem, touchpad.get());
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }
    QVERIFY(rootFlickable->contentY() > 0);
    QCOMPARE(rootFlickable->contentX(), qreal(0));
    {
        QWheelEvent event(pos, window.mapToGlobal(pos), QPoint(0,0), QPoint(0,0),
                          Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false,
                          Qt::MouseEventSynthesizedBySystem, touchpad.get());
        event.setAccepted(false);
        event.setTimestamp(timestamp++);
        QGuiApplication::sendEvent(&window, &event);
    }
    QTRY_COMPARE(outerMoveEndSpy.size(), 1);
}

void tst_qquickflickable::nestedSameDirectionTrackpad() // QTBUG-124478
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedSameDirection.qml")));
    QQuickFlickable *rootFlickable = qmlobject_cast<QQuickFlickable *>(window.rootObject());
    QVERIFY(rootFlickable);
    QQuickFlickable *flickable1 = rootFlickable->findChild<QQuickFlickable *>("nested1");
    QVERIFY(flickable1);
    QQuickFlickable *flickable2 = rootFlickable->findChild<QQuickFlickable *>("nested2");
    QVERIFY(flickable2);

    QVERIFY(rootFlickable->isAtYBeginning());
    const QPoint pixelDelta(0, -100);
    const QPointF pos(50, 50);
    const int interval = 20;
    quint64 timestamp = 1000;

    auto sendTrackpadScroll = [pixelDelta, pos, &timestamp, &window, this](Qt::ScrollPhase phase) {
        QWheelEvent ev(pos, window.mapToGlobal(pos), pixelDelta, pixelDelta, Qt::NoButton, Qt::NoModifier,
                       phase, false, Qt::MouseEventSynthesizedBySystem, touchpad.get());
        ev.setTimestamp(timestamp);
        QGuiApplication::sendEvent(&window, &ev);
        timestamp += interval; // for next time
    };

    // Send enough pixelDelta wheel events for flickable1 to reach its end.
    sendTrackpadScroll(Qt::ScrollBegin);

    for (int i = 1; i < 50; ++i) {
        if (lcTests().isDebugEnabled())
            QTest::qWait(interval);
        sendTrackpadScroll(Qt::ScrollUpdate);
    }

    // We expect that the rootFlickable has moved, due to receiving some of the
    // wheel events that were sent after flickable1 reached its end.
    // Then flickable2 began to move as well, after it got under the mouse position.
    QTRY_VERIFY(flickable1->isAtYEnd());
    QCOMPARE(rootFlickable->isAtYBeginning(), false);
    QCOMPARE(flickable2->isAtYBeginning(), false);
}

void tst_qquickflickable::movingAndFlicking_data()
{
    const QPointingDevice *constMouse = mouseDevice.get();
    const QPointingDevice *constTouchscreen = touchscreen.get();

    QTest::addColumn<bool>("verticalEnabled");
    QTest::addColumn<bool>("horizontalEnabled");
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("testButton");
    QTest::addColumn<const QPointingDevice *>("device");
    QTest::addColumn<QPoint>("flickToWithoutSnapBack");
    QTest::addColumn<QPoint>("flickToWithSnapBack");

    QTest::newRow("vertical")
            << true << false << Qt::MouseButtons(Qt::LeftButton) << Qt::LeftButton << constMouse
            << QPoint(50, 100)
            << QPoint(50, 300);

    QTest::newRow("horizontal")
            << false << true << Qt::MouseButtons(Qt::LeftButton) << Qt::LeftButton << constMouse
            << QPoint(-50, 200)
            << QPoint(150, 200);

    QTest::newRow("both")
            << true << true << Qt::MouseButtons(Qt::LeftButton) << Qt::LeftButton << constMouse
            << QPoint(-50, 100)
            << QPoint(150, 300);

    QTest::newRow("mouse disabled")
            << true << true << Qt::MouseButtons(Qt::NoButton) << Qt::LeftButton << constMouse
            << QPoint(-50, 100)
            << QPoint(150, 300);

    QTest::newRow("wrong button")
            << true << true << Qt::MouseButtons(Qt::RightButton) << Qt::LeftButton << constMouse
            << QPoint(-50, 100)
            << QPoint(150, 300);

    QTest::newRow("right button")
            << true << true << Qt::MouseButtons(Qt::RightButton) << Qt::RightButton << constMouse
            << QPoint(-50, 100)
            << QPoint(150, 300);

    QTest::newRow("touch")
            << true << true << Qt::MouseButtons(Qt::NoButton) << Qt::LeftButton << constTouchscreen
            << QPoint(-50, 100)
            << QPoint(150, 300);
}

void tst_qquickflickable::movingAndFlicking()
{
    QFETCH(bool, verticalEnabled);
    QFETCH(bool, horizontalEnabled);
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, testButton);
    QFETCH(QPoint, flickToWithoutSnapBack);
    QFETCH(QPoint, flickToWithSnapBack);
    QFETCH(const QPointingDevice *, device);

    const QPoint flickFrom(50, 200);   // centre

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);
    flickable->setAcceptedButtons(acceptedButtons);

    QSignalSpy vMoveSpy(flickable, SIGNAL(movingVerticallyChanged()));
    QSignalSpy hMoveSpy(flickable, SIGNAL(movingHorizontallyChanged()));
    QSignalSpy moveSpy(flickable, SIGNAL(movingChanged()));
    QSignalSpy vFlickSpy(flickable, SIGNAL(flickingVerticallyChanged()));
    QSignalSpy hFlickSpy(flickable, SIGNAL(flickingHorizontallyChanged()));
    QSignalSpy flickSpy(flickable, SIGNAL(flickingChanged()));

    QSignalSpy moveStartSpy(flickable, SIGNAL(movementStarted()));
    QSignalSpy moveEndSpy(flickable, SIGNAL(movementEnded()));
    QSignalSpy flickStartSpy(flickable, SIGNAL(flickStarted()));
    QSignalSpy flickEndSpy(flickable, SIGNAL(flickEnded()));

    // do a flick that keeps the view within the bounds
    QQuickTest::pointerFlick(device, &window, 0, flickFrom, flickToWithoutSnapBack, 200, testButton);

    if (!(acceptedButtons & testButton) && device->type() != QInputDevice::DeviceType::TouchScreen) {
        QVERIFY(!flickable->isMoving());
        return;
    }

    QTRY_VERIFY(flickable->isMoving());
    QCOMPARE(flickable->isMovingHorizontally(), horizontalEnabled);
    QCOMPARE(flickable->isMovingVertically(), verticalEnabled);
    QVERIFY(flickable->isFlicking());
    QCOMPARE(flickable->isFlickingHorizontally(), horizontalEnabled);
    QCOMPARE(flickable->isFlickingVertically(), verticalEnabled);
    // contentX/contentY are either unchanged, or moving is true when the value changed.
    QCOMPARE(flickable->property("movingInContentX").value<bool>(), true);
    QCOMPARE(flickable->property("movingInContentY").value<bool>(), true);

    QCOMPARE(moveSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), verticalEnabled ? 1 : 0);
    QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 1 : 0);
    QCOMPARE(flickSpy.size(), 1);
    QCOMPARE(vFlickSpy.size(), verticalEnabled ? 1 : 0);
    QCOMPARE(hFlickSpy.size(), horizontalEnabled ? 1 : 0);

    QCOMPARE(moveStartSpy.size(), 1);
    QCOMPARE(flickStartSpy.size(), 1);

    // wait for any motion to end
    QTRY_VERIFY(!flickable->isMoving());

    QVERIFY(!flickable->isMovingHorizontally());
    QVERIFY(!flickable->isMovingVertically());
    QVERIFY(!flickable->isFlicking());
    QVERIFY(!flickable->isFlickingHorizontally());
    QVERIFY(!flickable->isFlickingVertically());

    QCOMPARE(moveSpy.size(), 2);
    QCOMPARE(vMoveSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 2 : 0);
    QCOMPARE(flickSpy.size(), 2);
    QCOMPARE(vFlickSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hFlickSpy.size(), horizontalEnabled ? 2 : 0);

    QCOMPARE(moveStartSpy.size(), 1);
    QCOMPARE(moveEndSpy.size(), 1);
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 1);

    // Stop on a full pixel after user interaction
    if (verticalEnabled)
        QCOMPARE(flickable->contentY(), (qreal)qRound(flickable->contentY()));
    if (horizontalEnabled)
        QCOMPARE(flickable->contentX(), (qreal)qRound(flickable->contentX()));

    // clear for next flick
    vMoveSpy.clear(); hMoveSpy.clear(); moveSpy.clear();
    vFlickSpy.clear(); hFlickSpy.clear(); flickSpy.clear();
    moveStartSpy.clear(); moveEndSpy.clear();
    flickStartSpy.clear(); flickEndSpy.clear();

    // do a flick that flicks the view out of bounds
    flickable->setContentX(0);
    flickable->setContentY(0);
    QTRY_VERIFY(!flickable->isMoving());
    QQuickTest::pointerFlick(device, &window, 0, flickFrom, flickToWithSnapBack, 10, testButton);

    QTRY_VERIFY(flickable->isMoving());
    QCOMPARE(flickable->isMovingHorizontally(), horizontalEnabled);
    QCOMPARE(flickable->isMovingVertically(), verticalEnabled);
    QVERIFY(flickable->isFlicking());
    QCOMPARE(flickable->isFlickingHorizontally(), horizontalEnabled);
    QCOMPARE(flickable->isFlickingVertically(), verticalEnabled);

    QCOMPARE(moveSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), verticalEnabled ? 1 : 0);
    QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 1 : 0);
    QCOMPARE(flickSpy.size(), 1);
    QCOMPARE(vFlickSpy.size(), verticalEnabled ? 1 : 0);
    QCOMPARE(hFlickSpy.size(), horizontalEnabled ? 1 : 0);

    QCOMPARE(moveStartSpy.size(), 1);
    QCOMPARE(moveEndSpy.size(), 0);
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 0);

    // wait for any motion to end
    QTRY_VERIFY(!flickable->isMoving());

    QVERIFY(!flickable->isMovingHorizontally());
    QVERIFY(!flickable->isMovingVertically());
    QVERIFY(!flickable->isFlicking());
    QVERIFY(!flickable->isFlickingHorizontally());
    QVERIFY(!flickable->isFlickingVertically());

    QCOMPARE(moveSpy.size(), 2);
    QCOMPARE(vMoveSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 2 : 0);
    QCOMPARE(flickSpy.size(), 2);
    QCOMPARE(vFlickSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hFlickSpy.size(), horizontalEnabled ? 2 : 0);

    QCOMPARE(moveStartSpy.size(), 1);
    QCOMPARE(moveEndSpy.size(), 1);
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 1);

    QCOMPARE(flickable->contentX(), 0.0);
    QCOMPARE(flickable->contentY(), 0.0);
}


void tst_qquickflickable::movingAndDragging_data()
{
    QTest::addColumn<bool>("verticalEnabled");
    QTest::addColumn<bool>("horizontalEnabled");
    QTest::addColumn<QPoint>("moveByWithoutSnapBack");
    QTest::addColumn<QPoint>("moveByWithSnapBack");

    QTest::newRow("vertical")
            << true << false
            << QPoint(0, -10)
            << QPoint(0, 20);

    QTest::newRow("horizontal")
            << false << true
            << QPoint(-10, 0)
            << QPoint(20, 0);

    QTest::newRow("both")
            << true << true
            << QPoint(-10, -10)
            << QPoint(20, 20);
}

void tst_qquickflickable::movingAndDragging()
{
    QFETCH(bool, verticalEnabled);
    QFETCH(bool, horizontalEnabled);
    QFETCH(QPoint, moveByWithoutSnapBack);
    QFETCH(QPoint, moveByWithSnapBack);
    const auto device = mouseDevice.get();

    const QPoint moveFrom(50, 200);   // centre

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QSignalSpy vDragSpy(flickable, SIGNAL(draggingVerticallyChanged()));
    QSignalSpy hDragSpy(flickable, SIGNAL(draggingHorizontallyChanged()));
    QSignalSpy dragSpy(flickable, SIGNAL(draggingChanged()));
    QSignalSpy vMoveSpy(flickable, SIGNAL(movingVerticallyChanged()));
    QSignalSpy hMoveSpy(flickable, SIGNAL(movingHorizontallyChanged()));
    QSignalSpy moveSpy(flickable, SIGNAL(movingChanged()));

    QSignalSpy dragStartSpy(flickable, SIGNAL(dragStarted()));
    QSignalSpy dragEndSpy(flickable, SIGNAL(dragEnded()));
    QSignalSpy moveStartSpy(flickable, SIGNAL(movementStarted()));
    QSignalSpy moveEndSpy(flickable, SIGNAL(movementEnded()));

    // start the drag
    QQuickTest::pointerMoveAndPress(device, &window, 0, moveFrom);
    QTest::mouseMove(&window, moveFrom + moveByWithoutSnapBack);
    QTest::mouseMove(&window, moveFrom + moveByWithoutSnapBack*2);
    QTest::mouseMove(&window, moveFrom + moveByWithoutSnapBack*3);

    QTRY_VERIFY(flickable->isMoving());
    QCOMPARE(flickable->isMovingHorizontally(), horizontalEnabled);
    QCOMPARE(flickable->isMovingVertically(), verticalEnabled);
    QVERIFY(flickable->isDragging());
    QCOMPARE(flickable->isDraggingHorizontally(), horizontalEnabled);
    QCOMPARE(flickable->isDraggingVertically(), verticalEnabled);
    // contentX/contentY are either unchanged, or moving and dragging are true when the value changes.
    QCOMPARE(flickable->property("movingInContentX").value<bool>(), true);
    QCOMPARE(flickable->property("movingInContentY").value<bool>(), true);
    QCOMPARE(flickable->property("draggingInContentX").value<bool>(), true);
    QCOMPARE(flickable->property("draggingInContentY").value<bool>(), true);

    QCOMPARE(moveSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), verticalEnabled ? 1 : 0);
    QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 1 : 0);
    QCOMPARE(dragSpy.size(), 1);
    QCOMPARE(vDragSpy.size(), verticalEnabled ? 1 : 0);
    QCOMPARE(hDragSpy.size(), horizontalEnabled ? 1 : 0);

    QCOMPARE(moveStartSpy.size(), 1);
    QCOMPARE(dragStartSpy.size(), 1);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, moveFrom + moveByWithoutSnapBack*3);

    QVERIFY(!flickable->isDragging());
    QVERIFY(!flickable->isDraggingHorizontally());
    QVERIFY(!flickable->isDraggingVertically());
    QCOMPARE(dragSpy.size(), 2);
    QCOMPARE(vDragSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hDragSpy.size(), horizontalEnabled ? 2 : 0);
    QCOMPARE(dragStartSpy.size(), 1);
    QCOMPARE(dragEndSpy.size(), 1);
    // Don't test whether moving finished because a flick could occur

    // wait for any motion to end
    QTRY_VERIFY(!flickable->isMoving());

    QVERIFY(!flickable->isMovingHorizontally());
    QVERIFY(!flickable->isMovingVertically());
    QVERIFY(!flickable->isDragging());
    QVERIFY(!flickable->isDraggingHorizontally());
    QVERIFY(!flickable->isDraggingVertically());

    QCOMPARE(dragSpy.size(), 2);
    QCOMPARE(vDragSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hDragSpy.size(), horizontalEnabled ? 2 : 0);
    QCOMPARE(moveSpy.size(), 2);
    QCOMPARE(vMoveSpy.size(), verticalEnabled ? 2 : 0);
    QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 2 : 0);

    QCOMPARE(dragStartSpy.size(), 1);
    QCOMPARE(dragEndSpy.size(), 1);
    QCOMPARE(moveStartSpy.size(), 1);
    QCOMPARE(moveEndSpy.size(), 1);

    // Stop on a full pixel after user interaction
    if (verticalEnabled)
        QCOMPARE(flickable->contentY(), (qreal)qRound(flickable->contentY()));
    if (horizontalEnabled)
        QCOMPARE(flickable->contentX(), (qreal)qRound(flickable->contentX()));

    // clear for next drag
     vMoveSpy.clear(); hMoveSpy.clear(); moveSpy.clear();
     vDragSpy.clear(); hDragSpy.clear(); dragSpy.clear();
     moveStartSpy.clear(); moveEndSpy.clear();
     dragStartSpy.clear(); dragEndSpy.clear();

     // do a drag that drags the view out of bounds
     flickable->setContentX(0);
     flickable->setContentY(0);
     QTRY_VERIFY(!flickable->isMoving());
     QQuickTest::pointerMoveAndPress(device, &window, 0, moveFrom);
     QTest::mouseMove(&window, moveFrom + moveByWithSnapBack);
     QTest::mouseMove(&window, moveFrom + moveByWithSnapBack*2);
     QTest::mouseMove(&window, moveFrom + moveByWithSnapBack*3);

     QVERIFY(flickable->isMoving());
     QCOMPARE(flickable->isMovingHorizontally(), horizontalEnabled);
     QCOMPARE(flickable->isMovingVertically(), verticalEnabled);
     QVERIFY(flickable->isDragging());
     QCOMPARE(flickable->isDraggingHorizontally(), horizontalEnabled);
     QCOMPARE(flickable->isDraggingVertically(), verticalEnabled);

     QCOMPARE(moveSpy.size(), 1);
     QCOMPARE(vMoveSpy.size(), verticalEnabled ? 1 : 0);
     QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 1 : 0);
     QCOMPARE(dragSpy.size(), 1);
     QCOMPARE(vDragSpy.size(), verticalEnabled ? 1 : 0);
     QCOMPARE(hDragSpy.size(), horizontalEnabled ? 1 : 0);

     QCOMPARE(moveStartSpy.size(), 1);
     QCOMPARE(moveEndSpy.size(), 0);
     QCOMPARE(dragStartSpy.size(), 1);
     QCOMPARE(dragEndSpy.size(), 0);

     QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, moveFrom + moveByWithSnapBack*3);

     // should now start snapping back to bounds (moving but not dragging)
     QVERIFY(flickable->isMoving());
     QCOMPARE(flickable->isMovingHorizontally(), horizontalEnabled);
     QCOMPARE(flickable->isMovingVertically(), verticalEnabled);
     QVERIFY(!flickable->isDragging());
     QVERIFY(!flickable->isDraggingHorizontally());
     QVERIFY(!flickable->isDraggingVertically());

     QCOMPARE(moveSpy.size(), 1);
     QCOMPARE(vMoveSpy.size(), verticalEnabled ? 1 : 0);
     QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 1 : 0);
     QCOMPARE(dragSpy.size(), 2);
     QCOMPARE(vDragSpy.size(), verticalEnabled ? 2 : 0);
     QCOMPARE(hDragSpy.size(), horizontalEnabled ? 2 : 0);

     QCOMPARE(moveStartSpy.size(), 1);
     QCOMPARE(moveEndSpy.size(), 0);

     // wait for any motion to end
     QTRY_VERIFY(!flickable->isMoving());

     QVERIFY(!flickable->isMovingHorizontally());
     QVERIFY(!flickable->isMovingVertically());
     QVERIFY(!flickable->isDragging());
     QVERIFY(!flickable->isDraggingHorizontally());
     QVERIFY(!flickable->isDraggingVertically());

     QCOMPARE(moveSpy.size(), 2);
     QCOMPARE(vMoveSpy.size(), verticalEnabled ? 2 : 0);
     QCOMPARE(hMoveSpy.size(), horizontalEnabled ? 2 : 0);
     QCOMPARE(dragSpy.size(), 2);
     QCOMPARE(vDragSpy.size(), verticalEnabled ? 2 : 0);
     QCOMPARE(hDragSpy.size(), horizontalEnabled ? 2 : 0);

     QCOMPARE(moveStartSpy.size(), 1);
     QCOMPARE(moveEndSpy.size(), 1);
     QCOMPARE(dragStartSpy.size(), 1);
     QCOMPARE(dragEndSpy.size(), 1);

     QCOMPARE(flickable->contentX(), 0.0);
     QCOMPARE(flickable->contentY(), 0.0);
}

void tst_qquickflickable::dragAndSwitchDirection_data()
{
    QTest::addColumn<const QPointingDevice *>("device");
    QTest::addColumn<bool>("dragH");

    const QPointingDevice *constMouse = mouseDevice.get();
    const QPointingDevice *constTouchscreen = touchscreen.get();
    QTest::newRow("mouse, drag horizontally") << constMouse << true;
    QTest::newRow("device, drag horizontally") << constTouchscreen << true;
    QTest::newRow("mouse, drag vertically") << constMouse << false;
    QTest::newRow("device, drag vertically") << constTouchscreen << false;
}

void tst_qquickflickable::dragAndSwitchDirection()
{
    // Check that you can start a drag in one direction, but then, without
    // lifting the mouse/finger, drag in the opposite direction, passing the
    // initial starting point of the drag.
    QFETCH(const QPointingDevice *, device);
    QFETCH(const bool, dragH);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable01.qml")));
    QVERIFY(window.rootObject() != nullptr);
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    // Flick the content item at bit away from the origin, to avoid
    // overshooting it while executing the rest of the test.
    flickable->setWidth(window.width());
    flickable->setHeight(window.height());
    flickable->setContentWidth(flickable->width() * 2);
    flickable->setContentHeight(flickable->height() * 2);
    flickable->setContentX(flickable->width() / 2);
    flickable->setContentY(flickable->height() / 2);

    const QPoint initialContentPos = QPoint(flickable->contentX(), flickable->contentY());
    const QPoint dragDirection = QPoint(dragH ? 1 : 0, dragH ? 0 : 1);
    const QPoint dragIncrement = dragDirection * qApp->styleHints()->startDragDistance() / 2;
    const QPoint dragFrom = QPoint(flickable->width(), flickable->height()) / 2;
    QPoint dragTo = dragFrom;

    // Start a drag in the direction we're testing
    QVERIFY(!flickable->isDragging());
    QQuickTest::pointerMoveAndPress(device, &window, 0, dragTo);
    for (int i = 0; i <= 6; ++i) {
        dragTo += dragIncrement;
        QQuickTest::pointerMove(device, &window, 0, dragTo);
    }

    // At this point, we don't care about the exact position of the contentItem.
    // What matters is that the flickable is now in a dragging state, and that
    // the contentItem has moved a bit in the expected direction.
    QVERIFY(flickable->isDragging());
    QPoint expectedContentPos(flickable->contentX(), flickable->contentY());
    QCOMPARE_NE(initialContentPos, expectedContentPos);

    // We now reverse the drag direction, and drag beyond the initial starting
    // position. We expect the content item to move for each movement of the
    // pointer, even if the expectedContentPos lands exactly on the initialContentPos.
    // If the flickable were to only look at the delta moved since the start of the
    // drag, it could easily be fooled into beliving that the drag threshold has yet
    // to be exceeded, and therefore not move.
    for (int i = 0; i <= 6; ++i) {
        dragTo -= dragIncrement;
        expectedContentPos += dragIncrement;
        QQuickTest::pointerMove(device, &window, 0, dragTo);
        QCOMPARE(flickable->contentX(), expectedContentPos.x());
        QCOMPARE(flickable->contentY(), expectedContentPos.y());
    }

    QQuickTest::pointerRelease(device, &window, 0, dragTo);
    QVERIFY(!flickable->isDragging());
}

void tst_qquickflickable::flickOnRelease()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    // Vertical with a quick press-move-release: should cause a flick in release.
    QSignalSpy vFlickSpy(flickable, SIGNAL(flickingVerticallyChanged()));
    // Use something that generates a huge velocity just to make it testable.
    // In practice this feature matters on touchscreen devices where the
    // underlying drivers will hopefully provide a pre-calculated velocity
    // (based on more data than what the UI gets), thus making this use case
    // working even with small movements.
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(50, 300));
    QTest::mouseMove(&window, QPoint(50, 10), 10);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 10), 10);

    QCOMPARE(vFlickSpy.size(), 1);

    // wait for any motion to end
    QTRY_VERIFY(!flickable->isMoving());

    // Stop on a full pixel after user interaction
    QCOMPARE(flickable->contentY(), (qreal)qRound(flickable->contentY()));
}

void tst_qquickflickable::pressWhileFlicking()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QSignalSpy vMoveSpy(flickable, SIGNAL(movingVerticallyChanged()));
    QSignalSpy hMoveSpy(flickable, SIGNAL(movingHorizontallyChanged()));
    QSignalSpy moveSpy(flickable, SIGNAL(movingChanged()));
    QSignalSpy hFlickSpy(flickable, SIGNAL(flickingHorizontallyChanged()));
    QSignalSpy vFlickSpy(flickable, SIGNAL(flickingVerticallyChanged()));
    QSignalSpy flickSpy(flickable, SIGNAL(flickingChanged()));
    QSignalSpy flickStartSpy(flickable, &QQuickFlickable::flickStarted);
    QSignalSpy flickEndSpy(flickable, &QQuickFlickable::flickEnded);

    // flick then press while it is still moving
    // flicking == false, moving == true;
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,190), QPoint(20, 50), 200);
    QVERIFY(flickable->verticalVelocity() > 0.0);
    QTRY_VERIFY(flickable->isFlicking());
    QVERIFY(flickable->isFlickingVertically());
    QVERIFY(!flickable->isFlickingHorizontally());
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isMovingVertically());
    QVERIFY(!flickable->isMovingHorizontally());
    QCOMPARE(vMoveSpy.size(), 1);
    QCOMPARE(hMoveSpy.size(), 0);
    QCOMPARE(moveSpy.size(), 1);
    QCOMPARE(vFlickSpy.size(), 1);
    QCOMPARE(hFlickSpy.size(), 0);
    QCOMPARE(flickSpy.size(), 1);
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 0);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(20, 50));
    QTRY_VERIFY(!flickable->isFlicking());
    QVERIFY(!flickable->isFlickingVertically());
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isMovingVertically());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(20,50));
    QVERIFY(!flickable->isFlicking());
    QVERIFY(!flickable->isFlickingVertically());
    QTRY_VERIFY(!flickable->isMoving());
    QVERIFY(!flickable->isMovingVertically());
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 1);
    // Stop on a full pixel after user interaction
    QCOMPARE(flickable->contentX(), (qreal)qRound(flickable->contentX()));
}

void tst_qquickflickable::dragWhileFlicking()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QSignalSpy vMoveSpy(flickable, &QQuickFlickable::movingVerticallyChanged);
    QSignalSpy hMoveSpy(flickable, &QQuickFlickable::movingHorizontallyChanged);
    QSignalSpy moveSpy(flickable, &QQuickFlickable::movingChanged);
    QSignalSpy hFlickSpy(flickable, &QQuickFlickable::flickingHorizontallyChanged);
    QSignalSpy vFlickSpy(flickable, &QQuickFlickable::flickingVerticallyChanged);
    QSignalSpy flickSpy(flickable, &QQuickFlickable::flickingChanged);
    QSignalSpy flickStartSpy(flickable, &QQuickFlickable::flickStarted);
    QSignalSpy flickEndSpy(flickable, &QQuickFlickable::flickEnded);

    // flick first, let it keep moving
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,190), QPoint(20, 50), 200);
    QVERIFY(flickable->verticalVelocity() > 0.0);
    QTRY_VERIFY(flickable->isFlicking());
    QVERIFY(flickable->isFlickingVertically());
    QCOMPARE(flickable->isFlickingHorizontally(), false);
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isMovingVertically());
    QCOMPARE(flickable->isMovingHorizontally(), false);
    QCOMPARE(vMoveSpy.size(), 1);
    QCOMPARE(hMoveSpy.size(), 0);
    QCOMPARE(moveSpy.size(), 1);
    QCOMPARE(vFlickSpy.size(), 1);
    QCOMPARE(hFlickSpy.size(), 0);
    QCOMPARE(flickSpy.size(), 1);
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 0);

    // then drag slowly while it's still flicking and moving
    const int dragStepDelay = 100;
    QQuickTest::pointerPress(device, &window, 0, QPoint(20, 70), Qt::LeftButton, Qt::NoModifier, 500);
    QTRY_COMPARE(flickable->isFlicking(), false);
    QCOMPARE(flickable->isFlickingVertically(), false);
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isMovingVertically());

    for (int y = 70; y > 50; y -= 5) {
        QQuickTest::pointerMove(device, &window, 0, QPoint(20, y), dragStepDelay);
        QVERIFY(flickable->isMoving());
        QVERIFY(flickable->isMovingVertically());
        // Flickable's timeline is real-time, so spoofing timestamps isn't enough
        QTest::qWait(dragStepDelay);
    }

    QQuickTest::pointerRelease(device, &window, 0, QPoint(20, 50), Qt::LeftButton, Qt::NoModifier, dragStepDelay);

    QCOMPARE(flickable->isFlicking(), false);
    QCOMPARE(flickable->isFlickingVertically(), false);
    QTRY_COMPARE(flickable->isMoving(), false);
    QCOMPARE(flickable->isMovingVertically(), false);
    QCOMPARE(flickStartSpy.size(), 1);
    QCOMPARE(flickEndSpy.size(), 1);
    QCOMPARE(vMoveSpy.size(), 2);
    QCOMPARE(hMoveSpy.size(), 0);
    QCOMPARE(moveSpy.size(), 2);
    QCOMPARE(vFlickSpy.size(), 2);
    QCOMPARE(hFlickSpy.size(), 0);
    // Stop on a full pixel after user interaction
    QCOMPARE(flickable->contentX(), (qreal)qRound(flickable->contentX()));
}

void tst_qquickflickable::disabled()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("disabled.qml")));

    QQuickFlickable *flick = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flick != nullptr);

    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(50, 90));

    QTest::mouseMove(&window, QPoint(50, 80));
    QTest::mouseMove(&window, QPoint(50, 70));
    QTest::mouseMove(&window, QPoint(50, 60));

    QVERIFY(!flick->isMoving());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 60));

    // verify that mouse clicks on other elements still work (QTBUG-20584)
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(50, 10));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 10));

    QTRY_VERIFY(window.rootObject()->property("clicked").toBool());
}

void tst_qquickflickable::flickVelocity()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    // flick up
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,190), QPoint(20, 50), 200);
    QVERIFY(flickable->verticalVelocity() > 0.0);
    QTRY_COMPARE(flickable->verticalVelocity(), 0.0);

    // flick down
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,10), QPoint(20, 140), 200);
    QTRY_VERIFY(flickable->verticalVelocity() < 0.0);
    QTRY_COMPARE(flickable->verticalVelocity(), 0.0);

#ifdef Q_OS_MACOS
    QSKIP("boost doesn't work on OS X");
    return;
#endif

    // Flick multiple times and verify that flick acceleration is applied.
    QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(flickable);
    bool boosted = false;
    for (int i = 0; i < 6; ++i) {
        QQuickTest::pointerFlick(device, &window, 0, QPoint(20,390), QPoint(20, 50), 100);
        boosted |= fp->flickBoost > 1.0;
    }
    QVERIFY(boosted);

           // Flick in opposite direction -> boost cancelled.
    QQuickTest::pointerFlick(device, &window, 0, QPoint(20,10), QPoint(20, 340), 200);
    QTRY_VERIFY(flickable->verticalVelocity() < 0.0);
    QCOMPARE(fp->flickBoost, 1.0);
}

void tst_qquickflickable::margins()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("margins.qml")));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(obj != nullptr);

    // starting state
    QCOMPARE(obj->contentX(), -40.);
    QCOMPARE(obj->contentY(), -20.);
    QCOMPARE(obj->contentWidth(), 1600.);
    QCOMPARE(obj->contentHeight(), 600.);
    QCOMPARE(obj->originX(), 0.);
    QCOMPARE(obj->originY(), 0.);

    // Reduce left margin
    obj->setLeftMargin(30);
    QTRY_COMPARE(obj->contentX(), -30.);

    // Reduce top margin
    obj->setTopMargin(10);
    QTRY_COMPARE(obj->contentY(), -10.);

    // position to the far right, including margin
    obj->setContentX(1600 + 50 - obj->width());
    obj->returnToBounds();
    QTRY_COMPARE(obj->contentX(), 1600. + 50. - obj->width());

    // position beyond the far right, including margin
    obj->setContentX(1600 + 50 - obj->width() + 1.);
    obj->returnToBounds();
    QTRY_COMPARE(obj->contentX(), 1600. + 50. - obj->width());

    // Reduce right margin
    obj->setRightMargin(40);
    QTRY_COMPARE(obj->contentX(), 1600. + 40. - obj->width());
    QCOMPARE(obj->contentWidth(), 1600.);

    // position to the far bottom, including margin
    obj->setContentY(600 + 30 - obj->height());
    obj->returnToBounds();
    QTRY_COMPARE(obj->contentY(), 600. + 30. - obj->height());

    // position beyond the far bottom, including margin
    obj->setContentY(600 + 30 - obj->height() + 1.);
    obj->returnToBounds();
    QTRY_COMPARE(obj->contentY(), 600. + 30. - obj->height());

    // Reduce bottom margin
    obj->setBottomMargin(20);
    QTRY_COMPARE(obj->contentY(), 600. + 20. - obj->height());
    QCOMPARE(obj->contentHeight(), 600.);
}

void tst_qquickflickable::cancelOnHide()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hide.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);

    QTest::mouseDClick(&window, Qt::LeftButton);
    QVERIFY(!flickable->isVisible());
    QVERIFY(!QQuickFlickablePrivate::get(flickable)->pressed);
}

void tst_qquickflickable::cancelOnMouseGrab()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("cancel.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(10, 10));
    // drag out of bounds
    QTest::mouseMove(&window, QPoint(50, 50));
    QTest::mouseMove(&window, QPoint(100, 100));
    QTest::mouseMove(&window, QPoint(150, 150));

    QVERIFY(flickable->contentX() != 0);
    QVERIFY(flickable->contentY() != 0);
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isDragging());

    // grabbing mouse will cancel flickable interaction.
    QQuickItem *item = window.rootObject()->findChild<QQuickItem*>("row");
    auto mouse = QPointingDevice::primaryPointingDevice();
    auto mousePriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(mouse));
    QMouseEvent fakeMouseEv(QEvent::MouseMove, QPoint(130, 100), QPoint(130, 100),
                            Qt::NoButton, Qt::LeftButton, Qt::NoModifier, mouse);
    mousePriv->setExclusiveGrabber(&fakeMouseEv, fakeMouseEv.points().first(), item);

    QTRY_COMPARE(flickable->contentX(), 0.);
    QTRY_COMPARE(flickable->contentY(), 0.);
    QTRY_VERIFY(!flickable->isMoving());
    QTRY_VERIFY(!flickable->isDragging());

    QQuickTest::pointerMoveAndRelease(device, &window, 0, QPoint(50, 10));
}

void tst_qquickflickable::clickAndDragWhenTransformed()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("transformedFlickable.qml")));

    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable != nullptr);

    // click outside child rect
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(190, 190));
    QTRY_COMPARE(flickable->property("itemPressed").toBool(), false);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(190, 190));

    // click inside child rect
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(200, 200));
    QTRY_COMPARE(flickable->property("itemPressed").toBool(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200, 200));

    // drag threshold is scaled according to the scene scaling
    const int threshold = qApp->styleHints()->startDragDistance() * flickable->parentItem()->scale();

    // drag outside bounds
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(160, 160));
    QTest::qWait(10);
    QTest::mouseMove(&window, QPoint(160 + threshold * 2, 160));
    QTest::mouseMove(&window, QPoint(160 + threshold * 3, 160));
    QCOMPARE(flickable->isDragging(), false);
    QCOMPARE(flickable->property("itemPressed").toBool(), false);
    QQuickTest::pointerMoveAndRelease(device, &window, 0, QPoint(180, 160));

    // drag inside bounds
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(200, 140));
    QCOMPARE(flickable->keepMouseGrab(), false);
    QTest::qWait(10);
    // Flickable should get interested in dragging when the drag is beyond the
    // threshold distance along the hypoteneuse of the 45° rotation
    const int deltaPastRotatedThreshold = threshold * 1.414 + 1;
    QTest::mouseMove(&window, QPoint(200 + deltaPastRotatedThreshold, 140));
    qCDebug(lcTests) << "transformed flickable dragging yet?" << flickable->isDragging() <<
            "after dragging by" << deltaPastRotatedThreshold << "past scaled threshold" << threshold;
    QCOMPARE(flickable->isDragging(), false);   // Flickable never grabs on the first drag past the threshold
    QCOMPARE(flickable->keepMouseGrab(), true); // but it plans to do it next time!
    QTest::mouseMove(&window, QPoint(200 + threshold * 2, 140));
    QCOMPARE(flickable->isDragging(), true);    // it grabs only during the second drag past the threshold
    QCOMPARE(flickable->property("itemPressed").toBool(), false);
    QQuickTest::pointerMoveAndRelease(device, &window, 0, QPoint(220, 140));
}

void tst_qquickflickable::flickTwiceUsingTouches()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("longList.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QCOMPARE(flickable->contentY(), 0.0f);

    QQuickTest::pointerFlick(touchscreen.get(), &window, 1, QPoint(100, 400), QPoint(100, 240), 100);

    qreal contentYAfterFirstFlick = flickable->contentY();
    qDebug() << "contentYAfterFirstFlick " << contentYAfterFirstFlick;
    QVERIFY(contentYAfterFirstFlick > 50.0f);
    // Wait until view stops moving
    QTRY_VERIFY(!flickable->isMoving());

    QQuickTest::pointerFlick(touchscreen.get(), &window, 1, QPoint(100, 400), QPoint(100, 240), 100);

    // In the original bug, that second flick would cause Flickable to halt immediately
    qreal contentYAfterSecondFlick = flickable->contentY();
    qDebug() << "contentYAfterSecondFlick " << contentYAfterSecondFlick;
    QTRY_VERIFY(contentYAfterSecondFlick > (contentYAfterFirstFlick + 80.0f));
}

void tst_qquickflickable::nestedStopAtBounds_data()
{
    QTest::addColumn<bool>("transpose");
    QTest::addColumn<bool>("invert");
    QTest::addColumn<int>("boundsBehavior");
    QTest::addColumn<qreal>("margin");
    QTest::addColumn<bool>("innerFiltering");
    QTest::addColumn<int>("pressDelay");
    QTest::addColumn<bool>("waitForPressDelay");

    QTest::newRow("left,stop") << false << false << int(QQuickFlickable::StopAtBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("right,stop") << false << true << int(QQuickFlickable::StopAtBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("top,stop") << true << false << int(QQuickFlickable::StopAtBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("bottom,stop") << true << true << int(QQuickFlickable::StopAtBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("left,over") << false << false << int(QQuickFlickable::DragOverBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("right,over") << false << true << int(QQuickFlickable::DragOverBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("top,over") << true << false << int(QQuickFlickable::DragOverBounds) << qreal(0) << false << 0 << false;
    QTest::newRow("bottom,over") << true << true << int(QQuickFlickable::DragOverBounds) << qreal(0) << false << 0 << false;

    QTest::newRow("left,stop,margin") << false << false << int(QQuickFlickable::StopAtBounds) << qreal(20) << false << 0 << false;
    QTest::newRow("right,stop,margin") << false << true << int(QQuickFlickable::StopAtBounds) << qreal(20) << false << 0 << false;
    QTest::newRow("top,stop,margin") << true << false << int(QQuickFlickable::StopAtBounds) << qreal(20) << false << 0 << false;
    QTest::newRow("bottom,stop,margin") << true << true << int(QQuickFlickable::StopAtBounds) << qreal(20) << false << 0 << false;

    QTest::newRow("left,stop,after press delay") << false << false << int(QQuickFlickable::StopAtBounds) << qreal(0) << true << 50 << true;
    QTest::newRow("left,stop,before press delay") << false << false << int(QQuickFlickable::StopAtBounds) << qreal(0) << true << 50 << false;
}

void tst_qquickflickable::nestedStopAtBounds()
{
    QFETCH(bool, transpose);
    QFETCH(bool, invert);
    QFETCH(int, boundsBehavior);
    QFETCH(qreal, margin);
    QFETCH(bool, innerFiltering);
    QFETCH(int, pressDelay);
    QFETCH(bool, waitForPressDelay);
    const auto device = mouseDevice.get();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedStopAtBounds.qml")));

    QQuickFlickable *outer = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(outer);

    QQuickFlickable *inner = outer->findChild<QQuickFlickable*>("innerFlickable");
    QVERIFY(inner);
    inner->setFlickableDirection(transpose ? QQuickFlickable::VerticalFlick : QQuickFlickable::HorizontalFlick);
    inner->setBoundsBehavior(QQuickFlickable::BoundsBehavior(boundsBehavior));

    invert ? inner->setRightMargin(margin) : inner->setLeftMargin(margin);
    invert ? inner->setBottomMargin(margin) : inner->setTopMargin(margin);

    inner->setContentX(invert ? -margin : 100 - margin);
    inner->setContentY(invert ? -margin : 100 - margin);
    inner->setContentWidth(400 - margin);
    inner->setContentHeight(400 - margin);

    QCOMPARE(inner->isAtXBeginning(), invert);
    QCOMPARE(inner->isAtXEnd(), !invert);
    QCOMPARE(inner->isAtYBeginning(), invert);
    QCOMPARE(inner->isAtYEnd(), !invert);

    inner->setPressDelay(pressDelay);

    QQuickMouseArea *mouseArea = inner->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea);
    mouseArea->setEnabled(innerFiltering);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 200);
    int &axis = transpose ? position.ry() : position.rx();

    // drag toward the aligned boundary.  Outer flickable dragged.
    QQuickTest::pointerMoveAndPress(device, &window, 0, position);
    if (waitForPressDelay) {
        QVERIFY(innerFiltering);    // isPressed will never be true if the mouse area isn't enabled.
        QTRY_VERIFY(mouseArea->isPressed());
    }

    axis += invert ? threshold * 2 : -threshold * 2;
    QTest::mouseMove(&window, position);
    axis += invert ? threshold : -threshold;
    QTest::mouseMove(&window, position);
    QCOMPARE(outer->isDragging(), true);
    QCOMPARE(outer->isMoving(), true);
    QCOMPARE(inner->isDragging(), false);
    QCOMPARE(inner->isMoving(), false);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!outer->isDragging());
    QTRY_VERIFY(!outer->isMoving());
    QVERIFY(!inner->isDragging());
    QVERIFY(!inner->isMoving());

    axis = 200;
    outer->setContentX(50);
    outer->setContentY(50);

    // drag away from the aligned boundary.  Inner flickable dragged.
    QQuickTest::pointerMoveAndPress(device, &window, 0, position);
    axis += invert ? -threshold * 2 : threshold * 2;
    QTest::mouseMove(&window, position);
    axis += invert ? -threshold : threshold;
    QTest::mouseMove(&window, position);
    QCOMPARE(outer->isDragging(), false);
    QCOMPARE(outer->isMoving(), false);
    QCOMPARE(inner->isDragging(), true);
    QCOMPARE(inner->isMoving(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!inner->isDragging());
    QTRY_VERIFY(!inner->isMoving());
    QVERIFY(!outer->isDragging());
    QVERIFY(!outer->isMoving());

    axis = 200;
    inner->setContentX(-margin);
    inner->setContentY(-margin);
    inner->setContentWidth(inner->width() - margin);
    inner->setContentHeight(inner->height() - margin);

    // Drag inner with equal size and contentSize
    QQuickTest::pointerMoveAndPress(device, &window, 0, position);
    axis += invert ? -threshold * 2 : threshold * 2;
    QTest::mouseMove(&window, position);
    axis += invert ? -threshold : threshold;
    QTest::mouseMove(&window, position);
    QCOMPARE(outer->isDragging(), true);
    QCOMPARE(outer->isMoving(), true);
    QCOMPARE(inner->isDragging(), false);
    QCOMPARE(inner->isMoving(), false);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!outer->isDragging());
    QTRY_VERIFY(!outer->isMoving());
    QVERIFY(!inner->isDragging());
    QVERIFY(!inner->isMoving());

    axis = 200;
    inner->setContentX(-margin);
    inner->setContentY(-margin);
    inner->setContentWidth(inner->width() - 100);
    inner->setContentHeight(inner->height() - 100);

    // Drag inner with size greater than contentSize
    QQuickTest::pointerMoveAndPress(device, &window, 0, position);
    axis += invert ? -threshold * 2 : threshold * 2;
    QTest::mouseMove(&window, position);
    axis += invert ? -threshold : threshold;
    QTest::mouseMove(&window, position);
    QCOMPARE(outer->isDragging(), true);
    QCOMPARE(outer->isMoving(), true);
    QCOMPARE(inner->isDragging(), false);
    QCOMPARE(inner->isMoving(), false);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!outer->isDragging());
    QTRY_VERIFY(!outer->isMoving());
    QVERIFY(!inner->isDragging());
    QVERIFY(!inner->isMoving());
}

void tst_qquickflickable::nestedDraggingFlickingMoving_data()
{
    QTest::addColumn<const QPointingDevice *>("device");
    const QPointingDevice *constMouse = mouseDevice.get();
    const QPointingDevice *constTouchscreen = touchscreen.get();

    QTest::newRow("mouse") << constMouse;
    QTest::newRow("touch") << constTouchscreen;
}

void tst_qquickflickable::nestedDraggingFlickingMoving()
{
    // Check that you can start a flick on the outer flickable, and then, while
    // it's flicking, try to start a new flick on the inner flickable. Since
    // the outer flickable is already moving from user interaction, this should
    // simply lead to the outer flickable continue being flicked.
    QFETCH(const QPointingDevice *, device);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedFlickables.qml")));
    QVERIFY(window.rootObject() != nullptr);
    QQuickFlickable *outer = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(outer);
    QQuickFlickable *inner = outer->findChild<QQuickFlickable*>("innerFlickable");
    QVERIFY(inner);

    QSignalSpy outerDraggingSpy(outer, &QQuickFlickable::draggingChanged);
    QSignalSpy innerDraggingSpy(inner, &QQuickFlickable::draggingChanged);

    const auto flickDistance = qApp->styleHints()->startDragDistance();
    const auto dragThreshold = QPoint(flickDistance * 2, flickDistance * 2);

    const auto outerPos = QPoint(inner->x() + inner->width() + 10, inner->y() + inner->height() + 10);
    QQuickTest::pointerFlick(device, &window, 0, outerPos, outerPos - dragThreshold, 100);
    QVERIFY(!outer->isDragging());
    QVERIFY(outer->isFlicking());
    QVERIFY(outer->isMoving());
    QVERIFY(!inner->isDragging());
    QVERIFY(!inner->isFlicking());
    QVERIFY(!inner->isMoving());
    QCOMPARE(outerDraggingSpy.count(), 2);
    QCOMPARE(innerDraggingSpy.count(), 0);

    // The outer flickable is in a "moving" state at this point, which means that it's
    // moving because of user interaction. Now, drag on top of the inner flickable. This
    // should only lead to outer flickable being flicked once more.
    auto innerPos = QPoint(inner->x() + inner->width() - 10, inner->y() + inner->height() - 10);
    QQuickTest::pointerMoveAndPress(device, &window, 0, innerPos);
    innerPos += dragThreshold;
    QQuickTest::pointerMove(device, &window, 0, innerPos);
    innerPos += QPoint(10, 10);
    QQuickTest::pointerMove(device, &window, 0, innerPos);
    innerPos += QPoint(10, 10);
    QQuickTest::pointerMove(device, &window, 0, innerPos);
    QVERIFY(outer->isDragging());
    QVERIFY(outer->isMoving());
    QVERIFY(!outer->isFlicking());
    QVERIFY(!inner->isDragging());
    QVERIFY(!inner->isMoving());
    QVERIFY(!inner->isFlicking());
    QCOMPARE(outerDraggingSpy.count(), 3);
    QCOMPARE(innerDraggingSpy.count(), 0);
    QQuickTest::pointerRelease(device, &window, 0, innerPos);
    QCOMPARE(innerDraggingSpy.count(), 0);
    QVERIFY(outer->isFlicking());
    QVERIFY(outer->isMoving());
    QVERIFY(!outer->isDragging());
    QVERIFY(!inner->isDragging());
    QVERIFY(!inner->isMoving());
    QVERIFY(!inner->isFlicking());
}

void tst_qquickflickable::stopAtBounds_data()
{
    QTest::addColumn<bool>("transpose");
    QTest::addColumn<bool>("invert");
    QTest::addColumn<bool>("pixelAligned");

    QTest::newRow("left") << false << false << false;
    QTest::newRow("right") << false << true << false;
    QTest::newRow("top") << true << false << false;
    QTest::newRow("bottom") << true << true << false;
    QTest::newRow("left,pixelAligned") << false << false << true;
    QTest::newRow("right,pixelAligned") << false << true << true;
    QTest::newRow("top,pixelAligned") << true << false << true;
    QTest::newRow("bottom,pixelAligned") << true << true << true;
}

void tst_qquickflickable::stopAtBounds()
{
    QFETCH(bool, transpose);
    QFETCH(bool, invert);
    QFETCH(bool, pixelAligned);
    const auto device = mouseDevice.get();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("stopAtBounds.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);

    if (transpose)
        flickable->setContentY(invert ? 100 : 0);
    else
        flickable->setContentX(invert ? 100 : 0);
    flickable->setPixelAligned(pixelAligned);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 200);
    int &axis = transpose ? position.ry() : position.rx();

    // drag away from the aligned boundary. View should not move
    QQuickTest::pointerMoveAndPress(device, &window, 0, position);
    QTest::qWait(10);
    for (int i = 0; i < 3; ++i) {
        axis += invert ? -threshold : threshold;
        QTest::mouseMove(&window, position);
    }
    QCOMPARE(flickable->isDragging(), false);
    if (invert)
        QCOMPARE(transpose ? flickable->isAtYEnd() : flickable->isAtXEnd(), true);
    else
        QCOMPARE(transpose ? flickable->isAtYBeginning() : flickable->isAtXBeginning(), true);

    QSignalSpy atXBeginningChangedSpy(flickable, &QQuickFlickable::atXBeginningChanged);
    QSignalSpy atYBeginningChangedSpy(flickable, &QQuickFlickable::atYBeginningChanged);
    QSignalSpy atXEndChangedSpy(flickable, &QQuickFlickable::atXEndChanged);
    QSignalSpy atYEndChangedSpy(flickable, &QQuickFlickable::atYEndChanged);
    // drag back towards boundary
    for (int i = 0; i < 24; ++i) {
        axis += invert ? threshold / 3 : -threshold / 3;
        QTest::mouseMove(&window, position);
    }
    QTRY_COMPARE(flickable->isDragging(), true);
    if (invert)
        QCOMPARE(transpose ? flickable->isAtYEnd() : flickable->isAtXEnd(), false);
    else
        QCOMPARE(transpose ? flickable->isAtYBeginning() : flickable->isAtXBeginning(), false);

    QCOMPARE(atXBeginningChangedSpy.size(), (!transpose && !invert) ? 1 : 0);
    QCOMPARE(atYBeginningChangedSpy.size(), ( transpose && !invert) ? 1 : 0);
    QCOMPARE(atXEndChangedSpy.size(),       (!transpose &&  invert) ? 1 : 0);
    QCOMPARE(atYEndChangedSpy.size(),       ( transpose &&  invert) ? 1 : 0);

    // Drag away from the aligned boundary again.
    // None of the mouse movements will position the view at the boundary exactly,
    // but the view should end up aligned on the boundary
    for (int i = 0; i < 5; ++i) {
        axis += invert ? -threshold * 2 : threshold * 2;
        QTest::mouseMove(&window, position);
    }
    QCOMPARE(flickable->isDragging(), true);

    // we should have hit the boundary and stopped
    if (invert) {
        QCOMPARE(transpose ? flickable->isAtYEnd() : flickable->isAtXEnd(), true);
        QCOMPARE(transpose ? flickable->contentY() : flickable->contentX(), 100.0);
    } else {
        QCOMPARE(transpose ? flickable->isAtYBeginning() : flickable->isAtXBeginning(), true);
        QCOMPARE(transpose ? flickable->contentY() : flickable->contentX(), 0.0);
    }

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    if (transpose) {
        flickable->setContentY(invert ? 100 : 0);
    } else {
        flickable->setContentX(invert ? 100 : 0);
    }

    QSignalSpy flickSignal(flickable, SIGNAL(flickingChanged()));
    if (invert)
        QQuickTest::pointerFlick(device, &window, 0, QPoint(20,20), QPoint(120,120), 100);
    else
        QQuickTest::pointerFlick(device, &window, 0, QPoint(120,120), QPoint(20,20), 100);

    QVERIFY(flickSignal.size() > 0);
    if (transpose) {
        if (invert)
            QTRY_COMPARE(flickable->isAtYBeginning(), true);
        else
            QTRY_COMPARE(flickable->isAtYEnd(), true);
    } else {
        if (invert)
            QTRY_COMPARE(flickable->isAtXBeginning(), true);
        else
            QTRY_COMPARE(flickable->isAtXEnd(), true);
    }
}

void tst_qquickflickable::nestedMouseAreaUsingTouch()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedmousearea.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QCOMPARE(flickable->contentY(), 50.0f);
    QQuickTest::pointerFlick(touchscreen.get(), &window, 1, QPoint(100, 300), QPoint(100, 200), 100);

    // flickable should not have moved
    QCOMPARE(flickable->contentY(), 50.0);

    // draggable item should have moved up
    QQuickItem *nested = window.rootObject()->findChild<QQuickItem*>("nested");
    QVERIFY(nested->y() < 100.0);
}

void tst_qquickflickable::nestedMouseAreaPropagateComposedEvents()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedmouseareapce.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QCOMPARE(flickable->contentY(), 50.0f);
    QQuickTest::pointerFlick(touchscreen.get(), &window, 1, QPoint(100, 300), QPoint(100, 200), 100);

    // flickable should have moved
    QVERIFY(!qFuzzyCompare(flickable->contentY(), 50.0));
}

void tst_qquickflickable::nestedSliderUsingTouch_data()
{
    QTest::addColumn<bool>("keepMouseGrab");
    QTest::addColumn<bool>("keepTouchGrab");
    QTest::addColumn<int>("minUpdates");
    QTest::addColumn<int>("releases");
    QTest::addColumn<int>("ungrabs");

    QTest::newRow("keepBoth") << true << true << 8 << 1 << 1;
    QTest::newRow("keepMouse") << true << false << 8 << 1 << 1;
    QTest::newRow("keepTouch") << false << true << 8 << 1 << 1;
    QTest::newRow("keepNeither") << false << false << 4 << 0 << 1;
}

void tst_qquickflickable::nestedSliderUsingTouch()
{
    QFETCH(bool, keepMouseGrab);
    QFETCH(bool, keepTouchGrab);
    QFETCH(int, minUpdates);
    QFETCH(int, releases);
    QFETCH(int, ungrabs);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedSlider.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);

    TouchDragArea *tda = flickable->findChild<TouchDragArea*>("drag");
    QVERIFY(tda);

    // Drag down and a little to the right: flickable will steal the grab only if tda allows it
    const int dragThreshold = qApp->styleHints()->startDragDistance();
    tda->setKeepMouseGrab(keepMouseGrab);
    tda->setKeepTouchGrab(keepTouchGrab);
    QPoint p0 = tda->mapToScene(QPoint(20, 20)).toPoint();
    QTest::touchEvent(&window, touchscreen.get()).press(0, p0, &window);
    QQuickTouchUtils::flush(&window);
    for (int i = 0; i < 8; ++i) {
        p0 += QPoint(dragThreshold / 6, dragThreshold / 4);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p0, &window);
        QQuickTouchUtils::flush(&window);
    }
    QCOMPARE(tda->active(), keepMouseGrab || keepTouchGrab);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p0, &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(tda->touchPointStates.first(), QEventPoint::State::Pressed);
    QTRY_VERIFY(tda->touchUpdates >= minUpdates);
    QTRY_COMPARE(tda->touchReleases, releases);
    QTRY_COMPARE(tda->ungrabs, ungrabs);
}

// QTBUG-31328
void tst_qquickflickable::pressDelayWithLoader()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pressDelayWithLoader.qml")));

    // do not crash
    QQuickTest::pointerMoveAndPress(device, &window, 0, QPoint(150, 150));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));
}

// QTBUG-34507
void tst_qquickflickable::movementFromProgrammaticFlick()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("movementSignals.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    // verify that the signals for movement and flicking are called in the right order
    flickable->flick(0, -1000);
    QTRY_COMPARE(flickable->property("signalString").toString(), QString("msfsfeme"));
}

// QTBUG_35038
void tst_qquickflickable::contentSize()
{
    QQuickFlickable flickable;
    QCOMPARE(flickable.contentWidth(), qreal(-1));
    QCOMPARE(flickable.contentHeight(), qreal(-1));

    QSignalSpy cwspy(&flickable, SIGNAL(contentWidthChanged()));
    QVERIFY(cwspy.isValid());

    QSignalSpy chspy(&flickable, SIGNAL(contentHeightChanged()));
    QVERIFY(chspy.isValid());

    flickable.setWidth(100);
    QCOMPARE(flickable.width(), qreal(100));
    QCOMPARE(flickable.contentWidth(), qreal(-1.0));
    QCOMPARE(cwspy.size(), 0);

    flickable.setContentWidth(10);
    QCOMPARE(flickable.width(), qreal(100));
    QCOMPARE(flickable.contentWidth(), qreal(10));
    QCOMPARE(cwspy.size(), 1);

    flickable.setHeight(100);
    QCOMPARE(flickable.height(), qreal(100));
    QCOMPARE(flickable.contentHeight(), qreal(-1.0));
    QCOMPARE(chspy.size(), 0);

    flickable.setContentHeight(10);
    QCOMPARE(flickable.height(), qreal(100));
    QCOMPARE(flickable.contentHeight(), qreal(10));
    QCOMPARE(chspy.size(), 1);
}

// QTBUG-53726
void tst_qquickflickable::ratios_smallContent()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("ratios_smallContent.qml")));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(obj != nullptr);

    //doublecheck the item, as specified by contentWidth/Height, fits in the view
    //use tryCompare to allow a bit of stabilization in component's properties
    QTRY_COMPARE(obj->leftMargin() + obj->contentWidth() + obj->rightMargin() <= obj->width(), true);
    QTRY_COMPARE(obj->topMargin() + obj->contentHeight() + obj->bottomMargin() <= obj->height(), true);

    //the whole item fits in the flickable, heightRatio should be 1
    QCOMPARE(obj->property("heightRatioIs").toDouble(), 1.);
    QCOMPARE(obj->property("widthRatioIs").toDouble(), 1.);
}

// QTBUG-48018
void tst_qquickflickable::contentXYNotTruncatedToInt()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contentXY.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);

    flickable->setContentX(1e10);
    QQuickTest::pointerFlick(device, &window, 0, QPoint(200, 100), QPoint(100, 100), 50);

    // make sure we are not clipped at 2^31
    QVERIFY(flickable->contentX() > qreal(1e10));
}

void tst_qquickflickable::keepGrab()
{
    const auto device = mouseDevice.get();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("keepGrab.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);

    QQuickMouseArea *ma = flickable->findChild<QQuickMouseArea*>("ma");
    QVERIFY(ma);
    ma->setPreventStealing(true);

    QPoint pos(250, 250);
    QQuickTest::pointerMoveAndPress(device, &window, 0, pos);
    for (int i = 0; i < 6; ++i) {
        pos += QPoint(10, 10);
        QTest::mouseMove(&window, pos);
        QTest::qWait(10);
    }
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(310, 310));
    QTest::qWait(10);

    QCOMPARE(flickable->contentX(), 0.0);
    QCOMPARE(flickable->contentY(), 0.0);

    ma->setPreventStealing(false);

    pos = QPoint(250, 250);
    QQuickTest::pointerMoveAndPress(device, &window, 0, pos);
    for (int i = 0; i < 6; ++i) {
        pos += QPoint(10, 10);
        QTest::mouseMove(&window, pos);
        QTest::qWait(10);
    }
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(310, 310));
    QTest::qWait(10);

    QVERIFY(flickable->contentX() != 0.0);
    QVERIFY(flickable->contentY() != 0.0);
}

Q_DECLARE_METATYPE(QQuickFlickable::BoundsBehavior)

void tst_qquickflickable::overshoot()
{
    QFETCH(QQuickFlickable::BoundsBehavior, boundsBehavior);
    QFETCH(int, boundsMovement);
    QFETCH(bool, pixelAligned);
    const auto device = mouseDevice.get();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("overshoot.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);
    flickable->setPixelAligned(pixelAligned);

    QCOMPARE(flickable->width(), 200.0);
    QCOMPARE(flickable->height(), 200.0);
    QCOMPARE(flickable->contentWidth(), 400.0);
    QCOMPARE(flickable->contentHeight(), 400.0);

    flickable->setBoundsBehavior(boundsBehavior);
    flickable->setBoundsMovement(QQuickFlickable::BoundsMovement(boundsMovement));

    // drag past the beginning
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
    QTest::mouseMove(&window, QPoint(20, 20));
    QTest::mouseMove(&window, QPoint(30, 30));
    QTest::mouseMove(&window, QPoint(40, 40));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));

    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) && (boundsBehavior & QQuickFlickable::DragOverBounds)) {
        QVERIFY(flickable->property("minContentX").toReal() < 0.0);
        QVERIFY(flickable->property("minContentY").toReal() < 0.0);
    } else {
        QCOMPARE(flickable->property("minContentX").toReal(), 0.0);
        QCOMPARE(flickable->property("minContentY").toReal(), 0.0);
    }
    if (boundsBehavior & QQuickFlickable::DragOverBounds) {
        QVERIFY(flickable->property("minHorizontalOvershoot").toReal() < 0.0);
        QVERIFY(flickable->property("minVerticalOvershoot").toReal() < 0.0);
    } else {
        QCOMPARE(flickable->property("minHorizontalOvershoot").toReal(), 0.0);
        QCOMPARE(flickable->property("minVerticalOvershoot").toReal(), 0.0);
    }
    if (bool(boundsMovement == QQuickFlickable::FollowBoundsBehavior) == bool(boundsBehavior & QQuickFlickable::DragOverBounds)) {
        QCOMPARE(flickable->property("minContentX").toReal(),
                 flickable->property("minHorizontalOvershoot").toReal());
        QCOMPARE(flickable->property("minContentY").toReal(),
                 flickable->property("minVerticalOvershoot").toReal());
    }
    QCOMPARE(flickable->property("maxContentX").toReal(), 0.0);
    QCOMPARE(flickable->property("maxContentY").toReal(), 0.0);
    QCOMPARE(flickable->property("maxHorizontalOvershoot").toReal(), 0.0);
    QCOMPARE(flickable->property("maxVerticalOvershoot").toReal(), 0.0);

    flickable->setContentX(20.0);
    flickable->setContentY(20.0);
    QMetaObject::invokeMethod(flickable, "reset");

    // flick past the beginning
    QQuickTest::pointerFlick(device, &window, 0, QPoint(10, 10), QPoint(50, 50), 50);
    QTRY_VERIFY(!flickable->property("flicking").toBool());

    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) && (boundsBehavior & QQuickFlickable::OvershootBounds)) {
        QVERIFY(flickable->property("minContentX").toReal() < 0.0);
        QVERIFY(flickable->property("minContentY").toReal() < 0.0);
    } else {
        QCOMPARE(flickable->property("minContentX").toReal(), 0.0);
        QCOMPARE(flickable->property("minContentY").toReal(), 0.0);
    }
    if (boundsBehavior & QQuickFlickable::OvershootBounds) {
        QVERIFY(flickable->property("minHorizontalOvershoot").toReal() < 0.0);
        QVERIFY(flickable->property("minVerticalOvershoot").toReal() < 0.0);
    } else {
        QCOMPARE(flickable->property("minHorizontalOvershoot").toReal(), 0.0);
        QCOMPARE(flickable->property("minVerticalOvershoot").toReal(), 0.0);
    }
    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) == (boundsBehavior & QQuickFlickable::OvershootBounds)) {
        QCOMPARE(flickable->property("minContentX").toReal(),
                 flickable->property("minHorizontalOvershoot").toReal());
        QCOMPARE(flickable->property("minContentY").toReal(),
                 flickable->property("minVerticalOvershoot").toReal());
    }
    QCOMPARE(flickable->property("maxContentX").toReal(), 20.0);
    QCOMPARE(flickable->property("maxContentY").toReal(), 20.0);
    QCOMPARE(flickable->property("maxHorizontalOvershoot").toReal(), 0.0);
    QCOMPARE(flickable->property("maxVerticalOvershoot").toReal(), 0.0);

    flickable->setContentX(200.0);
    flickable->setContentY(200.0);
    QMetaObject::invokeMethod(flickable, "reset");

    // drag past the end
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QTest::mouseMove(&window, QPoint(40, 40));
    QTest::mouseMove(&window, QPoint(30, 30));
    QTest::mouseMove(&window, QPoint(20, 20));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));

    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) && (boundsBehavior & QQuickFlickable::DragOverBounds)) {
        QVERIFY(flickable->property("maxContentX").toReal() > 200.0);
        QVERIFY(flickable->property("maxContentX").toReal() > 200.0);
    } else {
        QCOMPARE(flickable->property("maxContentX").toReal(), 200.0);
        QCOMPARE(flickable->property("maxContentY").toReal(), 200.0);
    }
    if (boundsBehavior & QQuickFlickable::DragOverBounds) {
        QVERIFY(flickable->property("maxHorizontalOvershoot").toReal() > 0.0);
        QVERIFY(flickable->property("maxVerticalOvershoot").toReal() > 0.0);
    } else {
        QCOMPARE(flickable->property("maxHorizontalOvershoot").toReal(), 0.0);
        QCOMPARE(flickable->property("maxVerticalOvershoot").toReal(), 0.0);
    }
    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) == (boundsBehavior & QQuickFlickable::DragOverBounds)) {
        QCOMPARE(flickable->property("maxContentX").toReal() - 200.0,
                 flickable->property("maxHorizontalOvershoot").toReal());
        QCOMPARE(flickable->property("maxContentY").toReal() - 200.0,
                 flickable->property("maxVerticalOvershoot").toReal());
    }
    QCOMPARE(flickable->property("minContentX").toReal(), 200.0);
    QCOMPARE(flickable->property("minContentY").toReal(), 200.0);
    QCOMPARE(flickable->property("minHorizontalOvershoot").toReal(), 0.0);
    QCOMPARE(flickable->property("minVerticalOvershoot").toReal(), 0.0);

    flickable->setContentX(180.0);
    flickable->setContentY(180.0);
    QMetaObject::invokeMethod(flickable, "reset");

    // flick past the end
    QQuickTest::pointerFlick(device, &window, 0, QPoint(50, 50), QPoint(10, 10), 50);
    QTRY_VERIFY(!flickable->property("flicking").toBool());

    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) && (boundsBehavior & QQuickFlickable::OvershootBounds)) {
        QVERIFY(flickable->property("maxContentX").toReal() > 200.0);
        QVERIFY(flickable->property("maxContentY").toReal() > 200.0);
    } else {
        QCOMPARE(flickable->property("maxContentX").toReal(), 200.0);
        QCOMPARE(flickable->property("maxContentY").toReal(), 200.0);
    }
    if (boundsBehavior & QQuickFlickable::OvershootBounds) {
        QVERIFY(flickable->property("maxHorizontalOvershoot").toReal() > 0.0);
        QVERIFY(flickable->property("maxVerticalOvershoot").toReal() > 0.0);
    } else {
        QCOMPARE(flickable->property("maxHorizontalOvershoot").toReal(), 0.0);
        QCOMPARE(flickable->property("maxVerticalOvershoot").toReal(), 0.0);
    }
    if ((boundsMovement == QQuickFlickable::FollowBoundsBehavior) == (boundsBehavior & QQuickFlickable::OvershootBounds)) {
        QCOMPARE(flickable->property("maxContentX").toReal() - 200.0,
                 flickable->property("maxHorizontalOvershoot").toReal());
        QCOMPARE(flickable->property("maxContentY").toReal() - 200.0,
                 flickable->property("maxVerticalOvershoot").toReal());
    }
    QCOMPARE(flickable->property("minContentX").toReal(), 180.0);
    QCOMPARE(flickable->property("minContentY").toReal(), 180.0);
    QCOMPARE(flickable->property("minHorizontalOvershoot").toReal(), 0.0);
    QCOMPARE(flickable->property("minVerticalOvershoot").toReal(), 0.0);
}

void tst_qquickflickable::overshoot_data()
{
    QTest::addColumn<QQuickFlickable::BoundsBehavior>("boundsBehavior");
    QTest::addColumn<int>("boundsMovement");
    QTest::addColumn<bool>("pixelAligned");

    QTest::newRow("StopAtBounds,FollowBoundsBehavior")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::StopAtBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << false;
    QTest::newRow("DragOverBounds,FollowBoundsBehavior")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << false;
    QTest::newRow("OvershootBounds,FollowBoundsBehavior")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << false;
    QTest::newRow("DragAndOvershootBounds,FollowBoundsBehavior")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << false;

    QTest::newRow("DragOverBounds,StopAtBounds")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds)
            << int(QQuickFlickable::StopAtBounds) << false;
    QTest::newRow("OvershootBounds,StopAtBounds")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds)
            << int(QQuickFlickable::StopAtBounds) << false;
    QTest::newRow("DragAndOvershootBounds,StopAtBounds")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds)
            << int(QQuickFlickable::StopAtBounds) << false;

    QTest::newRow("StopAtBounds,FollowBoundsBehavior,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::StopAtBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << true;
    QTest::newRow("DragOverBounds,FollowBoundsBehavior,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << true;
    QTest::newRow("OvershootBounds,FollowBoundsBehavior,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << true;
    QTest::newRow("DragAndOvershootBounds,FollowBoundsBehavior,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds)
            << int(QQuickFlickable::FollowBoundsBehavior) << true;

    QTest::newRow("DragOverBounds,StopAtBounds,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds)
            << int(QQuickFlickable::StopAtBounds) << true;
    QTest::newRow("OvershootBounds,StopAtBounds,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds)
            << int(QQuickFlickable::StopAtBounds) << true;
    QTest::newRow("DragAndOvershootBounds,StopAtBounds,pixelAligned")
            << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds)
            << int(QQuickFlickable::StopAtBounds) << true;
}

void tst_qquickflickable::overshoot_reentrant()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("overshoot_reentrant.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);

    // horizontal
    flickable->setContentX(-10.0);
    QCOMPARE(flickable->contentX(), -10.0);
    QCOMPARE(flickable->horizontalOvershoot(), -10.0);

    flickable->setProperty("contentPosAdjustment", -5.0);
    flickable->setContentX(-20.0);
    QCOMPARE(flickable->contentX(), -25.0);
    QCOMPARE(flickable->horizontalOvershoot(), -25.0);

    flickable->setContentX(210);
    QCOMPARE(flickable->contentX(), 210.0);
    QCOMPARE(flickable->horizontalOvershoot(), 10.0);

    flickable->setProperty("contentPosAdjustment", 5.0);
    flickable->setContentX(220.0);
    QCOMPARE(flickable->contentX(), 225.0);
    QCOMPARE(flickable->horizontalOvershoot(), 25.0);

    // vertical
    flickable->setContentY(-10.0);
    QCOMPARE(flickable->contentY(), -10.0);
    QCOMPARE(flickable->verticalOvershoot(), -10.0);

    flickable->setProperty("contentPosAdjustment", -5.0);
    flickable->setContentY(-20.0);
    QCOMPARE(flickable->contentY(), -25.0);
    QCOMPARE(flickable->verticalOvershoot(), -25.0);

    flickable->setContentY(210);
    QCOMPARE(flickable->contentY(), 210.0);
    QCOMPARE(flickable->verticalOvershoot(), 10.0);

    flickable->setProperty("contentPosAdjustment", 5.0);
    flickable->setContentY(220.0);
    QCOMPARE(flickable->contentY(), 225.0);
    QCOMPARE(flickable->verticalOvershoot(), 25.0);
}

void tst_qquickflickable::synchronousDrag_data()
{
    QTest::addColumn<bool>("synchronousDrag");

    QTest::newRow("default") << false;
    QTest::newRow("synch") << true;
}

void tst_qquickflickable::synchronousDrag()
{
    QFETCH(bool, synchronousDrag);
    const auto device = mouseDevice.get();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("longList.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);
    QCOMPARE(flickable->synchronousDrag(), false);
    flickable->setSynchronousDrag(synchronousDrag);

    QPoint p1(100, 100);
    QPoint p2(95, 95);
    QPoint p3(70, 70);
    QPoint p4(50, 50);
    QPoint p5(30, 30);
    QCOMPARE(flickable->contentY(), 0.0f);

    // Drag via mouse
    QQuickTest::pointerMoveAndPress(device, &window, 0, p1);
    QTest::mouseMove(&window, p2);
    QTest::mouseMove(&window, p3);
    QTest::mouseMove(&window, p4);
    QCOMPARE(flickable->contentY(), synchronousDrag ? 50.0f : 0.0f);
    QTest::mouseMove(&window, p5);
    if (!synchronousDrag)
        QVERIFY(flickable->contentY() < 50.0f);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p5);

    // Reset to initial condition
    flickable->setContentY(0);

    // Drag via touch
    QTest::touchEvent(&window, touchscreen.get()).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p2, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p3, &window);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p4, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(flickable->contentY(), synchronousDrag ? 50.0f : 0.0f);
    QTest::touchEvent(&window, touchscreen.get()).move(0, p5, &window);
    QQuickTouchUtils::flush(&window);
    if (!synchronousDrag)
        QVERIFY(flickable->contentY() < 50.0f);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p5, &window);
}

// QTBUG-81098: tests that a binding to visibleArea doesn't result
// in a division-by-zero exception (when exceptions are enabled).
void tst_qquickflickable::visibleAreaBinding()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("visibleAreaBinding.qml")));
    QTRY_COMPARE(window.status(), QQuickView::Ready);
    // Shouldn't crash.
}

void tst_qquickflickable::parallelTouch() // QTBUG-30840
{
    const int threshold = qApp->styleHints()->startDragDistance();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("parallel.qml")));
    QQuickFlickable *flickable1 = window.rootObject()->findChild<QQuickFlickable*>("fl1");
    QVERIFY(flickable1);
    QQuickFlickable *flickable2 = window.rootObject()->findChild<QQuickFlickable*>("fl2");
    QVERIFY(flickable2);

    // Drag both in parallel via touch, opposite directions
    QPoint p0(80, 240);
    QPoint p1(240, 240);
    QTest::touchEvent(&window, touchscreen.get()).press(0, p0, &window).press(1, p1, &window);
    int began1After = -1;
    int began2After = -1;
    for (int i = 0; i < 8; ++i) {
        p0 += QPoint(0, threshold);
        p1 -= QPoint(0, threshold);
        QTest::touchEvent(&window, touchscreen.get()).move(0, p0, &window).move(1, p1, &window);
        QQuickTouchUtils::flush(&window);
        if (began1After < 0 && flickable1->isDragging())
            began1After = i;
        if (began2After < 0 && flickable2->isDragging())
            began2After = i;
    }
    qCDebug(lcTests, "flickables began dragging after %d and %d events respectively", began1After, began2After);
    QVERIFY(flickable1->isDraggingVertically());
    QVERIFY(flickable2->isDraggingVertically());
    QCOMPARE(began1After, 2);
    QCOMPARE(began2After, 2);
    QTest::touchEvent(&window, touchscreen.get()).release(0, p0, &window).release(1, p1, &window);
    QTRY_VERIFY(!flickable1->isMoving());
    QTRY_VERIFY(!flickable2->isMoving());
}

void tst_qquickflickable::ignoreNonLeftMouseButtons() // QTBUG-96909
{
    QFETCH(Qt::MouseButton, otherButton);
    const auto device = mouseDevice.get();
    const int threshold = qApp->styleHints()->startDragDistance();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragon.qml")));
    QQuickFlickable *flickable = static_cast<QQuickFlickable *>(window.rootObject());
    QSignalSpy dragSpy(flickable, &QQuickFlickable::draggingChanged);

    // Drag with left button
    QPoint p1(100, 100);
    QQuickTest::pointerMoveAndPress(device, &window, 0, p1);
    for (int i = 0; i < 8; ++i) {
        p1 -= QPoint(threshold, threshold);
        QQuickTest::pointerMove(device, &window, 0, p1, 50);
    }
    QVERIFY(flickable->isDragging());
    QCOMPARE(dragSpy.size(), 1);

    // Press other button too, then release left button: dragging changes to false
    QQuickTest::pointerPress(device, &window, 0, p1, otherButton);
    QQuickTest::pointerRelease(device, &window, 0, p1, Qt::LeftButton);
    QTRY_COMPARE(flickable->isDragging(), false);
    QCOMPARE(dragSpy.size(), 2);

    // Drag further with the other button held: Flickable ignores it
    for (int i = 0; i < 8; ++i) {
        p1 -= QPoint(threshold, threshold);
        QQuickTest::pointerMove(device, &window, 0, p1, 50);
    }
    QCOMPARE(flickable->isDragging(), false);
    QCOMPARE(dragSpy.size(), 2);

    // Release other button: nothing happens
    QQuickTest::pointerRelease(device, &window, 0, p1, otherButton);
    QCOMPARE(dragSpy.size(), 2);
}

void tst_qquickflickable::ignoreNonLeftMouseButtons_data()
{
    QTest::addColumn<Qt::MouseButton>("otherButton");

    QTest::newRow("right") << Qt::RightButton;
    QTest::newRow("middle") << Qt::MiddleButton;
}

void tst_qquickflickable::receiveTapOutsideContentItem()
{
    // Check that if we add a TapHandler handler to a flickable, we
    // can tap on the whole flickable area inside it, which includes
    // the extents in addition to the content item.
    QQuickView window;
    window.resize(200, 200);
    FlickableWithExtents flickable;
    flickable.setParentItem(window.contentItem());
    flickable.setWidth(200);
    flickable.setHeight(200);
    flickable.setContentWidth(100);
    flickable.setContentHeight(100);

    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QQuickTapHandler tapHandler(&flickable);
    QSignalSpy clickedSpy(&tapHandler, SIGNAL(tapped(QEventPoint,Qt::MouseButton)));

    // Tap outside the content item in the top-left corner
    QTest::mouseClick(&window, Qt::LeftButton, {}, QPoint(5, 5));
    QCOMPARE(clickedSpy.size(), 1);

    // Tap outside the content item in the bottom-right corner
    const QPoint bottomRight(flickable.contentItem()->width() + 5, flickable.contentItem()->height() + 5);
    QTest::mouseClick(&window, Qt::LeftButton, {}, bottomRight);
    QCOMPARE(clickedSpy.size(), 2);
}

void tst_qquickflickable::flickWhenRotated_data()
{
    QTest::addColumn<qreal>("rootRotation");
    QTest::addColumn<qreal>("flickableRotation");
    QTest::addColumn<qreal>("scale");

    static constexpr qreal rotations[] = { 0, 30, 90, 180, 270 };
    static constexpr qreal scales[] = { 0.3, 1, 1.5 };

    for (const auto pr : rotations) {
        for (const auto fr : rotations) {
            if (pr <= 90) {
                for (const auto s : scales)
                    QTest::addRow("parent: %g, flickable: %g, scale: %g", pr, fr, s) << pr << fr << s;
            } else {
                // don't bother trying every scale with every set of rotations, to save time
                QTest::addRow("parent: %g, flickable: %g, scale: 1", pr, fr) << pr << fr << qreal(1);
            }
        }
    }
}

void tst_qquickflickable::flickWhenRotated() // QTBUG-99639
{
    QFETCH(qreal, rootRotation);
    QFETCH(qreal, flickableRotation);
    QFETCH(qreal, scale);
    const auto device = mouseDevice.get();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("rotatedFlickable.qml")));
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);

    window.rootObject()->setRotation(rootRotation);
    flickable->setRotation(flickableRotation);
    window.rootObject()->setScale(scale);
    QVERIFY(flickable->isAtYBeginning());

    // Flick in Y direction in Flickable's coordinate system and check how much it moved
    const QPointF startPoint = flickable->mapToGlobal(QPoint(20, 180));
    const QPointF endPoint = flickable->mapToGlobal(QPoint(20, 40));
    QQuickTest::pointerFlick(device, &window, 0, window.mapFromGlobal(startPoint).toPoint(), window.mapFromGlobal(endPoint).toPoint(), 100);
    QTRY_VERIFY(flickable->isMoving());
    QTRY_VERIFY(!flickable->isMoving());
    qCDebug(lcTests) << "flicking from" << startPoint << "to" << endPoint << ": ended at contentY" << flickable->contentY();
    // usually flickable->contentY() is at 275 or very close
    QVERIFY(!flickable->isAtYBeginning());
}

void tst_qquickflickable::flickAndReleaseOutsideBounds() // QTBUG-104987
{
    // Check that flicking works when the mouse release happens
    // outside the bounds of the flickable (and the flick started on top
    // of a TapHandler that has a passive grab).
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickableWithTapHandler.qml")));
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flickable);
    QQuickItem *childItem = flickable->findChild<QQuickItem*>("childItem");
    QVERIFY(childItem);

    QVERIFY(flickable->isAtYBeginning());

    // Startpoint is on top of the tapHandler, while the endpoint is outside the flickable
    const QPointF startPos = childItem->mapToGlobal(QPoint(10, 10));
    const QPointF endPos = flickable->mapToGlobal(QPoint(10, -10));
    const QPoint globalStartPos = window.mapFromGlobal(startPos).toPoint();
    const QPoint globalEndPos = window.mapFromGlobal(endPos).toPoint();
    const qreal dragDistance = 20;

    // Note: here we need to initiate a flick using raw events, rather than
    // flickable.flick(), since we're testing if the mouse events takes the
    // correct path to starts a flick (among passive and exclusive grabbers, combined
    // with childMouseEventFilter()).
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, globalStartPos);
    QTest::mouseMove(&window, globalStartPos - QPoint(0, dragDistance / 2));
    QTest::mouseMove(&window, globalStartPos - QPoint(0, dragDistance));
    QTest::mouseMove(&window, globalEndPos);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, globalEndPos);

    // Ensure that the content item ends up being moved more than what we dragged
    QTRY_VERIFY(flickable->contentY() > dragDistance * 2);
}

void tst_qquickflickable::scrollingWithFractionalExtentSize_data()
{
    QTest::addColumn<QByteArray>("property");
    QTest::addColumn<bool>("isYAxis");
    QTest::addColumn<bool>("atBeginning");
    QTest::addColumn<QQuickFlickable::BoundsBehaviorFlag>("boundsBehaviour");

    QTest::newRow("minyextent") << QByteArray("topMargin") << true << true << QQuickFlickable::StopAtBounds;
    QTest::newRow("maxyextent") << QByteArray("bottomMargin") << true << false << QQuickFlickable::StopAtBounds;
    QTest::newRow("minxextent") << QByteArray("leftMargin") << false << true << QQuickFlickable::StopAtBounds;
    QTest::newRow("maxxextent") << QByteArray("rightMargin") << false << false << QQuickFlickable::StopAtBounds;

    QTest::newRow("minyextent") << QByteArray("topMargin") << true << true << QQuickFlickable::DragAndOvershootBounds;
    QTest::newRow("maxyextent") << QByteArray("bottomMargin") << true << false << QQuickFlickable::DragAndOvershootBounds;
    QTest::newRow("minxextent") << QByteArray("leftMargin") << false << true << QQuickFlickable::DragAndOvershootBounds;
    QTest::newRow("maxxextent") << QByteArray("rightMargin") << false << false << QQuickFlickable::DragAndOvershootBounds;
}

void tst_qquickflickable::scrollingWithFractionalExtentSize() // QTBUG-101268
{
    QFETCH(QByteArray, property);
    QFETCH(bool, isYAxis);
    QFETCH(bool, atBeginning);
    QFETCH(QQuickFlickable::BoundsBehaviorFlag, boundsBehaviour);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("fractionalExtent.qml")));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(window.rootObject());
    QVERIFY(flickable);
    flickable->setBoundsBehavior(boundsBehaviour);

    qreal value = 100.5;
    flickable->setProperty(property.constData(), 100.5);
    if (isYAxis)
        flickable->setContentY(atBeginning ? -value : value + qAbs(flickable->height() - flickable->contentHeight()));
    else
        flickable->setContentX(atBeginning ? -value : value + qAbs(flickable->width() - flickable->contentWidth()));

    if (isYAxis) {
        QVERIFY(atBeginning ? flickable->isAtYBeginning() : flickable->isAtYEnd());
        QVERIFY(!flickable->isMovingVertically());
    } else {
        QVERIFY(atBeginning ? flickable->isAtXBeginning() : flickable->isAtXEnd());
        QVERIFY(!flickable->isMovingHorizontally());
    }

    QPointF pos(flickable->x() + flickable->width() / 2, flickable->y() + flickable->height() / 2);
    QPoint angleDelta(isYAxis ? 0 : (atBeginning ? -50 : 50), isYAxis ? (atBeginning ? -50 : 50) : 0);

    QWheelEvent wheelEvent1(pos, window.mapToGlobal(pos), QPoint(), angleDelta,
                               Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);

    QGuiApplication::sendEvent(&window, &wheelEvent1);
    qApp->processEvents();
    if (isYAxis) {
        QVERIFY(flickable->isMovingVertically());
        QTRY_VERIFY(!flickable->isMovingVertically());
        QVERIFY(!(atBeginning ? flickable->isAtYBeginning() : flickable->isAtYEnd()));
    } else {
        QVERIFY(flickable->isMovingHorizontally());
        QTRY_VERIFY(!flickable->isMovingHorizontally());
        QVERIFY(!(atBeginning ? flickable->isAtXBeginning() : flickable->isAtXEnd()));
    }

    QWheelEvent wheelEvent2(pos, window.mapToGlobal(pos), QPoint(), -angleDelta,
                               Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    wheelEvent2.setTimestamp(wheelEvent1.timestamp() + 10);

    QGuiApplication::sendEvent(&window, &wheelEvent2);
    qApp->processEvents();

    if (isYAxis) {
        QVERIFY(flickable->isMovingVertically());
        QTRY_VERIFY(!flickable->isMovingVertically());
        QVERIFY(atBeginning ? flickable->isAtYBeginning() : flickable->isAtYEnd());
    } else {
        QVERIFY(flickable->isMovingHorizontally());
        QTRY_VERIFY(!flickable->isMovingHorizontally());
        QVERIFY(atBeginning ? flickable->isAtXBeginning() : flickable->isAtXEnd());
    }
}

void tst_qquickflickable::setContentPositionWhileDragging_data()
{
    QTest::addColumn<bool>("isHorizontal");
    QTest::addColumn<int>("newPos");
    QTest::addColumn<int>("newExtent");
    QTest::newRow("horizontal, setContentX") << true << 0 << -1;
    QTest::newRow("vertical, setContentY") << false << 0 << -1;
    QTest::newRow("horizontal, setContentWidth") << true << -1 << 200;
    QTest::newRow("vertical, setContentHeight") << false << -1 << 200;
}

void tst_qquickflickable::setContentPositionWhileDragging() // QTBUG-104966
{
    QFETCH(bool, isHorizontal);
    QFETCH(int, newPos);
    QFETCH(int, newExtent);
    const auto device = mouseDevice.get();

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("contentPosWhileDragging.qml")));
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable *>();
    QVERIFY(flickable);

    const auto contentPos = [flickable]() -> QPoint {
        return QPoint(flickable->contentX(), flickable->contentY());
    };
    const qreal threshold =
            qApp->styleHints()->startDragDistance() * flickable->parentItem()->scale();
    const QPoint thresholdPnt(qRound(threshold), qRound(threshold));
    const auto flickableCenterPos = flickable->mapToScene({flickable->width() / 2, flickable->height() / 2}).toPoint();

    // Drag the mouse until we have surpassed the mouse drag threshold and a drag is initiated
    // by checking for  flickable->isDragging()
    QPoint pos = flickableCenterPos;
    QQuickTest::pointerMoveAndPress(device, &window, 0, pos);
    int j = 1;
    QVERIFY(!flickable->isDragging());
    while (!flickable->isDragging()) {
        pos = flickableCenterPos - QPoint(j, j);
        QTest::mouseMove(&window, pos);
        j++;
    }

    // Now we have entered the drag state
    QVERIFY(flickable->isDragging());
    QCOMPARE(flickable->contentX(), 0);
    QCOMPARE(flickable->contentY(), 0);
    QVERIFY(flickable->width() > 0);
    QVERIFY(flickable->height() > 0);


    const int moveLength = 50;
    const QPoint unitDelta(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
    const QPoint moveDelta = unitDelta * moveLength;

    pos -= 3*moveDelta;
    QTest::mouseMove(&window, pos);
    // Should be positive because we drag in the opposite direction
    QCOMPARE(contentPos(), 3 * moveDelta);
    QPoint expectedContentPos;

    // Set the content item position back to zero *while dragging* (!!)
    if (newPos >= 0) {
        if (isHorizontal) {
            flickable->setContentX(newPos);
        } else {
            flickable->setContentY(newPos);
        }
        // Continue dragging
        pos -= moveDelta;
        expectedContentPos = moveDelta;
    } else if (newExtent >= 0) {
        // ...or reduce the content size be be less than current (contentX, contentY) position
        // This forces the content item to move.
        // contentY: 150
        // 320 - 150 = 170 pixels down to bottom
        // Now reduce contentHeight to 200
        // since we are at the bottom, and the flickable is 100 pixels tall, contentY must land
        // at newExtent - 100.

        if (isHorizontal) {
            flickable->setContentWidth(newExtent);
        } else {
            flickable->setContentHeight(newExtent);
        }
        // Assumption is that the contentItem is aligned to the bottom of the flickable
        // We therefore cannot scroll/flick it further down. Drag it up towards the top instead
        // (by moving mouse down).
        pos += moveDelta;
        expectedContentPos = unitDelta * (newExtent - (isHorizontal ? flickable->width() : flickable->height()));
    }

    QTest::mouseMove(&window, pos);

    // Make sure that the contentItem was only dragged the delta in mouse movement since the last
    // setContentX/Y() call.
    QCOMPARE(contentPos(), expectedContentPos);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY(!flickable->isDragging());
}

void tst_qquickflickable::coalescedMove()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QSignalSpy movementStartedSpy(flickable, SIGNAL(movementStarted()));
    QSignalSpy movementEndedSpy(flickable, SIGNAL(movementEnded()));
    QSignalSpy flickStartedSpy(flickable, SIGNAL(flickStarted()));
    QSignalSpy flickEndedSpy(flickable, SIGNAL(flickEnded()));

    QTest::touchEvent(&window, touchscreen.get()).press(0, {10, 10}).commit();

    QTest::touchEvent(&window, touchscreen.get()).move(0, {10, 40}).commit();

    QTest::touchEvent(&window, touchscreen.get()).move(0, {10, 100}).commit();

    QTest::touchEvent(&window, touchscreen.get()).release(0, {10, 150}).commit();
    QQuickTouchUtils::flush(&window);

    QTRY_VERIFY(!flickable->isMoving());

    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(flickStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 1);
    QCOMPARE(flickEndedSpy.size(), 1);
}

void tst_qquickflickable::onlyOneMove()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QSignalSpy movementStartedSpy(flickable, SIGNAL(movementStarted()));
    QSignalSpy movementEndedSpy(flickable, SIGNAL(movementEnded()));
    QSignalSpy flickStartedSpy(flickable, SIGNAL(flickStarted()));
    QSignalSpy flickEndedSpy(flickable, SIGNAL(flickEnded()));

    QTest::touchEvent(&window, touchscreen.get()).press(0, {10, 10}).commit();
    QQuickTouchUtils::flush(&window);

    QTest::touchEvent(&window, touchscreen.get()).move(0, {10, 100}).commit();
    QQuickTouchUtils::flush(&window);

    QTest::touchEvent(&window, touchscreen.get()).release(0, {10, 200}).commit();
    QQuickTouchUtils::flush(&window);

    QTRY_VERIFY(!flickable->isMoving());

    QCOMPARE(movementStartedSpy.size(), 1);
    QCOMPARE(flickStartedSpy.size(), 1);
    QCOMPARE(movementEndedSpy.size(), 1);
    QCOMPARE(flickEndedSpy.size(), 1);
}

void tst_qquickflickable::proportionalWheelScrolling() // QTBUG-106338 etc.
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("wheel.qml")));
    QQuickFlickable *flickable = window.rootObject()->findChild<QQuickFlickable *>();
    QVERIFY(flickable);

    QVERIFY(!flickable->property("ended").value<bool>());

    QPointF pos(flickable->x() + flickable->width() / 2, flickable->y() + flickable->height() / 2);
    QPoint angleDelta(0, -120);
    QWheelEvent wheelEvent(pos, window.mapToGlobal(pos), QPoint(), angleDelta,
                               Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);

    QGuiApplication::sendEvent(&window, &wheelEvent);
    qApp->processEvents();

    // Verify that scrolling is proportional to the wheel delta
    QVERIFY(flickable->isMovingVertically());
    QTRY_VERIFY(!flickable->isMovingVertically());

    // The current movement formula being used is: delta / 120 * wheelScrollLines * 24
    const int defaultWheelDecel = 15000;
    bool wheelDecelerationEnvSet = false;
    const int wheelDecelerationEnv = qEnvironmentVariableIntValue("QT_QUICK_FLICKABLE_WHEEL_DECELERATION", &wheelDecelerationEnvSet);
    const qreal wheelDecel = wheelDecelerationEnvSet ? wheelDecelerationEnv : defaultWheelDecel;
    const bool proportionalWheel = wheelDecel >= 15000;
    qCDebug(lcTests) << "platform wheel decel" << defaultWheelDecel
                     << "env wheel decel" << wheelDecelerationEnv
                     << "expect proportional scrolling?" << proportionalWheel;
    const qreal expectedMovementFromWheelClick = qAbs(angleDelta.y()) / 120 * qApp->styleHints()->wheelScrollLines() * 24;

    if (proportionalWheel)
        QCOMPARE(flickable->contentY(), expectedMovementFromWheelClick);

    QVERIFY(flickable->property("ended").value<bool>());
    QCOMPARE(flickable->property("movementsAfterEnd").value<int>(), 0);

    flickable->setProperty("ended", QVariant::fromValue(false));
    flickable->setContentY(0);

    // Verify that multiple wheel events in a row won't accumulate the scroll distance, before the timeline completes
    wheelEvent.setTimestamp(wheelEvent.timestamp() + 2000);
    QGuiApplication::sendEvent(&window, &wheelEvent);

    wheelEvent.setTimestamp(wheelEvent.timestamp() + 10);
    QGuiApplication::sendEvent(&window, &wheelEvent);

    wheelEvent.setTimestamp(wheelEvent.timestamp() + 10);
    QGuiApplication::sendEvent(&window, &wheelEvent);

    qApp->processEvents();

    QVERIFY(flickable->isMovingVertically());
    QTRY_VERIFY(!flickable->isMovingVertically());

    if (proportionalWheel) {
        QVERIFY2(flickable->contentY() >= expectedMovementFromWheelClick, "The contentItem moved shorter than expected from a wheelEvent");
        QCOMPARE_LT(flickable->contentY(), expectedMovementFromWheelClick * 3);
    }

    QVERIFY(flickable->property("ended").value<bool>());
    QCOMPARE(flickable->property("movementsAfterEnd").value<int>(), 0);
}

void tst_qquickflickable::touchCancel()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("flickable03.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QSignalSpy movementStartedSpy(flickable, SIGNAL(movementStarted()));
    QSignalSpy movementEndedSpy(flickable, SIGNAL(movementEnded()));

    int touchPosY = 10;
    QTest::touchEvent(&window, touchscreen.get()).press(0, {10, touchPosY}).commit();
    QQuickTouchUtils::flush(&window);

    for (int i = 0; i < 3; ++i) {
        touchPosY += qApp->styleHints()->startDragDistance();
        QTest::touchEvent(&window, touchscreen.get()).move(0, {10, touchPosY}).commit();
        QQuickTouchUtils::flush(&window);
    }

    QTRY_COMPARE(movementStartedSpy.size(), 1);
    QWindowSystemInterface::handleTouchCancelEvent(nullptr, touchscreen.get());
    QTRY_COMPARE(movementEndedSpy.size(), 1);
}

void tst_qquickflickable::pixelAlignedEndPoints()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("endpoints.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);
    flickable->setPixelAligned(true);
    QVERIFY(flickable->isAtYBeginning());

    QSignalSpy isAtEndSpy(flickable, &QQuickFlickable::atYEndChanged);
    QSignalSpy isAtBeginningSpy(flickable, &QQuickFlickable::atYBeginningChanged);

    flickable->setContentY(199.99);
    QCOMPARE(flickable->contentY(), 200);
    QVERIFY(!flickable->isAtYBeginning());
    QVERIFY(flickable->isAtYEnd());
    QCOMPARE(isAtEndSpy.count(), 1);
    QCOMPARE(isAtBeginningSpy.count(), 1);

    flickable->setContentY(0.01);
    QCOMPARE(flickable->contentY(), 0);
    QVERIFY(flickable->isAtYBeginning());
    QVERIFY(!flickable->isAtYEnd());
    QCOMPARE(isAtEndSpy.count(), 2);
    QCOMPARE(isAtBeginningSpy.count(), 2);
}

void tst_qquickflickable::nestedWheelEventPropagation_data()
{
    QTest::addColumn<bool>("isHorizontal");
    QTest::addColumn<bool>("isInverted");
    QTest::addColumn<bool>("offsetStart");
    QTest::newRow("top") << false << false << false;
    QTest::newRow("bottom") << false << true << false;
    QTest::newRow("left") << true << false << false;
    QTest::newRow("right") << true << true << false;
    QTest::newRow("top with offset") << false << false << true;
    QTest::newRow("bottom with offset") << false << true << true;
    QTest::newRow("left with offset") << true << false << true;
    QTest::newRow("right with offset") << true << true << true;
}

void tst_qquickflickable::nestedWheelEventPropagation()
{
    // Arrange
    QFETCH(bool, isHorizontal);
    QFETCH(bool, isInverted);
    QFETCH(bool, offsetStart);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedWheel.qml")));

    QQuickMouseArea *outer = qobject_cast<QQuickMouseArea *>(window.rootObject());
    QVERIFY(outer);

    QQuickFlickable *inner = outer->findChild<QQuickFlickable *>("innerFlickable");
    QVERIFY(inner);
    inner->setFlickableDirection(isHorizontal ? QQuickFlickable::HorizontalFlick
                                              : QQuickFlickable::VerticalFlick);

    // Setting isInverted property moves the Flickable viewable area to the opposite corner
    inner->setProperty("isInverted", isInverted);
    QCOMPARE(inner->contentX(), isInverted ? 100 : 0);
    QCOMPARE(inner->contentY(), isInverted ? 100 : 0);

    if (offsetStart) {
        // Set starting position slightly away from the bound
        inner->setContentX(isInverted ? 99.0 : 1.0);
        inner->setContentY(isInverted ? 99.0 : 1.0);
    }

    QSignalSpy propagateSpy(outer, &QQuickMouseArea::wheel);

    // Act
    QPoint position(50, 50);
    QWheelEvent event(position, window.mapToGlobal(position), QPoint(),
                      QPoint(isHorizontal ? (isInverted ? -120 : 120) : 0,
                             !isHorizontal ? (isInverted ? -120 : 120) : 0),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    event.setAccepted(false);
    QGuiApplication::sendEvent(&window, &event);

    // Assert
    QCOMPARE(inner->isMoving(), offsetStart);
    QCOMPARE(propagateSpy.count(), 1);
}

QTEST_MAIN(tst_qquickflickable)

#include "tst_qquickflickable.moc"
