// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickpointhandler_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qhighdpiscaling_p.h>
#include <private/qquickwindow_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QtCore/qpointer.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_PointHandler : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_PointHandler()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void initTestCase() override;

    void singleTouch();
    void tabletStylus_data();
    void tabletStylus();
    void simultaneousMultiTouch();
    void pressedMultipleButtons_data();
    void pressedMultipleButtons();
    void ignoreSystemSynthMouse();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
};

void tst_PointHandler::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickVisualTestUtils::centerOnScreen(window.data());
    QQuickVisualTestUtils::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

void tst_PointHandler::initTestCase()
{
    // This test assumes that we don't get synthesized mouse events from QGuiApplication
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    QQmlDataTest::initTestCase();
}

void tst_PointHandler::singleTouch()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "pointTracker.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem *tracker = window->rootObject()->findChild<QQuickItem *>("pointTracker");
    QVERIFY(tracker);
    QQuickPointHandler *handler = window->rootObject()->findChild<QQuickPointHandler *>("pointHandler");
    QVERIFY(handler);

    QSignalSpy activeSpy(handler, SIGNAL(activeChanged()));
    QSignalSpy pointSpy(handler, SIGNAL(pointChanged()));
    QSignalSpy translationSpy(handler, SIGNAL(translationChanged()));

    QPoint point(100,100);
    QTest::touchEvent(window, touchscreen.get()).press(1, point, window);
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 1);
    QCOMPARE(handler->point().position().toPoint(), point);
    QCOMPARE(handler->point().scenePosition().toPoint(), point);
    QCOMPARE(handler->point().pressedButtons(), Qt::NoButton);
    QCOMPARE(handler->translation(), QVector2D());
    QCOMPARE(translationSpy.size(), 1);

    point += QPoint(10, 10);
    QTest::touchEvent(window, touchscreen.get()).move(1, point, window);
    QQuickTouchUtils::flush(window);
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 2);
    QCOMPARE(handler->point().position().toPoint(), point);
    QCOMPARE(handler->point().scenePosition().toPoint(), point);
    QCOMPARE(handler->point().pressPosition().toPoint(), QPoint(100, 100));
    QCOMPARE(handler->point().scenePressPosition().toPoint(), QPoint(100, 100));
    QCOMPARE(handler->point().pressedButtons(), Qt::NoButton);
    QVERIFY(handler->point().velocity().x() > 0);
    QVERIFY(handler->point().velocity().y() > 0);
    QCOMPARE(handler->translation(), QVector2D(10, 10));
    QCOMPARE(translationSpy.size(), 2);

    QTest::touchEvent(window, touchscreen.get()).release(1, point, window);
    QQuickTouchUtils::flush(window);
    QTRY_COMPARE(handler->active(), false);
    QCOMPARE(activeSpy.size(), 2);
    QCOMPARE(pointSpy.size(), 3);
    QCOMPARE(handler->translation(), QVector2D());
    QCOMPARE(translationSpy.size(), 3);
}

void tst_PointHandler::tabletStylus_data()
{
    QTest::addColumn<bool>("guiSynthMouse");

    QTest::newRow("nosynth") << false;
    QTest::newRow("synth") << true;
}

