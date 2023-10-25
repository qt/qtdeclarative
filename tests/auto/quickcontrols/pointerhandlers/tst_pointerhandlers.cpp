// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>

#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/private/qpointingdevice_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

class tst_pointerhandlers : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_pointerhandlers();

private slots:
    void hover_controlInsideControl();
    void hover_controlAndMouseArea();
    void buttonTapHandler_data();
    void buttonTapHandler();
    void buttonDragHandler_data();
    void buttonDragHandler();

private:
    QScopedPointer<QPointingDevice> touchscreen = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
};

tst_pointerhandlers::tst_pointerhandlers()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_pointerhandlers::hover_controlInsideControl()
{
    // Test that if you move the mouse over a control that is
    // a child of another control, both controls end up hovered.
    // A control should basically not block (accept) hover events.
    QQuickView view(testFileUrl("controlinsidecontrol.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);
    QQuickWindow *window = rootItem->window();
    QVERIFY(window);

    const auto context = qmlContext(rootItem);
    auto outerButton = context->contextProperty("outerButton").value<QQuickButton *>();
    auto innerButton = context->contextProperty("innerButton").value<QQuickButton *>();
    QVERIFY(outerButton);
    QVERIFY(innerButton);

    const QPoint posInWindow(1, 1);
    const QPoint posOnOuterButton = rootItem->mapFromItem(outerButton, QPointF(0, 0)).toPoint();
    const QPoint posOnInnerButton = rootItem->mapFromItem(innerButton, QPointF(0, 0)).toPoint();

    // Start by moving the mouse to the window
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerButton->isHovered(), false);
    QCOMPARE(innerButton->isHovered(), false);

    // Move the mouse over the outer control
    QTest::mouseMove(window, posOnOuterButton);
    QCOMPARE(outerButton->isHovered(), true);
    QCOMPARE(innerButton->isHovered(), false);

    // Move the mouse over the inner control
    QTest::mouseMove(window, posOnInnerButton);
    QCOMPARE(outerButton->isHovered(), true);
    QCOMPARE(innerButton->isHovered(), true);

    // Move the mouse over the outer control again
    QTest::mouseMove(window, posOnOuterButton);
    QCOMPARE(outerButton->isHovered(), true);
    QCOMPARE(innerButton->isHovered(), false);

    // Move the mouse outside all controls
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerButton->isHovered(), false);
    QCOMPARE(innerButton->isHovered(), false);
}

void tst_pointerhandlers::hover_controlAndMouseArea()
{
    QQuickView view(testFileUrl("controlandmousearea.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);
    QQuickWindow *window = rootItem->window();
    QVERIFY(window);

    const auto context = qmlContext(rootItem);
    auto outerMouseArea = context->contextProperty("outerMouseArea").value<QQuickMouseArea *>();
    auto buttonInTheMiddle = context->contextProperty("buttonInTheMiddle").value<QQuickButton *>();
    auto innerMouseArea = context->contextProperty("innerMouseArea").value<QQuickMouseArea *>();
    QVERIFY(outerMouseArea);
    QVERIFY(buttonInTheMiddle);
    QVERIFY(innerMouseArea);

    const QPoint posInWindow(1, 1);
    const QPoint posOnOuterMouseArea = rootItem->mapFromItem(outerMouseArea, QPointF(0, 0)).toPoint();
    const QPoint posOnButtonInTheMiddle = rootItem->mapFromItem(buttonInTheMiddle, QPointF(0, 0)).toPoint();
    const QPoint posOnInnerMouseArea = rootItem->mapFromItem(innerMouseArea, QPointF(0, 0)).toPoint();

    // Start by moving the mouse to the window
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerMouseArea->hovered(), false);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the outer mousearea
    QTest::mouseMove(window, posOnOuterMouseArea);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the button in the middle
    QTest::mouseMove(window, posOnButtonInTheMiddle);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), true);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the inner mousearea
    QTest::mouseMove(window, posOnInnerMouseArea);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), true);
    QCOMPARE(innerMouseArea->hovered(), true);

    // Move the mouse over the button in the middle again
    QTest::mouseMove(window, posOnButtonInTheMiddle);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), true);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse over the outer mousearea again
    QTest::mouseMove(window, posOnOuterMouseArea);
    QCOMPARE(outerMouseArea->hovered(), true);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);

    // Move the mouse outside all items
    QTest::mouseMove(window, posInWindow);
    QCOMPARE(outerMouseArea->hovered(), false);
    QCOMPARE(buttonInTheMiddle->isHovered(), false);
    QCOMPARE(innerMouseArea->hovered(), false);
}

