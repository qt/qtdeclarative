// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QtGui/qstylehints.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickrepeater_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/private/qpointingdevice_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

#include <QtCore/qpointer.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_DragHandler : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_DragHandler()
         : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void initTestCase() override;

    void defaultPropertyValues();
    void touchDrag_data();
    void touchDrag();
    void mouseDrag_data();
    void mouseDrag();
    void mouseDragThreshold_data();
    void mouseDragThreshold();
    void dragFromMargin();
    void snapMode_data();
    void snapMode();
    void twoFingerDrag_data();
    void twoFingerDrag();
    void touchDragMulti();
    void touchDragMultiSliders_data();
    void touchDragMultiSliders();
    void touchPassiveGrabbers_data();
    void touchPassiveGrabbers();
    void touchPinchAndMouseMove();
    void unsuitableEventDuringDrag();
    void underModalLayer();
    void interruptedByIrrelevantButton();
    void touchDragExclusiveGrabber();
    void touchDragSceneRotated_data();
    void touchDragSceneRotated();

private:
    void sendWheelEvent(QQuickView &window, QPoint pos, QPoint angleDelta, QPoint pixelDelta, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase, bool inverted);
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QSet<QQuickPointerHandler *> passiveGrabbers(QQuickWindow *window, int pointId = 0);
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
};

void tst_DragHandler::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtils::centerOnScreen(window.data());
    QQuickViewTestUtils::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

