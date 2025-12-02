// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qguiapplication_p.h>
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
    void disabledHoverHandlerAndUnderlyingMouseArea();
    void hoverHandlerOnDisabledItem();
    void movingItemWithHoverHandler();
    void margin();
    void window();
    void deviceCursor_data();
    void deviceCursor();
    void addHandlerFromCpp();
    void ensureHoverHandlerWorksWhenItemHasHoverDisabled();
    void changeCursor();
    void touchDrag();
    void twoHandlersTwoTouches();
    void asProperty();
    void effectivelyClips_data();
    void effectivelyClips();
    void grandChildOutOfBounds();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);

    std::unique_ptr<QPointingDevice> touchscreen{QTest::createTouchDevice()};
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
    QCOMPARE(sidebarHoveredSpy.size(), 0);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(topSidebarHH->isHovered(), !blocking);
    QCOMPARE(sidebarHoveredSpy.size(), blocking ? 2 : 1);
    QCOMPARE(buttonHH->isHovered(), true);
    QCOMPARE(buttonHoveredSpy.size(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::PointingHandCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), blocking ? 3 : 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(topSidebarHH->isHovered(), false);
    QCOMPARE(sidebarHoveredSpy.size(), blocking ? 4 : 2);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
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
    QCOMPARE(sidebarHoveredSpy.size(), 0);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonMA->hovered(), true);
    QCOMPARE(buttonHoveredSpy.size(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::UpArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(topSidebarHH->isHovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(topSidebarHH->isHovered(), false);
    QCOMPARE(sidebarHoveredSpy.size(), 2);
    QCOMPARE(buttonMA->hovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
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
    QCOMPARE(sidebarHoveredSpy.size(), 0);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(bottomSidebarMA->hovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ClosedHandCursor);
#endif

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(bottomSidebarMA->hovered(), false);
    QCOMPARE(sidebarHoveredSpy.size(), 2);
    QCOMPARE(buttonHH->isHovered(), true);
    QCOMPARE(buttonHoveredSpy.size(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::PointingHandCursor);
#endif

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(bottomSidebarMA->hovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 3);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ClosedHandCursor);
#endif

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(bottomSidebarMA->hovered(), false);
    QCOMPARE(sidebarHoveredSpy.size(), 4);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif
}

void tst_HoverHandler::disabledHoverHandlerAndUnderlyingMouseArea()
{
    // Check that if a disabled HoverHandler is installed on an item, it
    // will not participate in hover event delivery, and as such, also
    // not block propagation to siblings.
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

    // By disabling the HoverHandler, it should no longer
    // block the sibling MouseArea underneath from receiving hover events.
    buttonHH->setEnabled(false);

    QPoint buttonCenter(button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QPoint rightOfButton(button->mapToScene(QPointF(button->width() + 2, button->height() / 2)).toPoint());
    QPoint outOfSidebar(bottomSidebar->mapToScene(QPointF(bottomSidebar->width() + 2, bottomSidebar->height() / 2)).toPoint());
    QSignalSpy sidebarHoveredSpy(bottomSidebarMA, SIGNAL(hoveredChanged()));
    QSignalSpy buttonHoveredSpy(buttonHH, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, outOfSidebar);
    QCOMPARE(bottomSidebarMA->hovered(), false);
    QCOMPARE(sidebarHoveredSpy.size(), 0);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(bottomSidebarMA->hovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(bottomSidebarMA->hovered(), true);
    QCOMPARE(sidebarHoveredSpy.size(), 1);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);
}

void tst_HoverHandler::hoverHandlerOnDisabledItem()
{
    // Check that if HoverHandler on a disabled item will
    // continue to receive hover events (QTBUG-30801)
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "lesHoverables.qml");
    QQuickView * window = windowPtr.data();
    QQuickItem * bottomSidebar = window->rootObject()->findChild<QQuickItem *>("bottomSidebar");
    QVERIFY(bottomSidebar);
    QQuickItem * button = bottomSidebar->findChild<QQuickItem *>("buttonWithHH");
    QVERIFY(button);
    QQuickHoverHandler *buttonHH = button->findChild<QQuickHoverHandler *>("buttonHH");
    QVERIFY(buttonHH);

    // Disable the button/rectangle item. This should not
    // block its HoverHandler from being hovered
    button->setEnabled(false);

    QPoint buttonCenter(button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QPoint rightOfButton(button->mapToScene(QPointF(button->width() + 2, button->height() / 2)).toPoint());
    QSignalSpy buttonHoveredSpy(buttonHH, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 0);

    QTest::mouseMove(window, buttonCenter);
    QCOMPARE(buttonHH->isHovered(), true);
    QCOMPARE(buttonHoveredSpy.size(), 1);

    QTest::mouseMove(window, rightOfButton);
    QCOMPARE(buttonHH->isHovered(), false);
    QCOMPARE(buttonHoveredSpy.size(), 2);
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
    // If the cursor is in a specific known position and the window is shown under it,
    // QGuiApplicationPrivate::lastCursorPosition must be set.
    // Usually, QGuiApplicationPrivate::processEnterEvent() will do that.
    // Otherwise this test will fail (not Qt Quick's fault).
    // We do not call QTest::mouseMove() here, because we are testing the expectation
    // that QQuickDeliveryAgentPrivate::flushFrameSynchronousEvents() updates the hover
    // state of items that move under or away from the last known mouse cursor position.
    if (!QTest::qWaitFor([paddlePos]() { return QGuiApplicationPrivate::lastCursorPosition.toPoint() == paddlePos; }))
        QSKIP("QCursor::setPos() doesn't work, or didn't update QGuiApplicationPrivate::lastCursorPosition");
    qCDebug(lcPointerTests) << "QGuiApplicationPrivate::lastCursorPosition after QCursor::setPos()"
                            << QGuiApplicationPrivate::lastCursorPosition.toPoint();

    QTRY_COMPARE(paddleHH->isHovered(), true);
    QTRY_COMPARE(window->cursor().shape(), Qt::SizeVerCursor);

    const auto &deliveryTargets =
            QQuickPointerHandlerPrivate::deviceDeliveryTargets(QPointingDevice::primaryPointingDevice());
    const auto targetsCount = deliveryTargets.size();
    qCDebug(lcPointerTests) << "deviceDeliveryTargets before paddle movement" << deliveryTargets;
    paddle->setX(100);
    QTRY_COMPARE(paddleHH->isHovered(), false);
    // QQuickDeliveryAgentPrivate::deliverHoverEvent() clears the deviceDeliveryTargets list,
    // and then each HoverHandler's QQuickPointerHandler::handlePointerEvent() adds itself again.
    // As long as we visit the same handlers each time, the list should not grow. (QTBUG-135975)
    qCDebug(lcPointerTests) << "deviceDeliveryTargets after paddle movement" << deliveryTargets;
    QCOMPARE_LE(deliveryTargets.size(), targetsCount);

    paddle->setX(p.x() - paddle->width() / 2);
    QTRY_COMPARE(paddleHH->isHovered(), true);
    QCOMPARE_LE(deliveryTargets.size(), targetsCount);

    paddle->setX(540);
    QTRY_COMPARE(paddleHH->isHovered(), false);
    QCOMPARE_LE(deliveryTargets.size(), targetsCount);
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
    const auto margin = hh->margin();
    const auto itemPriv = QQuickItemPrivate::get(hh->parentItem());
    QCOMPARE(itemPriv->biggestPointerHandlerMargin(), margin);
    QCOMPARE(itemPriv->eventHandlingBounds(),
             hh->parentItem()->boundingRect().marginsAdded({margin, margin, margin, margin}));

    QPoint itemCenter(item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
    QPoint leftMargin = itemCenter - QPoint(35, 35);
    QSignalSpy hoveredSpy(hh, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, {10, 10});
    QCOMPARE(hh->isHovered(), false);
    QCOMPARE(hoveredSpy.size(), 0);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(window, leftMargin);
    QCOMPARE(hh->isHovered(), true);
    QCOMPARE(hoveredSpy.size(), 1);
#if QT_CONFIG(cursor)
    QCOMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif

    QTest::mouseMove(window, itemCenter);
    QCOMPARE(hh->isHovered(), true);
    QCOMPARE(hoveredSpy.size(), 1);
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
    const QPoint pos(100, 100);
    component.loadUrl(testFileUrl("windowCursorShape.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->setFramePosition(pos);
    window->show();
    QTRY_COMPARE(window->framePosition(), pos);
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
#if !QT_CONFIG(tabletevent)
    QSKIP("This test depends on QTabletEvent delivery.");
#endif
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
    QQuickHoverHandler *conflictingMouseHandler = root->findChild<QQuickHoverHandler *>("conflictingMouse");
    QVERIFY(conflictingMouseHandler);

    QPoint point(100, 100);

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
        qCDebug(lcPointerTests) << "mouse HoverHandlers hovered?"
                                << mouseHandler->isHovered() << conflictingMouseHandler->isHovered();
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

    qCDebug(lcPointerTests) << "---- no more tablet events, now we send a mouse move";

    // move the mouse: the mouse-specific HoverHandler gets to set the cursor only if
    // more than kCursorOverrideTimeout ms have elapsed (100ms)
    QTest::mouseMove(&window, point, 100);
    QTRY_IMPL(mouseHandler->isHovered() == true, 500);
    const bool afterTimeout =
            QQuickPointerHandlerPrivate::get(airbrushEraserHandler)->lastEventTime + 100 <
            QQuickPointerHandlerPrivate::get(mouseHandler)->lastEventTime;
    qCDebug(lcPointerTests) << "airbrush handler reacted last time:" << QQuickPointerHandlerPrivate::get(airbrushEraserHandler)->lastEventTime
                            << "and the mouse handler reacted at time:" << QQuickPointerHandlerPrivate::get(mouseHandler)->lastEventTime
                            << "so > 100 ms have elapsed?" << afterTimeout;
    if (afterTimeout)
        QCOMPARE(mouseHandler->isHovered(), true);
    else
        QSKIP("Failed to delay mouse move 100ms after the previous tablet event");

#if QT_CONFIG(cursor)
    QCOMPARE(window.cursor().shape(), afterTimeout ? Qt::IBeamCursor : Qt::OpenHandCursor);
#endif
    QCOMPARE(stylusHandler->isHovered(), false);
    QCOMPARE(eraserHandler->isHovered(), false);
    QCOMPARE(aibrushHandler->isHovered(), false);
    QCOMPARE(airbrushEraserHandler->isHovered(), true); // there was no fresh QTabletEvent to tell it not to be hovered

    // hover with the stylus again, then move the mouse outside the handlers' parent item
    testStylusDevice(QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
                     Qt::CrossCursor, stylusHandler);
    QTest::mouseMove(&window, QPoint(180, 180));
    // the mouse has left the item: all its HoverHandlers should be unhovered (QTBUG-116505)
    QCOMPARE(stylusHandler->isHovered(), false);
    QCOMPARE(eraserHandler->isHovered(), false);
    QCOMPARE(aibrushHandler->isHovered(), false);
    QCOMPARE(airbrushEraserHandler->isHovered(), false);
    QCOMPARE(mouseHandler->isHovered(), false);
}

void tst_HoverHandler::addHandlerFromCpp()
{
    // Check that you can create a hover handler from c++, and add it
    // as a child of an existing item. Continue to check that you can
    // also change the parent item at runtime.
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("nohandler.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickItem *childItem = window->findChild<QQuickItem *>("childItem");
    QVERIFY(childItem);

    // Move mouse outside child
    const QPoint outside(200, 200);
    const QPoint inside(50, 50);
    QTest::mouseMove(window.data(), outside);

    QQuickHoverHandler *handler = new QQuickHoverHandler(childItem);
    QSignalSpy spy(handler, &QQuickHoverHandler::hoveredChanged);

    // Move mouse inside child
    QTest::mouseMove(window.data(), inside);
    QVERIFY(handler->isHovered());
    QCOMPARE(spy.size(), 1);

    // Move mouse outside child
    QTest::mouseMove(window.data(), outside);
    QVERIFY(!handler->isHovered());
    QCOMPARE(spy.size(), 2);

    // Remove the parent item from the handler
    spy.clear();
    handler->setParentItem(nullptr);

    // Move mouse inside child
    QTest::mouseMove(window.data(), inside);
    QVERIFY(!handler->isHovered());
    QCOMPARE(spy.size(), 0);

    // Move mouse outside child
    QTest::mouseMove(window.data(), outside);
    QVERIFY(!handler->isHovered());
    QCOMPARE(spy.size(), 0);

    // Reparent back the item to the handler
    spy.clear();
    handler->setParentItem(childItem);

    // Move mouse inside child
    QTest::mouseMove(window.data(), inside);
    QVERIFY(handler->isHovered());
    QCOMPARE(spy.size(), 1);

    // Move mouse outside child
    QTest::mouseMove(window.data(), outside);
    QVERIFY(!handler->isHovered());
    QCOMPARE(spy.size(), 2);
}

void tst_HoverHandler::ensureHoverHandlerWorksWhenItemHasHoverDisabled()
{
    // Check that a hover handler with a leaf item as parent, continues to
    // receive hover, even if the item itself stops listening for hover.
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("nohandler.qml"));
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(component.create()));
    QVERIFY(!window.isNull());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickItem *childItem = window->findChild<QQuickItem *>("childItem");
    QVERIFY(childItem);

    // Move mouse outside child
    const QPoint outside(200, 200);
    const QPoint inside(50, 50);
    QTest::mouseMove(window.data(), outside);

    QQuickHoverHandler *handler = new QQuickHoverHandler(childItem);

    // Toggle hover on the item. This should not clear subtreeHoverEnabled
    // on the item as a whole, since it still has a hover handler.
    childItem->setAcceptHoverEvents(true);
    childItem->setAcceptHoverEvents(false);
    QSignalSpy spy(handler, &QQuickHoverHandler::hoveredChanged);

    // Move mouse inside child
    QTest::mouseMove(window.data(), inside);
    QVERIFY(handler->isHovered());
    QCOMPARE(spy.size(), 1);

    // Move mouse outside child
    QTest::mouseMove(window.data(), outside);
    QVERIFY(!handler->isHovered());
    QCOMPARE(spy.size(), 2);
}

void tst_HoverHandler::changeCursor()
{
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "changingCursor.qml");
    QQuickView * window = windowPtr.data();
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickItem *item = window->findChild<QQuickItem *>("brownRect");
    QVERIFY(item);
    QQuickHoverHandler *hh = item->findChild<QQuickHoverHandler *>();
    QVERIFY(hh);

    QPoint itemCenter(item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
    QSignalSpy hoveredSpy(hh, SIGNAL(hoveredChanged()));

    QTest::mouseMove(window, itemCenter);

    QTRY_COMPARE(hoveredSpy.size(), 1);

#if QT_CONFIG(cursor)
    QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
    QTRY_COMPARE(window->cursor().shape(), Qt::OpenHandCursor);
    QTRY_COMPARE(window->cursor().shape(), Qt::CrossCursor);
    QTRY_COMPARE(window->cursor().shape(), Qt::OpenHandCursor);
#endif
}

void tst_HoverHandler::touchDrag()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverHandler.qml")));
    const QQuickItem *root = window.rootObject();
    QQuickHoverHandler *handler = root->findChild<QQuickHoverHandler *>();
    QVERIFY(handler);

    // polishAndSync() calls flushFrameSynchronousEvents() before emitting afterAnimating()
    QSignalSpy frameSyncSpy(&window, &QQuickWindow::afterAnimating);

    const QPoint out(root->width() - 1, root->height() / 2);
    QPoint in(root->width() / 2, root->height() / 2);

    QTest::touchEvent(&window, touchscreen.get()).press(0, out, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(handler->isHovered(), false);

    frameSyncSpy.clear();
    QTest::touchEvent(&window, touchscreen.get()).move(0, in, &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(handler->isHovered(), true);
    QCOMPARE(handler->point().scenePosition().toPoint(), in);

    in += {10, 10};
    QTest::touchEvent(&window, touchscreen.get()).move(0, in, &window);
    QQuickTouchUtils::flush(&window);
    // ensure that the color change is visible
    QTRY_COMPARE_GE(frameSyncSpy.size(), 1);
    QCOMPARE(handler->isHovered(), true);
    QCOMPARE(handler->point().scenePosition().toPoint(), in);

    QTest::touchEvent(&window, touchscreen.get()).move(0, out, &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE_GE(frameSyncSpy.size(), 2);
    QCOMPARE(handler->isHovered(), false);

    QTest::touchEvent(&window, touchscreen.get()).release(0, out, &window);
}

void tst_HoverHandler::twoHandlersTwoTouches()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoHandlers.qml")));
    const QQuickItem *root = window.rootObject();
    QQuickHoverHandler *left = root->findChild<QQuickHoverHandler *>("left");
    QVERIFY(left);
    QQuickHoverHandler *right = root->findChild<QQuickHoverHandler *>("right");
    QVERIFY(right);

    const QPoint pl = left->parentItem()->boundingRect().center().toPoint();
    const QPoint pr = right->parentItem()->position().toPoint() + QPoint(10, 10);

    // showView() moved the mouse outside the window before showing it,
    // so we don't expect mouse interference: this is a pure touchscreen test.
    // Press the left HoverHandler: flushFrameSynchronousEvents acts
    // as if the cursor is there, and sends a hover event.
    QTest::touchEvent(&window, touchscreen.get()).press(0, pl, &window);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(left->isHovered(), true);
    QCOMPARE(right->isHovered(), false);

    // press the right HoverHandler too: it doesn't hover, because only one subtree can be hovered (for now, at least)
    QTest::touchEvent(&window, touchscreen.get()).stationary(0).press(1, pr, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(right->isHovered(), false);
    QCOMPARE(left->isHovered(), true);

    // release the left: neither HoverHandler is hovered, even though the right one is still pressed
    QTest::touchEvent(&window, touchscreen.get()).release(0, pl, &window).stationary(1);
    QQuickTouchUtils::flush(&window);
    QTRY_COMPARE(left->isHovered(), false);
    QCOMPARE(right->isHovered(), false);

    // release the right
    QTest::touchEvent(&window, touchscreen.get()).release(1, pr, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(left->isHovered(), false);
    QCOMPARE(right->isHovered(), false);
}

void tst_HoverHandler::asProperty()
{
    QQuickView window;
    window.setFlag(Qt::FramelessWindowHint, true);
    QVERIFY(QQuickTest::showView(window, testFileUrl("asProperty.qml")));
    const QQuickItem *root = window.rootObject();
    QQuickHoverHandler *handler = root->property("handler").value<QQuickHoverHandler *>();
    QVERIFY(handler);
    QCOMPARE(handler->isHovered(), false);
    QTest::mouseMove(&window, root->boundingRect().center().toPoint());
    QTRY_COMPARE(handler->isHovered(), true);
}

void tst_HoverHandler::effectivelyClips_data()
{
    QTest::addColumn<QPoint>("cursorPos");
    QTest::addColumn<QPoint>("goatPos");
    QTest::addColumn<qreal>("scale");
    QTest::addColumn<int>("rotation");
    QTest::addColumn<bool>("expectRootContainsChildren");
    QTest::addColumn<bool>("expectShadowContainsChildren");
    QTest::addColumn<bool>("expectFrameContainsChildren");
    QTest::addColumn<bool>("expectShadowHovered");
    QTest::addColumn<bool>("expectFrameHovered");
    QTest::addColumn<bool>("expectGoatHovered");
    QTest::addColumn<Qt::CursorShape>("expectedCursor");

    QTest::newRow("shrinkAndRotate") << QPoint(90, 150) << QPoint() << 0.7 << 15
                                     << true << true << false   << false << true << false << Qt::UpArrowCursor;
    QTest::newRow("rotate") << QPoint(90, 150) << QPoint() << 1.0 << 10
                            << true << true << false   << false << true << true << Qt::SizeAllCursor;
    QTest::newRow("pokeHornsOut") << QPoint(90, 150) << QPoint(0, -10) << 1.0 << 0
                                  << true << true << false   << false << true << false << Qt::UpArrowCursor;
    QTest::newRow("pokeHornsWayOut") << QPoint(90, 150) << QPoint(0, -30) << 1.0 << 0
                                     << true << true << false   << false << true << false << Qt::UpArrowCursor;
}

void tst_HoverHandler::effectivelyClips() // QTBUG-140340
{
    QFETCH(QPoint, cursorPos);
    QFETCH(QPoint, goatPos);
    QFETCH(qreal, scale);
    QFETCH(int, rotation);
    QFETCH(bool, expectRootContainsChildren);
    QFETCH(bool, expectShadowContainsChildren);
    QFETCH(bool, expectFrameContainsChildren);
    QFETCH(bool, expectShadowHovered);
    QFETCH(bool, expectFrameHovered);
    QFETCH(bool, expectGoatHovered);
    QFETCH(Qt::CursorShape, expectedCursor);

    // reset counters
#ifdef QT_BUILD_INTERNAL
    QQuickItemPrivate::eventHandlingChildrenWithinBounds_counter = 0;
    QQuickItemPrivate::itemToParentTransform_counter = 0;
    QQuickItemPrivate::itemToWindowTransform_counter = 0;
    QQuickItemPrivate::windowToItemTransform_counter = 0;
    QQuickItemPrivate::effectiveClippingSkips_counter = 0;
#endif

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("goat.qml")));
    QSignalSpy renderSpy(&window, &QQuickWindow::afterRendering);
    QQuickItem *root = window.rootObject();
    QQuickItemPrivate *rootPrivate = QQuickItemPrivate::get(root);
    QQuickHoverHandler *shadowHandler = root->findChild<QQuickHoverHandler *>("shadow");
    QVERIFY(shadowHandler);
    QQuickItemPrivate *shadowPrivate = QQuickItemPrivate::get(shadowHandler->parentItem());
    QQuickHoverHandler *frameHandler = root->findChild<QQuickHoverHandler *>("frame");
    QVERIFY(frameHandler);
    QQuickItemPrivate *framePrivate = QQuickItemPrivate::get(frameHandler->parentItem());
    QQuickHoverHandler *goatHandler = root->findChild<QQuickHoverHandler *>("goat");
    QQuickItem *goat = goatHandler->parentItem();
    QVERIFY(goatHandler);
    QQuickHoverHandler *pupilHandler = root->findChild<QQuickHoverHandler *>("pupil");
    QVERIFY(pupilHandler);

    // nothing poking out, so far
    QVERIFY(rootPrivate->effectivelyClipsEventHandlingChildren());
    QVERIFY(shadowPrivate->effectivelyClipsEventHandlingChildren());
    QVERIFY(framePrivate->effectivelyClipsEventHandlingChildren());

    // expect to initially hover the pupil of the eye
    const QPoint cursorGlobalPos = window.mapToGlobal(cursorPos);
    QCursor::setPos(cursorGlobalPos);
    bool cursorSet = true;
    if (!QTest::qWaitFor([cursorGlobalPos]() {
            return QGuiApplicationPrivate::lastCursorPosition.toPoint() == cursorGlobalPos; })) {
        qCDebug(lcPointerTests) << "QCursor::setPos doesn't work: expected"
                                << cursorGlobalPos << "got" << QGuiApplicationPrivate::lastCursorPosition;
        cursorSet = false;
    }

    auto checkPupilHovered = [pupilHandler, goatHandler, frameHandler, shadowHandler, &window]() {
        QTRY_COMPARE(pupilHandler->isHovered(), true);
        QCOMPARE(goatHandler->isHovered(), true);
        QCOMPARE(frameHandler->isHovered(), true);
        QCOMPARE(shadowHandler->isHovered(), false);
        QCOMPARE(window.cursor(), Qt::CrossCursor);
    };
    auto checkOtherHovered = [pupilHandler, goatHandler, frameHandler, shadowHandler,
                              expectShadowHovered, expectFrameHovered, expectGoatHovered,
                              &window, expectedCursor]() {
        qCDebug(lcPointerTests) << "hovered"
                                << pupilHandler->isHovered() << shadowHandler->isHovered()
                                << frameHandler->isHovered() << goatHandler->isHovered()
                                << "cursor" << window.cursor();
        QTRY_COMPARE(pupilHandler->isHovered(), false);
        QCOMPARE(goatHandler->isHovered(), expectGoatHovered);
        QCOMPARE(frameHandler->isHovered(), expectFrameHovered);
        QCOMPARE(shadowHandler->isHovered(), expectShadowHovered);
        QCOMPARE(window.cursor(), expectedCursor);
    };
    if (cursorSet)
        checkPupilHovered();

    // fake an animation by changing properties back and forth, watch hover and cursor changes
    for (int i = 0; i < 10; ++i) {
        int renderCount = renderSpy.size();
        if (i % 2) {
            goat->setPosition({});
            goat->setScale(1);
            goat->setRotation(0);
            QTRY_COMPARE_GT(renderSpy.size(), renderCount);
            if (cursorSet)
                checkPupilHovered();
        } else {
            // If the goat's rectangular bounds poke out of the frame, the frame notices;
            // but the shadow has no child items.
            // If it pokes outside the declared root item as well, though,
            // rootPrivate->eventHandlingChildrenWithinBounds doesn't currently get updated.
            // Perhaps it should: but that would be more expensive
            // (transformChanged() would need to traverse up the hierarchy every time).
            goat->setPosition(goatPos);
            goat->setScale(scale);
            goat->setRotation(rotation);
            QTRY_COMPARE_GT(renderSpy.size(), renderCount);
            QCOMPARE(shadowPrivate->effectivelyClipsEventHandlingChildren(), expectShadowContainsChildren);
            qCDebug(lcPointerTests) << "step" << i << ": item contains children:"
                                    << rootPrivate->effectivelyClipsEventHandlingChildren()
                                    << framePrivate->effectivelyClipsEventHandlingChildren()
                                    << "expected" << expectRootContainsChildren << expectFrameContainsChildren;
            if (i > 0)
                QCOMPARE(framePrivate->effectivelyClipsEventHandlingChildren(), expectFrameContainsChildren);
            QCOMPARE(rootPrivate->effectivelyClipsEventHandlingChildren(), expectRootContainsChildren);
            if (cursorSet)
                checkOtherHovered();
        }
    }

#ifdef QT_BUILD_INTERNAL
    qCDebug(lcPointerTests) << "counters"
        << QQuickItemPrivate::eventHandlingChildrenWithinBounds_counter
        << QQuickItemPrivate::itemToParentTransform_counter
        << QQuickItemPrivate::itemToWindowTransform_counter
        << QQuickItemPrivate::windowToItemTransform_counter
        << QQuickItemPrivate::effectiveClippingSkips_counter;
    // Example counts:
    // 6 231 127 27 10
    // 6 157 55 9 10
    // 6 237 135 29 10

    // Check that we didn't call the transform functions exceessively often
    // (these numbers can be adjusted if we do something that causes a moderate increase,
    // but try to avoid really pessimizing it again)
    QCOMPARE_LT(QQuickItemPrivate::itemToParentTransform_counter, 280ull);
    QCOMPARE_LT(QQuickItemPrivate::itemToWindowTransform_counter, 160ull);
    QCOMPARE_LT(QQuickItemPrivate::windowToItemTransform_counter, 36ull);
    // Check that we were able to skip hover delivery to some items because
    // eventHandlingChildrenWithinBounds was true and the mouse position was outside.
    QCOMPARE_GE(QQuickItemPrivate::effectiveClippingSkips_counter, 5ull);
#endif
}

void tst_HoverHandler::grandChildOutOfBounds()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("grandchildOutOfBounds.qml")));
    QQuickItem *root = window.rootObject();
    QQuickHoverHandler *handler = root->findChild<QQuickHoverHandler *>();
    QVERIFY(handler);
    QQuickItem *grandchild = handler->parentItem();
    QVERIFY(grandchild);
    QQuickItem *grandparent = grandchild->parentItem()->parentItem();
    QCOMPARE(grandparent->parentItem(), root);
    QQuickItemPrivate *grandparentPriv = QQuickItemPrivate::get(grandparent);

    const QPoint bottomRight = QPoint(root->width() - 20, root->height() - 20);
    const QPoint bottomRightG = window.mapToGlobal(bottomRight);
    const QPoint pos = handler->parentItem()->mapToScene({10, handler->parentItem()->height() - 10}).toPoint();

    // showView() positions the mouse cursor to the right of the window, if possible;
    // so approach from the right side to avoid crossing any children.
    // We use QCursor::setPos() if possible, because it's a more thorough test
    // (if the cursor moves and the test fails, something is wrong); but the test
    // can _pass_ just as well with mouseMove(), so fall back to that if necessary.
    QCursor::setPos(bottomRightG);
    bool canSetCursorPos = true;
    if (!QTest::qWaitFor([bottomRightG]() {
            return QGuiApplicationPrivate::lastCursorPosition.toPoint() == bottomRightG; })) {
        qCDebug(lcPointerTests) << "QCursor::setPos doesn't work: expected"
                                << bottomRightG << "got" << QGuiApplicationPrivate::lastCursorPosition;
        canSetCursorPos = false;
    }
    if (!canSetCursorPos)
        QTest::mouseMove(&window, bottomRight);

    // Hover the outer end of the "diving board": it should work,
    // even though that part of the item is outside its parent and grandparent items.
    if (canSetCursorPos)
        QCursor::setPos(window.mapToGlobal(pos));
    else
        QTest::mouseMove(&window, pos);
    QTRY_COMPARE(handler->isHovered(), true);
#if QT_CONFIG(cursor)
    if (canSetCursorPos)
        QTRY_COMPARE(window.cursor().shape(), Qt::ForbiddenCursor);
#endif

    // Move it within its grandparent's bounds, but remember where it was.
    const auto yWas = grandchild->y();
    grandchild->setY(-70);
    // Grandparent would have children within bounds now, but it doesn't recheck
    // (this is considered an optimization, but can be reconsidered if necessary).
    QCOMPARE(grandparentPriv->eventHandlingChildrenWithinBounds, false);
    // The cursor didn't move, so now HoverHandler is no longer hovered.
    QTRY_COMPARE(handler->isHovered(), false);
#if QT_CONFIG(cursor)
    if (canSetCursorPos)
        QTRY_COMPARE(window.cursor().shape(), Qt::ArrowCursor);
#endif

    // Put it back where it was. Presto, it's hovered again.
    grandchild->setY(yWas);
    QCOMPARE(grandparentPriv->eventHandlingChildrenWithinBounds, false);
    QTRY_COMPARE(handler->isHovered(), true);
#if QT_CONFIG(cursor)
    if (canSetCursorPos)
        QTRY_COMPARE(window.cursor().shape(), Qt::ForbiddenCursor);
#endif
}

QTEST_MAIN(tst_HoverHandler)

#include "tst_qquickhoverhandler.moc"