void tst_PointHandler::tabletStylus()
{
    QFETCH(bool, guiSynthMouse);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents, guiSynthMouse);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pointTracker.qml")));

    QQuickPointHandler *handler = window.rootObject()->findChild<QQuickPointHandler *>("pointHandler");
    QVERIFY(handler);
    handler->setAcceptedDevices(QInputDevice::DeviceType::Stylus);
    // avoid generating a double click
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 10);

    QSignalSpy activeSpy(handler, &QQuickPointHandler::activeChanged);
    QSignalSpy pointSpy(handler, &QQuickPointHandler::pointChanged);
    QSignalSpy translationSpy(handler, &QQuickPointHandler::translationChanged);

    QPoint point(100,100);
    QPoint pointLocalDPI = QHighDpi::fromNativeLocalPosition(point, &window);
    const qint64 stylusId = 1234567890;
    ulong timestamp = 1000;

    QWindowSystemInterface::handleTabletEvent(&window, timestamp++, point, window.mapToGlobal(point),
        int(QInputDevice::DeviceType::Stylus), int(QPointingDevice::PointerType::Pen),
        Qt::LeftButton, 0.5, 25, 35, 0.6, 12.3, 3, stylusId, Qt::NoModifier);
    QTRY_COMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), guiSynthMouse ? 3 : 1);
    QCOMPARE(handler->point().position().toPoint(), pointLocalDPI);
    QCOMPARE(handler->point().scenePosition().toPoint(), pointLocalDPI);
    QCOMPARE(handler->point().pressedButtons(), Qt::LeftButton);
    QCOMPARE(handler->point().pressure(), 0.5);
    QCOMPARE(handler->point().rotation(), 12.3);
    QCOMPARE(handler->point().uniqueId().numericId(), stylusId);
    QCOMPARE(handler->translation(), QVector2D());
    QCOMPARE(translationSpy.size(), guiSynthMouse ? 3 : 1);

    QPoint delta(10, 10);
    QPoint deltaLocalDPI = QHighDpi::fromNativeLocalPosition(delta, &window);
    point += delta;
    QWindowSystemInterface::handleTabletEvent(&window, timestamp++, point, window.mapToGlobal(point),
        int(QInputDevice::DeviceType::Stylus), int(QPointingDevice::PointerType::Pen),
        Qt::LeftButton, 0.45, 23, 33, 0.57, 15.6, 3, stylusId, Qt::NoModifier);
    QTRY_COMPARE(pointSpy.size(), guiSynthMouse ? 5 : 2);
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(handler->point().position().toPoint(), pointLocalDPI + deltaLocalDPI);
    QCOMPARE(handler->point().scenePosition().toPoint(), pointLocalDPI + deltaLocalDPI);
    QCOMPARE(handler->point().pressPosition().toPoint(), pointLocalDPI);
    QCOMPARE(handler->point().scenePressPosition().toPoint(), pointLocalDPI);
    QCOMPARE(handler->point().pressedButtons(), Qt::LeftButton);
    QCOMPARE(handler->point().pressure(), 0.45);
    QCOMPARE(handler->point().rotation(), 15.6);
    QCOMPARE(handler->point().uniqueId().numericId(), stylusId);
    QCOMPARE_GT(handler->point().velocity().x(), 0);
    QCOMPARE_GT(handler->point().velocity().y(), 0);
    QCOMPARE(handler->translation(), QVector2D(deltaLocalDPI));
    QCOMPARE(translationSpy.size(), guiSynthMouse ? 5 : 2);

    QWindowSystemInterface::handleTabletEvent(&window, timestamp++, point, window.mapToGlobal(point),
        int(QInputDevice::DeviceType::Stylus), int(QPointingDevice::PointerType::Pen),
        Qt::NoButton, 0, 0, 0, 0, 0, 0, stylusId, Qt::NoModifier);
    QTRY_COMPARE(handler->active(), false);
    QCOMPARE(activeSpy.size(), 2);
    QCOMPARE(pointSpy.size(), guiSynthMouse ? 6 : 3);
    QCOMPARE(handler->translation(), QVector2D());
    QCOMPARE(translationSpy.size(), guiSynthMouse ? 6 : 3);
}