void tst_pointerhandlers::buttonTapHandler_data()
{
    QTest::addColumn<QPointingDevice::DeviceType>("deviceType");
    QTest::addColumn<Qt::MouseButton>("mouseButton");

    QTest::newRow("left mouse") << QPointingDevice::DeviceType::Mouse << Qt::LeftButton;
    QTest::newRow("right mouse") << QPointingDevice::DeviceType::Mouse << Qt::RightButton;
    QTest::newRow("touch") << QPointingDevice::DeviceType::TouchScreen << Qt::NoButton;
}

void tst_pointerhandlers::buttonTapHandler() // QTBUG-105609
{
    QFETCH(QPointingDevice::DeviceType, deviceType);
    QFETCH(Qt::MouseButton, mouseButton);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("tapHandlerButton.qml")));

    QPointer<QQuickTapHandler> handler = window.rootObject()->findChild<QQuickTapHandler*>();
    QVERIFY(handler);
    handler->setAcceptedButtons(mouseButton);
    QQuickItem *target = handler->target();
    QVERIFY(target);
    QSignalSpy tappedSpy(handler, &QQuickTapHandler::tapped);
    QSignalSpy clickedSpy(target, SIGNAL(clicked())); // avoid #include for this signal

    const QPoint pos(10, 10);
    switch (static_cast<QPointingDevice::DeviceType>(deviceType)) {
    case QPointingDevice::DeviceType::Mouse:
        // click it
        QTest::mouseClick(&window, mouseButton, Qt::NoModifier, pos);
        QTRY_COMPARE(clickedSpy.size(), mouseButton == Qt::RightButton ? 0 : 1);
        QCOMPARE(tappedSpy.size(), 1);
        break;

    case QPointingDevice::DeviceType::TouchScreen: {
        // tap it
        QTest::QTouchEventSequence touch = QTest::touchEvent(&window, touchscreen.data());
        touch.press(0, pos, &window).commit();
        QTRY_COMPARE(target->property("pressed").toBool(), true);
        touch.release(0, pos, &window).commit();
        QTRY_COMPARE(clickedSpy.size(), 1);
        QCOMPARE(tappedSpy.size(), 1);
        break;
    }
    default:
        break;
    }
    QCOMPARE(handler->isPressed(), false);
}

void tst_pointerhandlers::buttonDragHandler_data()
{
    QTest::addColumn<QPointingDevice::DeviceType>("deviceType");

    QTest::newRow("mouse") << QPointingDevice::DeviceType::Mouse;
    QTest::newRow("touch") << QPointingDevice::DeviceType::TouchScreen;
}

void tst_pointerhandlers::buttonDragHandler() // QTBUG-105610
{
    QFETCH(QPointingDevice::DeviceType, deviceType);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("draggableButton.qml")));

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();

    QPointer<QQuickPointerHandler> handler = window.rootObject()->findChild<QQuickPointerHandler*>();
    QVERIFY(handler);
    QQuickItem *target = handler->target();
    QVERIFY(target);
    QSignalSpy clickedSpy(target, SIGNAL(clicked()));

    QPoint dragPos(10, 10);
    switch (static_cast<QPointingDevice::DeviceType>(deviceType)) {
    case QPointingDevice::DeviceType::Mouse:
        // click it
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, dragPos);
        QTRY_COMPARE(clickedSpy.size(), 1);

        // drag it
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, dragPos);
        dragPos += QPoint(dragThreshold, dragThreshold);
        QTest::mouseMove(&window, dragPos);
        dragPos += QPoint(1, 1);
        QTest::mouseMove(&window, dragPos);
        qCDebug(lcPointerTests) << handler << "dragged" << target << "to" << target->position();
        QTRY_VERIFY(handler->active());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, dragPos);
        break;

    case QPointingDevice::DeviceType::TouchScreen: {
        QTest::QTouchEventSequence touch = QTest::touchEvent(&window, touchscreen.data());

        // tap it
        touch.press(0, dragPos, &window).commit();
        touch.release(0, dragPos, &window).commit();
        QTRY_COMPARE(clickedSpy.size(), 1);

        // drag it
        touch.press(0, dragPos, &window).commit();
        dragPos += QPoint(dragThreshold, dragThreshold);
        touch.move(0, dragPos, &window).commit();
        dragPos += QPoint(1, 1);
        touch.move(0, dragPos, &window).commit();
        qCDebug(lcPointerTests) << handler << "dragged" << target << "to" << target->position();
        QTRY_VERIFY(handler->active());
        touch.release(0, dragPos, &window).commit();
        break;
    }
    default:
        break;
    }
    QTRY_COMPARE(handler->active(), false);

    // click it again
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, dragPos);
    QTRY_COMPARE(clickedSpy.size(), 2);
}

QTEST_MAIN(tst_pointerhandlers)

#include "tst_pointerhandlers.moc"