QSet<QQuickPointerHandler*> tst_DragHandler::passiveGrabbers(QQuickWindow *window, int pointId /*= 0*/)
{
    Q_UNUSED(window);
    QSet<QQuickPointerHandler*> result;
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    for (auto &epd : devPriv->activePoints.values()) {
        auto passives = epd.passiveGrabbers;
        if (!pointId || epd.eventPoint.id() == pointId) {
            for (auto it = passives.constBegin(); it != passives.constEnd(); ++it)
                result << qobject_cast<QQuickPointerHandler *>(it->data());
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
    QCOMPARE(dragHandler->persistentTranslation(), QVector2D());
    QCOMPARE(dragHandler->activeTranslation(), QVector2D());
    QCOMPARE(dragHandler->centroid().position(), QPointF());
    QCOMPARE(dragHandler->centroid().scenePosition(), QPointF());
    QCOMPARE(dragHandler->centroid().pressPosition(), QPointF());
    QCOMPARE(dragHandler->centroid().scenePressPosition(), QPointF());
    QCOMPARE(dragHandler->centroid().sceneGrabPosition(), QPointF());
}

void tst_DragHandler::touchDrag_data()
{
    QTest::addColumn<int>("dragThreshold");
    QTest::newRow("threshold zero") << 0;
    QTest::newRow("threshold one") << 1;
    QTest::newRow("threshold 20") << 20;
    QTest::newRow("threshold default") << -1;
}

void tst_DragHandler::touchDrag()
{
    QFETCH(int, dragThreshold);
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);
    if (dragThreshold < 0) {
        dragThreshold = QGuiApplication::styleHints()->startDragDistance();
        QCOMPARE(dragHandler->dragThreshold(), dragThreshold);
    } else {
        dragHandler->setDragThreshold(dragThreshold);
    }

    QSignalSpy translationChangedSpy(dragHandler, &QQuickDragHandler::translationChanged);
    QSignalSpy centroidChangedSpy(dragHandler, SIGNAL(centroidChanged()));
    QSignalSpy xDeltaSpy(dragHandler->xAxis(), &QQuickDragAxis::activeValueChanged);

    QPointF ballCenter = ball->clipRect().center();
    QPointF scenePressPos = ball->mapToScene(ballCenter);
    QPoint p1 = scenePressPos.toPoint();
    QTest::touchEvent(window, touchscreen.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().position(), ballCenter);
    QCOMPARE(dragHandler->centroid().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->centroid().scenePosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().velocity(), QVector2D());
    QCOMPARE(centroidChangedSpy.size(), 1);
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    qCDebug(lcPointerTests) << "velocity after drag" << dragHandler->centroid().velocity();
    if (dragThreshold > 0)
        QTRY_VERIFY(!qFuzzyIsNull(dragHandler->centroid().velocity().x()));
    QCOMPARE(centroidChangedSpy.size(), 2);
    QVERIFY(!dragHandler->active());
    p1 += QPoint(1, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(translationChangedSpy.size(), 0);
    QCOMPARE(xDeltaSpy.size(), 0);
    QCOMPARE(centroidChangedSpy.size(), 3);
    QCOMPARE(dragHandler->persistentTranslation().x(), 0);
    QCOMPARE(dragHandler->activeTranslation().x(), 0);
    QPointF sceneGrabPos = p1;
    QCOMPARE(dragHandler->centroid().sceneGrabPosition(), sceneGrabPos);
    p1 += QPoint(19, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(dragHandler->centroid().position(), ballCenter);
    QCOMPARE(dragHandler->centroid().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->centroid().scenePosition(), ball->mapToScene(ballCenter));
    QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().sceneGrabPosition(), sceneGrabPos);
    QCOMPARE(dragHandler->persistentTranslation().x(), dragThreshold + 20);
    QCOMPARE(dragHandler->activeTranslation().x(), dragThreshold + 20);
    QCOMPARE(dragHandler->persistentTranslation().y(), 0);
    QCOMPARE(dragHandler->activeTranslation().y(), 0);
    QCOMPARE(translationChangedSpy.size(), 1);
    QCOMPARE(translationChangedSpy.first().first().value<QVector2D>(), QVector2D(dragThreshold + 20, 0));
    QVERIFY(dragHandler->centroid().velocity().x() > 0);
    QCOMPARE(centroidChangedSpy.size(), 4);
    QTest::touchEvent(window, touchscreen.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().pressedButtons(), Qt::NoButton);
    QCOMPARE(dragHandler->centroid().velocity(), QVector2D());
    QCOMPARE(ball->mapToScene(ballCenter).toPoint(), p1);
    QCOMPARE(translationChangedSpy.size(), 1);
    QCOMPARE(xDeltaSpy.size(), 1);
    QCOMPARE(xDeltaSpy.first().first().toReal(), dragThreshold + 20);
    QCOMPARE(centroidChangedSpy.size(), 5);
    QCOMPARE(dragHandler->persistentTranslation().x(), dragThreshold + 20);

    // Drag again: activeTranslation starts over, while persistentTranslation accumulates
    p1 = ball->mapToScene(ballCenter).toPoint();
    QTest::touchEvent(window, touchscreen.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(dragHandler->persistentTranslation().x(), dragThreshold + 20);
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    p1 += QPoint(1, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());
    p1 += QPoint(9, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(xDeltaSpy.size(), 2);
    QCOMPARE(xDeltaSpy.last().first().toReal(), dragThreshold + 10);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(dragHandler->activeTranslation().x(), dragThreshold + 20);
    QCOMPARE(dragHandler->persistentTranslation().x(), dragThreshold * 2 + 40);
    QCOMPARE(xDeltaSpy.size(), 3);
    QCOMPARE(xDeltaSpy.last().first().toReal(), 10);
    QTest::touchEvent(window, touchscreen.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);

    // Call setPersistentTranslation and drag yet again:
    // activeTranslation starts over, while persistentTranslation adds the drags onto the new basis
    dragHandler->setPersistentTranslation({10, 10});
    p1 = ball->mapToScene(ballCenter).toPoint();
    QTest::touchEvent(window, touchscreen.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(dragHandler->persistentTranslation().x(), 10);
    p1 += QPoint(dragThreshold, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    p1 += QPoint(1, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler->active());
    p1 += QPoint(9, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    p1 += QPoint(10, 0);
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(dragHandler->activeTranslation().x(), dragThreshold + 20);
    QCOMPARE(dragHandler->persistentTranslation().x(), dragThreshold + 30);
    QCOMPARE(xDeltaSpy.size(), 6);
    QCOMPARE(xDeltaSpy.last().first().toReal(), 10);
    QTest::touchEvent(window, touchscreen.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);
}

void tst_DragHandler::mouseDrag_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButtons>("dragButton");
    QTest::newRow("left: drag") << Qt::MouseButtons(Qt::LeftButton) << Qt::MouseButtons(Qt::LeftButton);
    QTest::newRow("right: don't drag") << Qt::MouseButtons(Qt::LeftButton) << Qt::MouseButtons(Qt::RightButton);
    QTest::newRow("left: don't drag") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::MouseButtons(Qt::LeftButton);
    QTest::newRow("right or middle: drag") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::MouseButtons(Qt::MiddleButton);
}

void tst_DragHandler::mouseDrag()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButtons, dragButton);
    bool shouldDrag = bool(acceptedButtons & dragButton);

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);
    dragHandler->setAcceptedButtons(acceptedButtons); // QTBUG-76875

    QSignalSpy translationChangedSpy(dragHandler, &QQuickDragHandler::translationChanged);
    QSignalSpy centroidChangedSpy(dragHandler, SIGNAL(centroidChanged()));
    QSignalSpy xDeltaSpy(dragHandler->xAxis(), &QQuickDragAxis::activeValueChanged);

    QPointF ballCenter = ball->clipRect().center();
    QPointF scenePressPos = ball->mapToScene(ballCenter);
    QPoint p1 = scenePressPos.toPoint();
    QTest::mousePress(window, static_cast<Qt::MouseButton>(int(dragButton)), Qt::NoModifier, p1, 500);
    QVERIFY(!dragHandler->active());
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
    if (shouldDrag) {
        QCOMPARE(dragHandler->centroid().position(), ballCenter);
        QCOMPARE(dragHandler->centroid().pressPosition(), ballCenter);
        QCOMPARE(dragHandler->centroid().scenePosition(), scenePressPos);
        QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
        QCOMPARE(dragHandler->centroid().velocity(), QVector2D());
        QCOMPARE(centroidChangedSpy.size(), 1);
    }
    p1 += QPoint(dragThreshold, 0);
    QTest::mouseMove(window, p1);
    if (shouldDrag) {
        QVERIFY(dragHandler->centroid().velocity().x() > 0);
        QCOMPARE(centroidChangedSpy.size(), 2);
        QVERIFY(!dragHandler->active());
#if QT_CONFIG(cursor)
        QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
    }
    p1 += QPoint(1, 0);
    QTest::mouseMove(window, p1);
    if (shouldDrag)
        QTRY_VERIFY(dragHandler->active());
    else
        QVERIFY(!dragHandler->active());
    QCOMPARE(translationChangedSpy.size(), 0);
    QCOMPARE(xDeltaSpy.size(), 0);
    if (shouldDrag)
        QCOMPARE(centroidChangedSpy.size(), 3);
    QCOMPARE(dragHandler->persistentTranslation().x(), 0.0);
    QCOMPARE(dragHandler->activeTranslation().x(), 0.0);
    QPointF sceneGrabPos = p1;
    if (shouldDrag)
        QCOMPARE(dragHandler->centroid().sceneGrabPosition(), sceneGrabPos);
    p1 += QPoint(19, 0);
    QTest::mouseMove(window, p1);
    QVERIFY(shouldDrag ? dragHandler->active() : !dragHandler->active());
    if (shouldDrag) {
        QCOMPARE(dragHandler->centroid().position(), ballCenter);
        QCOMPARE(dragHandler->centroid().pressPosition(), ballCenter);
        QCOMPARE(dragHandler->centroid().scenePosition(), ball->mapToScene(ballCenter));
        QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
        QCOMPARE(dragHandler->centroid().sceneGrabPosition(), sceneGrabPos);
        QCOMPARE(dragHandler->persistentTranslation().x(), dragThreshold + 20.0);
        QCOMPARE(dragHandler->activeTranslation().x(), dragThreshold + 20.0);
        QCOMPARE(dragHandler->persistentTranslation().y(), 0.0);
        QCOMPARE(dragHandler->activeTranslation().y(), 0.0);
        QVERIFY(dragHandler->centroid().velocity().x() > 0);
        QCOMPARE(centroidChangedSpy.size(), 4);
#if QT_CONFIG(cursor)
        QCOMPARE(window->cursor().shape(), Qt::ClosedHandCursor);
#endif
    }
    QTest::mouseRelease(window, static_cast<Qt::MouseButton>(int(dragButton)), Qt::NoModifier, p1);
    QTRY_VERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().pressedButtons(), Qt::NoButton);
    QCOMPARE(translationChangedSpy.size(), shouldDrag ? 1 : 0);
    if (shouldDrag) {
        QCOMPARE(ball->mapToScene(ballCenter).toPoint(), p1);
        QCOMPARE(translationChangedSpy.first().first().value<QVector2D>(), QVector2D(dragThreshold + 20, 0));
    }
    QCOMPARE(xDeltaSpy.size(), shouldDrag ? 1 : 0);
    QCOMPARE(centroidChangedSpy.size(), shouldDrag ? 5 : 0);
#if QT_CONFIG(cursor)
    QTest::mouseMove(window, p1 + QPoint(1, 0)); // TODO after fixing QTBUG-53987, don't send mouseMove
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_DragHandler::mouseDragThreshold_data()
{
    QTest::addColumn<int>("dragThreshold");
    QTest::newRow("threshold zero") << 0;
    QTest::newRow("threshold one") << 1;
    QTest::newRow("threshold 20") << 20;
    QTest::newRow("threshold default") << -1;
}

void tst_DragHandler::mouseDragThreshold()
{
    QFETCH(int, dragThreshold);
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draggables.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);
    if (dragThreshold < 0) {
        dragThreshold = QGuiApplication::styleHints()->startDragDistance();
        QCOMPARE(dragHandler->dragThreshold(), dragThreshold);
    } else {
        dragHandler->setDragThreshold(dragThreshold);
    }

    QSignalSpy translationChangedSpy(dragHandler, &QQuickDragHandler::translationChanged);
    QSignalSpy centroidChangedSpy(dragHandler, SIGNAL(centroidChanged()));
    QSignalSpy xDeltaSpy(dragHandler->xAxis(), &QQuickDragAxis::activeValueChanged);

    QPointF ballCenter = ball->clipRect().center();
    QPointF scenePressPos = ball->mapToScene(ballCenter);
    QPoint p1 = scenePressPos.toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().position(), ballCenter);
    QCOMPARE(dragHandler->centroid().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->centroid().scenePosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().velocity(), QVector2D());
    QCOMPARE(centroidChangedSpy.size(), 1);
    p1 += QPoint(qMax(1, dragThreshold), 0); // QTBUG-85431: zero-distance mouse moves are not delivered
    QTest::mouseMove(window, p1);
    if (dragThreshold > 0)
        QTRY_VERIFY(dragHandler->centroid().velocity().x() > 0);
    QCOMPARE(centroidChangedSpy.size(), 2);
    // the handler is not yet active, unless the drag threshold was already exceeded
    QCOMPARE(dragHandler->active(), dragThreshold == 0);
    p1 += QPoint(1, 0);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(translationChangedSpy.size(), dragThreshold ? 0 : 1);
    if (!dragThreshold)
        QCOMPARE(translationChangedSpy.first().first().value<QVector2D>(), QVector2D(2, 0));
    QCOMPARE(xDeltaSpy.size(), dragThreshold ? 0 : 1);
    QCOMPARE(centroidChangedSpy.size(), 3);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler->translation().x(), dragThreshold ? 0 : 2);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler->activeTranslation().x(), dragThreshold ? 0 : 2);
    QPointF sceneGrabPos = dragThreshold ? p1 : p1 - QPoint(1, 0);
    QCOMPARE(dragHandler->centroid().sceneGrabPosition(), sceneGrabPos);
    p1 += QPoint(19, 0);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(dragHandler->centroid().position(), ballCenter);
    QCOMPARE(dragHandler->centroid().pressPosition(), ballCenter);
    QCOMPARE(dragHandler->centroid().scenePosition(), ball->mapToScene(ballCenter));
    QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().sceneGrabPosition(), sceneGrabPos);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler->translation().x(), dragThreshold + (dragThreshold ? 20 : 21));
    QCOMPARE(dragHandler->translation().y(), 0.0);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler->activeTranslation().x(), dragThreshold + (dragThreshold ? 20 : 21));
    QCOMPARE(dragHandler->activeTranslation().y(), 0.0);
    QCOMPARE(translationChangedSpy.size(), dragThreshold ? 1 : 2);
    QCOMPARE(translationChangedSpy.first().first().value<QVector2D>(),
             QVector2D(dragThreshold ? dragThreshold + 20 : 2, 0));
    QVERIFY(dragHandler->centroid().velocity().x() > 0);
    QCOMPARE(centroidChangedSpy.size(), 4);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().pressedButtons(), Qt::NoButton);
    QCOMPARE(ball->mapToScene(ballCenter).toPoint(), p1);
    QCOMPARE(translationChangedSpy.size(), dragThreshold ? 1 : 2);
    QCOMPARE(xDeltaSpy.size(), dragThreshold ? 1 : 2);
    QCOMPARE(centroidChangedSpy.size(), 5);
}

