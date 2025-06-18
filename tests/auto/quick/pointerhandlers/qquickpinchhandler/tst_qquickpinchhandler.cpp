// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtGui/QStyleHints>
#include <QtGui/private/qeventpoint_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtQuick/private/qquickpinchhandler_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class PinchHandler : public QQuickPinchHandler {
public:
    const QQuickHandlerPoint &firstPoint() { return currentPoints().first(); }
    const QQuickHandlerPoint &lastPoint() { return currentPoints().last(); }
};

class tst_QQuickPinchHandler: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickPinchHandler() : QQmlDataTest(QT_QMLTEST_DATADIR) { }
private slots:
    void cleanupTestCase();
    void pinchProperties();
    void scale_data();
    void scale();
    void scaleThreeFingers();
    void scaleNativeGesture_data();
    void scaleNativeGesture();
    void cumulativeNativeGestures_data();
    void cumulativeNativeGestures();
    void pan();
    void dragAxesEnabled_data();
    void dragAxesEnabled();
    void retouch();
    void cancel();
    void transformedpinchHandler_data();
    void transformedpinchHandler();
    void dragVsPinch_data();
    void dragVsPinch();
    void pinchStartPos_data();
    void pinchStartPos();

private:
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
    std::unique_ptr<QPointingDevice> touchpad{QTest::createTouchDevice(QInputDevice::DeviceType::TouchPad)};
};

void tst_QQuickPinchHandler::cleanupTestCase()
{

}

static bool withinBounds(qreal lower, qreal num, qreal upper)
{
    return num >= lower && num <= upper;
}

void tst_QQuickPinchHandler::pinchProperties()
{
    QScopedPointer<QQuickView> window(QQuickViewTestUtils::createView());
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(window->rootObject() != nullptr);

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, pinchHandler->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(pinchHandler, SIGNAL(targetChanged()));
    pinchHandler->setTarget(rootItem);
    QCOMPARE(targetSpy.size(),1);
    pinchHandler->setTarget(rootItem);
    QCOMPARE(targetSpy.size(),1);

    // drag axes
    QCOMPARE(pinchHandler->xAxis()->enabled(), true);
    QCOMPARE(pinchHandler->yAxis()->enabled(), true);
    QSignalSpy xEnabledSpy(pinchHandler->xAxis(), &QQuickDragAxis::enabledChanged);
    QSignalSpy yEnabledSpy(pinchHandler->yAxis(), &QQuickDragAxis::enabledChanged);
    QSignalSpy scaleEnabledSpy(pinchHandler->scaleAxis(), &QQuickDragAxis::enabledChanged);
    QSignalSpy rotationEnabledSpy(pinchHandler->rotationAxis(), &QQuickDragAxis::enabledChanged);
    pinchHandler->xAxis()->setEnabled(false);
    QCOMPARE(xEnabledSpy.count(), 1);
    pinchHandler->yAxis()->setEnabled(false);
    QCOMPARE(yEnabledSpy.count(), 1);
    pinchHandler->scaleAxis()->setEnabled(false);
    QCOMPARE(scaleEnabledSpy.count(), 1);
    pinchHandler->rotationAxis()->setEnabled(false);
    QCOMPARE(rotationEnabledSpy.count(), 1);

    // minimum and maximum drag properties
    QSignalSpy xminSpy(pinchHandler->xAxis(), &QQuickDragAxis::minimumChanged);
    QSignalSpy xmaxSpy(pinchHandler->xAxis(), &QQuickDragAxis::maximumChanged);
    QSignalSpy yminSpy(pinchHandler->yAxis(), &QQuickDragAxis::minimumChanged);
    QSignalSpy ymaxSpy(pinchHandler->yAxis(), &QQuickDragAxis::maximumChanged);

    QCOMPARE(pinchHandler->xAxis()->minimum(), std::numeric_limits<qreal>::lowest());
    QCOMPARE(pinchHandler->xAxis()->maximum(), 140);
    QCOMPARE(pinchHandler->yAxis()->minimum(), std::numeric_limits<qreal>::lowest());
    QCOMPARE(pinchHandler->yAxis()->maximum(), 170);

    pinchHandler->xAxis()->setMinimum(10);
    pinchHandler->xAxis()->setMaximum(10);
    pinchHandler->yAxis()->setMinimum(10);
    pinchHandler->yAxis()->setMaximum(10);

    QCOMPARE(pinchHandler->xAxis()->minimum(), 10);
    QCOMPARE(pinchHandler->xAxis()->maximum(), 10);
    QCOMPARE(pinchHandler->yAxis()->minimum(), 10);
    QCOMPARE(pinchHandler->yAxis()->maximum(), 10);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    pinchHandler->xAxis()->setMinimum(10);
    pinchHandler->xAxis()->setMaximum(10);
    pinchHandler->yAxis()->setMinimum(10);
    pinchHandler->yAxis()->setMaximum(10);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    // minimum and maximum scale properties
    QSignalSpy scaleAxisMinSpy(pinchHandler->scaleAxis(), &QQuickDragAxis::minimumChanged);
    QSignalSpy scaleAxisMaxSpy(pinchHandler->scaleAxis(), &QQuickDragAxis::maximumChanged);

    QCOMPARE(pinchHandler->scaleAxis()->minimum(), 0.5);
    QCOMPARE(pinchHandler->scaleAxis()->maximum(), 4);

#if QT_DEPRECATED_SINCE(6, 5)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(pinchHandler->minimumScale(), 0.5);
    QCOMPARE(pinchHandler->maximumScale(), 4);
QT_WARNING_POP
#endif

    pinchHandler->scaleAxis()->setMinimum(0.25);
    pinchHandler->scaleAxis()->setMaximum(1.5);

    QCOMPARE(pinchHandler->scaleAxis()->minimum(), 0.25);
    QCOMPARE(pinchHandler->scaleAxis()->maximum(), 1.5);

#if QT_DEPRECATED_SINCE(6, 5)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(pinchHandler->minimumScale(), 0.25);
    QCOMPARE(pinchHandler->maximumScale(), 1.5);
QT_WARNING_POP
#endif

    QCOMPARE(scaleAxisMinSpy.size(),1);
    QCOMPARE(scaleAxisMaxSpy.size(),1);

    pinchHandler->scaleAxis()->setMinimum(0.25);
    pinchHandler->scaleAxis()->setMaximum(1.5);

    QCOMPARE(scaleAxisMinSpy.size(),1);
    QCOMPARE(scaleAxisMaxSpy.size(),1);

    // minimum and maximum rotation properties
    QSignalSpy rotAxisMinSpy(pinchHandler->rotationAxis(), &QQuickDragAxis::minimumChanged);
    QSignalSpy rotAxisMaxSpy(pinchHandler->rotationAxis(), &QQuickDragAxis::maximumChanged);

#if QT_DEPRECATED_SINCE(6, 5)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(pinchHandler->minimumRotation(), 0);
    QCOMPARE(pinchHandler->maximumRotation(), 90);
QT_WARNING_POP
#endif
    QCOMPARE(pinchHandler->rotationAxis()->minimum(), 0);
    QCOMPARE(pinchHandler->rotationAxis()->maximum(), 90);

    pinchHandler->rotationAxis()->setMinimum(-90);
    pinchHandler->rotationAxis()->setMaximum(45);

#if QT_DEPRECATED_SINCE(6, 5)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(pinchHandler->minimumRotation(), -90);
    QCOMPARE(pinchHandler->maximumRotation(), 45);
QT_WARNING_POP
#endif

    QCOMPARE(rotAxisMinSpy.size(),1);
    QCOMPARE(rotAxisMaxSpy.size(),1);

    pinchHandler->rotationAxis()->setMinimum(-90);
    pinchHandler->rotationAxis()->setMaximum(45);

    QCOMPARE(rotAxisMinSpy.size(),1);
    QCOMPARE(rotAxisMaxSpy.size(),1);
}

