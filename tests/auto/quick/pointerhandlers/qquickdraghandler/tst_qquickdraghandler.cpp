/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickrepeater_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include "../../../shared/util.h"
#include "../../shared/viewtestutil.h"

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_DragHandler : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_DragHandler()
        :touchDevice(QTest::createTouchDevice())
    {}

private slots:
    void initTestCase();

    void defaultPropertyValues();
    void touchDrag();
    void mouseDrag();
    void touchDragMulti();
    void touchDragMultiSliders_data();
    void touchDragMultiSliders();
    void touchPassiveGrabbers_data();
    void touchPassiveGrabbers();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QSet<QQuickPointerHandler *> passiveGrabbers(QQuickWindow *window, int pointId = 0);
    QTouchDevice *touchDevice;
};

void tst_DragHandler::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtil::centerOnScreen(window.data());
    QQuickViewTestUtil::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

QSet<QQuickPointerHandler*> tst_DragHandler::passiveGrabbers(QQuickWindow *window, int pointId /*= 0*/)
{
    QSet<QQuickPointerHandler*> result;
    QQuickWindowPrivate *winp = QQuickWindowPrivate::get(window);
    if (QQuickPointerDevice* device = QQuickPointerDevice::touchDevice(touchDevice)) {
        QQuickPointerEvent *pointerEvent = winp->pointerEventInstance(device);
        for (int i = 0; i < pointerEvent->pointCount(); ++i) {
            QQuickEventPoint *eventPoint = pointerEvent->point(i);
            QVector<QPointer <QQuickPointerHandler> > passives = eventPoint->passiveGrabbers();
            if (!pointId || eventPoint->pointId() == pointId) {
                for (auto it = passives.constBegin(); it != passives.constEnd(); ++it)
                    result << it->data();
            }
        }
    }
    return result;
}

void tst_DragHandler::initTestCase()
{
    // This test assumes that we don't get synthesized mouse events from QGuiApplication
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    QQmlDataTest::initTestCase();
}

void tst_DragHandler::defaultPropertyValues()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QCOMPARE(dragHandler->acceptedButtons(), Qt::LeftButton);
    QCOMPARE(dragHandler->translation(), QVector2D());
    QCOMPARE(dragHandler->point().position(), QPointF());
    QCOMPARE(dragHandler->point().scenePosition(), QPointF());
    QCOMPARE(dragHandler->point().pressPosition(), QPointF());
    QCOMPARE(dragHandler->point().scenePressPosition(), QPointF());
    QCOMPARE(dragHandler->point().sceneGrabPosition(), QPointF());
}

void tst_DragHandler::touchDrag()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QSignalSpy translationChangedSpy(dragHandler, SIGNAL(translationChanged()));

    QPointF ballCenter = ball->clipRect().center();
    QPointF scenePressPos = ball->mapToScene(ballCenter);
    QPoint p1 = scenePressPos.toPoint();
    QTest::touchEvent(window, touchDevice).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->point().position(), ballCenter);
    QCOMPARE(dragHandler->point().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->point().scenePosition(), scenePressPos);
    QCOMPARE(dragHandler->point().scenePressPosition(), scenePressPos);
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler->active());
    p1 += QPoint(1, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(translationChangedSpy.count(), 0);
    QCOMPARE(dragHandler->translation().x(), 0.0);
    QPointF sceneGrabPos = p1;
    QCOMPARE(dragHandler->point().sceneGrabPosition(), sceneGrabPos);
    p1 += QPoint(19, 0);
    QTest::touchEvent(window, touchDevice).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(dragHandler->point().position(), ballCenter);
    QCOMPARE(dragHandler->point().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->point().scenePosition(), ball->mapToScene(ballCenter));
    QCOMPARE(dragHandler->point().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->point().sceneGrabPosition(), sceneGrabPos);
    QCOMPARE(dragHandler->translation().x(), dragThreshold + 20.0);
    QCOMPARE(dragHandler->translation().y(), 0.0);
    QTest::touchEvent(window, touchDevice).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->point().pressedButtons(), Qt::NoButton);
    QCOMPARE(ball->mapToScene(ballCenter).toPoint(), p1);
    QCOMPARE(translationChangedSpy.count(), 1);
}