void tst_DragHandler::dragFromMargin() // QTBUG-74966
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragMargin.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *draggableItem = window->rootObject()->childItems().first();
    QVERIFY(draggableItem);
    QQuickDragHandler *dragHandler = draggableItem->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QPointF originalPos = draggableItem->position();
    QPointF scenePressPos = originalPos - QPointF(10, 0);
    QPoint p1 = scenePressPos.toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().scenePosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
    p1 += QPoint(dragThreshold * 2, 0);
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(dragHandler->centroid().scenePressPosition(), scenePressPos);
    QCOMPARE(dragHandler->centroid().sceneGrabPosition(), p1);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler->translation().x(), 0.0); // hmm that's odd
    QCOMPARE(dragHandler->translation().y(), 0.0);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler->activeTranslation().x(), 0.0); // hmm that's odd
    QCOMPARE(dragHandler->activeTranslation().y(), 0.0);
    QCOMPARE(draggableItem->position(), originalPos + QPointF(dragThreshold * 2, 0));
#if QT_CONFIG(cursor)
    // The cursor doesn't change until the next event after the handler becomes active.
    p1 += QPoint(1, 0);
    QTest::mouseMove(window, p1);
    QTRY_COMPARE(window->cursor().shape(), Qt::ClosedHandCursor);
