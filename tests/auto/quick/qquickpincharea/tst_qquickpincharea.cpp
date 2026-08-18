// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/private/qvariantanimation_p.h>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtGui/QStyleHints>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qquickpincharea_p.h>
#include <QtQuick/private/qquickpathview_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickPinchArea: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickPinchArea() : QQmlDataTest(QT_QMLTEST_DATADIR) { }
private slots:
    void cleanupTestCase();
    void pinchProperties();
    void scale();
    void pan();
    void retouch();
    void cancel();
    void transformedPinchArea_data();
    void transformedPinchArea();
    void dragTransformedPinchArea_data();
    void dragTransformedPinchArea();
    void pinchAreaKeepsDragInView();
    void pinchInPathView();

private:
    QQuickView *createView();
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
};

void tst_QQuickPinchArea::cleanupTestCase()
{

}
void tst_QQuickPinchArea::pinchProperties()
{
    QScopedPointer<QQuickView> window(createView());
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(window->rootObject() != nullptr);

    QQuickPinchArea *pinchArea = window->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != nullptr);
    QVERIFY(pinch != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, pinch->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(pinch, SIGNAL(targetChanged()));
    pinch->setTarget(rootItem);
    QCOMPARE(targetSpy.size(),1);
    pinch->setTarget(rootItem);
    QCOMPARE(targetSpy.size(),1);

    // axis
    QCOMPARE(pinch->axis(), QQuickPinch::XAndYAxis);
    QSignalSpy axisSpy(pinch, SIGNAL(dragAxisChanged()));
    pinch->setAxis(QQuickPinch::XAxis);
    QCOMPARE(pinch->axis(), QQuickPinch::XAxis);
    QCOMPARE(axisSpy.size(),1);
    pinch->setAxis(QQuickPinch::XAxis);
    QCOMPARE(axisSpy.size(),1);

    // minimum and maximum drag properties
    QSignalSpy xminSpy(pinch, SIGNAL(minimumXChanged()));
    QSignalSpy xmaxSpy(pinch, SIGNAL(maximumXChanged()));
    QSignalSpy yminSpy(pinch, SIGNAL(minimumYChanged()));
    QSignalSpy ymaxSpy(pinch, SIGNAL(maximumYChanged()));

    QCOMPARE(pinch->xmin(), 0.0);
    QCOMPARE(pinch->xmax(), rootItem->width()-blackRect->width());
    QCOMPARE(pinch->ymin(), 0.0);
    QCOMPARE(pinch->ymax(), rootItem->height()-blackRect->height());

    pinch->setXmin(10);
    pinch->setXmax(10);
    pinch->setYmin(10);
    pinch->setYmax(10);

    QCOMPARE(pinch->xmin(), 10.0);
    QCOMPARE(pinch->xmax(), 10.0);
    QCOMPARE(pinch->ymin(), 10.0);
    QCOMPARE(pinch->ymax(), 10.0);

    QCOMPARE(xminSpy.size(),1);
    QCOMPARE(xmaxSpy.size(),1);
    QCOMPARE(yminSpy.size(),1);
    QCOMPARE(ymaxSpy.size(),1);

    pinch->setXmin(10);
    pinch->setXmax(10);
    pinch->setYmin(10);
    pinch->setYmax(10);

    QCOMPARE(xminSpy.size(),1);
    QCOMPARE(xmaxSpy.size(),1);
    QCOMPARE(yminSpy.size(),1);
    QCOMPARE(ymaxSpy.size(),1);

    // minimum and maximum scale properties
    QSignalSpy scaleMinSpy(pinch, SIGNAL(minimumScaleChanged()));
    QSignalSpy scaleMaxSpy(pinch, SIGNAL(maximumScaleChanged()));

    QCOMPARE(pinch->minimumScale(), 1.0);
    QCOMPARE(pinch->maximumScale(), 2.0);

    pinch->setMinimumScale(0.5);
    pinch->setMaximumScale(1.5);

    QCOMPARE(pinch->minimumScale(), 0.5);
    QCOMPARE(pinch->maximumScale(), 1.5);

    QCOMPARE(scaleMinSpy.size(),1);
    QCOMPARE(scaleMaxSpy.size(),1);

    pinch->setMinimumScale(0.5);
    pinch->setMaximumScale(1.5);

    QCOMPARE(scaleMinSpy.size(),1);
    QCOMPARE(scaleMaxSpy.size(),1);

    // minimum and maximum rotation properties
    QSignalSpy rotMinSpy(pinch, SIGNAL(minimumRotationChanged()));
    QSignalSpy rotMaxSpy(pinch, SIGNAL(maximumRotationChanged()));

    QCOMPARE(pinch->minimumRotation(), 0.0);
    QCOMPARE(pinch->maximumRotation(), 90.0);

    pinch->setMinimumRotation(-90.0);
    pinch->setMaximumRotation(45.0);

    QCOMPARE(pinch->minimumRotation(), -90.0);
    QCOMPARE(pinch->maximumRotation(), 45.0);

    QCOMPARE(rotMinSpy.size(),1);
    QCOMPARE(rotMaxSpy.size(),1);

    pinch->setMinimumRotation(-90.0);
    pinch->setMaximumRotation(45.0);

    QCOMPARE(rotMinSpy.size(),1);
    QCOMPARE(rotMaxSpy.size(),1);
}

