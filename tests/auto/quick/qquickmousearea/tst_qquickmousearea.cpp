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

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquickflickable_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include "../../shared/util.h"
#include <QtGui/qstylehints.h>

class tst_QQuickMouseArea: public QQmlDataTest
{
    Q_OBJECT
private slots:
    void dragProperties();
    void resetDrag();
    void dragging_data() { acceptedButton_data(); }
    void dragging();
    void invalidDrag_data() { rejectedButton_data(); }
    void invalidDrag();
    void setDragOnPressed();
    void updateMouseAreaPosOnClick();
    void updateMouseAreaPosOnResize();
    void noOnClickedWithPressAndHold();
    void onMousePressRejected();
    void pressedCanceledOnWindowDeactivate();
    void doubleClick_data() { acceptedButton_data(); }
    void doubleClick();
    void clickTwice_data() { acceptedButton_data(); }
    void clickTwice();
    void invalidClick_data() { rejectedButton_data(); }
    void invalidClick();
    void pressedOrdering();
    void preventStealing();
    void clickThrough();
    void hoverPosition();
    void hoverPropagation();
    void hoverVisible();
    void hoverAfterPress();
    void disableAfterPress();
    void onWheel();
    void transformedMouseArea_data();
    void transformedMouseArea();
    void pressedMultipleButtons_data();
    void pressedMultipleButtons();
    void changeAxis();
#ifndef QT_NO_CURSOR
    void cursorShape();
#endif
    void moveAndReleaseWithoutPress();
    void nestedStopAtBounds();
    void nestedStopAtBounds_data();

private:
    void acceptedButton_data();
    void rejectedButton_data();

    QQuickView *createView();
};

Q_DECLARE_METATYPE(Qt::MouseButton)
Q_DECLARE_METATYPE(Qt::MouseButtons)

void tst_QQuickMouseArea::acceptedButton_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("left") << Qt::MouseButtons(Qt::LeftButton) << Qt::LeftButton;
    QTest::newRow("right") << Qt::MouseButtons(Qt::RightButton) << Qt::RightButton;
    QTest::newRow("middle") << Qt::MouseButtons(Qt::MiddleButton) << Qt::MiddleButton;

    QTest::newRow("left (left|right)") << Qt::MouseButtons(Qt::LeftButton | Qt::RightButton) << Qt::LeftButton;
    QTest::newRow("right (right|middle)") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::RightButton;
    QTest::newRow("middle (left|middle)") << Qt::MouseButtons(Qt::LeftButton | Qt::MiddleButton) << Qt::MiddleButton;
}

void tst_QQuickMouseArea::rejectedButton_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("middle (left|right)") << Qt::MouseButtons(Qt::LeftButton | Qt::RightButton) << Qt::MiddleButton;
    QTest::newRow("left (right|middle)") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::LeftButton;
    QTest::newRow("right (left|middle)") << Qt::MouseButtons(Qt::LeftButton | Qt::MiddleButton) << Qt::RightButton;
}

