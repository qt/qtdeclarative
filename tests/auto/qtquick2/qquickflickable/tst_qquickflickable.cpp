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
#include <QtTest/QSignalSpy>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickflickable_p.h>
#include <private/qquickflickable_p_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <math.h>
#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"
#include <QtOpenGL/QGLShaderProgram>

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

class tst_qquickflickable : public QDeclarativeDataTest
{
    Q_OBJECT
public:

private slots:
    void create();
    void horizontalViewportSize();
    void verticalViewportSize();
    void properties();
    void boundsBehavior();
    void maximumFlickVelocity();
    void flickDeceleration();
    void pressDelay();
    void nestedPressDelay();
    void flickableDirection();
    void resizeContent();
    void returnToBounds();
    void wheel();
    void movingAndDragging();
    void disabled();
    void flickVelocity();
    void margins();

private:
    QDeclarativeEngine engine;
};

void tst_qquickflickable::create()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("flickable01.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != 0);
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
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("flickable02.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != 0);
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
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("flickable03.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentWidth(), 200.);
    QCOMPARE(obj->contentHeight(), 6000.);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);

    delete obj;
}

void tst_qquickflickable::properties()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("flickable04.qml"));
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->isInteractive(), false);
    QCOMPARE(obj->boundsBehavior(), QQuickFlickable::StopAtBounds);
    QCOMPARE(obj->pressDelay(), 200);
    QCOMPARE(obj->maximumFlickVelocity(), 2000.);

    QVERIFY(obj->property("ok").toBool() == false);
    QMetaObject::invokeMethod(obj, "check");
    QVERIFY(obj->property("ok").toBool() == true);

    delete obj;
}

void tst_qquickflickable::boundsBehavior()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { boundsBehavior: Flickable.StopAtBounds }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(boundsBehaviorChanged()));

    QVERIFY(flickable);
    QVERIFY(flickable->boundsBehavior() == QQuickFlickable::StopAtBounds);

    flickable->setBoundsBehavior(QQuickFlickable::DragAndOvershootBounds);
    QVERIFY(flickable->boundsBehavior() == QQuickFlickable::DragAndOvershootBounds);
    QCOMPARE(spy.count(),1);
    flickable->setBoundsBehavior(QQuickFlickable::DragAndOvershootBounds);
    QCOMPARE(spy.count(),1);

    flickable->setBoundsBehavior(QQuickFlickable::DragOverBounds);
    QVERIFY(flickable->boundsBehavior() == QQuickFlickable::DragOverBounds);
    QCOMPARE(spy.count(),2);
    flickable->setBoundsBehavior(QQuickFlickable::DragOverBounds);
    QCOMPARE(spy.count(),2);

    flickable->setBoundsBehavior(QQuickFlickable::StopAtBounds);
    QVERIFY(flickable->boundsBehavior() == QQuickFlickable::StopAtBounds);
    QCOMPARE(spy.count(),3);
    flickable->setBoundsBehavior(QQuickFlickable::StopAtBounds);
    QCOMPARE(spy.count(),3);
}

void tst_qquickflickable::maximumFlickVelocity()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { maximumFlickVelocity: 1.0; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(maximumFlickVelocityChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->maximumFlickVelocity(), 1.0);

    flickable->setMaximumFlickVelocity(2.0);
    QCOMPARE(flickable->maximumFlickVelocity(), 2.0);
    QCOMPARE(spy.count(),1);
    flickable->setMaximumFlickVelocity(2.0);
    QCOMPARE(spy.count(),1);
}

void tst_qquickflickable::flickDeceleration()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { flickDeceleration: 1.0; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(flickDecelerationChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->flickDeceleration(), 1.0);

    flickable->setFlickDeceleration(2.0);
    QCOMPARE(flickable->flickDeceleration(), 2.0);
    QCOMPARE(spy.count(),1);
    flickable->setFlickDeceleration(2.0);
    QCOMPARE(spy.count(),1);
}

void tst_qquickflickable::pressDelay()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { pressDelay: 100; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(pressDelayChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->pressDelay(), 100);

    flickable->setPressDelay(200);
    QCOMPARE(flickable->pressDelay(), 200);
    QCOMPARE(spy.count(),1);
    flickable->setPressDelay(200);
    QCOMPARE(spy.count(),1);
}