#endif
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().pressedButtons(), Qt::NoButton);
#if QT_CONFIG(cursor)
    QTRY_COMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_DragHandler::snapMode_data()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QTest::addColumn<QString>("subTree");
    QTest::addColumn<int>("snapMode");
    QTest::addColumn<QPoint>("startDragPos");
    QTest::addColumn<QPoint>("dragMovement");
    QTest::addColumn<QPoint>("expectedMovement");

    struct TestEntry {
        const char *desc;
        const char *subTree;
        QQuickDragHandler::SnapMode mode;
        QPoint startDragPos;
        QPoint dragMovement;
        QPoint expectedMovement;
    };

    TestEntry testdata[] = {
        {"outside the target", "rect1", QQuickDragHandler::SnapAuto, QPoint(45, -10), QPoint(dragThreshold*2, 0), QPoint(dragThreshold*2, 0)},
        {"inside the  target", "rect1", QQuickDragHandler::SnapAuto, QPoint(45,  10), QPoint(dragThreshold*2, 0), QPoint(dragThreshold*2, 0)},
        {"outside the target", "rect1", QQuickDragHandler::SnapAlways, QPoint(45, -10), QPoint(dragThreshold*2, 0), QPoint(dragThreshold*2, -50-10)},
        {"outside the target", "rect1", QQuickDragHandler::NoSnap, QPoint(45, -10), QPoint(dragThreshold*2, 0), QPoint(dragThreshold*2, 0)},
        {"outside the target", "rect1", QQuickDragHandler::SnapIfPressedOutsideTarget, QPoint(45, -10), QPoint(dragThreshold*2, 0), QPoint(dragThreshold*2, -50-10)},
        {"inside the target", "rect1", QQuickDragHandler::SnapIfPressedOutsideTarget, QPoint(45, 10), QPoint(dragThreshold*2, 0), QPoint(dragThreshold*2, 0)},
        //targets y pos moves from -25 to (25 + dragThreshold*2) because of snapping to center:
        {"outside target, should snap", "rect2", QQuickDragHandler::SnapAuto, QPoint(45, 50), QPoint(0, dragThreshold*2), QPoint(0, 25 + 25 + dragThreshold*2)},
        {"inside target, shouldn't snap", "rect2", QQuickDragHandler::SnapAuto, QPoint(45, 10), QPoint(0, dragThreshold*2), QPoint(0, dragThreshold*2)}
    };

    for (const TestEntry& e : testdata) {
        const QMetaEnum menum = QMetaEnum::fromType<QQuickDragHandler::SnapMode>();
        const QString dataTag = QString::fromLatin1("%1, %2, %3").arg(e.subTree).arg(menum.valueToKey(e.mode)).arg(e.desc);
        QTest::newRow(dataTag.toUtf8().constData()) << e.subTree << (int)e.mode
                << e.startDragPos << e.dragMovement << e.expectedMovement;
    }
}

void tst_DragHandler::snapMode()
{
    QFETCH(QString, subTree);
    QFETCH(QPoint, startDragPos);
    QFETCH(QPoint, dragMovement);
    QFETCH(int, snapMode);
    QFETCH(QPoint, expectedMovement);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "snapMode.qml");
    QQuickView * window = windowPtr.data();

    QQuickItem *rect1 = window->rootObject()->findChild<QQuickItem*>(subTree);
    QVERIFY(rect1);
    QQuickItem *rect1b = rect1->childItems().first();
    QVERIFY(rect1b);
    QQuickDragHandler *dragHandler1 = rect1->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler1);
    dragHandler1->setSnapMode((QQuickDragHandler::SnapMode)snapMode);
    QQuickItem *dragTarget = dragHandler1->target();
    QPointF oldTargetPos = dragTarget->position();

    QPoint p1 = rect1->mapToScene(QPointF(startDragPos)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!dragHandler1->active());
    p1 += dragMovement;
    QTest::mouseMove(window, p1);
    QTRY_VERIFY(dragHandler1->active());
    QCOMPARE(dragTarget->position(), oldTargetPos + expectedMovement);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_VERIFY(!dragHandler1->active());
    QCOMPARE(dragHandler1->centroid().pressedButtons(), Qt::NoButton);
}

void tst_DragHandler::twoFingerDrag_data()
{
    QTest::addColumn<QPoint>("p1");
    QTest::addColumn<QPoint>("p2");
    QTest::addColumn<bool>("shouldDrag");

    QTest::newRow("both start outside")
            << QPoint(80, 45) << QPoint(120, 45) << false;
    QTest::newRow("one starts outside")
            << QPoint(80, 45) << QPoint(120, 55) << false;
    QTest::newRow("both start inside")
            << QPoint(80, 55) << QPoint(120, 55) << true;
}