QEventPoint makeTouchPoint(int id, QPoint p, QQuickView *v, QQuickItem *i)
{
    QEventPoint touchPoint(id);
    QMutableEventPoint::setPosition(touchPoint, i->mapFromScene(p));
    QMutableEventPoint::setGlobalPosition(touchPoint, v->mapToGlobal(p));
    QMutableEventPoint::setScenePosition(touchPoint, p);
    return touchPoint;
}

void tst_QQuickPinchHandler::scale_data()
{
    QTest::addColumn<QUrl>("qmlfile");
    QTest::addColumn<bool>("hasTarget");
    QTest::addColumn<bool>("axisEnabled");
    QTest::newRow("targetModifying") << testFileUrl("pinchproperties.qml") << true << true;
    QTest::newRow("nullTarget") << testFileUrl("nullTarget.qml") << false << true;
    QTest::newRow("axisDiabled") << testFileUrl("pinchproperties.qml") << true << false;
}

void tst_QQuickPinchHandler::scale()
{
    QFETCH(QUrl, qmlfile);
    QFETCH(bool, hasTarget);
    QFETCH(bool, axisEnabled);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, qmlfile));
    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root != nullptr);
    auto *pinchHandler = static_cast<PinchHandler *>(root->findChild<QQuickPinchHandler*>());
    QVERIFY(pinchHandler != nullptr);
    pinchHandler->scaleAxis()->setEnabled(axisEnabled);
    QQuickItem *blackRect = (hasTarget ? pinchHandler->target() : pinchHandler->parentItem());
    QVERIFY(blackRect != nullptr);
    QSignalSpy grabChangedSpy(pinchHandler, SIGNAL(grabChanged(QPointingDevice::GrabTransition,QEventPoint)));
    QSignalSpy scaleChangedSpy(pinchHandler, &QQuickPinchHandler::scaleChanged);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&window, touchscreen.get());
    pinchSequence.press(0, p0, &window).commit();
    QQuickTouchUtils::flush(&window);
    // In order for the stationary point to remember its previous position,
    // we have to reuse the same pinchSequence object.  Otherwise if we let it
    // be destroyed and then start a new sequence, point 0 will default to being
    // stationary at 0, 0, and pinchHandler will filter out that touchpoint because
    // it is outside its bounds.
    pinchSequence.stationary(0).press(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QCOMPARE(grabChangedSpy.size(), 1); // passive grab

    QPoint pd(10, 10);
    // move one point until PinchHandler activates
    for (int pi = 0; pi < 10 && !pinchHandler->active(); ++pi) {
        p1 += pd;
        pinchSequence.stationary(0).move(1, p1, &window).commit();
        QQuickTouchUtils::flush(&window);
    }
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QCOMPARE(pinchHandler->active(), true);
    // grabs occur when the handler becomes active; at that time, QQuickHandlerPoint.sceneGrabPosition should be correct
    QVERIFY(pinchHandler->firstPoint().sceneGrabPosition() != QPointF());
    QVERIFY(pinchHandler->lastPoint().sceneGrabPosition() != QPointF());
    QCOMPARE(pinchHandler->firstPoint().sceneGrabPosition(), pinchHandler->firstPoint().scenePosition());
    QCOMPARE(pinchHandler->lastPoint().sceneGrabPosition(), pinchHandler->lastPoint().scenePosition());
    // first point got a passive grab; both points got exclusive grabs
    QCOMPARE(grabChangedSpy.size(), 3);
    QLineF line(p0, p1);
    const qreal startLength = line.length();
    // to be redefined below
    qreal lastScale = pinchHandler->persistentScale();
    qreal expectedIncrement = 0;

    // move the same point even further and observe the change in scale
    for (int i = 0; i < 2; ++i) {
        lastScale = pinchHandler->activeScale();
        p1 += pd;
        pinchSequence.stationary(0).move(1, p1, &window).commit();
        QQuickTouchUtils::flush(&window);
        if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
        line.setP2(p1);
        qreal expectedScale = line.length() / startLength;
        qCDebug(lcPointerTests) << "pinchScale" << root->property("pinchScale").toReal()
                                << "expected" << expectedScale << "; target scale" << blackRect->scale()
                                << "increments" << scaleChangedSpy.size()
                                << "multiplier" << (scaleChangedSpy.isEmpty() ? 1 : scaleChangedSpy.last().first().toReal());
        if (axisEnabled) {
            QVERIFY(qFloatDistance(root->property("pinchScale").toReal(), expectedScale) < 10);
            QVERIFY(qFloatDistance(blackRect->scale(), expectedScale) < 10);
        } else {
            QCOMPARE(root->property("pinchScale").toInt(), 1);
            QCOMPARE(blackRect->scale(), 1);
        }
        QCOMPARE(pinchHandler->persistentScale(), root->property("pinchScale").toReal());
        QCOMPARE(pinchHandler->persistentScale(), pinchHandler->activeScale()); // in sync for the first gesture
        QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), pinchHandler->activeScale());
        QCOMPARE(pinchHandler->scaleAxis()->activeValue(), pinchHandler->activeScale());
        expectedIncrement = pinchHandler->activeScale() / lastScale;
        QCOMPARE(scaleChangedSpy.size(), axisEnabled ? i + 1 : 0);
        if (axisEnabled)
            QCOMPARE(scaleChangedSpy.last().first().toReal(), expectedIncrement);
        QPointF expectedCentroid = p0 + (p1 - p0) / 2;
        QCOMPARE(pinchHandler->centroid().scenePosition(), expectedCentroid);
    }

    lastScale = pinchHandler->persistentScale();
    pinchSequence.release(0, p0, &window).release(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    // scale property is persistent after release
    QCOMPARE(pinchHandler->persistentScale(), lastScale);
    QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), lastScale);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), 1);

    // pinch a second time: scale picks up where we left off
    p0 = QPoint(80, 80);
    p1 = QPoint(100, 100);
    pinchSequence.press(0, p0, &window).press(1, p1, &window).commit();
    // move one point until PinchHandler activates
    for (int pi = 0; pi < 10 && !pinchHandler->active(); ++pi) {
        p1 += pd;
        pinchSequence.stationary(0).move(1, p1, &window).commit();
        QQuickTouchUtils::flush(&window);
    }
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QCOMPARE(pinchHandler->active(), true);
    QCOMPARE(pinchHandler->persistentScale(), lastScale); // just activated, not scaling further yet
    QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), lastScale);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), 1);
    for (int i = 0; i < 2; ++i) {
        lastScale = pinchHandler->persistentScale();
        p1 += pd;
        pinchSequence.stationary(0).move(1, p1, &window).commit();
        QQuickTouchUtils::flush(&window);
        if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
        if (axisEnabled)
            QCOMPARE_GT(pinchHandler->persistentScale(), lastScale);
        else
            QCOMPARE(pinchHandler->persistentScale(), 1);
        line.setP2(p1);
        qreal expectedActiveScale = line.length() / startLength;
        qCDebug(lcPointerTests) << i << "activeScale" << pinchHandler->activeScale()
                                << "expected" << expectedActiveScale << "; scale" << pinchHandler->persistentScale()
                                << "increments" << scaleChangedSpy.size()
                                << "multiplier" << (scaleChangedSpy.isEmpty() ? 1 : scaleChangedSpy.last().first().toReal());
        if (axisEnabled) {
            QVERIFY(qFloatDistance(pinchHandler->activeScale(), expectedActiveScale) < 10);
            QCOMPARE_NE(pinchHandler->persistentScale(), pinchHandler->activeScale()); // not in sync anymore
        } else {
            QCOMPARE(pinchHandler->persistentScale(), pinchHandler->activeScale());
        }
        QCOMPARE(pinchHandler->persistentScale(), root->property("pinchScale").toReal());
        QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), root->property("pinchScale").toReal());
        QCOMPARE(pinchHandler->scaleAxis()->activeValue(), pinchHandler->activeScale());
        expectedIncrement = pinchHandler->persistentScale() / lastScale;
        QCOMPARE(scaleChangedSpy.size(), axisEnabled ? i + 3 : 0);
        if (axisEnabled)
            QCOMPARE(scaleChangedSpy.last().first().toReal(), expectedIncrement);
    }

    // scale beyond maximumScale
    lastScale = pinchHandler->activeScale();
    p1 = QPoint(310, 310);
    pinchSequence.stationary(0).move(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QCOMPARE(blackRect->scale(), axisEnabled ? 4 : 1);
    QCOMPARE(pinchHandler->persistentScale(), axisEnabled ? 4 : 1); // limited by maximumScale
    QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), axisEnabled ? 4 : 1);
    expectedIncrement = pinchHandler->activeScale() / lastScale;
    QCOMPARE(scaleChangedSpy.size(), axisEnabled ? 5 : 0);
    if (axisEnabled)
        QCOMPARE(scaleChangedSpy.last().first().toReal(), expectedIncrement);
    pinchSequence.release(0, p0, &window).release(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(pinchHandler->active(), false);
}