void tst_DragHandler::mouseDrag()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QSignalSpy translationChangedSpy(dragHandler, SIGNAL(translationChanged()));

    QPointF ballCenter = ball->clipRect().center();
    QPointF scenePressPos = ball->mapToScene(ballCenter);
    QPoint p1 = scenePressPos.toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->point().position(), ballCenter);
    QCOMPARE(dragHandler->point().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->point().scenePosition(), scenePressPos);
    QCOMPARE(dragHandler->point().scenePressPosition(), scenePressPos);
    p1 += QPoint(dragThreshold, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(!dragHandler->active());
    p1 += QPoint(1, 0);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(translationChangedSpy.count(), 0);
    QCOMPARE(dragHandler->translation().x(), 0.0);
    QPointF sceneGrabPos = p1;
    QCOMPARE(dragHandler->point().sceneGrabPosition(), sceneGrabPos);
    p1 += QPoint(19, 0);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(dragHandler->point().position(), ballCenter);
    QCOMPARE(dragHandler->point().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->point().scenePosition(), ball->mapToScene(ballCenter));
    QCOMPARE(dragHandler->point().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->point().sceneGrabPosition(), sceneGrabPos);
    QCOMPARE(dragHandler->translation().x(), dragThreshold + 20.0);
    QCOMPARE(dragHandler->translation().y(), 0.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->point().pressedButtons(), Qt::NoButton);
    QCOMPARE(ball->mapToScene(ballCenter).toPoint(), p1);
    QCOMPARE(translationChangedSpy.count(), 1);
}

void tst_DragHandler::touchDragMulti()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball1 = window->rootObject()->childItems().first();
    QVERIFY(ball1);
    QQuickDragHandler *dragHandler1 = ball1->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler1);
    QSignalSpy translationChangedSpy1(dragHandler1, SIGNAL(translationChanged()));

    QQuickItem *ball2 = window->rootObject()->childItems().at(1);
    QVERIFY(ball2);
    QQuickDragHandler *dragHandler2 = ball2->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler2);
    QSignalSpy translationChangedSpy2(dragHandler2, SIGNAL(translationChanged()));

    QPointF ball1Center = ball1->clipRect().center();
    QPointF scenePressPos1 = ball1->mapToScene(ball1Center);
    QPoint p1 = scenePressPos1.toPoint();
    QPointF ball2Center = ball2->clipRect().center();
    QPointF scenePressPos2 = ball2->mapToScene(ball2Center);
    QPoint p2 = scenePressPos2.toPoint();

    QTest::touchEvent(window, touchDevice).press(1, p1, window).press(2, p2, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler1->active());
    QCOMPARE(dragHandler1->point().position(), ball1Center);
    QCOMPARE(dragHandler1->point().pressPosition(), ball1Center);
    QCOMPARE(dragHandler1->point().scenePosition(), scenePressPos1);
    QCOMPARE(dragHandler1->point().scenePressPosition(), scenePressPos1);
    QVERIFY(!dragHandler2->active());
    QCOMPARE(dragHandler2->point().position(), ball2Center);
    QCOMPARE(dragHandler2->point().pressPosition(), ball2Center);
    QCOMPARE(dragHandler2->point().scenePosition(), scenePressPos2);
    QCOMPARE(dragHandler2->point().scenePressPosition(), scenePressPos2);
    p1 += QPoint(dragThreshold, 0);
    p2 += QPoint(0, dragThreshold);
    QTest::touchEvent(window, touchDevice).move(1, p1, window).move(2, p2, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler1->active());
    p1 += QPoint(1, 0);
    p2 += QPoint(0, 1);
    QTest::touchEvent(window, touchDevice).move(1, p1, window).move(2, p2, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler1->active());
    QVERIFY(dragHandler2->active());
    QCOMPARE(translationChangedSpy1.count(), 0);
    QCOMPARE(dragHandler1->translation().x(), 0.0);
    QPointF sceneGrabPos1 = p1;
    QPointF sceneGrabPos2 = p2;
    QCOMPARE(dragHandler1->point().sceneGrabPosition(), sceneGrabPos1);
    QCOMPARE(dragHandler2->point().sceneGrabPosition(), sceneGrabPos2);
    p1 += QPoint(19, 0);
    p2 += QPoint(0, 19);
    QVERIFY(dragHandler2->active());
    QCOMPARE(translationChangedSpy2.count(), 0);
    QCOMPARE(dragHandler2->translation().x(), 0.0);
    QCOMPARE(dragHandler2->point().sceneGrabPosition(), sceneGrabPos2);
    QTest::touchEvent(window, touchDevice).move(1, p1, window).move(2, p2, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(dragHandler1->active());
    QVERIFY(dragHandler2->active());
    QCOMPARE(dragHandler1->point().position(), ball1Center);
    QCOMPARE(dragHandler1->point().pressPosition(), ball1Center);
    QCOMPARE(dragHandler1->point().scenePosition(), ball1->mapToScene(ball1Center));
    QCOMPARE(dragHandler1->point().scenePressPosition(), scenePressPos1);
    QCOMPARE(dragHandler1->point().sceneGrabPosition(), sceneGrabPos1);
    QCOMPARE(dragHandler1->translation().x(), dragThreshold + 20.0);
    QCOMPARE(dragHandler1->translation().y(), 0.0);
    QCOMPARE(dragHandler2->point().position(), ball2Center);
    QCOMPARE(dragHandler2->point().pressPosition(), ball2Center);
    QCOMPARE(dragHandler2->point().scenePosition(), ball2->mapToScene(ball2Center));
    QCOMPARE(dragHandler2->point().scenePressPosition(), scenePressPos2);
    QCOMPARE(dragHandler2->point().sceneGrabPosition(), sceneGrabPos2);
    QCOMPARE(dragHandler2->translation().x(), 0.0);
    QCOMPARE(dragHandler2->translation().y(), dragThreshold + 20.0);
    QTest::touchEvent(window, touchDevice).release(1, p1, window).stationary(2);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!dragHandler1->active());
    QVERIFY(dragHandler2->active());
    QCOMPARE(dragHandler1->point().pressedButtons(), Qt::NoButton);
    QCOMPARE(ball1->mapToScene(ball1Center).toPoint(), p1);
    QCOMPARE(translationChangedSpy1.count(), 1);
    QTest::touchEvent(window, touchDevice).release(2, p2, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!dragHandler2->active());
    QCOMPARE(ball2->mapToScene(ball2Center).toPoint(), p2);
    QCOMPARE(translationChangedSpy2.count(), 1);
}