void tst_DragHandler::twoFingerDrag()
{
    QFETCH(QPoint, p1);
    QFETCH(QPoint, p2);
    QFETCH(bool, shouldDrag);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("multiFingerDrag.qml")));
    QQuickDragHandler *dragHandler = window.rootObject()->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);
    QSignalSpy activeSpy(dragHandler, &QQuickDragHandler::activeChanged);
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();

    // press two points, inside or outside the Rectangle
    QTest::QTouchEventSequence seq = QTest::touchEvent(&window, touchscreen.get());
    seq.press(1, p1, &window).press(2, p2, &window).commit();
    QQuickTouchUtils::flush(&window);

    // drag downwards and check whether DragHandler activates
    for (int i = 1; i <= 4; ++i) {
        p1 += QPoint(0, dragThreshold);
        p2 += QPoint(0, dragThreshold);
        if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
        seq.move(1, p1, &window).move(2, p2, &window).commit();
        QQuickTouchUtils::flush(&window);
        qCDebug(lcPointerTests) << i << "active" << dragHandler->active() << "pts" << p1 << p2;
        if (!shouldDrag)
            QCOMPARE(dragHandler->active(), false);
    }
    if (lcPointerTests().isDebugEnabled()) QTest::qWait(500);
    seq.release(1, p1, &window).release(2, p2, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(activeSpy.size(), shouldDrag ? 2 : 0);
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
    QSignalSpy translationChangedSpy1(dragHandler1, &QQuickDragHandler::translationChanged);
    QSignalSpy centroidChangedSpy1(dragHandler1, SIGNAL(centroidChanged()));
    QSignalSpy xDeltaSpy1(dragHandler1->xAxis(), &QQuickDragAxis::activeValueChanged);

    QQuickItem *ball2 = window->rootObject()->childItems().at(1);
    QVERIFY(ball2);
    QQuickDragHandler *dragHandler2 = ball2->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler2);
    QSignalSpy translationChangedSpy2(dragHandler2, &QQuickDragHandler::translationChanged);
    QSignalSpy centroidChangedSpy2(dragHandler1, SIGNAL(centroidChanged()));
    QSignalSpy yDeltaSpy2(dragHandler2->yAxis(), &QQuickDragAxis::activeValueChanged);

    QPointF ball1Center = ball1->clipRect().center();
    QPointF scenePressPos1 = ball1->mapToScene(ball1Center);
    QPoint p1 = scenePressPos1.toPoint();
    QPointF ball2Center = ball2->clipRect().center();
    QPointF scenePressPos2 = ball2->mapToScene(ball2Center);
    QPoint p2 = scenePressPos2.toPoint();
    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(window, touchscreen.get(), false);

    touchSeq.press(1, p1, window).commit();
    QQuickTouchUtils::flush(window);
    touchSeq.stationary(1).press(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler1->active());
    QCOMPARE(centroidChangedSpy1.size(), 2);
    QCOMPARE(dragHandler1->centroid().position(), ball1Center);
    QCOMPARE(dragHandler1->centroid().pressPosition(), ball1Center);
    QCOMPARE(dragHandler1->centroid().scenePosition(), scenePressPos1);
    QCOMPARE(dragHandler1->centroid().scenePressPosition(), scenePressPos1);
    QVERIFY(!dragHandler2->active());
    QCOMPARE(centroidChangedSpy2.size(), 2);
    QCOMPARE(dragHandler2->centroid().position(), ball2Center);
    QCOMPARE(dragHandler2->centroid().pressPosition(), ball2Center);
    QCOMPARE(dragHandler2->centroid().scenePosition(), scenePressPos2);
    QCOMPARE(dragHandler2->centroid().scenePressPosition(), scenePressPos2);
    p1 += QPoint(dragThreshold, 0);
    p2 += QPoint(0, dragThreshold);
    touchSeq.move(1, p1, window).move(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(!dragHandler1->active());
    QCOMPARE(centroidChangedSpy1.size(), 3);
    QCOMPARE(dragHandler1->centroid().position(), ball1Center + QPointF(dragThreshold, 0));
    QCOMPARE(dragHandler1->centroid().pressPosition(), ball1Center);
    QCOMPARE(dragHandler1->centroid().scenePosition().toPoint(), p1);
    QCOMPARE(dragHandler1->centroid().scenePressPosition(), scenePressPos1);
    QVERIFY(!dragHandler2->active());
    QCOMPARE(centroidChangedSpy2.size(), 3);
    QCOMPARE(dragHandler2->centroid().position(), ball2Center + QPointF(0, dragThreshold));
    QCOMPARE(dragHandler2->centroid().pressPosition(), ball2Center);
    QCOMPARE(dragHandler2->centroid().scenePosition().toPoint(), p2);
    QCOMPARE(dragHandler2->centroid().scenePressPosition(), scenePressPos2);
    p1 += QPoint(1, 0);
    p2 += QPoint(0, 1);
    touchSeq.move(1, p1, window).move(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(dragHandler1->active());
    QVERIFY(dragHandler2->active());
    QCOMPARE(translationChangedSpy1.size(), 0);
    QCOMPARE(xDeltaSpy1.size(), 0);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler1->translation().x(), 0.0);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler1->activeTranslation().x(), 0.0);
    QPointF sceneGrabPos1 = p1;
    QPointF sceneGrabPos2 = p2;
    QCOMPARE(dragHandler1->centroid().sceneGrabPosition(), sceneGrabPos1);
    QCOMPARE(dragHandler2->centroid().sceneGrabPosition(), sceneGrabPos2);
    p1 += QPoint(19, 0);
    p2 += QPoint(0, 19);
    QVERIFY(dragHandler2->active());
    QCOMPARE(translationChangedSpy2.size(), 0);
    QCOMPARE(yDeltaSpy2.size(), 0);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler2->translation().x(), 0.0);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler2->activeTranslation().x(), 0.0);
    QCOMPARE(dragHandler2->centroid().sceneGrabPosition(), sceneGrabPos2);
    touchSeq.move(1, p1, window).move(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(dragHandler1->active());
    QVERIFY(dragHandler2->active());
    QCOMPARE(dragHandler1->centroid().position(), ball1Center);
    QCOMPARE(dragHandler1->centroid().pressPosition(), ball1Center);
    QCOMPARE(dragHandler1->centroid().scenePosition(), ball1->mapToScene(ball1Center));
    QCOMPARE(dragHandler1->centroid().scenePressPosition(), scenePressPos1);
    QCOMPARE(dragHandler1->centroid().sceneGrabPosition(), sceneGrabPos1);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler1->translation().x(), dragThreshold + 20.0);
    QCOMPARE(dragHandler1->translation().y(), 0.0);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler1->activeTranslation().x(), dragThreshold + 20.0);
    QCOMPARE(dragHandler1->activeTranslation().y(), 0.0);
    QCOMPARE(dragHandler2->centroid().position(), ball2Center);
    QCOMPARE(dragHandler2->centroid().pressPosition(), ball2Center);
    QCOMPARE(dragHandler2->centroid().scenePosition(), ball2->mapToScene(ball2Center));
    QCOMPARE(dragHandler2->centroid().scenePressPosition(), scenePressPos2);
    QCOMPARE(dragHandler2->centroid().sceneGrabPosition(), sceneGrabPos2);