void tst_QQuickPinchHandler::scaleThreeFingers()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("threeFingers.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    // center of blackrect is at 150,150
    QPoint p0(80, 80);
    QPoint p1(220, 80);
    QPoint p2(150, 220);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and pinchHandler will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        pinchSequence.stationary(0).stationary(1).press(2, p2, window).commit();
        QQuickTouchUtils::flush(window);
        for (int i = 0; i < 5;++i) {
            p0 += QPoint(-4, -4);
            p1 += QPoint(+4, -4);
            p2 += QPoint( 0, +6);
            pinchSequence.move(0, p0,window).move(1, p1,window).move(2, p2,window).commit();
            QQuickTouchUtils::flush(window);
        }

        QCOMPARE(pinchHandler->active(), true);
        // scale we got was 1.1729088738267854364, but keep some slack
        qCDebug(lcPointerTests) << "pinch scale" << pinchHandler->persistentScale() << "expected 1.173";
        QVERIFY(withinBounds(1.163, pinchHandler->persistentScale(), 1.183));
        // should not rotate
        QCOMPARE(root->rotation(), 0);
        // rotation should be 0, but could be something tiny
        qCDebug(lcPointerTests) << "pinch scale expected zero:" << pinchHandler->activeRotation()
                                << pinchHandler->rotationAxis()->activeValue()
                                << pinchHandler->rotationAxis()->persistentValue();
        QCOMPARE_LE(qAbs(pinchHandler->activeRotation()), 0.001);
        QCOMPARE(pinchHandler->rotationAxis()->activeValue(), pinchHandler->activeRotation());
        QCOMPARE(pinchHandler->rotationAxis()->persistentValue(), 0);

        for (int i = 0; i < 5;++i) {
            p0 += QPoint(-4, -4);
            p1 += QPoint(+4, -4);
            p2 += QPoint( 0, +6);
            pinchSequence.move(0, p0,window).move(1, p1,window).move(2, p2,window).commit();
            QQuickTouchUtils::flush(window);
        }
        // scale we got was 1.4613, but keep some slack
        QVERIFY(withinBounds(1.361, pinchHandler->persistentScale(), 1.561));

        // since points were moved symetrically around the y axis, centroid should remain at x:150
        QCOMPARE(pinchHandler->centroid().scenePosition().x(), 150); // blackrect is at 50,50

        // scale beyond bound, we should reach the maximumScale
        p0 += QPoint(-40, -40);
        p1 += QPoint(+40, -40);
        p2 += QPoint(  0, +60);
        pinchSequence.move(0, p0,window).move(1, p1,window).move(2, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(pinchHandler->persistentScale(), 2);
        QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), 2);
        QCOMPARE(pinchHandler->scaleAxis()->activeValue(), 2);
        pinchSequence.release(0, p0, window).release(1, p1, window).release(2, p2, window).commit();
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(pinchHandler->active(), false);
}