void tst_QQuickMouseArea::dragProperties()
{
    QQuickView *window = createView();

    window->setSource(testFileUrl("dragproperties.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseRegion = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem != 0);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);

    // axis
    QCOMPARE(drag->axis(), QQuickDrag::XAndYAxis);
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

    delete window;
}

void tst_QQuickMouseArea::resetDrag()
{
    QQuickView *window = createView();

    window->rootContext()->setContextProperty("haveTarget", QVariant(true));
    window->setSource(testFileUrl("dragreset.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseRegion = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window->rootObject());
    QVERIFY(rootItem != 0);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    QVERIFY(drag->target() != 0);
    window->rootContext()->setContextProperty("haveTarget", QVariant(false));
    QCOMPARE(targetSpy.count(),1);
    QVERIFY(drag->target() == 0);

    delete window;
}

void tst_QQuickMouseArea::dragging()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView *window = createView();

    window->setSource(testFileUrl("dragging.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseRegion = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    mouseRegion->setAcceptedButtons(acceptedButtons);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());

    QVERIFY(!drag->active());

    QTest::mousePress(window, button, 0, QPoint(100,100));

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.
    // The item is moved relative to the position of the mouse when the drag
    // was triggered, this prevents a sudden change in position when the drag
    // threshold is exceeded.
    QTest::mouseMove(window, QPoint(111,111), 50);
    QTest::mouseMove(window, QPoint(116,116), 50);
    QTest::mouseMove(window, QPoint(122,122), 50);

    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    QTest::mouseRelease(window, button, 0, QPoint(122,122));

    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    delete window;
}

void tst_QQuickMouseArea::invalidDrag()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView *window = createView();

    window->setSource(testFileUrl("dragging.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseRegion = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    mouseRegion->setAcceptedButtons(acceptedButtons);

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());

    QVERIFY(!drag->active());

    QTest::mousePress(window, button, 0, QPoint(100,100));

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    QTest::mouseMove(window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(window, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseRelease(window, button, 0, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    delete window;
}

void tst_QQuickMouseArea::setDragOnPressed()
{
    QQuickView *window = createView();

    window->setSource(testFileUrl("setDragOnPressed.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(window->rootObject());
    QVERIFY(mouseArea);

    // target
    QQuickItem *target = mouseArea->findChild<QQuickItem*>("target");
    QVERIFY(target);

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));

    QQuickDrag *drag = mouseArea->drag();
    QVERIFY(drag);
    QVERIFY(!drag->active());

    QCOMPARE(target->x(), 50.0);
    QCOMPARE(target->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    QTest::mouseMove(window, QPoint(111,102));
    QTest::qWait(50);
    QTest::mouseMove(window, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(drag->active());
    QCOMPARE(target->x(), 61.0);
    QCOMPARE(target->y(), 50.0);

    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(target->x(), 61.0);
    QCOMPARE(target->y(), 50.0);

    delete window;
}

QQuickView *tst_QQuickMouseArea::createView()
{
    QQuickView *window = new QQuickView(0);
    window->setBaseSize(QSize(240,320));

    return window;
}

void tst_QQuickMouseArea::updateMouseAreaPosOnClick()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("updateMousePosOnClick.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseRegion = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != 0);

    QQuickRectangle *rect = window->rootObject()->findChild<QQuickRectangle*>("ball");
    QVERIFY(rect != 0);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    QMouseEvent event(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &event);

    QCOMPARE(mouseRegion->mouseX(), 100.0);
    QCOMPARE(mouseRegion->mouseY(), 100.0);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    delete window;
}

void tst_QQuickMouseArea::updateMouseAreaPosOnResize()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("updateMousePosOnResize.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseRegion = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != 0);

    QQuickRectangle *rect = window->rootObject()->findChild<QQuickRectangle*>("brother");
    QVERIFY(rect != 0);

    QCOMPARE(mouseRegion->mouseX(), 0.0);
    QCOMPARE(mouseRegion->mouseY(), 0.0);

    QMouseEvent event(QEvent::MouseButtonPress, rect->position().toPoint(), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &event);

    QVERIFY(!mouseRegion->property("emitPositionChanged").toBool());
    QVERIFY(mouseRegion->property("mouseMatchesPos").toBool());

    QCOMPARE(mouseRegion->property("x1").toReal(), 0.0);
    QCOMPARE(mouseRegion->property("y1").toReal(), 0.0);

    QCOMPARE(mouseRegion->property("x2").toReal(), rect->x());
    QCOMPARE(mouseRegion->property("y2").toReal(), rect->y());

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    delete window;
}

void tst_QQuickMouseArea::noOnClickedWithPressAndHold()
{
    {
        // We handle onPressAndHold, therefore no onClicked
        QQuickView *window = createView();
        window->setSource(testFileUrl("clickandhold.qml"));
        window->show();
        QTest::qWaitForWindowExposed(window);
        QVERIFY(window->rootObject() != 0);
        QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea*>(window->rootObject()->children().first());
        QVERIFY(mouseArea);

        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(window, &pressEvent);

        QVERIFY(mouseArea->pressedButtons() == Qt::LeftButton);
        QVERIFY(!window->rootObject()->property("clicked").toBool());
        QVERIFY(!window->rootObject()->property("held").toBool());

        // timeout is 800 (in qquickmousearea.cpp)
        QTest::qWait(1000);
        QCoreApplication::processEvents();

        QVERIFY(!window->rootObject()->property("clicked").toBool());
        QVERIFY(window->rootObject()->property("held").toBool());

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(window, &releaseEvent);

        QTRY_VERIFY(window->rootObject()->property("held").toBool());
        QVERIFY(!window->rootObject()->property("clicked").toBool());

        delete window;
    }

    {
        // We do not handle onPressAndHold, therefore we get onClicked
        QQuickView *window = createView();
        window->setSource(testFileUrl("noclickandhold.qml"));
        window->show();
        QTest::qWaitForWindowExposed(window);
        QVERIFY(window->rootObject() != 0);

        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(window, &pressEvent);

        QVERIFY(!window->rootObject()->property("clicked").toBool());

        QTest::qWait(1000);

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
        QGuiApplication::sendEvent(window, &releaseEvent);

        QVERIFY(window->rootObject()->property("clicked").toBool());

        delete window;
    }
}

void tst_QQuickMouseArea::onMousePressRejected()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("rejectEvent.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);
    QVERIFY(window->rootObject()->property("enabled").toBool());

    QVERIFY(!window->rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!window->rootObject()->property("mr1_released").toBool());
    QVERIFY(!window->rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!window->rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!window->rootObject()->property("mr2_released").toBool());
    QVERIFY(!window->rootObject()->property("mr2_canceled").toBool());

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &pressEvent);

    QVERIFY(window->rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!window->rootObject()->property("mr1_released").toBool());
    QVERIFY(!window->rootObject()->property("mr1_canceled").toBool());
    QVERIFY(window->rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!window->rootObject()->property("mr2_released").toBool());
    QVERIFY(window->rootObject()->property("mr2_canceled").toBool());

    QTest::qWait(200);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QVERIFY(window->rootObject()->property("mr1_released").toBool());
    QVERIFY(!window->rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!window->rootObject()->property("mr2_released").toBool());

    delete window;
}
void tst_QQuickMouseArea::pressedCanceledOnWindowDeactivate()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("pressedCanceled.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);
    QVERIFY(!window->rootObject()->property("pressed").toBool());
    QVERIFY(!window->rootObject()->property("canceled").toBool());
    QVERIFY(!window->rootObject()->property("released").toBool());

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &pressEvent);

    QVERIFY(window->rootObject()->property("pressed").toBool());
    QVERIFY(!window->rootObject()->property("canceled").toBool());
    QVERIFY(!window->rootObject()->property("released").toBool());

    QWindow *secondWindow = qvariant_cast<QWindow*>(window->rootObject()->property("secondWindow"));
    secondWindow->setProperty("visible", true);
    QTest::qWaitForWindowExposed(secondWindow);

    QVERIFY(!window->rootObject()->property("pressed").toBool());
    QVERIFY(window->rootObject()->property("canceled").toBool());
    QVERIFY(!window->rootObject()->property("released").toBool());

    //press again
    QGuiApplication::sendEvent(window, &pressEvent);
    QVERIFY(window->rootObject()->property("pressed").toBool());
    QVERIFY(!window->rootObject()->property("canceled").toBool());
    QVERIFY(!window->rootObject()->property("released").toBool());

    QTest::qWait(200);

    //release
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &releaseEvent);
    QVERIFY(!window->rootObject()->property("pressed").toBool());
    QVERIFY(!window->rootObject()->property("canceled").toBool());
    QVERIFY(window->rootObject()->property("released").toBool());

    delete window;
}

void tst_QQuickMouseArea::doubleClick()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView *window = createView();
    window->setSource(testFileUrl("doubleclick.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    // The sequence for a double click is:
    // press, release, (click), press, double click, release
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("released").toInt(), 1);

    QGuiApplication::sendEvent(window, &pressEvent);
    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &pressEvent);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("clicked").toInt(), 1);
    QCOMPARE(window->rootObject()->property("doubleClicked").toInt(), 1);
    QCOMPARE(window->rootObject()->property("released").toInt(), 2);

    delete window;
}

// QTBUG-14832
void tst_QQuickMouseArea::clickTwice()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView *window = createView();
    window->setSource(testFileUrl("clicktwice.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window->rootObject()->property("released").toInt(), 1);
    QCOMPARE(window->rootObject()->property("clicked").toInt(), 1);

    QGuiApplication::sendEvent(window, &pressEvent);
    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &pressEvent);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("pressed").toInt(), 2);
    QCOMPARE(window->rootObject()->property("released").toInt(), 2);
    QCOMPARE(window->rootObject()->property("clicked").toInt(), 2);

    delete window;
}

void tst_QQuickMouseArea::invalidClick()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView *window = createView();
    window->setSource(testFileUrl("doubleclick.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    // The sequence for a double click is:
    // press, release, (click), press, double click, release
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("released").toInt(), 0);

    QGuiApplication::sendEvent(window, &pressEvent);
    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), button, button, 0);
    QGuiApplication::sendEvent(window, &pressEvent);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("clicked").toInt(), 0);
    QCOMPARE(window->rootObject()->property("doubleClicked").toInt(), 0);
    QCOMPARE(window->rootObject()->property("released").toInt(), 0);

    delete window;
}

void tst_QQuickMouseArea::pressedOrdering()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("pressedOrdering.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QCOMPARE(window->rootObject()->property("value").toString(), QLatin1String("base"));

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &pressEvent);

    QCOMPARE(window->rootObject()->property("value").toString(), QLatin1String("pressed"));

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, 0);
    QGuiApplication::sendEvent(window, &releaseEvent);

    QCOMPARE(window->rootObject()->property("value").toString(), QLatin1String("toggled"));

    QGuiApplication::sendEvent(window, &pressEvent);

    QCOMPARE(window->rootObject()->property("value").toString(), QLatin1String("pressed"));

    delete window;
}