QEventPoint makeTouchPoint(int id, QPoint p, QQuickView *v, QQuickItem *i)
{
    QEventPoint touchPoint(id);
    QMutableEventPoint::setPosition(touchPoint, i->mapFromScene(p));
    QMutableEventPoint::setGlobalPosition(touchPoint, v->mapToGlobal(p));
    QMutableEventPoint::setScenePosition(touchPoint, p);
    return touchPoint;
}

void tst_QQuickPinchArea::scale()
{
    QQuickView *window = createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = window->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != nullptr);
    QVERIFY(pinch != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p1(80, 80);
    QPoint p2(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and PinchArea will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1,window).move(1, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QVERIFY(root->property("pinchActive").toBool());

        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1,window).move(1, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.5);
        QCOMPARE(root->property("center").toPointF(), QPointF(40, 40)); // blackrect is at 50,50
        QCOMPARE(blackRect->scale(), 1.5);
    }

    // scale beyond bound
    p1 -= QPoint(50,50);
    p2 += QPoint(50,50);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(blackRect->scale(), 2.0);
        pinchSequence.release(0, p1, window).release(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
    }
    QVERIFY(!root->property("pinchActive").toBool());
}

void tst_QQuickPinchArea::pan()
{
    QQuickView *window = createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = window->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != nullptr);
    QVERIFY(pinch != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p1(80, 80);
    QPoint p2(100, 100);
    {
        const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(root->property("scale").toReal(), -1.0);

        p1 += QPoint(dragThreshold - 1, 0);
        p2 += QPoint(dragThreshold - 1, 0);
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        // movement < dragThreshold: pinch not yet active
        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(root->property("scale").toReal(), -1.0);

        // exactly the dragThreshold: pinch starts
        p1 += QPoint(1, 0);
        p2 += QPoint(1, 0);
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        QVERIFY(root->property("pinchActive").toBool());
        QCOMPARE(root->property("scale").toReal(), 1.0);

        // Calculation of the center point is tricky at first:
        // center point of the two touch points in item coordinates:
        // scene coordinates: (80, 80) + (dragThreshold, 0), (100, 100) + (dragThreshold, 0)
        //                    = ((180+dT)/2, 180/2) = (90+dT, 90)
        // item  coordinates: (scene) - (50, 50) = (40+dT, 40)
        QCOMPARE(root->property("center").toPointF(), QPointF(40 + dragThreshold, 40));
        // pan started, but no actual movement registered yet:
        // blackrect starts at 50,50
        QCOMPARE(blackRect->x(), 50.0);
        QCOMPARE(blackRect->y(), 50.0);

        p1 += QPoint(10, 0);
        p2 += QPoint(10, 0);
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        QCOMPARE(root->property("center").toPointF(), QPointF(40 + 10 + dragThreshold, 40));
        QCOMPARE(blackRect->x(), 60.0);
        QCOMPARE(blackRect->y(), 50.0);

        p1 += QPoint(0, 10);
        p2 += QPoint(0, 10);
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        // next big surprise: the center is in item local coordinates and the item was just
        // moved 10 to the right... which offsets the center point 10 to the left
        QCOMPARE(root->property("center").toPointF(), QPointF(40 + 10 - 10 + dragThreshold, 40 + 10));
        QCOMPARE(blackRect->x(), 60.0);
        QCOMPARE(blackRect->y(), 60.0);

        p1 += QPoint(10, 10);
        p2 += QPoint(10, 10);
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        // now the item moved again, thus the center point of the touch is moved in total by (10, 10)
        QCOMPARE(root->property("center").toPointF(), QPointF(50 + dragThreshold, 50));
        QCOMPARE(blackRect->x(), 70.0);
        QCOMPARE(blackRect->y(), 70.0);
    }

    // pan x beyond bound
    p1 += QPoint(100,100);
    p2 += QPoint(100,100);
    QTest::touchEvent(window, touchscreen.get()).move(0, p1, window).move(1, p2, window);
    QQuickTouchUtils::flush(window);

    QCOMPARE(blackRect->x(), 140.0);
    QCOMPARE(blackRect->y(), 170.0);

    QTest::touchEvent(window, touchscreen.get()).release(0, p1, window).release(1, p2, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!root->property("pinchActive").toBool());
}