// QTBUG-17361
void tst_qquickflickable::nestedPressDelay()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(testFileUrl("nestedPressDelay.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *outer = qobject_cast<QQuickFlickable*>(canvas->rootObject());
    QVERIFY(outer != 0);

    QQuickFlickable *inner = canvas->rootObject()->findChild<QQuickFlickable*>("innerFlickable");
    QVERIFY(inner != 0);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(150, 150));
    // the MouseArea is not pressed immediately
    QVERIFY(outer->property("pressed").toBool() == false);

    // The outer pressDelay will prevail (50ms, vs. 10sec)
    // QTRY_VERIFY() has 5sec timeout, so will timeout well within 10sec.
    QTRY_VERIFY(outer->property("pressed").toBool() == true);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(150, 150));

    delete canvas;
}

void tst_qquickflickable::flickableDirection()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; Flickable { flickableDirection: Flickable.VerticalFlick; }", QUrl::fromLocalFile(""));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(flickableDirectionChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::VerticalFlick);

    flickable->setFlickableDirection(QQuickFlickable::HorizontalAndVerticalFlick);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::HorizontalAndVerticalFlick);
    QCOMPARE(spy.count(),1);

    flickable->setFlickableDirection(QQuickFlickable::AutoFlickDirection);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::AutoFlickDirection);
    QCOMPARE(spy.count(),2);

    flickable->setFlickableDirection(QQuickFlickable::HorizontalFlick);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::HorizontalFlick);
    QCOMPARE(spy.count(),3);

    flickable->setFlickableDirection(QQuickFlickable::HorizontalFlick);
    QCOMPARE(flickable->flickableDirection(), QQuickFlickable::HorizontalFlick);
    QCOMPARE(spy.count(),3);
}

// QtQuick 1.1
void tst_qquickflickable::resizeContent()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("resize.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(c.create());
    QQuickFlickable *obj = findItem<QQuickFlickable>(root, "flick");

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);
    QCOMPARE(obj->contentWidth(), 300.);
    QCOMPARE(obj->contentHeight(), 300.);

    QMetaObject::invokeMethod(root, "resizeContent");

    QCOMPARE(obj->contentX(), 100.);
    QCOMPARE(obj->contentY(), 100.);
    QCOMPARE(obj->contentWidth(), 600.);
    QCOMPARE(obj->contentHeight(), 600.);

    delete root;
}

// QtQuick 1.1
void tst_qquickflickable::returnToBounds()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("resize.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(c.create());
    QQuickFlickable *obj = findItem<QQuickFlickable>(root, "flick");

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);
    QCOMPARE(obj->contentWidth(), 300.);
    QCOMPARE(obj->contentHeight(), 300.);

    obj->setContentX(100);
    obj->setContentY(400);
    QTRY_COMPARE(obj->contentX(), 100.);
    QTRY_COMPARE(obj->contentY(), 400.);

    QMetaObject::invokeMethod(root, "returnToBounds");

    QTRY_COMPARE(obj->contentX(), 0.);
    QTRY_COMPARE(obj->contentY(), 0.);

    delete root;
}