void tst_QQuickMouseArea::preventStealing()
{
    QQuickView *window = createView();

    window->setSource(testFileUrl("preventstealing.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window->rootObject());
    QVERIFY(flickable != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea*>("mousearea");
    QVERIFY(mouseArea != 0);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(80, 80));

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    QTest::mouseMove(window,QPoint(69,69));
    QTest::mouseMove(window,QPoint(58,58));
    QTest::mouseMove(window,QPoint(47,47));

    // We should have received all three move events
    QCOMPARE(mousePositionSpy.count(), 3);
    QVERIFY(mouseArea->pressed());

    // Flickable content should not have moved.
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);

    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(47, 47));

    // Now allow stealing and confirm Flickable does its thing.
    window->rootObject()->setProperty("stealing", false);

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(80, 80));

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    QTest::mouseMove(window,QPoint(69,69));
    QTest::mouseMove(window,QPoint(58,58));
    QTest::mouseMove(window,QPoint(47,47));

    // We should only have received the first move event
    QCOMPARE(mousePositionSpy.count(), 4);
    // Our press should be taken away
    QVERIFY(!mouseArea->pressed());

    // Flickable content should have moved.

    QCOMPARE(flickable->contentX(), 11.);
    QCOMPARE(flickable->contentY(), 11.);

    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(50, 50));

    delete window;
}

