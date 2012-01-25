/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquickflickable_p.h>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtOpenGL/QGLShaderProgram>
#include "../../shared/util.h"

//#define OLDWAY

class tst_QQuickMouseArea: public QDeclarativeDataTest
{
    Q_OBJECT
private slots:
    void dragProperties();
    void resetDrag();
    void dragging();
    void updateMouseAreaPosOnClick();
    void updateMouseAreaPosOnResize();
    void noOnClickedWithPressAndHold();
    void onMousePressRejected();
    void pressedCanceledOnWindowDeactivate();
    void doubleClick();
    void clickTwice();
    void pressedOrdering();
    void preventStealing();
    void clickThrough();
    void testQtQuick11Attributes();
    void testQtQuick11Attributes_data();
    void hoverPosition();
    void hoverPropagation();

private:
    QQuickView *createView();
};

void tst_QQuickMouseArea::dragProperties()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("dragproperties.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickMouseArea *mouseRegion = canvas->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootItem != 0);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);

    // axis
    QCOMPARE(drag->axis(), QQuickDrag::XandYAxis);
    QSignalSpy axisSpy(drag, SIGNAL(axisChanged()));
    drag->setAxis(QQuickDrag::XAxis);
    QCOMPARE(drag->axis(), QQuickDrag::XAxis);
    QCOMPARE(axisSpy.count(),1);
    drag->setAxis(QQuickDrag::XAxis);
    QCOMPARE(axisSpy.count(),1);

    // minimum and maximum properties
    QSignalSpy xminSpy(drag, SIGNAL(minimumXChanged()));
    QSignalSpy xmaxSpy(drag, SIGNAL(maximumXChanged()));
    QSignalSpy yminSpy(drag, SIGNAL(minimumYChanged()));
    QSignalSpy ymaxSpy(drag, SIGNAL(maximumYChanged()));

    QCOMPARE(drag->xmin(), 0.0);
    QCOMPARE(drag->xmax(), rootItem->width()-blackRect->width());
    QCOMPARE(drag->ymin(), 0.0);
    QCOMPARE(drag->ymax(), rootItem->height()-blackRect->height());

    drag->setXmin(10);
    drag->setXmax(10);
    drag->setYmin(10);
    drag->setYmax(10);

    QCOMPARE(drag->xmin(), 10.0);
    QCOMPARE(drag->xmax(), 10.0);
    QCOMPARE(drag->ymin(), 10.0);
    QCOMPARE(drag->ymax(), 10.0);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    drag->setXmin(10);
    drag->setXmax(10);
    drag->setYmin(10);
    drag->setYmax(10);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    // filterChildren
    QSignalSpy filterChildrenSpy(drag, SIGNAL(filterChildrenChanged()));

    drag->setFilterChildren(true);

    QVERIFY(drag->filterChildren());
    QCOMPARE(filterChildrenSpy.count(), 1);

    drag->setFilterChildren(true);
    QCOMPARE(filterChildrenSpy.count(), 1);

    delete canvas;
}

void tst_QQuickMouseArea::resetDrag()
{
    QQuickView *canvas = createView();

    canvas->rootContext()->setContextProperty("haveTarget", QVariant(true));
    canvas->setSource(testFileUrl("dragreset.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickMouseArea *mouseRegion = canvas->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootItem != 0);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    QVERIFY(drag->target() != 0);
    canvas->rootContext()->setContextProperty("haveTarget", QVariant(false));
    QCOMPARE(targetSpy.count(),1);
    QVERIFY(drag->target() == 0);

    delete canvas;
}


void tst_QQuickMouseArea::dragging()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("dragging.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWait(20);
    QVERIFY(canvas->rootObject() != 0);

    QQuickMouseArea *mouseRegion = canvas->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());

    QVERIFY(!drag->active());

#ifdef OLDWAY
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);
#else
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
#endif

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    QTest::mouseMove(canvas, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(canvas, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(drag->active());
    QCOMPARE(blackRect->x(), 72.0);
    QCOMPARE(blackRect->y(), 72.0);

#ifdef OLDWAY
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &releaseEvent);
#else
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(122,122));
    QTest::qWait(50);
#endif

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 72.0);
    QCOMPARE(blackRect->y(), 72.0);

    delete canvas;
}