void tst_QQuickPinchHandler::scaleNativeGesture_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<qreal>("scale");

    QTest::newRow("just pinch") << "pinchproperties.qml" << 1.1;
    QTest::newRow("pinch & drag") << "pinchAndDrag.qml" << 1.1;
    QTest::newRow("bigger than limit") << "pinchproperties.qml" << 5.0;
    QTest::newRow("smaller than limit") << "pinchproperties.qml" << 0.25;
}

void tst_QQuickPinchHandler::scaleNativeGesture()
{
    QFETCH(QString, qmlfile);
    QFETCH(qreal, scale);

    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl(qmlfile));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);
    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);
    QQuickItem *target = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(target != nullptr);

    QPointF targetPos = target->position();
    ulong ts = 1;

    // first pinch: scale it
    const qreal expectedScale = qBound(qreal(0.5), scale, qreal(4));
    QPointF pinchPos(75, 75);
    QPointF pinchLocalPos = target->mapFromScene(pinchPos);
    // target position is adjusted in QQuickItemPrivate::adjustedPosForTransform()
    // so as to compensate for the change in size, to hold the centroid in place
    const QPointF expectedPos = targetPos + QPointF( (pinchPos.x() - target->x()) * (expectedScale - 1),
                                                     (pinchPos.y() - target->y()) * (expectedScale - 1) );
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad.get(),
                                               Qt::BeginNativeGesture, pinchPos, pinchPos);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QWindowSystemInterface::handleGestureEventWithRealValue(window, ts++, touchpad.get(),
                                                            Qt::ZoomNativeGesture, scale - 1, pinchPos, pinchPos);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    QTRY_COMPARE(target->scale(), expectedScale);
    QCOMPARE(pinchHandler->active(), true);
    qCDebug(lcPointerTests) << "centroid: local" << pinchHandler->centroid().position()
                            << "scene" << pinchHandler->centroid().scenePosition();
    QCOMPARE(pinchHandler->centroid().position().toPoint(), pinchLocalPos.toPoint());
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), pinchPos.toPoint());
    QVERIFY(qAbs(target->position().x() - expectedPos.x()) < 0.001);
    QVERIFY(qAbs(target->position().y() - expectedPos.y()) < 0.001);
    QCOMPARE(pinchHandler->persistentScale(), expectedScale);
    QCOMPARE(pinchHandler->activeScale(), scale);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), scale);
    QCOMPARE(pinchHandler->activeTranslation(), QPointF());
    QCOMPARE(pinchHandler->activeRotation(), 0);
    QCOMPARE(pinchHandler->rotationAxis()->persistentValue(), 0);
    QCOMPARE(pinchHandler->rotationAxis()->activeValue(), 0);
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad.get(),
                                               Qt::EndNativeGesture, pinchPos, pinchPos);
    QTRY_COMPARE(pinchHandler->active(), false);
    QCOMPARE(target->scale(), expectedScale);
    QCOMPARE(pinchHandler->persistentScale(), expectedScale);
    QCOMPARE(pinchHandler->activeScale(), 1);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), 1);
    QCOMPARE(pinchHandler->activeTranslation(), QPointF());
    QCOMPARE(pinchHandler->activeRotation(), 0);
    QCOMPARE(pinchHandler->rotationAxis()->persistentValue(), 0);
    QCOMPARE(pinchHandler->rotationAxis()->activeValue(), 0);

    // second pinch at a different position: scale it back to original size again
    // but remove the limits first, so that we can scale arbitrarily
    pinchHandler->scaleAxis()->setMaximum(qInf());
    pinchHandler->scaleAxis()->setMinimum(-qInf());
    const qreal reverseScale = (1 / expectedScale);
    pinchPos = QPointF(110, 110);
    pinchLocalPos = target->mapFromScene(pinchPos);
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad.get(),
                                               Qt::BeginNativeGesture, pinchPos, pinchPos);
    QWindowSystemInterface::handleGestureEventWithRealValue(window, ts++, touchpad.get(),
                                                            Qt::ZoomNativeGesture, reverseScale - 1, pinchPos, pinchPos);
    QTRY_COMPARE(target->scale(), 1);
    QCOMPARE(pinchHandler->active(), true);
    qCDebug(lcPointerTests) << "centroid: local" << pinchHandler->centroid().position()
                            << "scene" << pinchHandler->centroid().scenePosition();
    QCOMPARE(pinchHandler->centroid().position().toPoint(), pinchLocalPos.toPoint());
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), pinchPos.toPoint());
    QCOMPARE(pinchHandler->persistentScale(), 1);
    QCOMPARE(pinchHandler->activeScale(), reverseScale);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), reverseScale);
    QWindowSystemInterface::handleGestureEvent(window, ts++, touchpad.get(),
                                               Qt::EndNativeGesture, pinchPos, pinchPos);
    QTRY_COMPARE(pinchHandler->active(), false);
    QCOMPARE(target->scale(), 1);
    QCOMPARE(pinchHandler->persistentScale(), 1);
    QCOMPARE(pinchHandler->activeScale(), 1);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), 1);
}