void tst_PointHandler::simultaneousMultiTouch()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "multiPointTracker.qml");
    QQuickView * window = windowPtr.data();
    QList<QQuickPointHandler *> handlers = window->rootObject()->findChildren<QQuickPointHandler *>();
    QCOMPARE(handlers.size(), 3);

    QVector<QSignalSpy*> activeSpies;
    QVector<QSignalSpy*> pointSpies;
    QVector<QSignalSpy*> translationSpies;
    QVector<QPoint> points{{100,100}, {200, 200}, {100, 300}};
    QVector<QPoint> pressPoints = points;
    for (auto h : handlers) {
        activeSpies << new QSignalSpy(h, SIGNAL(activeChanged()));
        pointSpies << new QSignalSpy(h, SIGNAL(pointChanged()));
        translationSpies << new QSignalSpy(h, SIGNAL(translationChanged()));
    }

    QTest::touchEvent(window, touchscreen.get()).press(1, points[0], window).press(2, points[1], window).press(3, points[2], window);
    QQuickTouchUtils::flush(window);
    QVector<int> pointIndexPerHandler;
    int i = 0;
    for (auto h : handlers) {
        QTRY_COMPARE(h->active(), true);
        QCOMPARE(activeSpies[i]->size(), 1);
        QCOMPARE(pointSpies[i]->size(), 1);
        int chosenPointIndex = points.indexOf(h->point().position().toPoint());
        QVERIFY(chosenPointIndex != -1);
        // Verify that each handler chose a unique point
        QVERIFY(!pointIndexPerHandler.contains(chosenPointIndex));
        pointIndexPerHandler.append(chosenPointIndex);
        QPoint point = points[chosenPointIndex];
        QCOMPARE(h->point().scenePosition().toPoint(), point);
        QCOMPARE(h->point().pressedButtons(), Qt::NoButton);
        QCOMPARE(h->translation(), QVector2D());
        QCOMPARE(translationSpies[i]->size(), 1);
        ++i;
    }

    for (int i = 0; i < 3; ++i)
        points[i] += QPoint(10 + 10 * i, 10 + 10 * i % 2);
    QTest::touchEvent(window, touchscreen.get()).move(1, points[0], window).move(2, points[1], window).move(3, points[2], window);
    QQuickTouchUtils::flush(window);
    i = 0;
    for (auto h : handlers) {
        QCOMPARE(h->active(), true);
        QCOMPARE(activeSpies[i]->size(), 1);
        QCOMPARE(pointSpies[i]->size(), 2);
        QCOMPARE(h->point().position().toPoint(), points[pointIndexPerHandler[i]]);
        QCOMPARE(h->point().scenePosition().toPoint(), points[pointIndexPerHandler[i]]);
        QCOMPARE(h->point().pressPosition().toPoint(), pressPoints[pointIndexPerHandler[i]]);
        QCOMPARE(h->point().scenePressPosition().toPoint(), pressPoints[pointIndexPerHandler[i]]);
        QCOMPARE(h->point().pressedButtons(), Qt::NoButton);
        QVERIFY(h->point().velocity().x() > 0);
        QVERIFY(h->point().velocity().y() > 0);
        QCOMPARE(h->translation(), QVector2D(10 + 10 * pointIndexPerHandler[i], 10 + 10 * pointIndexPerHandler[i] % 2));
        QCOMPARE(translationSpies[i]->size(), 2);
        ++i;
    }

    QTest::touchEvent(window, touchscreen.get()).release(1, points[0], window).release(2, points[1], window).release(3, points[2], window);
    QQuickTouchUtils::flush(window);
    i = 0;
    for (auto h : handlers) {
        QTRY_COMPARE(h->active(), false);
        QCOMPARE(activeSpies[i]->size(), 2);
        QCOMPARE(pointSpies[i]->size(), 3);
        QCOMPARE(h->translation(), QVector2D());
        QCOMPARE(translationSpies[i]->size(), 3);
        ++i;
    }

    qDeleteAll(activeSpies);
    qDeleteAll(pointSpies);
    qDeleteAll(translationSpies);
}