void tst_QQuickMouseArea::clickThrough()
{
    QSKIP("QTBUG-23976 Unstable");
    //With no handlers defined click, doubleClick and PressAndHold should propagate to those with handlers
    QQuickView *window = createView();
    window->setSource(testFileUrl("clickThrough.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));

    QTRY_COMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 1);

    // to avoid generating a double click.
    int doubleClickInterval = qApp->styleHints()->mouseDoubleClickInterval() + 10;
    QTest::qWait(doubleClickInterval);

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));

    QTRY_COMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 1);
    QTRY_COMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    QTest::mouseDClick(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 2);
    QTRY_COMPARE(window->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    delete window;

    //With handlers defined click, doubleClick and PressAndHold should propagate only when explicitly ignored
    window = createView();
    window->setSource(testFileUrl("clickThrough2.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 0);

    QTest::qWait(doubleClickInterval); // to avoid generating a double click.

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 0);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 0);

    QTest::mouseDClick(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 0);
    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 0);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 0);

    window->rootObject()->setProperty("letThrough", QVariant(true));

    QTest::qWait(doubleClickInterval); // to avoid generating a double click.
    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 1);

    QTest::qWait(doubleClickInterval); // to avoid generating a double click.
    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    QTest::mouseDClick(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 2);
    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    window->rootObject()->setProperty("noPropagation", QVariant(true));

    QTest::qWait(doubleClickInterval); // to avoid generating a double click.
    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));

    QTest::qWait(doubleClickInterval); // to avoid generating a double click.
    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(1000);
    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QTest::mouseDClick(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 2);
    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    delete window;
}