void tst_qquickflickable::wheel()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(testFileUrl("wheel.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flick = canvas->rootObject()->findChild<QQuickFlickable*>("flick");
    QVERIFY(flick != 0);

    {
        QWheelEvent event(QPoint(200, 200), -120, Qt::NoButton, Qt::NoModifier, Qt::Vertical);
        event.setAccepted(false);
        QGuiApplication::sendEvent(canvas, &event);
    }

    QTRY_VERIFY(flick->contentY() > 0);
    QVERIFY(flick->contentX() == 0);

    flick->setContentY(0);
    QVERIFY(flick->contentY() == 0);

    {
        QWheelEvent event(QPoint(200, 200), -120, Qt::NoButton, Qt::NoModifier, Qt::Horizontal);
        event.setAccepted(false);
        QGuiApplication::sendEvent(canvas, &event);
    }

    QTRY_VERIFY(flick->contentX() > 0);
    QVERIFY(flick->contentY() == 0);

    delete canvas;
}

void tst_qquickflickable::movingAndDragging()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(testFileUrl("flickable03.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(canvas->rootObject());
    QVERIFY(flickable != 0);

    QSignalSpy vDragSpy(flickable, SIGNAL(draggingVerticallyChanged()));
    QSignalSpy hDragSpy(flickable, SIGNAL(draggingHorizontallyChanged()));
    QSignalSpy dragSpy(flickable, SIGNAL(draggingChanged()));
    QSignalSpy vMoveSpy(flickable, SIGNAL(movingVerticallyChanged()));
    QSignalSpy hMoveSpy(flickable, SIGNAL(movingHorizontallyChanged()));
    QSignalSpy moveSpy(flickable, SIGNAL(movingChanged()));
    QSignalSpy dragStartSpy(flickable, SIGNAL(dragStarted()));
    QSignalSpy dragEndSpy(flickable, SIGNAL(dragEnded()));

    //Vertical
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50, 90));

    QTest::mouseMove(canvas, QPoint(50, 80));
    QTest::mouseMove(canvas, QPoint(50, 70));
    QTest::mouseMove(canvas, QPoint(50, 60));

    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(50, 80), Qt::LeftButton, Qt::LeftButton, 0);

    QVERIFY(!flickable->isDraggingHorizontally());
    QVERIFY(flickable->isDraggingVertically());
    QVERIFY(flickable->isDragging());
    QCOMPARE(vDragSpy.count(), 1);
    QCOMPARE(dragSpy.count(), 1);
    QCOMPARE(hDragSpy.count(), 0);
    QCOMPARE(dragStartSpy.count(), 1);
    QCOMPARE(dragEndSpy.count(), 0);

    QVERIFY(!flickable->isMovingHorizontally());
    QVERIFY(flickable->isMovingVertically());
    QVERIFY(flickable->isMoving());
    QCOMPARE(vMoveSpy.count(), 1);
    QCOMPARE(moveSpy.count(), 1);
    QCOMPARE(hMoveSpy.count(), 0);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50, 60));

    QTRY_VERIFY(!flickable->isDraggingVertically());
    QVERIFY(!flickable->isDragging());
    QCOMPARE(vDragSpy.count(), 2);
    QCOMPARE(dragSpy.count(), 2);
    QCOMPARE(hDragSpy.count(), 0);
    QCOMPARE(dragStartSpy.count(), 1);
    QCOMPARE(dragEndSpy.count(), 1);

    // wait for any motion to end
    QTRY_VERIFY(flickable->isMoving() == false);

    //Horizontal
    vDragSpy.clear();
    hDragSpy.clear();
    dragSpy.clear();
    vMoveSpy.clear();
    hMoveSpy.clear();
    moveSpy.clear();
    dragStartSpy.clear();
    dragEndSpy.clear();

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(90, 50));

    QTest::mouseMove(canvas, QPoint(80, 50));
    QTest::mouseMove(canvas, QPoint(70, 50));
    QTest::mouseMove(canvas, QPoint(60, 50));

    QVERIFY(!flickable->isDraggingVertically());
    QVERIFY(flickable->isDraggingHorizontally());
    QVERIFY(flickable->isDragging());
    QCOMPARE(vDragSpy.count(), 0);
    QCOMPARE(dragSpy.count(), 1);
    QCOMPARE(hDragSpy.count(), 1);
    QCOMPARE(dragStartSpy.count(), 1);
    QCOMPARE(dragEndSpy.count(), 0);

    QVERIFY(!flickable->isMovingVertically());
    QVERIFY(flickable->isMovingHorizontally());
    QVERIFY(flickable->isMoving());
    QCOMPARE(vMoveSpy.count(), 0);
    QCOMPARE(moveSpy.count(), 1);
    QCOMPARE(hMoveSpy.count(), 1);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(60, 50));

    QTRY_VERIFY(!flickable->isDraggingHorizontally());
    QVERIFY(!flickable->isDragging());
    QCOMPARE(vDragSpy.count(), 0);
    QCOMPARE(dragSpy.count(), 2);
    QCOMPARE(hDragSpy.count(), 2);
    QCOMPARE(dragStartSpy.count(), 1);
    QCOMPARE(dragEndSpy.count(), 1);
    // Don't test moving because a flick could occur

#ifdef Q_OS_MAC
    QSKIP("Producing flicks on Mac CI impossible due to timing problems");
#endif

    QTRY_VERIFY(!flickable->isMoving());

    vMoveSpy.clear();
    hMoveSpy.clear();
    moveSpy.clear();
    QSignalSpy vFlickSpy(flickable, SIGNAL(flickingVerticallyChanged()));
    QSignalSpy hFlickSpy(flickable, SIGNAL(flickingHorizontallyChanged()));
    QSignalSpy flickSpy(flickable, SIGNAL(flickingChanged()));

    // flick then press while it is still moving
    // flicking == false, moving == true;
    flick(canvas, QPoint(20,190), QPoint(20, 50), 200);
    QVERIFY(flickable->verticalVelocity() > 0.0);
    QVERIFY(flickable->isFlicking());
    QVERIFY(flickable->isFlickingVertically());
    QVERIFY(!flickable->isFlickingHorizontally());
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isMovingVertically());
    QVERIFY(!flickable->isMovingHorizontally());
    QCOMPARE(vMoveSpy.count(), 1);
    QCOMPARE(hMoveSpy.count(), 0);
    QCOMPARE(moveSpy.count(), 1);
    QCOMPARE(vFlickSpy.count(), 1);
    QCOMPARE(hFlickSpy.count(), 0);
    QCOMPARE(flickSpy.count(), 1);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(20, 50));
    QTRY_VERIFY(!flickable->isFlicking());
    QVERIFY(!flickable->isFlickingVertically());
    QVERIFY(flickable->isMoving());
    QVERIFY(flickable->isMovingVertically());

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(20,50));
    QVERIFY(!flickable->isFlicking());
    QVERIFY(!flickable->isFlickingVertically());
    QTRY_VERIFY(!flickable->isMoving());
    QVERIFY(!flickable->isMovingVertically());

    delete canvas;
}