// test pinch, release one point, touch again to continue pinch
void tst_QQuickPinchArea::retouch()
{
    QQuickView *window = createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = window->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != nullptr);
    QVERIFY(pinch != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    QSignalSpy startedSpy(pinchArea, SIGNAL(pinchStarted(QQuickPinchEvent*)));
    QSignalSpy finishedSpy(pinchArea, SIGNAL(pinchFinished(QQuickPinchEvent*)));

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p1(80, 80);
    QPoint p2(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1,window).move(1, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QVERIFY(root->property("pinchActive").toBool());

        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1,window).move(1, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(startedSpy.size(), 1);

        QCOMPARE(root->property("scale").toReal(), 1.5);
        QCOMPARE(root->property("center").toPointF(), QPointF(40, 40)); // blackrect is at 50,50
        QCOMPARE(blackRect->scale(), 1.5);

        QCOMPARE(window->rootObject()->property("pointCount").toInt(), 2);

        QCOMPARE(startedSpy.size(), 1);
        QCOMPARE(finishedSpy.size(), 0);

        // Hold down the first finger but release the second one
        pinchSequence.stationary(0).release(1, p2, window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(startedSpy.size(), 1);
        QCOMPARE(finishedSpy.size(), 0);

        QCOMPARE(window->rootObject()->property("pointCount").toInt(), 1);

        // Keep holding down the first finger and re-touch the second one, then move them both
        pinchSequence.stationary(0).press(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1, window).move(1, p2, window).commit();
        QQuickTouchUtils::flush(window);

        // Lifting and retouching results in onPinchStarted being called again
        QCOMPARE(startedSpy.size(), 2);
        QCOMPARE(finishedSpy.size(), 0);

        QCOMPARE(window->rootObject()->property("pointCount").toInt(), 2);

        pinchSequence.release(0, p1, window).release(1, p2, window).commit();
        QQuickTouchUtils::flush(window);

        QVERIFY(!root->property("pinchActive").toBool());
        QCOMPARE(startedSpy.size(), 2);
        QCOMPARE(finishedSpy.size(), 1);
    }
}

void tst_QQuickPinchArea::cancel()
{
    QQuickView *window = createView();
    QScopedPointer<QQuickView> scope(window);
    window->setSource(testFileUrl("pinchproperties.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = window->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != nullptr);
    QVERIFY(pinch != nullptr);

    QQuickItem *root = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(root != nullptr);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);

    QPoint p1(80, 80);
    QPoint p2(100, 100);
    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(window, touchscreen.get());
        pinchSequence.press(0, p1, window).commit();
        QQuickTouchUtils::flush(window);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.  Otherwise if we let it
        // be destroyed and then start a new sequence, point 0 will default to being
        // stationary at 0, 0, and PinchArea will filter out that touchpoint because
        // it is outside its bounds.
        pinchSequence.stationary(0).press(1, p2, window).commit();
        QQuickTouchUtils::flush(window);
        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1,window).move(1, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QVERIFY(root->property("pinchActive").toBool());

        p1 -= QPoint(10,10);
        p2 += QPoint(10,10);
        pinchSequence.move(0, p1,window).move(1, p2,window).commit();
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.5);
        QCOMPARE(root->property("center").toPointF(), QPointF(40, 40)); // blackrect is at 50,50
        QCOMPARE(blackRect->scale(), 1.5);

        QTouchEvent cancelEvent(QEvent::TouchCancel, touchscreen.get());
        QCoreApplication::sendEvent(window, &cancelEvent);
        QQuickTouchUtils::flush(window);

        QCOMPARE(root->property("scale").toReal(), 1.0);
        QCOMPARE(root->property("center").toPointF(), QPointF(40, 40)); // blackrect is at 50,50
        QCOMPARE(blackRect->scale(), 1.0);
        QVERIFY(!root->property("pinchActive").toBool());
    }
}