void tst_PointHandler::pressedMultipleButtons_data()
{
    QTest::addColumn<Qt::MouseButtons>("accepted");
    QTest::addColumn<QList<Qt::MouseButtons> >("buttons");
    QTest::addColumn<QList<bool> >("active");
    QTest::addColumn<QList<Qt::MouseButtons> >("pressedButtons");
    QTest::addColumn<int>("changeCount");
    QTest::addColumn<int>("activeChangeCount");

    QList<Qt::MouseButtons> buttons;
    QList<bool> active;
    QList<Qt::MouseButtons> pressedButtons;
    buttons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << Qt::NoButton;
    active << true
           << true
           << true
           << false;
    pressedButtons << Qt::LeftButton
                   << (Qt::LeftButton | Qt::RightButton)
                   << Qt::LeftButton
                   << Qt::NoButton;
    QTest::newRow("Accept Left - Press left, Press Right, Release Right")
            << Qt::MouseButtons(Qt::LeftButton) << buttons << active << pressedButtons << 4 << 4;

    buttons.clear();
    active.clear();
    pressedButtons.clear();
    buttons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::RightButton
            << Qt::NoButton;
    active << true
           << true
           << false
           << false;
    pressedButtons << Qt::LeftButton
                   << (Qt::LeftButton | Qt::RightButton)
                   << Qt::NoButton // Not the "truth" but filtered according to this handler's acceptedButtons
                   << Qt::NoButton;
    QTest::newRow("Accept Left - Press left, Press Right, Release Left")
            << Qt::MouseButtons(Qt::LeftButton) << buttons << active << pressedButtons << 3 << 4;

    buttons.clear();
    active.clear();
    pressedButtons.clear();
    buttons << Qt::LeftButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << Qt::NoButton;
    active << true
           << true
           << true
           << false;
    pressedButtons << Qt::LeftButton
                   << (Qt::LeftButton | Qt::RightButton)
                   << Qt::LeftButton
                   << Qt::NoButton;
    QTest::newRow("Accept Left|Right - Press left, Press Right, Release Right")
            << (Qt::LeftButton | Qt::RightButton) << buttons << active << pressedButtons << 4 << 4;

    buttons.clear();
    active.clear();
    pressedButtons.clear();
    buttons << Qt::RightButton
            << (Qt::LeftButton | Qt::RightButton)
            << Qt::LeftButton
            << Qt::NoButton;
    active << true
           << true
           << false
           << false;
    pressedButtons << Qt::RightButton
                   << (Qt::LeftButton | Qt::RightButton)
                   << Qt::NoButton // Not the "truth" but filtered according to this handler's acceptedButtons
                   << Qt::NoButton;
    QTest::newRow("Accept Right - Press Right, Press Left, Release Right")
            << Qt::MouseButtons(Qt::RightButton) << buttons << active << pressedButtons << 3 << 4;
}

void tst_PointHandler::pressedMultipleButtons()
{
    QFETCH(Qt::MouseButtons, accepted);
    QFETCH(QList<Qt::MouseButtons>, buttons);
    QFETCH(QList<bool>, active);
    QFETCH(QList<Qt::MouseButtons>, pressedButtons);
    QFETCH(int, changeCount);
    QFETCH(int, activeChangeCount);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "pointTracker.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem *tracker = window->rootObject()->findChild<QQuickItem *>("pointTracker");
    QVERIFY(tracker);
    QQuickPointHandler *handler = window->rootObject()->findChild<QQuickPointHandler *>("pointHandler");
    QVERIFY(handler);

    QSignalSpy activeSpy(handler, SIGNAL(activeChanged()));
    QSignalSpy pointSpy(handler, SIGNAL(pointChanged()));
    handler->setAcceptedButtons(accepted);

    QPoint point(100,100);

    for (int i = 0; i < buttons.size(); ++i) {
        int btns = int(buttons.at(i));
        int release = 0;
        if (i > 0) {
            int lastBtns = int(buttons.at(i - 1));
            release = lastBtns & ~btns;
        }
        if (release)
            QTest::mouseRelease(windowPtr.data(), Qt::MouseButton(release), Qt::NoModifier, point);
        else
            QTest::mousePress(windowPtr.data(), Qt::MouseButton(btns), Qt::NoModifier, point);

        qCDebug(lcPointerTests) << i << ": acceptedButtons" << handler->acceptedButtons()
                                << "; comparing" << handler->point().pressedButtons() << pressedButtons.at(i);
        QCOMPARE(handler->point().pressedButtons(), pressedButtons.at(i));
        QCOMPARE(handler->active(), active.at(i));
    }

    QTest::mousePress(windowPtr.data(), Qt::NoButton, Qt::NoModifier, point);
    QCOMPARE(handler->active(), false);
    QCOMPARE(activeSpy.size(), activeChangeCount);
    QCOMPARE(pointSpy.size(), changeCount);
}