void tst_DragHandler::touchDragMultiSliders_data()
{
    QTest::addColumn<int>("sliderRow");
    QTest::addColumn<QVector<int> >("whichSliders");
    QTest::addColumn<QVector<int> >("startingCenterOffsets");
    QTest::addColumn<QVector<QVector2D> >("movements");

    QTest::newRow("Drag Knob: start on the knobs, drag down") <<
        0 << QVector<int> { 0, 1, 2 } << QVector<int> { 0, 0, 0 } << QVector<QVector2D> { {0, 60}, {0, 60}, {0, 60} };
    QTest::newRow("Drag Knob: start on the knobs, drag diagonally downward") <<
        0 << QVector<int> { 0, 1, 2 } << QVector<int> { 0, 0, 0 } << QVector<QVector2D> { {20, 40}, {20, 60}, {20, 80} };
    QTest::newRow("Drag Anywhere: start on the knobs, drag down") <<
        1 << QVector<int> { 0, 1, 2 } << QVector<int> { 0, 0, 0 } << QVector<QVector2D> { {0, 60}, {0, 60}, {0, 60} };
    QTest::newRow("Drag Anywhere: start on the knobs, drag diagonally downward") <<
        1 << QVector<int> { 0, 1, 2 } << QVector<int> { 0, 0, 0 } << QVector<QVector2D> { {20, 40}, {20, 60}, {20, 80} };
    // TODO these next two fail because the DragHandler grabs when a finger
    // drags across it from outside, but should rather start only if it is pressed inside
//    QTest::newRow("Drag Knob: start above the knobs, drag down") <<
//        0 << QVector<int> { 0, 1, 2 } << QVector<int> { -30, -30, -30 } << QVector<QVector2D> { {0, 40}, {0, 60}, {0, 80} };
//    QTest::newRow("Drag Knob: start above the knobs, drag diagonally downward") <<
//        0 << QVector<int> { 0, 1, 2 } << QVector<int> { -30, -30, -30 } << QVector<QVector2D> { {20, 40}, {20, 60}, {20, 80} };
    QTest::newRow("Drag Anywhere: start above the knobs, drag down") <<
        1 << QVector<int> { 0, 1, 2 } << QVector<int> { -20, -30, -40 } << QVector<QVector2D> { {0, 60}, {0, 60}, {0, 60} };
    QTest::newRow("Drag Anywhere: start above the knobs, drag diagonally downward") <<
        1 << QVector<int> { 0, 1, 2 } << QVector<int> { -20, -30, -40 } << QVector<QVector2D> { {20, 40}, {20, 60}, {20, 80} };
}

