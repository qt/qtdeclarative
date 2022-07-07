// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qquickwindow_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QQmlComponent>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

static bool isPlatformWayland()
{
    return !QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive);
}

class tst_HoverHandler : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_HoverHandler()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void hoverHandlerAndUnderlyingHoverHandler_data();
    void hoverHandlerAndUnderlyingHoverHandler();
    void mouseAreaAndUnderlyingHoverHandler();
    void hoverHandlerAndUnderlyingMouseArea();
    void movingItemWithHoverHandler();
    void margin();
    void window();
    void deviceCursor_data();
    void deviceCursor();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
};

void tst_HoverHandler::createView(QScopedPointer<QQuickView> &window, const char *fileName)
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

void tst_HoverHandler::hoverHandlerAndUnderlyingHoverHandler_data()
{
    QTest::addColumn<bool>("blocking");

    QTest::newRow("default: nonblocking") << false;
    QTest::newRow("blocking") << true;
}

void tst_HoverHandler::hoverHandlerAndUnderlyingHoverHandler()
{
    QFETCH(bool, blocking);

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "lesHoverables.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem * topSidebar = window->rootObject()->findChild<QQuickItem *>("topSidebar");
    QVERIFY(topSidebar);
    QQuickItem * button = topSidebar->findChild<QQuickItem *>("buttonWithHH");
    QVERIFY(button);
    QQuickHoverHandler *topSidebarHH = topSidebar->findChild<QQuickHoverHandler *>("topSidebarHH");
    QVERIFY(topSidebarHH);
    QQuickHoverHandler *buttonHH = button->findChild<QQuickHoverHandler *>("buttonHH");
    QVERIFY(buttonHH);

    QCOMPARE(buttonHH->isBlocking(), false); // default property value
    buttonHH->setBlocking(blocking);

    QPoint buttonCenter(button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QPoint rightOfButton(button->mapToScene(QPointF(button->width() + 2, button->height() / 2)).toPoint());
    QPoint outOfSidebar(topSidebar->mapToScene(QPointF(topSidebar->width() + 2, topSidebar->height() / 2)).toPoint());
    QSignalSpy sidebarHoveredSpy(topSidebarHH, SIGNAL(hoveredChanged()));
    QSignalSpy buttonHoveredSpy(buttonHH, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(topSidebarHH->isHovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), 0);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(topSidebarHH->isHovered(), !blocking);
    QCOMPARE(sidebarHoveredSpy.count(), blocking ? 2 : 1);
    QCOMPARE(buttonHH->isHovered(), true);
    QCOMPARE(buttonHoveredSpy.count(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::PointingHandCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), blocking ? 3 : 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(topSidebarHH->isHovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), blocking ? 4 : 2);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_HoverHandler::mouseAreaAndUnderlyingHoverHandler()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "lesHoverables.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem * topSidebar = window->rootObject()->findChild<QQuickItem *>("topSidebar");
    QVERIFY(topSidebar);
    QQuickMouseArea * buttonMA = topSidebar->findChild<QQuickMouseArea *>("buttonMA");
    QVERIFY(buttonMA);
    QQuickHoverHandler *topSidebarHH = topSidebar->findChild<QQuickHoverHandler *>("topSidebarHH");
    QVERIFY(topSidebarHH);

    // Ensure that we don't get extra hover events delivered on the
    // side, since it can affect the number of hover move events we receive below.
    QQuickWindowPrivate::get(window)->deliveryAgentPrivate()->frameSynchronousHoverEnabled = false;
    // And flush out any mouse events that might be queued up
    // in QPA, since QTest::mouseMove() calls processEvents.
    qGuiApp->processEvents();

    QPoint buttonCenter(buttonMA->mapToScene(QPointF(buttonMA->width() / 2, buttonMA->height() / 2)).toPoint());
    QPoint rightOfButton(buttonMA->mapToScene(QPointF(buttonMA->width() + 2, buttonMA->height() / 2)).toPoint());
    QPoint outOfSidebar(topSidebar->mapToScene(QPointF(topSidebar->width() + 2, topSidebar->height() / 2)).toPoint());
    QSignalSpy sidebarHoveredSpy(topSidebarHH, SIGNAL(hoveredChanged()));
    QSignalSpy buttonHoveredSpy(buttonMA, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(topSidebarHH->isHovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), 0);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), 1);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), 1);
    QCOMPARE(buttonMA->hovered(), true);
    QCOMPARE(buttonHoveredSpy.count(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::UpArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), 1);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(topSidebarHH->isHovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), 2);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_HoverHandler::hoverHandlerAndUnderlyingMouseArea()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "lesHoverables.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem * bottomSidebar = window->rootObject()->findChild<QQuickItem *>("bottomSidebar");
    QVERIFY(bottomSidebar);
    QQuickMouseArea *bottomSidebarMA = bottomSidebar->findChild<QQuickMouseArea *>("bottomSidebarMA");
    QVERIFY(bottomSidebarMA);
    QQuickItem * button = bottomSidebar->findChild<QQuickItem *>("buttonWithHH");
    QVERIFY(button);
    QQuickHoverHandler *buttonHH = button->findChild<QQuickHoverHandler *>("buttonHH");
    QVERIFY(buttonHH);

    QPoint buttonCenter(button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QPoint rightOfButton(button->mapToScene(QPointF(button->width() + 2, button->height() / 2)).toPoint());
    QPoint outOfSidebar(bottomSidebar->mapToScene(QPointF(bottomSidebar->width() + 2, bottomSidebar->height() / 2)).toPoint());
    QSignalSpy sidebarHoveredSpy(bottomSidebarMA, SIGNAL(hoveredChanged()));
    QSignalSpy buttonHoveredSpy(buttonHH, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(bottomSidebarMA->hovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), 0);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(bottomSidebarMA->hovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ClosedHandCursor);
#endif

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(bottomSidebarMA->hovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), 2);
    QCOMPARE(buttonHH->isHovered(), true);
    QCOMPARE(buttonHoveredSpy.count(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::PointingHandCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(bottomSidebarMA->hovered(), true);
    QCOMPARE(sidebarHoveredSpy.count(), 3);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ClosedHandCursor);
#endif

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(bottomSidebarMA->hovered(), false);
    QCOMPARE(sidebarHoveredSpy.count(), 4);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_HoverHandler::movingItemWithHoverHandler()
{
   if (isPlatformWayland())
        QSKIP("Wayland: QCursor::setPos() doesn't work.");

    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "lesHoverables.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem * paddle = window->rootObject()->findChild<QQuickItem *>("paddle");
    QVERIFY(paddle);
    QQuickHoverHandler *paddleHH = paddle->findChild<QQuickHoverHandler *>("paddleHH");
    QVERIFY(paddleHH);

    // Find the global coordinate of the paddle
    const QPoint p(paddle->mapToScene(paddle->clipRect().center()).toPoint());
    const QPoint paddlePos = window->mapToGlobal(p);

    // Now hide the window, put the cursor where the paddle was and show it again
    window->hide();
    QTRY_COMPARE(window->isVisible(), false);
    QCursor::setPos(paddlePos);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QTRY_COMPARE(paddleHH->isHovered(), true);
    // TODO check the cursor shape after fixing QTBUG-53987

    paddle->setX(100);
    QTRY_COMPARE(paddleHH->isHovered(), false);

    paddle->setX(p.x() - paddle->width() / 2);
    QTRY_COMPARE(paddleHH->isHovered(), true);

    paddle->setX(540);
    QTRY_COMPARE(paddleHH->isHovered(), false);
}

void tst_HoverHandler::margin() // QTBUG-85303
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "hoverMargin.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem * item = window->rootObject()->findChild<QQuickItem *>();
    QVERIFY(item);
    QQuickHoverHandler *hh = item->findChild<QQuickHoverHandler *>();
    QVERIFY(hh);

    QPoint itemCenter(item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
    QPoint leftMargin = itemCenter - QPoint(35, 35);
    QSignalSpy hoveredSpy(hh, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, {10, 10});
    QCOMPARE(hh->isHovered(), false);
    QCOMPARE(hoveredSpy.count(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, leftMargin);
    QCOMPARE(hh->isHovered(), true);
    QCOMPARE(hoveredSpy.count(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, itemCenter);
    QCOMPARE(hh->isHovered(), true);
    QCOMPARE(hoveredSpy.count(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, leftMargin);
    QCOMPARE(hh->isHovered(), true);
//    QCOMPARE(hoveredSpy.count(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, {10, 10});
    QCOMPARE(hh->isHovered(), false);
//    QCOMPARE(hoveredSpy.count(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_HoverHandler::window() // QTBUG-98717
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("windowCursorShape.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
#if QT_CONFIG(cursor)
    if (isPlatformWayland())
         QSKIP("Wayland: QCursor::setPos() doesn't work.");
    auto cursorPos = window->mapToGlobal(QPoint(100, 100));
    qCDebug(lcPointerTests) << "in window @" << window->position() << "setting cursor pos" << cursorPos;
    QCursor::setPos(cursorPos);
    if (!QTest::qWaitFor([cursorPos]{ return QCursor::pos() == cursorPos; }))
        QSKIP("QCursor::setPos() doesn't work (QTBUG-76312).");
    QTRY_COMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif
}

void tst_HoverHandler::deviceCursor_data()
{
    QTest::addColumn<bool>("synthMouseForTabletEvents");
    QTest::addColumn<bool>("earlierTabletBeforeMouse");

    QTest::newRow("nosynth, tablet wins") << false << false;
    QTest::newRow("synth, tablet wins") << true << false;
    QTest::newRow("synth, mouse wins") << true << true;
}

void tst_HoverHandler::deviceCursor()
{
    QFETCH(bool, synthMouseForTabletEvents);
    QFETCH(bool, earlierTabletBeforeMouse);
    qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents, synthMouseForTabletEvents);
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverDeviceCursors.qml")));
    // Ensure that we don't get extra hover events delivered on the side
    QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->frameSynchronousHoverEnabled = false;
    // And flush out any mouse events that might be queued up in QPA, since QTest::mouseMove() calls processEvents.
    qGuiApp->processEvents();
    const QQuickItem *root = window.rootObject();
    QQuickHoverHandler *stylusHandler = root->findChild<QQuickHoverHandler *>("stylus");
    QVERIFY(stylusHandler);
    QQuickHoverHandler *eraserHandler = root->findChild<QQuickHoverHandler *>("stylus eraser");
    QVERIFY(eraserHandler);
    QQuickHoverHandler *aibrushHandler = root->findChild<QQuickHoverHandler *>("airbrush");
    QVERIFY(aibrushHandler);
    QQuickHoverHandler *airbrushEraserHandler = root->findChild<QQuickHoverHandler *>("airbrush eraser");
    QVERIFY(airbrushEraserHandler);
    QQuickHoverHandler *mouseHandler = root->findChild<QQuickHoverHandler *>("mouse");
    QVERIFY(mouseHandler);

    QPoint point(100, 100);

#if QT_CONFIG(tabletevent)
    const qint64 stylusId = 1234567890;
    QElapsedTimer timer;
    timer.start();
    auto testStylusDevice = [&](QInputDevice::DeviceType dt, QPointingDevice::PointerType pt,
                                Qt::CursorShape expectedCursor, QQuickHoverHandler* expectedActiveHandler) {
        // We will follow up with a mouse event afterwards, and we want to simulate that the tablet events occur
        // either slightly before (earlierTabletBeforeMouse == true) or some time before.
        // It turns out that the first mouse move happens at timestamp 501 (simulated).
        const ulong timestamp = (earlierTabletBeforeMouse ? 0 : 400) + timer.elapsed();
        qCDebug(lcPointerTests) << "@" << timestamp << "sending" << dt << pt << "expecting" << expectedCursor << expectedActiveHandler->objectName();
        QWindowSystemInterface::handleTabletEvent(&window, timestamp, point, window.mapToGlobal(point),
                int(dt), int(pt), Qt::NoButton, 0, 0, 0, 0, 0, 0, stylusId, Qt::NoModifier);
        point += QPoint(1, 0);
#if QT_CONFIG(cursor)
        // QQuickItem::setCursor() doesn't get called: we only have HoverHandlers in this test
        QCOMPARE(root->cursor().shape(), Qt::ArrowCursor);
        QTRY_COMPARE(window.cursor().shape(), expectedCursor);
#endif
        QCOMPARE(stylusHandler->isHovered(), stylusHandler == expectedActiveHandler);
        QCOMPARE(eraserHandler->isHovered(), eraserHandler == expectedActiveHandler);
        QCOMPARE(aibrushHandler->isHovered(), aibrushHandler == expectedActiveHandler);
        QCOMPARE(airbrushEraserHandler->isHovered(), airbrushEraserHandler == expectedActiveHandler);
    };

    // simulate move events from various tablet stylus types
    testStylusDevice(QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
                     Qt::CrossCursor, stylusHandler);
    testStylusDevice(QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Eraser,
                     Qt::PointingHandCursor, eraserHandler);
    testStylusDevice(QInputDevice::DeviceType::Airbrush, QPointingDevice::PointerType::Pen,
                     Qt::BusyCursor, aibrushHandler);
    testStylusDevice(QInputDevice::DeviceType::Airbrush, QPointingDevice::PointerType::Eraser,
                     Qt::OpenHandCursor, airbrushEraserHandler);

    QTest::qWait(200);
    qCDebug(lcPointerTests) << "---- no more tablet events, now we send a mouse move";
#endif

    // move the mouse: the mouse-specific HoverHandler gets to set the cursor only if
    // more than kCursorOverrideTimeout ms have elapsed
    QTest::mouseMove(&window, point);
    QTRY_COMPARE(mouseHandler->isHovered(), true);
    const bool afterTimeout =
            QQuickPointerHandlerPrivate::get(airbrushEraserHandler)->lastEventTime + 100 <
            QQuickPointerHandlerPrivate::get(mouseHandler)->lastEventTime;
    qCDebug(lcPointerTests) << "airbrush handler reacted last time:" << QQuickPointerHandlerPrivate::get(airbrushEraserHandler)->lastEventTime
                            << "and the mouse handler reacted at time:" << QQuickPointerHandlerPrivate::get(mouseHandler)->lastEventTime
                            << "so > 100 ms have elapsed?" << afterTimeout;
#if QT_CONFIG(cursor)
    QCOMPARE(window.cursor().shape(), afterTimeout ? Qt::IBeamCursor : Qt::OpenHandCursor);
#endif
    QCOMPARE(stylusHandler->isHovered(), false);
    QCOMPARE(eraserHandler->isHovered(), false);
    QCOMPARE(aibrushHandler->isHovered(), false);
#if QT_CONFIG(tabletevent)
    QCOMPARE(airbrushEraserHandler->isHovered(), true); // there was no fresh QTabletEvent to tell it not to be hovered
#endif
}

QTEST_MAIN(tst_HoverHandler)

#include "tst_qquickhoverhandler.moc"