void tst_QQuickMouseArea::hoverPosition()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("hoverPosition.qml"));

    QQuickItem *root = window->rootObject();
    QVERIFY(root != 0);

    QCOMPARE(root->property("mouseX").toReal(), qreal(0));
    QCOMPARE(root->property("mouseY").toReal(), qreal(0));

    QTest::mouseMove(window,QPoint(10,32));


    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));

    delete window;
}

void tst_QQuickMouseArea::hoverPropagation()
{
    //QTBUG-18175, to behave like GV did.
    QQuickView *window = createView();
    window->setSource(testFileUrl("hoverPropagation.qml"));

    QQuickItem *root = window->rootObject();
    QVERIFY(root != 0);

    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), false);

    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(32, 32), Qt::NoButton, Qt::NoButton, 0);
    QGuiApplication::sendEvent(window, &moveEvent);

    QCOMPARE(root->property("point1").toBool(), true);
    QCOMPARE(root->property("point2").toBool(), false);

    QMouseEvent moveEvent2(QEvent::MouseMove, QPoint(232, 32), Qt::NoButton, Qt::NoButton, 0);
    QGuiApplication::sendEvent(window, &moveEvent2);
    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), true);

    delete window;
}

void tst_QQuickMouseArea::hoverVisible()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("hoverVisible.qml"));

    QQuickItem *root = window->rootObject();
    QVERIFY(root != 0);

    QQuickMouseArea *mouseTracker = window->rootObject()->findChild<QQuickMouseArea*>("mousetracker");
    QVERIFY(mouseTracker != 0);

    QSignalSpy enteredSpy(mouseTracker, SIGNAL(entered()));

    // Note: We need to use a position that is different from the position in the last event
    // generated in the previous test case. Otherwise it is not interpreted as a move.
    QTest::mouseMove(window,QPoint(11,33));

    QCOMPARE(mouseTracker->hovered(), false);
    QCOMPARE(enteredSpy.count(), 0);

    mouseTracker->setVisible(true);

    QCOMPARE(mouseTracker->hovered(), true);
    QCOMPARE(enteredSpy.count(), 1);

    QCOMPARE(QPointF(mouseTracker->mouseX(), mouseTracker->mouseY()), QPointF(11,33));

    delete window;
}

void tst_QQuickMouseArea::hoverAfterPress()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("hoverAfterPress.qml"));

    QQuickItem *root = window->rootObject();
    QVERIFY(root != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != 0);
    QTest::mouseMove(window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QTest::mouseMove(window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QTest::mouseMove(window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    delete window;
}

void tst_QQuickMouseArea::disableAfterPress()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("dragging.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseArea->drag();
    QVERIFY(mouseArea != 0);
    QVERIFY(drag != 0);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));
    QSignalSpy mousePressSpy(mouseArea, SIGNAL(pressed(QQuickMouseEvent*)));
    QSignalSpy mouseReleaseSpy(mouseArea, SIGNAL(released(QQuickMouseEvent*)));

    // target
    QQuickItem *blackRect = window->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());

    QVERIFY(!drag->active());

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));

    QTRY_COMPARE(mousePressSpy.count(), 1);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    QTest::mouseMove(window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(window, QPoint(122,122));

    QTRY_COMPARE(mousePositionSpy.count(), 2);

    QVERIFY(drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    mouseArea->setEnabled(false);

    // move should still be acted upon
    QTest::mouseMove(window, QPoint(133,133));
    QTest::qWait(50);
    QTest::mouseMove(window, QPoint(144,144));

    QTRY_COMPARE(mousePositionSpy.count(), 4);

    QVERIFY(drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 83.0);

    QVERIFY(mouseArea->pressed());
    QVERIFY(mouseArea->hovered());

    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(144,144));

    QTRY_COMPARE(mouseReleaseSpy.count(), 1);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 83.0);

    QVERIFY(!mouseArea->pressed());
    QVERIFY(!mouseArea->hovered()); // since hover is not enabled

    // Next press will be ignored
    blackRect->setX(50);
    blackRect->setY(50);

    mousePressSpy.clear();
    mousePositionSpy.clear();
    mouseReleaseSpy.clear();

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));
    QTest::qWait(50);
    QCOMPARE(mousePressSpy.count(), 0);

    QTest::mouseMove(window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(window, QPoint(122,122));
    QTest::qWait(50);

    QCOMPARE(mousePositionSpy.count(), 0);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(122,122));
    QTest::qWait(50);

    QCOMPARE(mouseReleaseSpy.count(), 0);

    delete window;
}