void tst_qquickflickable::disabled()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(testFileUrl("disabled.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flick = canvas->rootObject()->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flick != 0);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50, 90));

    QTest::mouseMove(canvas, QPoint(50, 80));
    QTest::mouseMove(canvas, QPoint(50, 70));
    QTest::mouseMove(canvas, QPoint(50, 60));

    QVERIFY(flick->isMoving() == false);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50, 60));

    // verify that mouse clicks on other elements still work (QTBUG-20584)
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(50, 10));
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50, 10));

    QTRY_VERIFY(canvas->rootObject()->property("clicked").toBool() == true);
}

void tst_qquickflickable::flickVelocity()
{
#ifdef Q_OS_MAC
    QSKIP("Producing flicks on Mac CI impossible due to timing problems");
#endif

    QQuickView *canvas = new QQuickView;
    canvas->setSource(testFileUrl("flickable03.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(canvas->rootObject());
    QVERIFY(flickable != 0);

    // flick up
    flick(canvas, QPoint(20,190), QPoint(20, 50), 200);
    QVERIFY(flickable->verticalVelocity() > 0.0);
    QTRY_VERIFY(flickable->verticalVelocity() == 0.0);

    // flick down
    flick(canvas, QPoint(20,10), QPoint(20, 140), 200);
    QVERIFY(flickable->verticalVelocity() < 0.0);
    QTRY_VERIFY(flickable->verticalVelocity() == 0.0);

    // Flick multiple times and verify that flick acceleration is applied.
    QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(flickable);
    bool boosted = false;
    for (int i = 0; i < 6; ++i) {
        flick(canvas, QPoint(20,390), QPoint(20, 50), 200);
        boosted |= fp->flickBoost > 1.0;
    }
    QVERIFY(boosted);

    // Flick in opposite direction -> boost cancelled.
    flick(canvas, QPoint(20,10), QPoint(20, 340), 200);
    QTRY_VERIFY(flickable->verticalVelocity() < 0.0);
    QVERIFY(fp->flickBoost == 1.0);

    delete canvas;
}

void tst_qquickflickable::margins()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("margins.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(c.create());
    QQuickFlickable *obj = qobject_cast<QQuickFlickable*>(root);
    QVERIFY(obj != 0);

    // starting state
    QCOMPARE(obj->contentX(), -40.);
    QCOMPARE(obj->contentY(), -20.);
    QCOMPARE(obj->contentWidth(), 1600.);
    QCOMPARE(obj->contentHeight(), 600.);
    QCOMPARE(obj->xOrigin(), 0.);
    QCOMPARE(obj->yOrigin(), 0.);

    // Reduce left margin
    obj->setLeftMargin(30);
    QTRY_COMPARE(obj->contentX(), -30.);

    // Reduce top margin
    obj->setTopMargin(20);
    QTRY_COMPARE(obj->contentY(), -20.);

    // position to the far right, including margin
    obj->setContentX(1600 + 50 - obj->width());
    obj->returnToBounds();
    QTest::qWait(200);
    QCOMPARE(obj->contentX(), 1600. + 50. - obj->width());

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
    QTest::qWait(200);
    QCOMPARE(obj->contentY(), 600. + 30. - obj->height());

    // position beyond the far bottom, including margin
    obj->setContentY(600 + 30 - obj->height() + 1.);
    obj->returnToBounds();
    QTRY_COMPARE(obj->contentY(), 600. + 30. - obj->height());

    // Reduce bottom margin
    obj->setBottomMargin(20);
    QTRY_COMPARE(obj->contentY(), 600. + 20. - obj->height());
    QCOMPARE(obj->contentHeight(), 600.);

    delete root;
}

QTEST_MAIN(tst_qquickflickable)

#include "tst_qquickflickable.moc"