QQuickView *tst_QQuickMouseArea::createView()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    return canvas;
}

void tst_QQuickMouseArea::updateMouseAreaPosOnClick()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("updateMousePosOnClick.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickMouseArea *mouseRegion = canvas->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != 0);

    QQuickRectangle *rect = canvas->rootObject()->findChild<QQuickRectangle*>("ball");
    QVERIFY(rect != 0);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    QMouseEvent event(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &event);

    QCOMPARE(mouseRegion->mouseX(), 100.0);
    QCOMPARE(mouseRegion->mouseY(), 100.0);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    delete canvas;
}

void tst_QQuickMouseArea::updateMouseAreaPosOnResize()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("updateMousePosOnResize.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickMouseArea *mouseRegion = canvas->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != 0);

    QQuickRectangle *rect = canvas->rootObject()->findChild<QQuickRectangle*>("brother");
    QVERIFY(rect != 0);

    QCOMPARE(mouseRegion->mouseX(), 0.0);
    QCOMPARE(mouseRegion->mouseY(), 0.0);

    QMouseEvent event(QEvent::MouseButtonPress, rect->pos().toPoint(), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &event);

    QVERIFY(!mouseRegion->property("emitPositionChanged").toBool());
    QVERIFY(mouseRegion->property("mouseMatchesPos").toBool());

    QCOMPARE(mouseRegion->property("x1").toReal(), 0.0);
    QCOMPARE(mouseRegion->property("y1").toReal(), 0.0);

    QCOMPARE(mouseRegion->property("x2").toReal(), rect->x());
    QCOMPARE(mouseRegion->property("y2").toReal(), rect->y());

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    delete canvas;
}

void tst_QQuickMouseArea::noOnClickedWithPressAndHold()
{
    {
        // We handle onPressAndHold, therefore no onClicked
        QQuickView *canvas = createView();
        canvas->setSource(testFileUrl("clickandhold.qml"));
        canvas->show();
        canvas->requestActivateWindow();
        QVERIFY(canvas->rootObject() != 0);

        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(canvas, &pressEvent);

        QVERIFY(!canvas->rootObject()->property("clicked").toBool());
        QVERIFY(!canvas->rootObject()->property("held").toBool());

        QTest::qWait(1000);

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(canvas, &releaseEvent);

        QVERIFY(!canvas->rootObject()->property("clicked").toBool());
        QVERIFY(canvas->rootObject()->property("held").toBool());

        delete canvas;
    }

    {
        // We do not handle onPressAndHold, therefore we get onClicked
        QQuickView *canvas = createView();
        canvas->setSource(testFileUrl("noclickandhold.qml"));
        canvas->show();
        canvas->requestActivateWindow();
        QVERIFY(canvas->rootObject() != 0);

        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(canvas, &pressEvent);

        QVERIFY(!canvas->rootObject()->property("clicked").toBool());

        QTest::qWait(1000);

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(canvas, &releaseEvent);

        QVERIFY(canvas->rootObject()->property("clicked").toBool());

        delete canvas;
    }
}