void tst_DragHandler::touchDragMultiSliders()
{
    QFETCH(int, sliderRow);
    QFETCH(QVector<int>, whichSliders);
    QFETCH(QVector<int>, startingCenterOffsets);
    QFETCH(QVector<QVector2D>, movements);
    const int moveCount = 8;

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "multipleSliders.qml");
    QQuickView * window = windowPtr.data();
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchDevice);

    QQuickRepeater *rowRepeater = window->rootObject()->findChildren<QQuickRepeater *>()[sliderRow];
    QVector<QQuickItem *> knobs;
    QVector<QQuickDragHandler *> dragHandlers;
    QVector<QQuickTapHandler *> tapHandlers;
    QVector<QPointF> startPoints;
    for (int sli : whichSliders) {
        QQuickItem *slider = rowRepeater->itemAt(sli);
        QVERIFY(slider);
        dragHandlers << slider->findChild<QQuickDragHandler*>();
        QVERIFY(dragHandlers[sli]);
        tapHandlers << slider->findChild<QQuickTapHandler*>();
        QVERIFY(tapHandlers[sli]);
        knobs << tapHandlers[sli]->parentItem();
        QPointF startPoint = knobs[sli]->mapToScene(knobs[sli]->clipRect().center());
        startPoint.setY(startPoint.y() + startingCenterOffsets[sli]);
        startPoints << startPoint;
        qCDebug(lcPointerTests) << "row" << sliderRow << "slider" << sli << slider->objectName() <<
            "start" << startingCenterOffsets[sli] << startPoints[sli];
    }
    QVector<QPointF> touchPoints = startPoints;

    // Press
    for (int sli : whichSliders)
        touch.press(sli, touchPoints[sli].toPoint());
    touch.commit();

    // Moves
    for (int m = 0; m < moveCount; ++m) {
        for (int sli : whichSliders) {
            QVector2D incr = movements[sli] / moveCount;
            touchPoints[sli] += incr.toPointF();
            touch.move(sli, touchPoints[sli].toPoint());
        }
        touch.commit();
        QQuickTouchUtils::flush(window);
    }

    // Check that they moved to where they should: since the slider is constrained,
    // only the y component should have an effect; knobs should not come out of their "grooves"
    for (int sli : whichSliders) {
        QPoint endPosition = knobs[sli]->mapToScene(knobs[sli]->clipRect().center()).toPoint();
        QPoint expectedEndPosition(startPoints[sli].x(), startPoints[sli].y() + movements[sli].y());
        if (sliderRow == 0 && qAbs(startingCenterOffsets[sli]) > knobs[sli]->height() / 2)
            expectedEndPosition = startPoints[sli].toPoint();
        qCDebug(lcPointerTests) << "slider " << knobs[sli]->objectName() << "started @" << startPoints[sli]
            << "tried to move by" << movements[sli] << "ended up @" << endPosition << "expected" << expectedEndPosition;
        QTRY_COMPARE(endPosition, expectedEndPosition);
    }

    // Release
    for (int sli : whichSliders)
        touch.release(sli, touchPoints[sli].toPoint());
    touch.commit();
}

void tst_DragHandler::touchPassiveGrabbers_data()
{
    QTest::addColumn<QString>("itemName");
    QTest::addColumn<QStringList>("expectedPassiveGrabberNames");

    QTest::newRow("Drag And Tap") << "dragAndTap" << QStringList({"drag", "tap"});
    QTest::newRow("Tap And Drag") << "tapAndDrag" << QStringList({"tap", "drag"});
    QTest::newRow("Drag And Tap (not siblings)") << "dragAndTapNotSiblings" << QStringList({"drag", "tap"});
    QTest::newRow("Tap And Drag (not siblings)") << "tapAndDragNotSiblings" << QStringList({"tap", "drag"});
}

void tst_DragHandler::touchPassiveGrabbers()
{
    QFETCH(QString, itemName);
    QFETCH(QStringList, expectedPassiveGrabberNames);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "simpleTapAndDragHandlers.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *row2 = window->rootObject()->findChild<QQuickItem*>(itemName);
    QSet<QQuickPointerHandler *> expectedPassiveGrabbers;
    for (QString objectName : expectedPassiveGrabberNames)
        expectedPassiveGrabbers << row2->findChild<QQuickPointerHandler*>(objectName);

    QPointF p1 = row2->mapToScene(row2->clipRect().center());
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchDevice);
    touch.press(1, p1.toPoint()).commit();
    QQuickTouchUtils::flush(window);

    QCOMPARE(passiveGrabbers(window), expectedPassiveGrabbers);

    QQuickDragHandler *dragHandler = nullptr;
    for (QQuickPointerHandler *handler: expectedPassiveGrabbers) {
        QCOMPARE(static_cast<QQuickSinglePointHandler *>(handler)->point().scenePressPosition(), p1);
        QQuickDragHandler *dh = qmlobject_cast<QQuickDragHandler *>(handler);
        if (dh)
            dragHandler = dh;
    }
    QVERIFY(dragHandler);
    QPointF initialPos = dragHandler->target()->position();

    p1 += QPointF(50, 50);
    touch.move(1, p1.toPoint()).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());

    p1 += QPointF(50, 50);
    touch.move(1, p1.toPoint()).commit();
    QQuickTouchUtils::flush(window);
    QPointF movementDelta = dragHandler->target()->position() - initialPos;
    qCDebug(lcPointerTests) << "DragHandler moved the target by" << movementDelta;
    QVERIFY(movementDelta.x() >= 100);
    QVERIFY(movementDelta.y() >= 100);

    QTest::qWait(500);

    touch.release(1, p1.toPoint());
    touch.commit();
    QQuickTouchUtils::flush(window);
}

QTEST_MAIN(tst_DragHandler)

#include "tst_qquickdraghandler.moc"