void tst_QQuickPinchHandler::cumulativeNativeGestures_data()
{
    QTest::addColumn<const QPointingDevice*>("device");
    QTest::addColumn<Qt::NativeGestureType>("gesture");
    QTest::addColumn<qreal>("value");
    QTest::addColumn<bool>("scalingEnabled");
    QTest::addColumn<bool>("rotationEnabled");
    QTest::addColumn<QList<QPoint>>("expectedTargetTranslations");

    const auto *touchpadDevice = touchpad.get();
    const auto *mouse = QPointingDevice::primaryPointingDevice();

    QTest::newRow("touchpad: rotate") << touchpadDevice << Qt::RotateNativeGesture << 5.0 << true << true
                                      << QList<QPoint>{{-2, 2}, {-5, 4}, {-7, 6}, {-10, 7}};
    QTest::newRow("touchpad: scale") << touchpadDevice << Qt::ZoomNativeGesture << 0.1 << true << true
                                     << QList<QPoint>{{3, 3}, {5, 5}, {8, 8}, {12, 12}};
    QTest::newRow("touchpad: rotate disabled") << touchpadDevice << Qt::RotateNativeGesture << 5.0 << true << false
                                      << QList<QPoint>{{-2, 2}, {-5, 4}, {-7, 6}, {-10, 7}};
    QTest::newRow("touchpad: scale disabled") << touchpadDevice << Qt::ZoomNativeGesture << 0.1 << false << true
                                     << QList<QPoint>{{3, 3}, {5, 5}, {8, 8}, {12, 12}};
    if (mouse->type() == QInputDevice::DeviceType::Mouse) {
        QTest::newRow("mouse: rotate") << mouse << Qt::RotateNativeGesture << 5.0 << true << true
                                       << QList<QPoint>{{-2, 2}, {-5, 4}, {-7, 6}, {-10, 7}};
        QTest::newRow("mouse: scale") << mouse << Qt::ZoomNativeGesture << 0.1 << true << true
                                      << QList<QPoint>{{3, 3}, {5, 5}, {8, 8}, {12, 12}};
    } else {
        qCWarning(lcPointerTests) << "skipping mouse tests: primary device is not a mouse" << mouse;
    }
}

void tst_QQuickPinchHandler::cumulativeNativeGestures()
{
    QFETCH(const QPointingDevice*, device);
    QFETCH(Qt::NativeGestureType, gesture);
    QFETCH(qreal, value);
    QFETCH(bool, scalingEnabled);
    QFETCH(bool, rotationEnabled);
    QFETCH(QList<QPoint>, expectedTargetTranslations);

    QCOMPARE(expectedTargetTranslations.size(), 4);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pinchproperties.qml")));
    QVERIFY(window.rootObject() != nullptr);
    qApp->processEvents();

    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root != nullptr);
    QQuickPinchHandler *pinchHandler = root->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);
    pinchHandler->scaleAxis()->setEnabled(scalingEnabled);
    pinchHandler->rotationAxis()->setEnabled(rotationEnabled);
    QQuickItem *target = root->findChild<QQuickItem*>("blackrect");
    QVERIFY(target != nullptr);
    QCOMPARE(pinchHandler->target(), target);

    ulong ts = 1;
    qreal expectedScale = 1;
    qreal expectedRotation = 0;
    QPointF pinchPos(75, 75);
    const QPointF initialTargetPos(target->position());
    QWindowSystemInterface::handleGestureEvent(&window, ts++, device,
                                               Qt::BeginNativeGesture, pinchPos, pinchPos);
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    for (int i = 1; i <= 4; ++i) {
        QWindowSystemInterface::handleGestureEventWithRealValue(&window, ts++, device,
                                                                gesture, value, pinchPos, pinchPos);
        qApp->processEvents();
        switch (gesture) {
        case Qt::ZoomNativeGesture:
            expectedScale = qBound(qreal(0.5), qPow(1 + value, i), qreal(4));
            break;
        case Qt::RotateNativeGesture:
            expectedRotation = qBound(qreal(0), value * i, qreal(90));
            break;
        default:
            break; // PinchHandler doesn't react to the others
        }
        if (!rotationEnabled)
            expectedRotation = 0;
        if (!scalingEnabled)
            expectedScale = 1;

        qCDebug(lcPointerTests) << i << gesture << "with value" << value
                                << ": scale" << target->scale() << "expected" << expectedScale
                                << ": rotation" << target->rotation() << "expected" << expectedRotation;
        if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
        QCOMPARE(target->scale(), expectedScale);
        QCOMPARE(target->rotation(), expectedRotation);
        QCOMPARE(pinchHandler->persistentScale(), expectedScale);
        QCOMPARE(pinchHandler->activeScale(), expectedScale);
        QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), expectedScale);
        QCOMPARE(pinchHandler->scaleAxis()->activeValue(), expectedScale);
        QCOMPARE(pinchHandler->persistentRotation(), expectedRotation);
        QCOMPARE(pinchHandler->activeRotation(), expectedRotation);
        QCOMPARE(pinchHandler->rotationAxis()->persistentValue(), expectedRotation);
        QCOMPARE(pinchHandler->rotationAxis()->activeValue(), expectedRotation);
        // The target gets transformed around the gesture position, for which
        // QQuickItemPrivate::adjustedPosForTransform() computes its new position to compensate.
        QPointF delta = target->position() - initialTargetPos;
        qCDebug(lcPointerTests) << "target moved by" << delta << "to" << target->position()
                                << "active trans" << pinchHandler->activeTranslation()
                                << "perst trans" << pinchHandler->persistentTranslation();
        if (scalingEnabled && rotationEnabled) {
            QCOMPARE_NE(target->position(), initialTargetPos);
            QCOMPARE(delta.toPoint(), expectedTargetTranslations.at(i - 1));
        }
        // The native pinch gesture cannot include a translation component (and
        // the cursor doesn't move while you are performing the gesture on a touchpad).
        QCOMPARE(pinchHandler->activeTranslation(), QPointF());
        // The target only moves to compensate for scale and rotation changes, and that's
        // not reflected in PinchHandler.persistentTranslation.
        QCOMPARE(pinchHandler->persistentTranslation(), QPointF());
    }
    QCOMPARE(pinchHandler->active(), true);
    qCDebug(lcPointerTests) << "centroid: local" << pinchHandler->centroid().position()
                            << "scene" << pinchHandler->centroid().scenePosition();
    QCOMPARE(pinchHandler->persistentScale(), expectedScale);
    QCOMPARE(pinchHandler->activeScale(), expectedScale);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), expectedScale);
    QWindowSystemInterface::handleGestureEvent(&window, ts++, device,
                                               Qt::EndNativeGesture, pinchPos, pinchPos);
    QTRY_COMPARE(pinchHandler->active(), false);
    QCOMPARE(target->scale(), expectedScale);
    QCOMPARE(target->rotation(), expectedRotation);
    QCOMPARE(pinchHandler->persistentScale(), expectedScale);
    QCOMPARE(pinchHandler->activeScale(), 1);
    QCOMPARE(pinchHandler->scaleAxis()->persistentValue(), expectedScale);
    QCOMPARE(pinchHandler->scaleAxis()->activeValue(), 1);
    QCOMPARE(pinchHandler->persistentRotation(), expectedRotation);
    QCOMPARE(pinchHandler->activeRotation(), 0);
    QCOMPARE(pinchHandler->rotationAxis()->persistentValue(), expectedRotation);
    QCOMPARE(pinchHandler->rotationAxis()->activeValue(), 0);
    QCOMPARE(pinchHandler->activeTranslation(), QPointF());
    QCOMPARE(pinchHandler->persistentTranslation(), QPointF());
}