void tst_QQuickMouseArea::onWheel()
{
    QQuickView *window = createView();
    window->setSource(testFileUrl("wheel.qml"));

    QQuickItem *root = window->rootObject();
    QVERIFY(root != 0);

    QWheelEvent wheelEvent(QPoint(10, 32), QPoint(10, 32), QPoint(60, 20), QPoint(0, 120),
                           0, Qt::Vertical,Qt::NoButton, Qt::ControlModifier);
    QGuiApplication::sendEvent(window, &wheelEvent);

    QCOMPARE(root->property("angleDeltaY").toInt(), 120);
    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));
    QCOMPARE(root->property("controlPressed").toBool(), true);

    delete window;
}

void tst_QQuickMouseArea::transformedMouseArea_data()
{
    QTest::addColumn<bool>("insideTarget");
    QTest::addColumn<QList<QPoint> >("points");

    QList<QPoint> pointsInside;
    pointsInside << QPoint(200, 140)
                 << QPoint(140, 200)
                 << QPoint(200, 200)
                 << QPoint(260, 200)
                 << QPoint(200, 260);
    QTest::newRow("checking points inside") << true << pointsInside;

    QList<QPoint> pointsOutside;
    pointsOutside << QPoint(140, 140)
                  << QPoint(260, 140)
                  << QPoint(120, 200)
                  << QPoint(280, 200)
                  << QPoint(140, 260)
                  << QPoint(260, 260);
    QTest::newRow("checking points outside") << false << pointsOutside;
}