void tst_QQuickPinchArea::transformedPinchArea_data()
{
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<QPoint>("p2");
    QTest::addColumn<bool>("shouldPinch");

    QTest::newRow("checking inner pinch 1")
        << QPoint(200, 140) << QPoint(200, 260) << true;

    QTest::newRow("checking inner pinch 2")
        << QPoint(140, 200) << QPoint(200, 140) << true;

    QTest::newRow("checking inner pinch 3")
        << QPoint(140, 200) << QPoint(260, 200) << true;

    QTest::newRow("checking outer pinch 1")
        << QPoint(140, 140) << QPoint(260, 260) << false;

    QTest::newRow("checking outer pinch 2")
        << QPoint(140, 140) << QPoint(200, 200) << false;

    QTest::newRow("checking outer pinch 3")
        << QPoint(140, 260) << QPoint(260, 260) << false;
}

void tst_QQuickPinchArea::transformedPinchArea()
{
    QFETCH(QPoint, p1);
    QFETCH(QPoint, p2);
    QFETCH(bool, shouldPinch);

    QQuickView *view = createView();
    QScopedPointer<QQuickView> scope(view);
    view->setSource(testFileUrl("transformedPinchArea.qml"));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view->rootObject() != nullptr);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = view->rootObject()->findChild<QQuickPinchArea*>("pinchArea");
    QVERIFY(pinchArea != nullptr);

    const int threshold = qApp->styleHints()->startDragDistance();

    {
        QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(view, touchscreen.get());
        // start pinch
        pinchSequence.press(0, p1, view).commit();
        QQuickTouchUtils::flush(view);
        // In order for the stationary point to remember its previous position,
        // we have to reuse the same pinchSequence object.
        pinchSequence.stationary(0).press(1, p2, view).commit();
        QQuickTouchUtils::flush(view);
        pinchSequence.stationary(0).move(1, p2 + QPoint(threshold * 2, 0), view).commit();
        QQuickTouchUtils::flush(view);
        QCOMPARE(pinchArea->property("pinching").toBool(), shouldPinch);

        // release pinch
        pinchSequence.release(0, p1, view).release(1, p2, view).commit();
        QQuickTouchUtils::flush(view);
        QCOMPARE(pinchArea->property("pinching").toBool(), false);
    }
}

void tst_QQuickPinchArea::dragTransformedPinchArea_data()
{
    QTest::addColumn<int>("rotation");
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<QPoint>("p2");
    QTest::addColumn<QPoint>("delta");

    QTest::newRow("unrotated")
            << 0 << QPoint(100, 100) << QPoint(200, 100) << QPoint(40, 40);
    QTest::newRow("20 deg")
            << 20 << QPoint(100, 100) << QPoint(200, 100) << QPoint(40, 40);
    QTest::newRow("90 deg")
            << 90 << QPoint(100, 100) << QPoint(200, 100) << QPoint(0, 40);
    QTest::newRow("180 deg")
            << 180 << QPoint(100, 100) << QPoint(200, 100) << QPoint(40, 0);
    QTest::newRow("225 deg")
            << 210 << QPoint(200, 200) << QPoint(300, 200) << QPoint(80, 80);
}