#if QT_DEPRECATED_SINCE(6, 2)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE(dragHandler2->translation().x(), 0.0);
    QCOMPARE(dragHandler2->translation().y(), dragThreshold + 20.0);
QT_WARNING_POP
#endif
    QCOMPARE(dragHandler2->activeTranslation().x(), 0.0);
    QCOMPARE(dragHandler2->activeTranslation().y(), dragThreshold + 20.0);
    QCOMPARE(xDeltaSpy1.size(), 1);
    QCOMPARE(xDeltaSpy1.first().first().toReal(), dragThreshold + 20);
    QCOMPARE(yDeltaSpy2.size(), 1);
    QCOMPARE(yDeltaSpy2.first().first().toReal(), dragThreshold + 20);
    QCOMPARE(translationChangedSpy1.size(), 1);
    QCOMPARE(translationChangedSpy1.first().first().value<QVector2D>(), QVector2D(dragThreshold + 20, 0));
    QCOMPARE(translationChangedSpy2.size(), 1);
    QCOMPARE(translationChangedSpy2.first().first().value<QVector2D>(), QVector2D(0, dragThreshold + 20));
    touchSeq.release(1, p1, window).stationary(2).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!dragHandler1->active());
    QVERIFY(dragHandler2->active());
    QCOMPARE(dragHandler1->centroid().pressedButtons(), Qt::NoButton);
    QCOMPARE(ball1->mapToScene(ball1Center).toPoint(), p1);
    QCOMPARE(translationChangedSpy1.size(), 1);
    touchSeq.release(2, p2, window).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!dragHandler2->active());
    QCOMPARE(ball2->mapToScene(ball2Center).toPoint(), p2);
    QCOMPARE(translationChangedSpy2.size(), 1);
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
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchscreen.get());

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
        QCOMPARE(endPosition, expectedEndPosition);
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
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchscreen.get());
    touch.press(1, p1.toPoint()).commit();
    QQuickTouchUtils::flush(window);

    QCOMPARE(passiveGrabbers(window), expectedPassiveGrabbers);

    QQuickDragHandler *dragHandler = nullptr;
    for (QQuickPointerHandler *handler: expectedPassiveGrabbers) {
        QPointF scenePressPos;
        if (QQuickMultiPointHandler *mph = qmlobject_cast<QQuickMultiPointHandler *>(handler))
            scenePressPos = mph->centroid().scenePressPosition();
        else
            scenePressPos = static_cast<QQuickSinglePointHandler *>(handler)->point().scenePressPosition();
        QCOMPARE(scenePressPos, p1);
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

void tst_DragHandler::touchPinchAndMouseMove()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "draghandler_and_pinchhandler.qml");
    QQuickView *window = windowPtr.data();
    QQuickItem *rect = window->rootObject()->findChild<QQuickItem*>(QLatin1String("Rect"));
    QQuickPointerHandler *pinchHandler = window->rootObject()->findChild<QQuickPointerHandler*>(QLatin1String("PinchHandler"));

    QPoint p1(150,200);
    QPoint p2(250,200);

    // Trigger a scale pinch, PinchHandler should activate
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchscreen.get());
    touch.press(1, p1).press(2, p2).commit();
    QQuickTouchUtils::flush(window);
    QPoint delta(10,0);
    for (int i = 0; i < 10 && !pinchHandler->active(); ++i) {
        p1-=delta;
        p2+=delta;
        touch.move(1, p1).move(2, p2).commit();
        QQuickTouchUtils::flush(window);
    }
    QCOMPARE(pinchHandler->active(), true);

    // While having the touch points pressed, send wrong mouse event as MS Windows did:
    //      * A MoveMove with LeftButton down
    // (in order to synthesize that, qtestMouseButtons needs to be modified)
    // (This will make the DragHandler do a passive grab)
    QTestPrivate::qtestMouseButtons = Qt::LeftButton;
    QTest::mouseMove(window, p1 + delta);

    touch.release(1, p1).release(2, p2).commit();
    QQuickTouchUtils::flush(window);

    // Now move the mouse with no buttons down and check if the rect did not move
    // At this point, no touch points are pressed and no mouse buttons are pressed.
    QTestPrivate::qtestMouseButtons = Qt::NoButton;
    QSignalSpy rectMovedSpy(rect, SIGNAL(xChanged()));
    for (int i = 0; i < 10; ++i) {
        p1 += delta;
        QTest::mouseMove(window, p1);
        QCOMPARE(rectMovedSpy.size(), 0);
    }
}