void tst_QQuickMouseArea::transformedMouseArea()
{
    QFETCH(bool, insideTarget);
    QFETCH(QList<QPoint>, points);

    QQuickView *window = createView();
    window->setSource(testFileUrl("transformedMouseArea.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(window->rootObject() != 0);

    QQuickMouseArea *mouseArea = window->rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea != 0);

    foreach (const QPoint &point, points) {
        // check hover
        QTest::mouseMove(window, point);
        QTest::qWait(10);
        QCOMPARE(mouseArea->property("containsMouse").toBool(), insideTarget);

        // check mouse press
        QTest::mousePress(window, Qt::LeftButton, 0, point);
        QTest::qWait(10);
        QCOMPARE(mouseArea->property("pressed").toBool(), insideTarget);

        // check mouse release
        QTest::mouseRelease(window, Qt::LeftButton, 0, point);
        QTest::qWait(10);
        QCOMPARE(mouseArea->property("pressed").toBool(), false);
    }

    delete window;
}

void tst_QQuickMouseArea::pressedMultipleButtons_data()
{
    QTest::addColumn<Qt::MouseButtons>("accepted");
    QTest::addColumn<QList<Qt::MouseButtons> >("buttons");
    QTest::addColumn<QList<bool> >("pressed");
    QTest::addColumn<QList<Qt::MouseButtons> >("pressedButtons");
    QTest::addColumn<int>("changeCount");

    QList<Qt::MouseButtons> buttons;
    QList<bool> pressed;
    QList<Qt::MouseButtons> pressedButtons;
    buttons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << 0;
    pressed << true
            << true
            << true
            << false;
    pressedButtons << Qt::LeftButton
            << Qt::LeftButton
            << Qt::LeftButton
            << 0;
    QTest::newRow("Accept Left - Press left, Press Right, Release Right")
            << Qt::MouseButtons(Qt::LeftButton) << buttons << pressed << pressedButtons << 2;

    buttons.clear();
    pressed.clear();
    pressedButtons.clear();
    buttons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::RightButton
            << 0;
    pressed << true
            << true
            << false
            << false;
    pressedButtons << Qt::LeftButton
            << Qt::LeftButton
            << 0
            << 0;
    QTest::newRow("Accept Left - Press left, Press Right, Release Left")
            << Qt::MouseButtons(Qt::LeftButton) << buttons << pressed << pressedButtons << 2;

    buttons.clear();
    pressed.clear();
    pressedButtons.clear();
    buttons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << 0;
    pressed << true
            << true
            << true
            << false;
    pressedButtons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << 0;
    QTest::newRow("Accept Left|Right - Press left, Press Right, Release Right")
        << (Qt::LeftButton | Qt::RightButton) << buttons << pressed << pressedButtons << 4;

    buttons.clear();
    pressed.clear();
    pressedButtons.clear();
    buttons << Qt::RightButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << 0;
    pressed << true
            << true
            << false
            << false;
    pressedButtons << Qt::RightButton
            << Qt::RightButton
            << 0
            << 0;
    QTest::newRow("Accept Right - Press Right, Press Left, Release Right")
            << Qt::MouseButtons(Qt::RightButton) << buttons << pressed << pressedButtons << 2;
}

void tst_QQuickMouseArea::pressedMultipleButtons()
{
    QFETCH(Qt::MouseButtons, accepted);
    QFETCH(QList<Qt::MouseButtons>, buttons);
    QFETCH(QList<bool>, pressed);
    QFETCH(QList<Qt::MouseButtons>, pressedButtons);
    QFETCH(int, changeCount);

    QQuickView *view = createView();
    view->setSource(testFileUrl("simple.qml"));
    view->show();
    QTest::qWaitForWindowExposed(view);
    QVERIFY(view->rootObject() != 0);

    QQuickMouseArea *mouseArea = view->rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea != 0);

    QSignalSpy pressedSpy(mouseArea, SIGNAL(pressedChanged()));
    QSignalSpy pressedButtonsSpy(mouseArea, SIGNAL(pressedButtonsChanged()));
    mouseArea->setAcceptedMouseButtons(accepted);

    QPoint point(10,10);

    for (int i = 0; i < buttons.count(); ++i) {
        int btns = buttons.at(i);

        // The windowsysteminterface takes care of sending releases
        QTest::mousePress(view, (Qt::MouseButton)btns, 0, point);

        QCOMPARE(mouseArea->pressed(), pressed.at(i));
        QCOMPARE(mouseArea->pressedButtons(), pressedButtons.at(i));
    }

    QTest::mousePress(view, Qt::NoButton, 0, point);
    QCOMPARE(mouseArea->pressed(), false);

    QCOMPARE(pressedSpy.count(), 2);
    QCOMPARE(pressedButtonsSpy.count(), changeCount);

    delete view;
}

void tst_QQuickMouseArea::changeAxis()
{
    QQuickView *view = createView();

    view->setSource(testFileUrl("changeAxis.qml"));
    view->show();
    QTest::qWaitForWindowExposed(view);
    QTRY_VERIFY(view->rootObject() != 0);

    QQuickMouseArea *mouseRegion = view->rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != 0);
    QVERIFY(drag != 0);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);

    // target
    QQuickItem *blackRect = view->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == drag->target());

    QVERIFY(!drag->active());

    // Start a diagonal drag
    QTest::mousePress(view, Qt::LeftButton, 0, QPoint(100, 100));

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseMove(view, QPoint(111, 111));
    QTest::qWait(50);
    QTest::mouseMove(view, QPoint(122, 122));

    QTRY_VERIFY(drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);
    QCOMPARE(drag->axis(), QQuickDrag::XAndYAxis);

    /* When blackRect.x becomes bigger than 75, the drag axis is changed to
     * Drag.YAxis by the QML code. Verify that this happens, and that the drag
     * movement is effectively constrained to the Y axis. */
    QTest::mouseMove(view, QPoint(144, 144));

    QTRY_COMPARE(blackRect->x(), 83.0);
    QTRY_COMPARE(blackRect->y(), 83.0);
    QTRY_COMPARE(drag->axis(), QQuickDrag::YAxis);

    QTest::mouseMove(view, QPoint(155, 155));

    QTRY_COMPARE(blackRect->y(), 94.0);
    QCOMPARE(blackRect->x(), 83.0);

    QTest::mouseRelease(view, Qt::LeftButton, 0, QPoint(155, 155));

    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 94.0);

    delete view;
}