void tst_QQuickPinchArea::dragTransformedPinchArea() // QTBUG-63673
{
    QFETCH(int, rotation);
    QFETCH(QPoint, p1);
    QFETCH(QPoint, p2);
    QFETCH(QPoint, delta);
    const int threshold = qApp->styleHints()->startDragDistance();

    QQuickView *view = createView();
    QScopedPointer<QQuickView> scope(view);
    view->setSource(testFileUrl("draggablePinchArea.qml"));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view->rootObject());
    QQuickPinchArea *pinchArea = view->rootObject()->findChild<QQuickPinchArea*>();
    QVERIFY(pinchArea);
    QQuickItem *pinchAreaTarget = pinchArea->parentItem();
    QVERIFY(pinchAreaTarget);
    QQuickItem *pinchAreaContainer = pinchAreaTarget->parentItem();
    QVERIFY(pinchAreaContainer);
    pinchAreaContainer->setRotation(rotation);

    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(view, touchscreen.get());
    // start pinch
    pinchSequence.press(1, pinchArea->mapToScene(p1).toPoint(), view)
                 .press(2, pinchArea->mapToScene(p2).toPoint(), view).commit();
    QQuickTouchUtils::flush(view);
    pinchSequence.move(1, pinchArea->mapToScene(p1 + QPoint(threshold, threshold)).toPoint(), view)
                 .move(2, pinchArea->mapToScene(p2 + QPoint(threshold, threshold)).toPoint(), view).commit();
    QQuickTouchUtils::flush(view);
    pinchSequence.move(1, pinchArea->mapToScene(p1 + delta).toPoint(), view)
                 .move(2, pinchArea->mapToScene(p2 + delta).toPoint(), view).commit();
    QQuickTouchUtils::flush(view);
    QCOMPARE(pinchArea->pinch()->active(), true);
    auto error = delta - QPoint(threshold, threshold) -
                pinchAreaTarget->position().toPoint(); // expect 0, 0
    QVERIFY(qAbs(error.x()) <= 1);
    QVERIFY(qAbs(error.y()) <= 1);

    // release pinch
    pinchSequence.release(1, p1, view).release(2, p2, view).commit();
    QQuickTouchUtils::flush(view);
    QCOMPARE(pinchArea->pinch()->active(), false);
}