void tst_QQuickPinchHandler::pan()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);
    QSignalSpy translationChangedSpy(pinchHandler, &QQuickPinchHandler::translationChanged);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(root->property("pinchScale").toReal(), -1.0);

        p0 += QPoint(dragThreshold, 0);
        p1 += QPoint(dragThreshold, 0);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // movement < dragThreshold: pinchHandler not yet active
        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(root->property("pinchScale").toReal(), -1.0);

        // just above the dragThreshold: pinchHandler starts
        p0 += QPoint(1, 0);
        p1 += QPoint(1, 0);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(pinchHandler->active(), true);
        QCOMPARE(root->property("pinchScale").toReal(), 1.0);

        // Calculation of the center point is tricky at first:
        // center point of the two touch points in item coordinates:
        // scene coordinates: (80, 80) + (dragThreshold, 0), (100, 100) + (dragThreshold, 0)
        //                    = ((180+dT)/2, 180/2) = (90+dT, 90)
        // item  coordinates: (scene) - (50, 50) = (40+dT, 40)
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 1, 90));
        // pan started, but no actual movement registered yet:
        // blackrect starts at 50,50
        QCOMPARE(blackRect->x(), 50.0);
        QCOMPARE(blackRect->y(), 50.0);
        QCOMPARE(translationChangedSpy.size(), 1);
        QCOMPARE(translationChangedSpy.first().first().value<QVector2D>(), QVector2D(0, 0));

        p0 += QPoint(10, 0);
        p1 += QPoint(10, 0);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 11, 90));
        QCOMPARE(blackRect->x(), 60.0);
        QCOMPARE(blackRect->y(), 50.0);
        QCOMPARE(translationChangedSpy.size(), 2);
        QCOMPARE(translationChangedSpy.last().first().value<QVector2D>(), QVector2D(10, 0));

        p0 += QPoint(0, 10);
        p1 += QPoint(0, 10);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 11, 90 + 10));
        QCOMPARE(blackRect->x(), 60.0);
        QCOMPARE(blackRect->y(), 60.0);
        QCOMPARE(translationChangedSpy.size(), 3);
        QCOMPARE(translationChangedSpy.last().first().value<QVector2D>(), QVector2D(0, 10));

        p0 += QPoint(10, 10);
        p1 += QPoint(10, 10);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // now the item moved again, thus the center point of the touch is moved in total by (10, 10)
        QCOMPARE(pinchHandler->centroid().scenePosition(), QPointF(90 + dragThreshold + 21, 90 + 20));
        QCOMPARE(blackRect->x(), 70.0);
        QCOMPARE(blackRect->y(), 70.0);
        QCOMPARE(translationChangedSpy.size(), 4);
        QCOMPARE(translationChangedSpy.last().first().value<QVector2D>(), QVector2D(10, 10));
    }

    // pan x beyond bound
    p0 += QPoint(100,100);
    p1 += QPoint(100,100);
    QTest::touchEvent(window, touchscreen.get()).move(0, p0, window).move(1, p1, window);
    QQuickTouchUtils::flush(window);

    QCOMPARE(blackRect->x(), 140.0);
    QCOMPARE(blackRect->y(), 170.0);
    QCOMPARE(translationChangedSpy.size(), 5);
    QCOMPARE(translationChangedSpy.last().first().value<QVector2D>(), QVector2D(100, 100));

    QTest::touchEvent(window, touchscreen.get()).release(0, p0, window).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!root->property("pinchActive").toBool());
}

void tst_QQuickPinchHandler::dragAxesEnabled_data()
{
    QTest::addColumn<bool>("xEnabled");
    QTest::addColumn<bool>("yEnabled");

    QTest::newRow("both enabled") << true << true;
    QTest::newRow("x enabled") << true << false;
    QTest::newRow("y enabled") << false << true;
    QTest::newRow("both disabled") << false << false;
}

void tst_QQuickPinchHandler::dragAxesEnabled()
{
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QQuickPinchHandler *pinchHandler = blackRect->findChild<QQuickPinchHandler*>();
    QVERIFY(pinchHandler != nullptr);

    QFETCH(bool, xEnabled);
    QFETCH(bool, yEnabled);
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    pinchHandler->xAxis()->setEnabled(xEnabled);
    pinchHandler->yAxis()->setEnabled(yEnabled);
    QPoint c = blackRect->mapToScene(blackRect->clipRect().center()).toPoint();
    QPoint p0 = c - QPoint(0, dragThreshold);
    QPoint p1 = c + QPoint(0, dragThreshold);
    QPoint blackRectPos = blackRect->position().toPoint();

    // press two points, one above the rectangle's center and one below
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
    pinchSequence.press(0, p0, window).press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);

    // expand the pinch vertically
    p0 -= QPoint(0, dragThreshold);
    p1 += QPoint(0, dragThreshold);
    pinchSequence.move(0, p0, window).move(1, p1, window).commit();
    for (int pi = 0; pi < 4; ++pi) {
        p0 -= QPoint(0, dragThreshold);
        p1 += QPoint(0, dragThreshold);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        qCDebug(lcPointerTests) << pi << "active" << pinchHandler->active() << "pts" << p0 << p1
                                << "centroid" << pinchHandler->centroid().scenePosition()
                                << "rect pos" << blackRect->position() << "scale" << blackRect->scale();
    }
    QCOMPARE(pinchHandler->active(), true);
    QVERIFY(blackRect->scale() >= 2.0);
    // drag started, but we only did scaling without any translation
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), c);
    QCOMPARE(blackRect->position().toPoint().x(), blackRectPos.x());
    QCOMPARE(blackRect->position().toPoint().y(), blackRectPos.y());

    // drag diagonally
    p0 += QPoint(150, 150);
    p1 += QPoint(150, 150);
    pinchSequence.move(0, p0, window).move(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    // the target should move if the xAxis is enabled, or stay in place if not
    qCDebug(lcPointerTests) << "after diagonal drag: pts" << p0 << p1
                            << "centroid" << pinchHandler->centroid().scenePosition()
                            << "rect pos" << blackRect->position() << "scale" << blackRect->scale();
    QCOMPARE(pinchHandler->centroid().scenePosition().toPoint(), QPoint(250, 250));
    QCOMPARE(blackRect->position().toPoint().x(), xEnabled ? 140 : blackRectPos.x()); // because of xAxis.maximum
    QCOMPARE(blackRect->position().toPoint().y(), yEnabled ? 170 : blackRectPos.y()); // because of yAxis.maximum

    QTest::touchEvent(window, touchscreen.get()).release(0, p0, window).release(1, p1, window);
    QQuickTouchUtils::flush(window);
}