void tst_DragHandler::unsuitableEventDuringDrag()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragAndWheel.qml");
    QQuickView *window = windowPtr.data();
    auto root = window->rootObject();
    QQmlProperty changeCount(root, "changeCount");
    QQmlProperty wheelHandlerEnabled(root, "wheelHandlerEnabled");
    bool ok = false;
    QCOMPARE(changeCount.read().toInt(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(wheelHandlerEnabled.read().toBool(), true);

    QPoint p1(100, 100);
    QPoint p2(150, 150);

    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchscreen.get());
    // When we start dragging...
    touch.press(3,p1).commit();
    touch.move(3, p2).commit();
    QQuickTouchUtils::flush(window);
    // the DragHandler becomes active
    ok = false;
    QCOMPARE(changeCount.read().toInt(&ok), 1);
    QVERIFY(ok);
    QCOMPARE(wheelHandlerEnabled.read().toBool(), false);

    // When a scroll event arrives while we are dragging
    sendWheelEvent(*window, p2, QPoint(160, 120), QPoint(-360, 120), Qt::NoModifier, Qt::ScrollBegin, false);
    // nothing changes because the DragHandler is still active, and the wheel handler stays disabled
    ok = false;
    QCOMPARE(changeCount.read().toInt(&ok), 1);
    QVERIFY(ok);
    QCOMPARE(wheelHandlerEnabled.read().toBool(), false);

    // When we stop dragging...
    touch.release(3, p2).commit();
    QQuickTouchUtils::flush(window);

    // the wheel handler becomes active again
    ok = false;
    QCOMPARE(changeCount.read().toInt(&ok), 2);
    QVERIFY(ok);
    QCOMPARE(wheelHandlerEnabled.read().toBool(), true);

    // During the whole sequence the wheel handler never got a wheel event
    // as it was disabled:
    QQmlProperty gotWheel(root, "gotWheel");
    QVERIFY(!gotWheel.read().toBool());

    // If the WheelHandler is unconditionally enabled...
    wheelHandlerEnabled.write(true);
    // it receives scroll events during drags.
    touch.press(4,p2).commit();
    touch.move(4, p1).commit();
    QQuickTouchUtils::flush(window);
    sendWheelEvent(*window, p2, QPoint(160, 120), QPoint(-360, 120), Qt::NoModifier, Qt::ScrollBegin, false);
    touch.release(4, p2).commit();
    QQuickTouchUtils::flush(window);
    QVERIFY(gotWheel.read().toBool());
}

void tst_DragHandler::sendWheelEvent(QQuickView &window, QPoint pos, QPoint angleDelta, QPoint pixelDelta, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase, bool inverted)
{
    QWheelEvent wheelEvent(pos, window.mapToGlobal(pos), pixelDelta, angleDelta,
                           Qt::NoButton, modifiers, phase, inverted);
    QGuiApplication::sendEvent(&window, &wheelEvent);
    qApp->processEvents();
    QQuickTouchUtils::flush(&window);
}

class ModalLayer : public QQuickItem {
public:
    explicit ModalLayer(QQuickItem* parent = nullptr) : QQuickItem(parent) {
        this->setAcceptedMouseButtons(Qt::AllButtons);
        this->setAcceptTouchEvents(true);
        this->setKeepMouseGrab(true);
        this->setKeepTouchGrab(true);
    }

    bool event(QEvent* event) override {
        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseTrackingChange:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchCancel:
        case QEvent::TouchEnd: {
            qCDebug(lcPointerTests) << "BLOCK!" << event->type();
            return true;
        }
        default: break;
        }
        return QQuickItem::event(event);
    }
};

void tst_DragHandler::underModalLayer() // QTBUG-78258
{
    qmlRegisterType<ModalLayer>("Test", 1, 0, "ModalLayer");

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragHandlerUnderModalLayer.qml");
    QQuickView * window = windowPtr.data();
    QPointer<QQuickDragHandler> dragHandler = window->rootObject()->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QPoint p1(250, 250);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::mouseMove(window, p1);
    QVERIFY(!dragHandler->active());
    p1 += QPoint(dragThreshold, dragThreshold);
    QTest::mouseMove(window, p1);
    QVERIFY(!dragHandler->active());
    QTest::mouseRelease(window, Qt::LeftButton);
}

void tst_DragHandler::interruptedByIrrelevantButton() // QTBUG-102201
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("draggables.qml")));

    QQuickItem *ball = window.rootObject()->childItems().first();
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QCOMPARE(dragHandler->acceptedButtons(), Qt::LeftButton); // the default

    QSignalSpy translationChangedSpy(dragHandler, &QQuickDragHandler::translationChanged);
    QSignalSpy cancelSpy(dragHandler, &QQuickDragHandler::canceled);
    QSignalSpy activeSpy(dragHandler, &QQuickDragHandler::activeChanged);
    QSignalSpy grabSpy(dragHandler, &QQuickDragHandler::grabChanged);

    QPoint p1 = ball->mapToScene(ball->clipRect().center()).toPoint();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(grabSpy.size(), 1); // passive grab
    p1 += QPoint(dragThreshold + 1, 0);
    QTest::mouseMove(&window, p1);
    QVERIFY(dragHandler->active());
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(grabSpy.size(), 2); // exclusive grab
    QCOMPARE(translationChangedSpy.size(), 0);
    p1 += QPoint(1, 0);
    QTest::mouseMove(&window, p1);
    QCOMPARE(translationChangedSpy.size(), 1);

    // Left button is already held, now press right button too (chording)
    QTest::mousePress(&window, Qt::RightButton, Qt::NoModifier, p1);
    // DragHandler will ungrab and deactivate, but not cancel
    QCOMPARE(dragHandler->active(), false);
    QCOMPARE(translationChangedSpy.size(), 1);
    QCOMPARE(cancelSpy.size(), 0);
    QCOMPARE(activeSpy.size(), 2);
    QCOMPARE_GT(grabSpy.size(), 2); // lost grabs

    // Release right button: no change in state
    QTest::mouseRelease(&window, Qt::RightButton, Qt::NoModifier, p1);
    QCOMPARE(dragHandler->active(), false);
    QCOMPARE(translationChangedSpy.size(), 1);
    QCOMPARE(cancelSpy.size(), 0);
    QCOMPARE(activeSpy.size(), 2);
    auto grabChangedCount = grabSpy.size();

    // But the left button is still held, and it's possible to resume dragging
    p1 += QPoint(dragThreshold + 1, 0);
    QTest::mouseMove(&window, p1);
    QCOMPARE_GT(grabSpy.size(), grabChangedCount); // re-grabbed
    p1 += QPoint(1, 0);
    QTest::mouseMove(&window, p1);
    QVERIFY(dragHandler->active());
    QCOMPARE(activeSpy.size(), 3);
    QCOMPARE(translationChangedSpy.size(), 2);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(!dragHandler->active());
    QCOMPARE(activeSpy.size(), 4);
    QCOMPARE(cancelSpy.size(), 0); // none of this caused a canceled() signal
}