// QTBUG-105058
void tst_QQuickPinchArea::pinchAreaKeepsDragInView()
{
    QQuickView view;
    view.setSource(testFileUrl("pinchAreaInPathView.qml"));
    QVERIFY(view.rootObject());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickPathView *pathView = qobject_cast<QQuickPathView*>(view.rootObject());
    QVERIFY(pathView);
    QCOMPARE(pathView->count(), 3);

    const QQuickItem *pinchDelegateItem = pathView->itemAtIndex(0);
    QQuickPinchArea *pinchArea = pinchDelegateItem->property("pinchArea").value<QQuickPinchArea*>();
    QVERIFY(pinchArea);

    // Press.
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&view, touchscreen.get());
    QPoint point1Start = { 80, 120 };
    QPoint point2Start = { 120, 80 };
    const int dragThreshold = qApp->styleHints()->startDragDistance();
    pinchSequence.press(1, pinchArea->mapToScene(point1Start).toPoint(), &view)
                 .press(2, pinchArea->mapToScene(point2Start).toPoint(), &view).commit();
    QQuickTouchUtils::flush(&view);

    // Move past the drag threshold to begin the pinch.
    const int steps = 30;
    QPoint point1End = point1Start + QPoint(-dragThreshold, dragThreshold);
    QPoint point2End = point2Start + QPoint(dragThreshold, -dragThreshold);

    forEachStep(steps, [&](qreal progress) {
        pinchSequence.move(1, lerpPoints(point1Start, point1End, progress), &view)
                     .move(2, lerpPoints(point2Start, point2End, progress), &view).commit();
        QQuickTouchUtils::flush(&view);
        QTest::qWait(5);
    });
    QCOMPARE(pinchArea->pinch()->active(), true);
    QVERIFY2(pinchDelegateItem->scale() > 1.0, qPrintable(QString::number(pinchDelegateItem->scale())));
    // The PathView contents shouldn't have moved.
    QCOMPARE(pathView->offset(), 0);

    // Release a touch point.
    pinchSequence.stationary(1).release(2, point2End, &view).commit();
    QQuickTouchUtils::flush(&view);

    // Press it again.
    pinchSequence.stationary(1).press(2, point2End, &view).commit();
    QQuickTouchUtils::flush(&view);
    QCOMPARE(pinchArea->pinch()->active(), true);

    // Drag to the right; the PathView still shouldn't move.
    point1Start = point1End;
    point2Start = point2End;
    point1End = point1Start + QPoint(100, 0);
    point2End = point2Start + QPoint(100, 0);
    forEachStep(steps, [&](qreal progress) {
        pinchSequence.move(1, lerpPoints(point1Start, point1End, progress), &view)
                     .move(2, lerpPoints(point2Start, point2End, progress), &view).commit();
        QQuickTouchUtils::flush(&view);
        QTest::qWait(5);
    });
    QCOMPARE(pinchArea->pinch()->active(), true);
    QVERIFY2(pinchDelegateItem->scale() > 1.0, qPrintable(QString::number(pinchDelegateItem->scale())));
    QCOMPARE(pathView->offset(), 0);

    // Release pinch.
    pinchSequence.release(1, point1End, &view).release(2, point2End, &view).commit();
    QQuickTouchUtils::flush(&view);
    QCOMPARE(pinchArea->pinch()->active(), false);
    QCOMPARE(pathView->offset(), 0);
}

void tst_QQuickPinchArea::pinchInPathView()
{
    QQuickView view;
    view.setSource(testFileUrl("pinchAreaInPathView.qml"));
    QVERIFY(view.rootObject());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickPathView *pathView = qobject_cast<QQuickPathView*>(view.rootObject());
    QVERIFY(pathView);
    QCOMPARE(pathView->count(), 3);

    const QQuickItem *pinchDelegateItem = pathView->itemAtIndex(0);
    QQuickPinchArea *pinchArea = pinchDelegateItem->property("pinchArea").value<QQuickPinchArea*>();
    QVERIFY(pinchArea);

    // press
    QTest::QTouchEventSequence pinchSequence = QTest::touchEvent(&view, touchscreen.get());
    QPoint point1Start = { 10, 10 };
    QPoint point2Start = { 100, 100 };
    const int dragThreshold = qApp->styleHints()->startDragDistance();
    pinchSequence.press(0, point1Start, &view)
                 .press(1, point2Start, &view)
                 .commit();
    QQuickTouchUtils::flush(&view);
    QTest::qWait(20);

    // move
    QPoint moveDistance = QPoint(dragThreshold * 3, dragThreshold * 3);
    QPoint point2End = point2Start + moveDistance;
    pinchSequence.stationary(0)
                 .move(1, point2End, &view)
                 .commit();
    QQuickTouchUtils::flush(&view);
    QTest::qWait(20);

    point2End += moveDistance;
    pinchSequence.stationary(0)
                 .move(1, point2End, &view)
                 .commit();
    QQuickTouchUtils::flush(&view);
    QTest::qWait(20);

    QCOMPARE(pinchArea->pinch()->active(), true);
    QVERIFY2(pinchDelegateItem->scale() > 1.0, qPrintable(QString::number(pinchDelegateItem->scale())));
    // PathView shouldn't have moved.
    QCOMPARE(pathView->offset(), 0);

    // release pinch.
    pinchSequence.release(0, point1Start, &view).release(1, point2End, &view).commit();
    QQuickTouchUtils::flush(&view);
    QCOMPARE(pinchArea->pinch()->active(), false);
    QCOMPARE(pathView->offset(), 0);
}

QQuickView *tst_QQuickPinchArea::createView()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setGeometry(0,0,240,320);

    return window;
}

QTEST_MAIN(tst_QQuickPinchArea)

#include "tst_qquickpincharea.moc"