void tst_PointHandler::ignoreSystemSynthMouse() // QTBUG-104890
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pointTracker.qml")));
    QQuickPointHandler *handler = window.rootObject()->findChild<QQuickPointHandler *>();
    QVERIFY(handler);
    auto devPriv = QPointingDevicePrivate::get(touchscreen.get());
    QSignalSpy activeSpy(handler, SIGNAL(activeChanged()));
    QSignalSpy pointSpy(handler, SIGNAL(pointChanged()));

    // touch press
    QPoint point(100,100);
    QTest::touchEvent(&window, touchscreen.get()).press(0, point, &window);
    QQuickTouchUtils::flush(&window);

    // touch move
    point += QPoint(10, 10);
    QTest::touchEvent(&window, touchscreen.get()).move(0, point, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 2);
    QVERIFY(devPriv->queryPointById(0)->passiveGrabbers.contains(handler));

    // Windows begins to synthesize mouse events in parallel with the touch event stream: move to touchpoint position, then press
    {
        QMouseEvent move(QEvent::MouseMove, point, point, window.mapToGlobal(point),
                          Qt::NoButton, Qt::NoButton, Qt::NoModifier, Qt::MouseEventSynthesizedBySystem, touchscreen.get());
        move.setTimestamp(235); // slightly after the last touch event
        QGuiApplication::sendEvent(&window, &move);
    }
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 2);
    QVERIFY(devPriv->queryPointById(0)->passiveGrabbers.contains(handler));
    {
        QMouseEvent press(QEvent::MouseButtonPress, point, point, window.mapToGlobal(point),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSynthesizedBySystem, touchscreen.get());
        press.setTimestamp(235);
        QGuiApplication::sendEvent(&window, &press);
    }
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 2);
    QVERIFY(devPriv->queryPointById(0)->passiveGrabbers.contains(handler));

    // another touch move
    point += QPoint(10, 10);
    QTest::touchEvent(&window, touchscreen.get()).move(0, point, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 3);
    QCOMPARE(handler->point().position().toPoint(), point);
    QCOMPARE(handler->point().scenePosition().toPoint(), point);
    QCOMPARE(handler->point().pressPosition().toPoint(), QPoint(100, 100));
    QCOMPARE(handler->point().scenePressPosition().toPoint(), QPoint(100, 100));
    QVERIFY(devPriv->queryPointById(0)->passiveGrabbers.contains(handler));

    // another fake mouse move
    {
        QMouseEvent move(QEvent::MouseMove, point, point, window.mapToGlobal(point),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSynthesizedBySystem, touchscreen.get());
        move.setTimestamp(240);
        QGuiApplication::sendEvent(&window, &move);
    }
    QCOMPARE(handler->active(), true);
    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(pointSpy.size(), 3);
    QCOMPARE(handler->point().position().toPoint(), point);
    QCOMPARE(handler->point().scenePosition().toPoint(), point);
    QCOMPARE(handler->point().pressPosition().toPoint(), QPoint(100, 100));
    QCOMPARE(handler->point().scenePressPosition().toPoint(), QPoint(100, 100));
    QVERIFY(devPriv->queryPointById(0)->passiveGrabbers.contains(handler));

    // end with released state
    QTest::touchEvent(&window, touchscreen.get()).release(0, point, &window);
    QMouseEvent release(QEvent::MouseButtonRelease, point, point, window.mapToGlobal(point),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    release.setTimestamp(280);
    QGuiApplication::sendEvent(&window, &release);
}

QTEST_MAIN(tst_PointHandler)

#include "tst_qquickpointhandler.moc"