// test pinchHandler, release one point, touch again to continue pinchHandler
void tst_QQuickPinchHandler::retouch()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        const QPoint delta(dragThreshold + 1, dragThreshold + 1);
        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("pinchScale").toReal(), 1.0);
        QCOMPARE(pinchHandler->active(), true);

        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(pinchHandler->active(), true);

        // accept some slack
        QVERIFY(withinBounds(1.4, root->property("pinchScale").toReal(), 1.6));
        QCOMPARE(pinchHandler->centroid().position().toPoint(), QPoint(40, 40)); // blackrect is at 50,50
        QVERIFY(withinBounds(1.4, blackRect->scale(), 1.6));

        QCOMPARE(root->property("activeCount").toInt(), 1);
        QCOMPARE(root->property("deactiveCount").toInt(), 0);

        // Hold down the first finger but release the second one
        pinchSequence.stationary(0).release(1, p1, window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("activeCount").toInt(), 1);
        QCOMPARE(root->property("deactiveCount").toInt(), 1);

        // Keep holding down the first finger and re-touch the second one, then move them both
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        p0 -= QPoint(10,10);
        p1 += QPoint(10,10);
        pinchSequence.move(0, p0, window).move(1, p1, window).commit();
        QQuickTouchUtils::flush(window);

        // Lifting and retouching results in onPinchStarted being called again
        QCOMPARE(root->property("activeCount").toInt(), 2);
        QCOMPARE(root->property("deactiveCount").toInt(), 1);

        pinchSequence.release(0, p0, window).release(1, p1, window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(pinchHandler->active(), false);
        QCOMPARE(root->property("activeCount").toInt(), 2);
        QCOMPARE(root->property("deactiveCount").toInt(), 2);
    }
}

void tst_QQuickPinchHandler::cancel()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView *window = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = window->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p0(80, 80);
    QPoint p1(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p0, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and pinchHandler will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p1, window).commit();
        QQuickTouchUtils::flush(window);
        const QPoint delta(dragThreshold + 1, dragThreshold + 1);
        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("pinchScale").toReal(), 1.0);
        QCOMPARE(pinchHandler->active(), true);

        p0 -= delta;
        p1 += delta;
        pinchSequence.move(0, p0,window).move(1, p1,window).commit();
        QQuickTouchUtils::flush(window);

        QVERIFY(withinBounds(1.4, root->property("pinchScale").toReal(), 1.6));
        QCOMPARE(pinchHandler->centroid().position().toPoint(), QPoint(40, 40)); // blackrect is at 50,50
        QVERIFY(withinBounds(1.4, blackRect->scale(), 1.6));

        QSKIP("cancel is not supported atm");

        QTouchEvent cancelEvent(QEvent::TouchCancel, touchscreen.get());
        QCoreApplication::sendEvent(window, &cancelEvent);
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("pinchScale").toReal(), 1.0);
        QCOMPARE(root->property("center").toPoint(), QPoint(40, 40)); // blackrect is at 50,50
        QCOMPARE(blackRect->scale(), 1.0);
        QVERIFY(!root->property("pinchActive").toBool());
    }
}

void tst_QQuickPinchHandler::transformedpinchHandler_data()
{
    QTest::addColumn<QPoint>("p0");
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<bool>("shouldPinch");

    QTest::newRow("checking inner pinchHandler 1")
        << QPoint(200, 140) << QPoint(200, 260) << true;

    QTest::newRow("checking inner pinchHandler 2")
        << QPoint(140, 200) << QPoint(200, 140) << true;

    QTest::newRow("checking inner pinchHandler 3")
        << QPoint(140, 200) << QPoint(260, 200) << true;

    QTest::newRow("checking outer pinchHandler 1")
        << QPoint(140, 140) << QPoint(260, 260) << false;

    QTest::newRow("checking outer pinchHandler 2")
        << QPoint(140, 140) << QPoint(200, 200) << false;

    QTest::newRow("checking outer pinchHandler 3")
        << QPoint(140, 260) << QPoint(260, 260) << false;
}

void tst_QQuickPinchHandler::transformedpinchHandler()
{
    QFETCH(QPoint, p0);
    QFETCH(QPoint, p1);
    QFETCH(bool, shouldPinch);

    QQuickView *view = QQuickViewTestUtils::createView();
    QScopedPointer<QQuickView> scope(view);
    view->setSource(testFileUrl("transformedPinchHandler.qml"));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchHandler *pinchHandler = view->rootObject()->findChild<QQuickPinchHandler*>("pinchHandler");
    QVERIFY(pinchHandler != nullptr);

    const int threshold = qApp->styleHints()->startDragDistance();

    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(view, touchscreen.get());
        // start pinchHandler
        pinchSequence.press(0, p0, view).commit();
        QQuickTouchUtils::flush(view);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p1, view).commit();
        QQuickTouchUtils::flush(view);

        // we move along the line that the two points form.
        // The distance we move should be above the threshold (threshold * 2 to be safe)
        QVector2D delta(p1 - p0);
        delta.normalize();
        QVector2D movement = delta * (threshold * 2);
        pinchSequence.stationary(0).move(1, p1 + movement.toPoint(), view).commit();
        QQuickTouchUtils::flush(view);
        QCOMPARE(pinchHandler->active(), shouldPinch);

        // release pinchHandler
        pinchSequence.release(0, p0, view).release(1, p1, view).commit();
        QQuickTouchUtils::flush(view);
        QCOMPARE(pinchHandler->active(), false);
    }
}

void tst_QQuickPinchHandler::dragVsPinch_data()
{
    // ptId is QEventPoint::id and also a 1-based index:
    // 1, 2, 3 activate DragHandlers; 4, 5 are for the PinchHandler.
    QTest::addColumn<int>("ptId1");
    QTest::addColumn<int>("ptId2");
    QTest::addColumn<QQuickPointerHandler::GrabPermission>("pinchGrabPermission");
    QTest::addColumn<int>("expectedPinchActivations");

    QTest::newRow("top two DragHandlers")
            << 1 << 2 << QQuickPointerHandler::TakeOverForbidden << 0;
    QTest::newRow("different DragHandlers")
            << 2 << 3 << QQuickPointerHandler::TakeOverForbidden << 0;
    QTest::newRow("one on DH, one on PH, TakeOverForbidden")
            << 3 << 4 << QQuickPointerHandler::TakeOverForbidden << 0;
    QTest::newRow("one on DH, one on PH, CanTakeOverFromAnything")
            << 3 << 4 << QQuickPointerHandler::CanTakeOverFromAnything << 2;
    QTest::newRow("both on PH")
            << 4 << 5 << QQuickPointerHandler::TakeOverForbidden << 2;
}