#ifndef QT_NO_CURSOR
void tst_QQuickMouseArea::cursorShape()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n MouseArea {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(object.data());
    QVERIFY(mouseArea);

    QSignalSpy spy(mouseArea, SIGNAL(cursorShapeChanged()));

    QCOMPARE(mouseArea->cursorShape(), Qt::ArrowCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::ArrowCursor);

    mouseArea->setCursorShape(Qt::IBeamCursor);
    QCOMPARE(mouseArea->cursorShape(), Qt::IBeamCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(spy.count(), 1);

    mouseArea->setCursorShape(Qt::IBeamCursor);
    QCOMPARE(spy.count(), 1);

    mouseArea->setCursorShape(Qt::WaitCursor);
    QCOMPARE(mouseArea->cursorShape(), Qt::WaitCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::WaitCursor);
    QCOMPARE(spy.count(), 2);
}
#endif

void tst_QQuickMouseArea::moveAndReleaseWithoutPress()
{
    QQuickView *window = createView();

    window->setSource(testFileUrl("moveAndReleaseWithoutPress.qml"));
    window->show();
    QTest::qWaitForWindowExposed(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QObject *root = window->rootObject();
    QVERIFY(root);

    QTest::mousePress(window, Qt::LeftButton, 0, QPoint(100,100));

    QTest::mouseMove(window, QPoint(110,110), 50);
    QTRY_COMPARE(root->property("hadMove").toBool(), false);

    QTest::mouseRelease(window, Qt::LeftButton, 0, QPoint(110,110));
    QTRY_COMPARE(root->property("hadRelease").toBool(), false);

    delete window;
}

void tst_QQuickMouseArea::nestedStopAtBounds_data()
{
    QTest::addColumn<bool>("transpose");
    QTest::addColumn<bool>("invert");

    QTest::newRow("left") << false << false;
    QTest::newRow("right") << false << true;
    QTest::newRow("top") << true << false;
    QTest::newRow("bottom") << true << true;
}

void tst_QQuickMouseArea::nestedStopAtBounds()
{
    QFETCH(bool, transpose);
    QFETCH(bool, invert);

    QQuickView view;
    view.setSource(testFileUrl("nestedStopAtBounds.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject());

    QQuickMouseArea *outer =  view.rootObject()->findChild<QQuickMouseArea*>("outer");
    QVERIFY(outer);

    QQuickMouseArea *inner = outer->findChild<QQuickMouseArea*>("inner");
    QVERIFY(inner);
    inner->drag()->setAxis(transpose ? QQuickDrag::YAxis : QQuickDrag::XAxis);
    inner->setX(invert ? 100 : 0);
    inner->setY(invert ? 100 : 0);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 200);
    int &axis = transpose ? position.ry() : position.rx();

    // drag toward the aligned boundary.  Outer mouse area dragged.
    QTest::mousePress(&view, Qt::LeftButton, 0, position);
    QTest::qWait(10);
    axis += invert ? threshold * 2 : -threshold * 2;
    QTest::mouseMove(&view, position);
    axis += invert ? threshold : -threshold;
    QTest::mouseMove(&view, position);
    QCOMPARE(outer->drag()->active(), true);
    QCOMPARE(inner->drag()->active(), false);
    QTest::mouseRelease(&view, Qt::LeftButton, 0, position);

    QVERIFY(!outer->drag()->active());

    axis = 200;
    outer->setX(50);
    outer->setY(50);

    // drag away from the aligned boundary.  Inner mouse area dragged.
    QTest::mousePress(&view, Qt::LeftButton, 0, position);
    QTest::qWait(10);
    axis += invert ? -threshold * 2 : threshold * 2;
    QTest::mouseMove(&view, position);
    axis += invert ? -threshold : threshold;
    QTest::mouseMove(&view, position);
    QCOMPARE(outer->drag()->active(), false);
    QCOMPARE(inner->drag()->active(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, 0, position);
}

QTEST_MAIN(tst_QQuickMouseArea)

#include "tst_qquickmousearea.moc"