class TouchItem : public QQuickItem {
public:
    TouchItem(QQuickItem *parent = nullptr) : QQuickItem(parent)
    {
        setAcceptTouchEvents(true);
    }

protected:
    void touchEvent(QTouchEvent *ev) override
    {
        switch (ev->type()) {
        case QEvent::TouchBegin:
            ev->accept();
            break;
        default:
            break;
        }
    }
};
void tst_DragHandler::touchDragExclusiveGrabber()
{
    qmlRegisterType<TouchItem>("Test", 1, 0, "TouchItem");
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragHandlerTakeOverForbidden.qml")));

    QQuickItem *rect = window.rootObject()->findChild<QQuickItem *>();
    QVERIFY(rect);
    QQuickDragHandler *dragHandler = rect->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);

    QSignalSpy centroidChangedSpy(dragHandler, &QQuickDragHandler::centroidChanged);
    QSignalSpy dragHandlerActiveChangedSpy(dragHandler, &QQuickDragHandler::activeChanged);
    QSignalSpy grabChangedSpy(dragHandler, &QQuickDragHandler::grabChanged);

    QPointF rectCenter = rect->clipRect().center();
    QPointF scenePressPos = rect->mapToScene(rectCenter);
    QPoint p1 = scenePressPos.toPoint();
    auto dragThreshold = dragHandler->dragThreshold();

    QTest::QTouchEventSequence touchSeq = QTest::touchEvent(&window, touchscreen.get(), false);
    touchSeq.press(1, p1, &window).commit();
    QVERIFY(!dragHandler->active());
    QCOMPARE(dragHandler->centroid().velocity(), QVector2D());
    QCOMPARE(centroidChangedSpy.size(), 1);
    QCOMPARE(grabChangedSpy.size(), 1);

    p1 += QPoint(dragThreshold + 1, dragThreshold + 1);
    touchSeq.move(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(dragHandler->active(), false); // TakeOverForbidden so approveGrabTransition says no
    QCOMPARE(centroidChangedSpy.size(), 1);
    QCOMPARE(grabChangedSpy.size(), 1);
    QCOMPARE(grabChangedSpy.takeLast().at(0).toInt(), QPointingDevice::GrabTransition::GrabPassive);

    touchSeq.release(1, p1, &window).commit();
    QQuickTouchUtils::flush(&window);
    QVERIFY(!dragHandler->active());
    QCOMPARE(centroidChangedSpy.size(), 1);
    QCOMPARE(grabChangedSpy.size(), 1);
}

void tst_DragHandler::touchDragSceneRotated_data()
{
    QTest::addColumn<QString>("objectName");
    QTest::addColumn<QPoint>("movePoint");
    QTest::addColumn<bool>("moveExpected");

    QTest::newRow("Drag X on rotated scene when Axis Y is disabled")
            << "ballX" << QPoint{ 50, 0 } << false;
    QTest::newRow("Drag Y on rotated scene when Axis Y is disabled")
            << "ballX" << QPoint{ 0, 50 } << true;
    QTest::newRow("Drag X on rotated scene when Axis X is disabled")
            << "ballY" << QPoint{ 50, 0 } << true;
    QTest::newRow("Drag Y on rotated scene when Axis X is disabled")
            << "ballY" << QPoint{ 0, 50 } << false;
}

void tst_DragHandler::touchDragSceneRotated()
{
    QFETCH(QString, objectName);
    QFETCH(QPoint, movePoint);
    QFETCH(bool, moveExpected);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragSceneRotated.qml");
    QQuickView *window = windowPtr.data();

    QQuickItem *ball = window->rootObject()->findChild<QQuickItem *>(objectName);
    QVERIFY(ball);
    QQuickDragHandler *dragHandler = ball->findChild<QQuickDragHandler *>();
    QVERIFY(dragHandler);

    QPointF ballCenter = ball->clipRect().center();
    QPointF scenePressPos = ball->mapToScene(ballCenter);
    QPoint p1 = scenePressPos.toPoint();
    QTest::touchEvent(window, touchscreen.get()).press(1, p1, window);
    QQuickTouchUtils::flush(window);

    p1 += movePoint;
    QTest::touchEvent(window, touchscreen.get()).move(1, p1, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(dragHandler->active(), moveExpected);

    QTest::touchEvent(window, touchscreen.get()).release(1, p1, window);
    QQuickTouchUtils::flush(window);

    QCOMPARE_EQ(ball->mapToScene(ballCenter).toPoint(),
                moveExpected ? p1 : scenePressPos.toPoint());
}

QTEST_MAIN(tst_DragHandler)

#include "tst_qquickdraghandler.moc"