void tst_QQuickPinchHandler::dragVsPinch()
{
    QFETCH(int, ptId1);
    QFETCH(int, ptId2);
    QFETCH(QQuickPointerHandler::GrabPermission, pinchGrabPermission);
    QFETCH(int, expectedPinchActivations);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pinchAndDragHandlers.qml")));
    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root);
    QQuickPinchHandler *pinchHandler = root->findChild<QQuickPinchHandler*>();
    QVERIFY(pinchHandler);
    pinchHandler->setGrabPermissions(pinchGrabPermission);
    QSignalSpy pinchActiveSpy(pinchHandler, &QQuickPinchHandler::activeChanged);
    QQuickMultiPointHandler *dh1 = root->findChild<QQuickMultiPointHandler*>("dh1");
    QVERIFY(dh1);
    QQuickMultiPointHandler *dh2 = root->findChild<QQuickMultiPointHandler*>("dh2");
    QVERIFY(dh2);
    QQuickMultiPointHandler *dh3 = root->findChild<QQuickMultiPointHandler*>("dh3");
    QVERIFY(dh3);
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    const QPoint rect1pos = dh1->parentItem()->position().toPoint();
    const QPoint rect2pos = dh2->parentItem()->position().toPoint();
    const QPoint rect3pos = dh3->parentItem()->position().toPoint();
    const QPoint off = {10, 10}; // how far to press inside
    const QList<QPoint> rectPos = {rect1pos, rect2pos, rect3pos};
    const QList<QPoint> pointPos = {rect1pos + off, rect2pos + off, rect3pos + off,
                                     rect2pos + QPoint(0, dh2->parentItem()->height() + 10),
                                     rect3pos + QPoint(0, dh3->parentItem()->height() + 10)};
    const QList<QQuickMultiPointHandler *> handlers = {dh1, dh2, dh3, pinchHandler, pinchHandler};

    // press two points, one in each DragHandler's parent Rectangle
    QPoint p1 = pointPos[ptId1 - 1];
    QPoint p2 = pointPos[ptId2 - 1];
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&window, touchscreen.get());
    pinchSequence.press(ptId1, p1, &window).press(ptId2, p2, &window).commit();
    QQuickTouchUtils::flush(&window);

    qCDebug(lcPointerTests) << "press pts" << p1 << p2;
    // drag outwards horizontally
    for (int i = 1; i <= 4; ++i) {
        p1 -= QPoint(dragThreshold, 0);
        p2 += QPoint(dragThreshold, 0);
        if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
        pinchSequence.move(ptId1, p1, &window).move(ptId2, p2, &window).commit();
        QQuickTouchUtils::flush(&window);
        qCDebug(lcPointerTests) << i << "active" << dh1->active() << dh2->active() << dh3->active() << pinchHandler->active() << "pts" << p1 << p2
                                << "rects @" << dh1->parentItem()->position() << dh2->parentItem()->position() << dh3->parentItem()->position();
        if (i > 1 && !expectedPinchActivations) {
            // We don't expect the PinchHandler to be active.  Check which DragHandlers are active.
            if (ptId1 <= 3) {
                QVERIFY(handlers[ptId1 - 1]->active());
                QCOMPARE(handlers[ptId1 - 1]->parentItem()->position().x(), rectPos[ptId1 - 1].x() - dragThreshold * i);
            }
            if (ptId2 <= 3) {
                QVERIFY(handlers[ptId2 - 1]->active());
                QCOMPARE(handlers[ptId2 - 1]->parentItem()->position().x(), rectPos[ptId2 - 1].x() + dragThreshold * i);
            }
        }
    }
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    pinchSequence.release(ptId1, p1, &window).release(ptId2, p2, &window).commit();
    // whether PinchHandler is activated depends on pinchHandler.grabPermissions
    // and whether DragHandlers handle either or both points
    QCOMPARE(pinchActiveSpy.size(), expectedPinchActivations);
}

void tst_QQuickPinchHandler::pinchStartPos_data()
{
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<QPoint>("p2");
    QTest::addColumn<bool>("shouldPinch");

    QTest::newRow("both start outside")
            << QPoint(45, 45) << QPoint(155, 155) << false;
    QTest::newRow("one starts outside")
            << QPoint(55, 55) << QPoint(155, 155) << false;
    QTest::newRow("both start inside")
            << QPoint(55, 55) << QPoint(145, 145) << true;
}

void tst_QQuickPinchHandler::pinchStartPos()
{
    QFETCH(QPoint, p1);
    QFETCH(QPoint, p2);
    QFETCH(bool, shouldPinch);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pinchproperties.qml")));
    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root);
    QQuickPinchHandler *pinchHandler = root->findChild<QQuickPinchHandler*>();
    QVERIFY(pinchHandler);
    QSignalSpy activeSpy(pinchHandler, &QQuickPinchHandler::activeChanged);
    const QList<QPoint> pointPos = {{40, 40}, {160, 160}};
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();

    // press two points, inside or outside the black Rectangle
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&window, touchscreen.get());
    pinchSequence.press(1, p1, &window).press(2, p2, &window).commit();
    QQuickTouchUtils::flush(&window);

    // drag inwards and check whether PinchHandler activates
    for (int i = 1; i <= 4; ++i) {
        p1 += QPoint(dragThreshold, dragThreshold);
        p2 -= QPoint(dragThreshold, dragThreshold);
        if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
        pinchSequence.move(1, p1, &window).move(2, p2, &window).commit();
        QQuickTouchUtils::flush(&window);
        qCDebug(lcPointerTests) << i << "active" << pinchHandler->active() << "pts" << p1 << p2;
        if (!shouldPinch)
            QCOMPARE(pinchHandler->active(), false);
    }
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    pinchSequence.release(1, p1, &window).release(2, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(activeSpy.size(), shouldPinch ? 2 : 0);
}

QTEST_MAIN(tst_QQuickPinchHandler)

#include "tst_qquickpinchhandler.moc"