void tst_QQuickMouseArea::onMousePressRejected()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("rejectEvent.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);
    QVERIFY(canvas->rootObject()->property("enabled").toBool());

    QVERIFY(!canvas->rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("mr1_released").toBool());
    QVERIFY(!canvas->rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!canvas->rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("mr2_released").toBool());
    QVERIFY(!canvas->rootObject()->property("mr2_canceled").toBool());

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QVERIFY(canvas->rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("mr1_released").toBool());
    QVERIFY(!canvas->rootObject()->property("mr1_canceled").toBool());
    QVERIFY(canvas->rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("mr2_released").toBool());
    QVERIFY(canvas->rootObject()->property("mr2_canceled").toBool());

    QTest::qWait(200);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &releaseEvent);

    QVERIFY(canvas->rootObject()->property("mr1_released").toBool());
    QVERIFY(!canvas->rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!canvas->rootObject()->property("mr2_released").toBool());

    delete canvas;
}
void tst_QQuickMouseArea::pressedCanceledOnWindowDeactivate()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("pressedCanceled.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);
    QVERIFY(!canvas->rootObject()->property("pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("canceled").toBool());
    QVERIFY(!canvas->rootObject()->property("released").toBool());

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QVERIFY(canvas->rootObject()->property("pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("canceled").toBool());
    QVERIFY(!canvas->rootObject()->property("released").toBool());

    QTest::qWait(200);

    QEvent windowDeactivateEvent(QEvent::WindowDeactivate);
    QGuiApplication::sendEvent(canvas, &windowDeactivateEvent);
    QVERIFY(!canvas->rootObject()->property("pressed").toBool());
    QVERIFY(canvas->rootObject()->property("canceled").toBool());
    QVERIFY(!canvas->rootObject()->property("released").toBool());

    QTest::qWait(200);

    //press again
    QGuiApplication::sendEvent(canvas, &pressEvent);
    QVERIFY(canvas->rootObject()->property("pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("canceled").toBool());
    QVERIFY(!canvas->rootObject()->property("released").toBool());

    QTest::qWait(200);

    //release
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &releaseEvent);
    QVERIFY(!canvas->rootObject()->property("pressed").toBool());
    QVERIFY(!canvas->rootObject()->property("canceled").toBool());
    QVERIFY(canvas->rootObject()->property("released").toBool());

    delete canvas;
}
void tst_QQuickMouseArea::doubleClick()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("doubleclick.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &releaseEvent);

    QCOMPARE(canvas->rootObject()->property("released").toInt(), 1);

    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QGuiApplication::sendEvent(canvas, &releaseEvent);

    QCOMPARE(canvas->rootObject()->property("clicked").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("doubleClicked").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("released").toInt(), 2);

    delete canvas;
}

// QTBUG-14832
void tst_QQuickMouseArea::clickTwice()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("clicktwice.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &releaseEvent);

    QCOMPARE(canvas->rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("released").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("clicked").toInt(), 1);

    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QGuiApplication::sendEvent(canvas, &pressEvent);
    QGuiApplication::sendEvent(canvas, &releaseEvent);

    QCOMPARE(canvas->rootObject()->property("pressed").toInt(), 2);
    QCOMPARE(canvas->rootObject()->property("released").toInt(), 2);
    QCOMPARE(canvas->rootObject()->property("clicked").toInt(), 2);

    delete canvas;
}

void tst_QQuickMouseArea::pressedOrdering()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("pressedOrdering.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QCOMPARE(canvas->rootObject()->property("value").toString(), QLatin1String("base"));

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &pressEvent);

    QCOMPARE(canvas->rootObject()->property("value").toString(), QLatin1String("pressed"));

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(canvas, &releaseEvent);

    QCOMPARE(canvas->rootObject()->property("value").toString(), QLatin1String("toggled"));

    QGuiApplication::sendEvent(canvas, &pressEvent);

    QCOMPARE(canvas->rootObject()->property("value").toString(), QLatin1String("pressed"));

    delete canvas;
}

void tst_QQuickMouseArea::preventStealing()
{
    QQuickView *canvas = createView();

    canvas->setSource(testFileUrl("preventstealing.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(canvas->rootObject());
    QVERIFY(flickable != 0);

    QQuickMouseArea *mouseArea = canvas->rootObject()->findChild<QQuickMouseArea*>("mousearea");
    QVERIFY(mouseArea != 0);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(80, 80));

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    QTest::mouseMove(canvas,QPoint(69,69));
    QTest::mouseMove(canvas,QPoint(58,58));
    QTest::mouseMove(canvas,QPoint(47,47));

    // We should have received all three move events
    QCOMPARE(mousePositionSpy.count(), 3);
    QVERIFY(mouseArea->pressed());

    // Flickable content should not have moved.
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(47, 47));

    // Now allow stealing and confirm Flickable does its thing.
    canvas->rootObject()->setProperty("stealing", false);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(80, 80));

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    QTest::mouseMove(canvas,QPoint(69,69));
    QTest::mouseMove(canvas,QPoint(58,58));
    QTest::mouseMove(canvas,QPoint(47,47));

    // We should only have received the first move event
    QCOMPARE(mousePositionSpy.count(), 4);
    // Our press should be taken away
    QVERIFY(!mouseArea->pressed());

    // Flickable content should have moved.

    QCOMPARE(flickable->contentX(), 11.);
    QCOMPARE(flickable->contentY(), 11.);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(50, 50));

    delete canvas;
}

void tst_QQuickMouseArea::clickThrough()
{
    //With no handlers defined click, doubleClick and PressAndHold should propagate to those with handlers
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("clickThrough.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));

    QTRY_COMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(canvas->rootObject()->property("clicks").toInt(), 1);

    QTest::qWait(800); // to avoid generating a double click.

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));

    QTRY_COMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(canvas->rootObject()->property("clicks").toInt(), 1);
    QTRY_COMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 1);

    QTest::mouseDClick(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(canvas->rootObject()->property("clicks").toInt(), 2);
    QTRY_COMPARE(canvas->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 1);

    delete canvas;

    //With handlers defined click, doubleClick and PressAndHold should propagate only when explicitly ignored
    canvas = createView();
    canvas->setSource(testFileUrl("clickThrough2.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("clicks").toInt(), 0);

    QTest::qWait(800); // to avoid generating a double click.

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("clicks").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 0);

    QTest::mouseDClick(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("clicks").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("doubleClicks").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 0);

    canvas->rootObject()->setProperty("letThrough", QVariant(true));

    QTest::qWait(800); // to avoid generating a double click.
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(canvas->rootObject()->property("clicks").toInt(), 1);

    QTest::qWait(800); // to avoid generating a double click.
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("clicks").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 1);

    QTest::mouseDClick(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(canvas->rootObject()->property("clicks").toInt(), 2);
    QCOMPARE(canvas->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 1);

    canvas->rootObject()->setProperty("noPropagation", QVariant(true));

    QTest::qWait(800); // to avoid generating a double click.
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));

    QTest::qWait(800); // to avoid generating a double click.
    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QTest::mouseDClick(canvas, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(canvas->rootObject()->property("clicks").toInt(), 2);
    QCOMPARE(canvas->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(canvas->rootObject()->property("pressAndHolds").toInt(), 1);

    delete canvas;
}

void tst_QQuickMouseArea::testQtQuick11Attributes()
{
    QFETCH(QString, code);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QObject *obj;

    QDeclarativeComponent valid(&engine);
    valid.setData("import QtQuick 1.1; MouseArea { " + code.toUtf8() + " }", QUrl(""));
    obj = valid.create();
    QVERIFY(obj);
    QVERIFY(valid.errorString().isEmpty());
    delete obj;

    QDeclarativeComponent invalid(&engine);
    invalid.setData("import QtQuick 1.0; MouseArea { " + code.toUtf8() + " }", QUrl(""));
    QTest::ignoreMessage(QtWarningMsg, warning.toUtf8());
    obj = invalid.create();
    QCOMPARE(invalid.errorString(), error);
    delete obj;
}

void tst_QQuickMouseArea::testQtQuick11Attributes_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("preventStealing") << "preventStealing: true"
        << "QDeclarativeComponent: Component is not ready"
        << ":1 \"MouseArea.preventStealing\" is not available in QtQuick 1.0.\n";
}

void tst_QQuickMouseArea::hoverPosition()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("hoverPosition.qml"));

    QQuickItem *root = canvas->rootObject();
    QVERIFY(root != 0);

    QCOMPARE(root->property("mouseX").toReal(), qreal(0));
    QCOMPARE(root->property("mouseY").toReal(), qreal(0));

    QTest::mouseMove(canvas,QPoint(10,32));


    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));

    delete canvas;
}

void tst_QQuickMouseArea::hoverPropagation()
{
    //QTBUG-18175, to behave like GV did.
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("hoverPropagation.qml"));

    QQuickItem *root = canvas->rootObject();
    QVERIFY(root != 0);

    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), false);

    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(32, 32), Qt::NoButton, Qt::NoButton, 0);
    QGuiApplication::sendEvent(canvas, &moveEvent);

    QCOMPARE(root->property("point1").toBool(), true);
    QCOMPARE(root->property("point2").toBool(), false);

    QMouseEvent moveEvent2(QEvent::MouseMove, QPoint(232, 32), Qt::NoButton, Qt::NoButton, 0);
    QGuiApplication::sendEvent(canvas, &moveEvent2);
    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), true);

    delete canvas;
}

QTEST_MAIN(tst_QQuickMouseArea)

#include "tst_qquickmousearea.moc"
