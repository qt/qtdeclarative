// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtGui/qpointingdevice.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/QQmlComponent>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickdrawer_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

//using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;
using namespace Qt::StringLiterals;

class tst_QQuickDrawer : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickDrawer();

private slots:
    void initTestCase() override;

    void defaults();
    void invalidEdge();

    void visible_data();
    void visible();

    void state();

    void position_data();
    void position();

    void dragMargin_data();
    void dragMargin();

    void reposition();
    void rotate();
    void header();

    void dragHandlerInteraction();

    void hover_data();
    void hover();

#if QT_CONFIG(wheelevent)
    void wheel_data();
    void wheel();
#endif

    void multiple();

    void touch_data();
    void touch();

    void multiTouch();

    void grabber();

    void interactive_data();
    void interactive();

    void flickable_data();
    void flickable();

    void dragOverModalShadow_data();
    void dragOverModalShadow();

    void nonModal_data();
    void nonModal();

    void slider_data();
    void slider();

    void topEdgeScreenEdge();

    void bookkeepingInOverlay();

    void touchOutsideOverlay();

    void destroyWhileVisible();

private:
    QScopedPointer<QPointingDevice> touchDevice;
};


tst_QQuickDrawer::tst_QQuickDrawer()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickDrawer::initTestCase()
{
    QQmlDataTest::initTestCase();
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");

    touchDevice.reset(QTest::createTouchDevice());
}

void tst_QQuickDrawer::defaults()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(!root.isNull(), qPrintable(component.errorString()));

    QQuickDrawer *drawer = root->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);
    QCOMPARE(drawer->edge(), Qt::LeftEdge);
    QCOMPARE(drawer->position(), 0.0);
    QCOMPARE(drawer->dragMargin(), qGuiApp->styleHints()->startDragDistance());
}

void tst_QQuickDrawer::invalidEdge()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(!root.isNull(), qPrintable(component.errorString()));

    QQuickDrawer *drawer = root->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    // Test an invalid value - it should warn and ignore it.
    QTest::ignoreMessage(QtWarningMsg, qUtf8Printable(testFileUrl("window.qml").toString() + ":14:5: QML Drawer: invalid edge value - valid values are: Qt.TopEdge, Qt.LeftEdge, Qt.RightEdge, Qt.BottomEdge"));
    drawer->setEdge(static_cast<Qt::Edge>(QQuickDrawer::Right));
    QCOMPARE(drawer->edge(), Qt::LeftEdge);
}

void tst_QQuickDrawer::visible_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("Window") << "window.qml";
    QTest::newRow("ApplicationWindow") << "applicationwindow.qml";
}

void tst_QQuickDrawer::visible()
{
    QFETCH(QString, source);
    QQuickControlsApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    QQuickItem *popupItem = drawer->popupItem();

    QCOMPARE(drawer->isVisible(), false);
    QCOMPARE(drawer->position(), qreal(0.0));

    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);
    QVERIFY(!overlay->childItems().contains(popupItem));

    drawer->open();
    QVERIFY(drawer->isVisible());
    QVERIFY(overlay->childItems().contains(popupItem));
    QTRY_COMPARE(drawer->position(), qreal(1.0));

    drawer->close();
    QTRY_VERIFY(!drawer->isVisible());
    QTRY_COMPARE(drawer->position(), qreal(0.0));
    QVERIFY(!overlay->childItems().contains(popupItem));

    drawer->setVisible(true);
    QVERIFY(drawer->isVisible());
    QVERIFY(overlay->childItems().contains(popupItem));
    QTRY_COMPARE(drawer->position(), qreal(1.0));

    drawer->setVisible(false);
    QTRY_VERIFY(!drawer->isVisible());
    QTRY_COMPARE(drawer->position(), qreal(0.0));
    QTRY_VERIFY(!overlay->childItems().contains(popupItem));
}

void tst_QQuickDrawer::state()
{
    QQuickControlsApplicationHelper helper(this, "applicationwindow.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);

    QCOMPARE(drawer->isVisible(), false);

    QSignalSpy visibleChangedSpy(drawer, SIGNAL(visibleChanged()));
    QSignalSpy aboutToShowSpy(drawer, SIGNAL(aboutToShow()));
    QSignalSpy aboutToHideSpy(drawer, SIGNAL(aboutToHide()));
    QSignalSpy openedSpy(drawer, SIGNAL(opened()));
    QSignalSpy closedSpy(drawer, SIGNAL(closed()));

    QVERIFY(visibleChangedSpy.isValid());
    QVERIFY(aboutToShowSpy.isValid());
    QVERIFY(aboutToHideSpy.isValid());
    QVERIFY(openedSpy.isValid());
    QVERIFY(closedSpy.isValid());

    int visibleChangedCount = 0;
    int aboutToShowCount = 0;
    int aboutToHideCount = 0;
    int openedCount = 0;
    int closedCount = 0;

    // open programmatically...
    drawer->open();
    QCOMPARE(visibleChangedSpy.size(), ++visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), ++aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), aboutToHideCount);
    QCOMPARE(openedSpy.size(), openedCount);
    QCOMPARE(closedSpy.size(), closedCount);

    // ...and wait until fully open
    QVERIFY(openedSpy.wait());
    QCOMPARE(visibleChangedSpy.size(), visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), aboutToHideCount);
    QCOMPARE(openedSpy.size(), ++openedCount);
    QCOMPARE(closedSpy.size(), closedCount);

    // close programmatically...
    drawer->close();
    QCOMPARE(visibleChangedSpy.size(), visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), ++aboutToHideCount);
    QCOMPARE(openedSpy.size(), openedCount);
    QCOMPARE(closedSpy.size(), closedCount);

    // ...and wait until fully closed
    QVERIFY(closedSpy.wait());
    QCOMPARE(visibleChangedSpy.size(), ++visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), aboutToHideCount);
    QCOMPARE(openedSpy.size(), openedCount);
    QCOMPARE(closedSpy.size(), ++closedCount);

    // open interactively...
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(0, drawer->height() / 2));
    QTest::mouseMove(window, QPoint(drawer->width() * 0.2, drawer->height() / 2), 16);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(drawer->width() * 0.8, drawer->height() / 2), 16);
    QCOMPARE(visibleChangedSpy.size(), ++visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), ++aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), aboutToHideCount);
    QCOMPARE(openedSpy.size(), openedCount);
    QCOMPARE(closedSpy.size(), closedCount);

    // ...and wait until fully open
    QVERIFY(openedSpy.wait());
    QCOMPARE(visibleChangedSpy.size(), visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), aboutToHideCount);
    QCOMPARE(openedSpy.size(), ++openedCount);
    QCOMPARE(closedSpy.size(), closedCount);

    // close interactively...
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(drawer->width(), drawer->height() / 2));
    QTest::mouseMove(window, QPoint(drawer->width() * 0.8, drawer->height() / 2), 16);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(drawer->width() * 0.2, drawer->height() / 2), 16);
    QCOMPARE(visibleChangedSpy.size(), visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), ++aboutToHideCount);
    QCOMPARE(openedSpy.size(), openedCount);
    QCOMPARE(closedSpy.size(), closedCount);

    // ...and wait until fully closed
    QVERIFY(closedSpy.wait());
    QCOMPARE(visibleChangedSpy.size(), ++visibleChangedCount);
    QCOMPARE(aboutToShowSpy.size(), aboutToShowCount);
    QCOMPARE(aboutToHideSpy.size(), aboutToHideCount);
    QCOMPARE(openedSpy.size(), openedCount);
    QCOMPARE(closedSpy.size(), ++closedCount);
}

void tst_QQuickDrawer::position_data()
{
    QTest::addColumn<Qt::Edge>("edge");
    QTest::addColumn<QPoint>("press");
    QTest::addColumn<QPoint>("from");
    QTest::addColumn<QPoint>("to");
    QTest::addColumn<qreal>("position");

    // We need to start swiping exactly from the selected edge, but on Android
    // ApplicationWindow will be fullscreen instead of the defined size, so
    // we need to extract the edge values from screen geometry.
#ifndef Q_OS_ANDROID
    const int rightMargin = 399;
    const int bottomMargin = 399;
#else
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const int rightMargin = screenGeometry.right();
    const int bottomMargin = screenGeometry.bottom();
#endif

    QTest::newRow("top") << Qt::TopEdge << QPoint(100, 0) << QPoint(100, 50) << QPoint(100, 150) << qreal(0.5);
    QTest::newRow("left") << Qt::LeftEdge << QPoint(0, 100) << QPoint(50, 100) << QPoint(150, 100) << qreal(0.5);
    QTest::newRow("right") << Qt::RightEdge << QPoint(rightMargin, 100)
                           << QPoint(rightMargin - 50, 100) << QPoint(rightMargin - 150, 100)
                           << qreal(0.5);
    QTest::newRow("bottom") << Qt::BottomEdge << QPoint(100, bottomMargin)
                            << QPoint(100, bottomMargin - 50) << QPoint(150, bottomMargin - 150)
                            << qreal(0.5);
}

void tst_QQuickDrawer::position()
{
    QFETCH(Qt::Edge, edge);
    QFETCH(QPoint, press);
    QFETCH(QPoint, from);
    QFETCH(QPoint, to);
    QFETCH(qreal, position);

    QQuickControlsApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = helper.appWindow->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setEdge(edge);

    // Give it some time (50 ms) before the press to avoid flakiness on OpenSUSE: QTBUG-77946
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, press, 50);
    QTest::mouseMove(window, from);
    QTest::mouseMove(window, to);
    QCOMPARE(drawer->position(), position);

    // moved half-way open at almost infinite speed => snap to open
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    QTRY_COMPARE(drawer->position(), 1.0);
}

void tst_QQuickDrawer::dragMargin_data()
{
    QTest::addColumn<Qt::Edge>("edge");
    QTest::addColumn<qreal>("dragMargin");
    QTest::addColumn<qreal>("dragFromLeft");
    QTest::addColumn<qreal>("dragFromRight");

    QTest::newRow("left:0") << Qt::LeftEdge << qreal(0) << qreal(0) << qreal(0);
    QTest::newRow("left:-1") << Qt::LeftEdge << qreal(-1) << qreal(0) << qreal(0);
    QTest::newRow("left:startDragDistance") << Qt::LeftEdge << qreal(QGuiApplication::styleHints()->startDragDistance()) << qreal(0.45) << qreal(0);
    QTest::newRow("left:startDragDistance*2") << Qt::LeftEdge << qreal(QGuiApplication::styleHints()->startDragDistance() * 2) << qreal(0.45) << qreal(0);

    QTest::newRow("right:0") << Qt::RightEdge << qreal(0) << qreal(0) << qreal(0);
    QTest::newRow("right:-1") << Qt::RightEdge << qreal(-1) << qreal(0) << qreal(0);
    QTest::newRow("right:startDragDistance") << Qt::RightEdge << qreal(QGuiApplication::styleHints()->startDragDistance()) << qreal(0) << qreal(0.75);
    QTest::newRow("right:startDragDistance*2") << Qt::RightEdge << qreal(QGuiApplication::styleHints()->startDragDistance() * 2) << qreal(0) << qreal(0.75);
}

void tst_QQuickDrawer::dragMargin()
{
    QFETCH(Qt::Edge, edge);
    QFETCH(qreal, dragMargin);
    QFETCH(qreal, dragFromLeft);
    QFETCH(qreal, dragFromRight);

    QQuickControlsApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = helper.appWindow->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setEdge(edge);
    drawer->setDragMargin(dragMargin);

    const int startDragDistance = qMax(20, QGuiApplication::styleHints()->startDragDistance() + 5) + 1;

    // drag from the left
    int leftX = qMax<int>(0, dragMargin);
    int leftDistance = startDragDistance + drawer->width() * 0.45;
    QVERIFY(leftDistance > QGuiApplication::styleHints()->startDragDistance());
    // Give it some time (50 ms) before the press to avoid flakiness on OpenSUSE: QTBUG-77946
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(leftX, drawer->height() / 2), 50);
    QTest::mouseMove(window, QPoint(leftX + startDragDistance, drawer->height() / 2));
    QTest::mouseMove(window, QPoint(leftX + leftDistance, drawer->height() / 2));
    QCOMPARE(drawer->position(), dragFromLeft);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(leftX + leftDistance, drawer->height() / 2));

    drawer->close();
    QTRY_COMPARE(drawer->position(), qreal(0.0));

    // drag from the right
    int rightX = qMin<int>(window->width() - 1, window->width() - dragMargin);
    int rightDistance = startDragDistance + drawer->width() * 0.75;
    QVERIFY(rightDistance > QGuiApplication::styleHints()->startDragDistance());
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(rightX, drawer->height() / 2));
    QTest::mouseMove(window, QPoint(rightX - startDragDistance, drawer->height() / 2));
    QTest::mouseMove(window, QPoint(rightX - rightDistance, drawer->height() / 2));
    QCOMPARE(drawer->position(), dragFromRight);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(rightX - rightDistance, drawer->height() / 2));
}

static QRectF geometry(const QQuickItem *item)
{
    return QRectF(item->x(), item->y(), item->width(), item->height());
}

void tst_QQuickDrawer::reposition()
{
    QQuickControlsApplicationHelper helper(this, QStringLiteral("reposition.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    QQuickItem *popupItem = drawer->popupItem();
    QVERIFY(popupItem);

    drawer->open();
    QQuickItem *dimmer = QQuickPopupPrivate::get(drawer)->dimmer;
    QVERIFY(dimmer);

    QCOMPARE(geometry(dimmer), QRectF(0, 0, window->width(), window->height()));
    QTRY_COMPARE(geometry(popupItem), QRectF(0, 0, window->width() / 2., window->height()));

    drawer->setY(100);
    QCOMPARE(geometry(dimmer), QRectF(0, 100, window->width(), window->height() - 100));
    QCOMPARE(geometry(popupItem), QRectF(0, 100, window->width() / 2., window->height() - 100));

    drawer->setHeight(window->height());
    QCOMPARE(geometry(dimmer), QRectF(0, 100, window->width(), window->height()));
    QCOMPARE(geometry(popupItem), QRectF(0, 100, window->width() / 2., window->height()));

    drawer->resetHeight();
    QCOMPARE(geometry(dimmer), QRectF(0, 100, window->width(), window->height() - 100));
    QCOMPARE(geometry(popupItem), QRectF(0, 100, window->width() / 2., window->height() - 100));

    drawer->setParentItem(window->contentItem());
    QCOMPARE(geometry(dimmer), QRectF(0, 150, window->width(), window->height() - 150));
    QCOMPARE(geometry(popupItem), QRectF(0, 150, window->width() / 2., window->height() - 150));

    drawer->setEdge(Qt::RightEdge);
    QCOMPARE(geometry(dimmer), QRectF(0, 150, window->width(), window->height() - 150));
    QTRY_COMPARE(geometry(popupItem), QRectF(window->width() - drawer->width(), 150, window->width() / 2., window->height() - 150));

    window->setWidth(window->width() + 100);
    QTRY_COMPARE(geometry(dimmer), QRectF(0, 150, window->width(), window->height() - 150));
    QTRY_COMPARE(geometry(popupItem), QRectF(window->width() - drawer->width(), 150, window->width() / 2., window->height() - 150));

    drawer->close();
    QTRY_COMPARE(geometry(popupItem), QRectF(window->width(), 150, window->width() / 2., window->height() - 150));

    QQuickDrawer *drawer2 = window->property("drawer2").value<QQuickDrawer *>();
    QVERIFY(drawer2);
    QQuickItem *popupItem2 = drawer2->popupItem();
    QVERIFY(popupItem2);

    drawer2->open();
    QVERIFY(popupItem2->isVisible());
    QCOMPARE(popupItem2->x(), -drawer2->width());
    QTRY_COMPARE(popupItem2->x(), 0.0);
}

void tst_QQuickDrawer::rotate()
{
    QQuickControlsApplicationHelper helper(this, u"rotate.qml"_s);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer_LR = window->property("drawer_LR").value<QQuickDrawer*>();
    QVERIFY(drawer_LR);
    QQuickItem *popupItem_LR = drawer_LR->popupItem();
    QVERIFY(popupItem_LR);

    QQuickDrawer *drawer_TB = window->property("drawer_TB").value<QQuickDrawer*>();
    QVERIFY(drawer_TB);
    QQuickItem *popupItem_TB = drawer_TB->popupItem();
    QVERIFY(popupItem_TB);

    drawer_LR->open();
    drawer_TB->open();

    const int drawerSize = window->property("drawerSize").toInt();
    int virtualWidth = window->width();
    int virtualHeight = window->height();

    // First iteration tests an unrotated window; 2nd iteration repeats the test with a rotated window
    for (int i = 0; i < 2; ++i) {
        drawer_LR->setEdge(Qt::LeftEdge);
        QTRY_COMPARE(geometry(popupItem_LR), QRectF(0, 0, drawerSize, virtualHeight));

        drawer_TB->setEdge(Qt::TopEdge);
        QTRY_COMPARE(geometry(popupItem_TB), QRectF(0, 0, virtualWidth, drawerSize));

        drawer_LR->setEdge(Qt::RightEdge);
        QTRY_COMPARE(geometry(popupItem_LR), QRectF(virtualWidth - drawerSize, 0, drawerSize, virtualHeight));

        drawer_TB->setEdge(Qt::BottomEdge);
        QTRY_COMPARE(geometry(popupItem_TB), QRectF(0, virtualHeight - drawerSize, virtualWidth, drawerSize));

        if (i == 0) {
            window->setProperty("rotated", true);
            std::swap(virtualWidth, virtualHeight);
        }
    }
}

void tst_QQuickDrawer::header()
{
    QQuickControlsApplicationHelper helper(this, QStringLiteral("header.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickItem *content = window->contentItem();
    QVERIFY(content);

    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    QQuickItem *popupItem = drawer->popupItem();

    QQuickButton *button = window->property("button").value<QQuickButton*>();
    QVERIFY(button);

    drawer->open();
    QVERIFY(drawer->isVisible());

    QCOMPARE(drawer->parentItem(), overlay);
    QCOMPARE(drawer->height(), overlay->height());
    QCOMPARE(popupItem->height(), overlay->height());

    drawer->setParentItem(content);
    QCOMPARE(drawer->parentItem(), content);
    QCOMPARE(drawer->height(), content->height());
    QCOMPARE(popupItem->height(), content->height());

    // must be possible to interact with the header when the drawer is below the header
    QSignalSpy clickSpy(button, SIGNAL(clicked()));
    QVERIFY(clickSpy.isValid());
    QPoint p = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(clickSpy.size(), 1);
}

void tst_QQuickDrawer::dragHandlerInteraction()
{
    QQuickControlsApplicationHelper helper(this, u"dragHandlerInteraction.qml"_s);
    QVERIFY2(helper.ready, helper.failureMessage());

    auto window = helper.appWindow;;
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTest::mousePress(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(250, 250));
    QTest::mouseMove(window, QPoint(100, 100));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(100, 100));
    QTRY_COMPARE(window->property("changedCounter").toInt(), 2); // became active and inactive
}

void tst_QQuickDrawer::hover_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("modal");

    QTest::newRow("Window:modal") << "window-hover.qml" << true;
    QTest::newRow("Window:modeless") << "window-hover.qml" << false;
    QTest::newRow("ApplicationWindow:modal") << "applicationwindow-hover.qml" << true;
    QTest::newRow("ApplicationWindow:modeless") << "applicationwindow-hover.qml" << false;
}

void tst_QQuickDrawer::hover()
{
    QFETCH(QString, source);
    QFETCH(bool, modal);

    QQuickControlsApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setModal(modal);

    QQuickControl *drawerItem = qobject_cast<QQuickControl *>(drawer->popupItem());
    QVERIFY(drawerItem);
    QVERIFY(drawerItem->isHoverEnabled());

    QQuickButton *backgroundButton = window->property("backgroundButton").value<QQuickButton*>();
    QVERIFY(backgroundButton);
    backgroundButton->setHoverEnabled(true);

    QQuickButton *drawerButton = window->property("drawerButton").value<QQuickButton*>();
    QVERIFY(drawerButton);
    drawerButton->setHoverEnabled(true);

    QSignalSpy openedSpy(drawer, SIGNAL(opened()));
    QVERIFY(openedSpy.isValid());
    drawer->open();
    QVERIFY(openedSpy.size() == 1 || openedSpy.wait());

    // hover the background button outside the drawer
    auto topSafeMargin = window->safeAreaMargins().top();
    QTest::mouseMove(window, QPoint(window->width() - 1, window->height() - topSafeMargin - 1));
    QCOMPARE(backgroundButton->isHovered(), !modal);
    QVERIFY(!drawerButton->isHovered());
    QVERIFY(!drawerItem->isHovered());

    // hover the drawer background
    QTest::mouseMove(window, QPoint(1, 1));
    QVERIFY(!backgroundButton->isHovered());
    QVERIFY(!drawerButton->isHovered());
    QVERIFY(drawerItem->isHovered());

    // hover the button in a drawer
    QTest::mouseMove(window, QPoint(2, 2));
    QVERIFY(!backgroundButton->isHovered());
    QVERIFY(drawerButton->isHovered());
    QVERIFY(drawerItem->isHovered());

    QSignalSpy closedSpy(drawer, SIGNAL(closed()));
    QVERIFY(closedSpy.isValid());
    drawer->close();
    QVERIFY(closedSpy.size() == 1 || closedSpy.wait());

    // hover the background button after closing the drawer
    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() / 2));
    QVERIFY(backgroundButton->isHovered());
    QVERIFY(!drawerButton->isHovered());
    QVERIFY(!drawerItem->isHovered());
}

#if QT_CONFIG(wheelevent)
void tst_QQuickDrawer::wheel_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("modal");

    QTest::newRow("Window:modal") << "window-wheel.qml" << true;
    QTest::newRow("Window:modeless") << "window-wheel.qml" << false;
    QTest::newRow("ApplicationWindow:modal") << "applicationwindow-wheel.qml" << true;
    QTest::newRow("ApplicationWindow:modeless") << "applicationwindow-wheel.qml" << false;
}

static bool sendWheelEvent(QQuickItem *item, const QPoint &localPos, int degrees)
{
    QQuickWindow *window = item->window();
    QWheelEvent wheelEvent(localPos, item->window()->mapToGlobal(localPos), QPoint(0, 0),
                           QPoint(0, 8 * degrees), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase,
                           false);
    QSpontaneKeyEvent::setSpontaneous(&wheelEvent);
    return qGuiApp->notify(window, &wheelEvent);
}

void tst_QQuickDrawer::wheel()
{
    QFETCH(QString, source);
    QFETCH(bool, modal);

    QQuickControlsApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickSlider *contentSlider = window->property("contentSlider").value<QQuickSlider*>();
    QVERIFY(contentSlider);

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer && drawer->contentItem());
    drawer->setModal(modal);

    QQuickSlider *drawerSlider = window->property("drawerSlider").value<QQuickSlider*>();
    QVERIFY(drawerSlider);

    {
        // wheel over the content
        qreal oldContentValue = contentSlider->value();
        qreal oldDrawerValue = drawerSlider->value();

        QVERIFY(sendWheelEvent(contentSlider, QPoint(contentSlider->width() / 2, contentSlider->height() / 2), 15));

        QVERIFY(!qFuzzyCompare(contentSlider->value(), oldContentValue)); // must have moved
        QVERIFY(qFuzzyCompare(drawerSlider->value(), oldDrawerValue)); // must not have moved
    }

    QSignalSpy openedSpy(drawer, SIGNAL(opened()));
    QVERIFY(openedSpy.isValid());
    drawer->open();
    QVERIFY(openedSpy.size() == 1 || openedSpy.wait());

    {
        // wheel over the drawer content
        qreal oldContentValue = contentSlider->value();
        qreal oldDrawerValue = drawerSlider->value();

        QVERIFY(sendWheelEvent(drawerSlider, QPoint(drawerSlider->width() / 2, drawerSlider->height() / 2), 15));

        QVERIFY(qFuzzyCompare(contentSlider->value(), oldContentValue)); // must not have moved
        QVERIFY(!qFuzzyCompare(drawerSlider->value(), oldDrawerValue)); // must have moved
    }

    {
        // wheel over the overlay
        qreal oldContentValue = contentSlider->value();
        qreal oldDrawerValue = drawerSlider->value();

        auto *overlay = QQuickOverlay::overlay(window);
        QVERIFY(sendWheelEvent(overlay, QPoint(0, overlay->height() / 2), 15));

        if (modal) {
            // the content below a modal overlay must not move
            QVERIFY(qFuzzyCompare(contentSlider->value(), oldContentValue));
        } else {
            // the content below a modeless overlay must move
            QVERIFY(!qFuzzyCompare(contentSlider->value(), oldContentValue));
        }
        QVERIFY(qFuzzyCompare(drawerSlider->value(), oldDrawerValue)); // must not have moved
    }
}
#endif

void tst_QQuickDrawer::multiple()
{
    QQuickControlsApplicationHelper helper(this, QStringLiteral("multiple.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *leftDrawer = window->property("leftDrawer").value<QQuickDrawer*>();
    QVERIFY(leftDrawer);
    QQuickButton *leftButton = window->property("leftButton").value<QQuickButton*>();
    QVERIFY(leftButton);
    QSignalSpy leftClickSpy(leftButton, SIGNAL(clicked()));
    QVERIFY(leftClickSpy.isValid());

    QQuickDrawer *rightDrawer = window->property("rightDrawer").value<QQuickDrawer*>();
    QVERIFY(rightDrawer);
    QQuickButton *rightButton = window->property("rightButton").value<QQuickButton*>();
    QVERIFY(rightButton);
    QSignalSpy rightClickSpy(rightButton, SIGNAL(clicked()));
    QVERIFY(rightClickSpy.isValid());

    QQuickButton *contentButton = window->property("contentButton").value<QQuickButton*>();
    QVERIFY(contentButton);
    QSignalSpy contentClickSpy(contentButton, SIGNAL(clicked()));
    QVERIFY(contentClickSpy.isValid());

    // no drawers open, click the content
    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 0);
    QCOMPARE(rightClickSpy.size(), 0);

    // drag the left drawer open
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(0, window->height() / 2));
    QTest::mouseMove(window, QPoint(leftDrawer->width() / 4, window->height() / 2));
    QTest::mouseMove(window, QPoint(leftDrawer->width() / 4 * 3, window->height() / 2));
    QCOMPARE(leftDrawer->position(), 0.5);
    QCOMPARE(rightDrawer->position(), 0.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(leftDrawer->width() / 2, window->height() / 2));
    QTRY_COMPARE(leftDrawer->position(), 1.0);
    QCOMPARE(rightDrawer->position(), 0.0);

    // cannot drag the right drawer while the left drawer is open
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 1, window->height() / 2));
    QTest::mouseMove(window, QPoint(window->width() - leftDrawer->width() / 2, window->height() / 2));
    QCOMPARE(leftDrawer->position(), 1.0);
    QCOMPARE(rightDrawer->position(), 0.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - leftDrawer->width() / 2, window->height() / 2));
    QCOMPARE(rightDrawer->position(), 0.0);
    QCOMPARE(leftDrawer->position(), 1.0);

    // open the right drawer below the left drawer
    rightDrawer->open();
    QTRY_COMPARE(rightDrawer->position(), 1.0);

    // click the left drawer's button
    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 0);

    // click the left drawer's background (button disabled, don't leak through to the right drawer below)
    leftButton->setEnabled(false);
    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 0);
    leftButton->setEnabled(true);

    // click the overlay of the left drawer (don't leak through to right drawer below)
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - (window->width() - leftDrawer->width()) / 2, window->height() / 2));
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 0);
    QTRY_VERIFY(!leftDrawer->isVisible());

    // click the right drawer's button
    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 1);

    // cannot drag the left drawer while the right drawer is open
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(0, window->height() / 2));
    QTest::mouseMove(window, QPoint(leftDrawer->width() / 2, window->height() / 2));
    QCOMPARE(leftDrawer->position(), 0.0);
    QCOMPARE(rightDrawer->position(), 1.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(leftDrawer->width() / 2, window->height() / 2));
    QCOMPARE(leftDrawer->position(), 0.0);
    QCOMPARE(rightDrawer->position(), 1.0);

    // click the right drawer's background (button disabled, don't leak through to the content below)
    rightButton->setEnabled(false);
    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 1);
    rightButton->setEnabled(true);

    // click the overlay of the right drawer (don't leak through to the content below)
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint((window->width() - rightDrawer->width()) / 2, window->height() / 2));
    QCOMPARE(contentClickSpy.size(), 1);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 1);
    QTRY_VERIFY(!rightDrawer->isVisible());

    // no drawers open, click the content
    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(contentClickSpy.size(), 2);
    QCOMPARE(leftClickSpy.size(), 1);
    QCOMPARE(rightClickSpy.size(), 1);

    // drag the right drawer open
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 1, window->height() / 2));
    QTest::mouseMove(window, QPoint(window->width() - rightDrawer->width() / 4, window->height() / 2));
    QTest::mouseMove(window, QPoint(window->width() - rightDrawer->width() / 4 * 3, window->height() / 2));
    QCOMPARE(rightDrawer->position(), 0.5);
    QCOMPARE(leftDrawer->position(), 0.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - rightDrawer->width() / 2, window->height() / 2));
    QTRY_COMPARE(rightDrawer->position(), 1.0);
    QCOMPARE(leftDrawer->position(), 0.0);

    // Hide the window, so it receives no more stray events
    window->hide();
    QVERIFY(QTest::qWaitFor([window](){ return !window->isVisible(); }));

    // Remove posted events, before the window goes out of scope
    QGuiApplication::removePostedEvents(window);
}

void tst_QQuickDrawer::touch_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QPoint>("from");
    QTest::addColumn<QPoint>("to");

    QTest::newRow("Window:inside") << "window.qml" << QPoint(150, 100) << QPoint(50, 100);
    QTest::newRow("Window:outside") << "window.qml" << QPoint(300, 100) << QPoint(100, 100);
    QTest::newRow("ApplicationWindow:inside") << "applicationwindow.qml" << QPoint(150, 100) << QPoint(50, 100);
    QTest::newRow("ApplicationWindow:outside") << "applicationwindow.qml" << QPoint(300, 100) << QPoint(100, 100);

    QTest::newRow("Window+Button:inside") << "window-button.qml" << QPoint(150, 100) << QPoint(50, 100);
    QTest::newRow("Window+Button:outside") << "window-button.qml" << QPoint(300, 100) << QPoint(100, 100);
    QTest::newRow("ApplicationWindow+Button:inside") << "applicationwindow-button.qml" << QPoint(150, 100) << QPoint(50, 100);
    QTest::newRow("ApplicationWindow+Button:outside") << "applicationwindow-button.qml" << QPoint(300, 100) << QPoint(100, 100);
}

void tst_QQuickDrawer::touch()
{
    QFETCH(QString, source);
    QFETCH(QPoint, from);
    QFETCH(QPoint, to);

    QQuickControlsApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);

    QSignalSpy drawerOpenedSpy(drawer, SIGNAL(opened()));
    QSignalSpy drawerClosedSpy(drawer, SIGNAL(closed()));
    QVERIFY(drawerOpenedSpy.isValid());
    QVERIFY(drawerClosedSpy.isValid());

    // drag to open
    QTest::touchEvent(window, touchDevice.data()).press(0, QPoint(0, 100));
    QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(50, 100));
    QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(150, 100));
    QTest::touchEvent(window, touchDevice.data()).release(0, QPoint(150, 100));
    QVERIFY(drawerOpenedSpy.wait());
    QCOMPARE(drawer->position(), 1.0);

    // drag to close
    QTest::touchEvent(window, touchDevice.data()).press(0, from);
    for (int x = from.x(); x > to.x(); x -= 10)
        QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(x, to.y()));
    QTest::touchEvent(window, touchDevice.data()).move(0, to);
    QTest::touchEvent(window, touchDevice.data()).release(0, to);
    QVERIFY(drawerClosedSpy.wait());
    QCOMPARE(drawer->position(), 0.0);
}

void tst_QQuickDrawer::multiTouch()
{
    QQuickControlsApplicationHelper helper(this, QStringLiteral("multiTouch.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    QQuickPopup *popup = window->property("popup").value<QQuickPopup *>();
    QVERIFY(popup);

    QQuickButton *button = window->property("button").value<QQuickButton *>();
    QVERIFY(button);

    QSignalSpy overlayPressedSpy(overlay, SIGNAL(pressed()));
    QSignalSpy overlayReleasedSpy(overlay, SIGNAL(released()));
    QVERIFY(overlayPressedSpy.isValid());
    QVERIFY(overlayReleasedSpy.isValid());

    QSignalSpy drawerOpenedSpy(drawer, SIGNAL(opened()));
    QVERIFY(drawerOpenedSpy.isValid());

    QSignalSpy buttonPressedSpy(button, SIGNAL(pressed()));
    QSignalSpy buttonReleasedSpy(button, SIGNAL(released()));
    QVERIFY(buttonPressedSpy.isValid());
    QVERIFY(buttonReleasedSpy.isValid());

    popup->open();
    QVERIFY(popup->isVisible());

    drawer->open();
    QVERIFY(drawer->isVisible());
    QVERIFY(drawerOpenedSpy.wait());

    // 1st press
    QTest::touchEvent(window, touchDevice.data()).press(0, QPoint(300, 100));
    QVERIFY(popup->isVisible());
    QVERIFY(drawer->isVisible());
    QCOMPARE(buttonPressedSpy.size(), 0);
    QCOMPARE(overlayPressedSpy.size(), 1);

    // 2nd press (blocked & ignored)
    QTest::touchEvent(window, touchDevice.data()).stationary(0).press(1, QPoint(300, 200));
    QVERIFY(popup->isVisible());
    QVERIFY(drawer->isVisible());
    QCOMPARE(buttonPressedSpy.size(), 0);
    QCOMPARE(overlayPressedSpy.size(), 2);

    // 2nd release (blocked & ignored)
    QTest::touchEvent(window, touchDevice.data()).stationary(0).release(1, QPoint(300, 200));
    QVERIFY(popup->isVisible());
    QVERIFY(drawer->isVisible());
    QCOMPARE(buttonPressedSpy.size(), 0);
    QCOMPARE(buttonReleasedSpy.size(), 0);
    QCOMPARE(overlayPressedSpy.size(), 2);
    QCOMPARE(overlayReleasedSpy.size(), 1);

    // 1st release
    QTest::touchEvent(window, touchDevice.data()).release(0, QPoint(300, 100));
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(!drawer->isVisible());
    QCOMPARE(buttonPressedSpy.size(), 0);
    QCOMPARE(buttonReleasedSpy.size(), 0);
    QCOMPARE(overlayPressedSpy.size(), 2);
    QCOMPARE(overlayReleasedSpy.size(), 2);

    drawer->open();
    QVERIFY(drawer->isVisible());
    QVERIFY(drawerOpenedSpy.wait());

    // 1st drag
    QTest::touchEvent(window, touchDevice.data()).press(0, QPoint(300, 100));
    QCOMPARE(buttonPressedSpy.size(), 0);
    QCOMPARE(overlayPressedSpy.size(), 3);
    for (int x = 300; x >= 100; x -= 10) {
        QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(x, 100));
        QVERIFY(popup->isVisible());
        QVERIFY(drawer->isVisible());
    }
    QCOMPARE(drawer->position(), 0.5);

    // 2nd drag (blocked & ignored)
    QTest::touchEvent(window, touchDevice.data()).stationary(0).press(1, QPoint(300, 200));
    QCOMPARE(buttonPressedSpy.size(), 0);
    QCOMPARE(overlayPressedSpy.size(), 4);
    for (int x = 300; x >= 0; x -= 10) {
        QTest::touchEvent(window, touchDevice.data()).stationary(0).move(1, QPoint(x, 200));
        QVERIFY(popup->isVisible());
        QVERIFY(drawer->isVisible());
    }
    QCOMPARE(drawer->position(), 0.5);

    // 2nd release (blocked & ignored)
    QTest::touchEvent(window, touchDevice.data()).stationary(0).release(1, QPoint(300, 0));
    QVERIFY(popup->isVisible());
    QVERIFY(drawer->isVisible());
    QCOMPARE(drawer->position(), 0.5);
    QCOMPARE(buttonReleasedSpy.size(), 0);
    QCOMPARE(overlayReleasedSpy.size(), 3);

    // 1st release
    QTest::touchEvent(window, touchDevice.data()).release(0, QPoint(300, 100));
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(!drawer->isVisible());
    QCOMPARE(buttonReleasedSpy.size(), 0);
    QCOMPARE(overlayReleasedSpy.size(), 4);
}

void tst_QQuickDrawer::grabber()
{
    QQuickControlsApplicationHelper helper(this, QStringLiteral("grabber.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    QSignalSpy drawerOpenedSpy(drawer, SIGNAL(opened()));
    QSignalSpy drawerClosedSpy(drawer, SIGNAL(closed()));
    QVERIFY(drawerOpenedSpy.isValid());
    QVERIFY(drawerClosedSpy.isValid());

    drawer->open();
    QVERIFY(drawerOpenedSpy.wait());

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 100));
    QVERIFY(drawerClosedSpy.wait());

    QQuickPopup *popup = window->property("popup").value<QQuickPopup *>();
    QVERIFY(popup);

    QSignalSpy popupOpenedSpy(popup, SIGNAL(opened()));
    QSignalSpy popupClosedSpy(popup, SIGNAL(closed()));
    QVERIFY(popupOpenedSpy.isValid());
    QVERIFY(popupClosedSpy.isValid());

    popup->open();
    QTRY_COMPARE(popupOpenedSpy.size(), 1);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 300));
    QTRY_COMPARE(popupClosedSpy.size(), 1);
}

void tst_QQuickDrawer::interactive_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("Window") << "window.qml";
    QTest::newRow("ApplicationWindow") << "applicationwindow.qml";
}

void tst_QQuickDrawer::interactive()
{
    SKIP_IF_NO_WINDOW_ACTIVATION;

    QFETCH(QString, source);
    QQuickControlsApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);

    drawer->setInteractive(false);

    QSignalSpy openedSpy(drawer, SIGNAL(opened()));
    QSignalSpy aboutToHideSpy(drawer, SIGNAL(aboutToHide()));
    QVERIFY(openedSpy.isValid());
    QVERIFY(aboutToHideSpy.isValid());

    drawer->open();
    QVERIFY(openedSpy.wait());

    // click outside
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 100));
    QCOMPARE(aboutToHideSpy.size(), 0);

    // drag inside
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(drawer->width(), 0));
    QTest::mouseMove(window, QPoint(0, 0));
    QCOMPARE(drawer->position(), 1.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(0, 0));
    QCOMPARE(drawer->position(), 1.0);
    QCOMPARE(aboutToHideSpy.size(), 0);

    // drag outside
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 1, 0));
    QTest::mouseMove(window, QPoint(0, 0));
    QCOMPARE(drawer->position(), 1.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(0, 0));
    QCOMPARE(drawer->position(), 1.0);
    QCOMPARE(aboutToHideSpy.size(), 0);

    // close on escape
    QTest::keyClick(window, Qt::Key_Escape);
    QCOMPARE(aboutToHideSpy.size(), 0);
}

void tst_QQuickDrawer::flickable_data()
{
    QTest::addColumn<bool>("mouse");
    QTest::addColumn<QPoint>("from");
    QTest::addColumn<QPoint>("to");

    QTest::newRow("mouse,straight") << true << QPoint(200, 200) << QPoint(200, 100);
    QTest::newRow("mouse,diagonal") << true << QPoint(200, 200) << QPoint(250, 100);
    QTest::newRow("touch,straight") << false << QPoint(200, 200) << QPoint(200, 100);
    QTest::newRow("touch,diagonal") << false << QPoint(200, 200) << QPoint(250, 100);
}

void tst_QQuickDrawer::flickable()
{
    QFETCH(bool, mouse);
    QFETCH(QPoint, from);
    QFETCH(QPoint, to);

    QQuickControlsApplicationHelper helper(this, QStringLiteral("flickable.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    QQuickFlickable *flickable = window->property("flickable").value<QQuickFlickable *>();
    QVERIFY(flickable);

    QSignalSpy drawerOpenedSpy(drawer, SIGNAL(opened()));
    QVERIFY(drawerOpenedSpy.isValid());

    drawer->open();
    QVERIFY(drawerOpenedSpy.wait());

    if (mouse)
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
    else
        QTest::touchEvent(window, touchDevice.data()).press(0, from);

    static const int steps = 10;
    const QPoint distance(to.x() - from.x(), to.y() - from.y());
    for (int i = 0; i <= steps; ++i) {
        const auto pos = from + QPoint(
            i * distance.x() / steps,
            i * distance.y() / steps
        );
        if (mouse)
            QTest::mouseMove(window, pos);
        else
            QTest::touchEvent(window, touchDevice.data()).move(0, pos);
        QTest::qWait(1); // avoid infinite velocity
    }

    QVERIFY(flickable->isDragging());

    if (mouse)
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    else
        QTest::touchEvent(window, touchDevice.data()).release(0, to);

    QVERIFY(!flickable->isDragging());
}

void tst_QQuickDrawer::dragOverModalShadow_data()
{
    QTest::addColumn<bool>("mouse");
    QTest::newRow("mouse") << true;
    QTest::newRow("touch") << false;
}

// QTBUG-60602
void tst_QQuickDrawer::dragOverModalShadow()
{
    QFETCH(bool, mouse);

    QQuickControlsApplicationHelper helper(this, QStringLiteral("dragOverModalShadow.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    QQuickPopup *popup = window->property("popup").value<QQuickPopup *>();
    QVERIFY(popup);

    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(!drawer->isVisible());

    const QPoint from(popup->x(), popup->y() + popup->height() + 5);
    const QPoint to(popup->x() + popup->width(), popup->y() + popup->height() + 5);

    if (mouse)
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
    else
        QTest::touchEvent(window, touchDevice.data()).press(0, from);
    QVERIFY(!drawer->isVisible());

    static const int steps = 10;
    for (int i = 0; i < steps; ++i) {
        int x = from.x() + i * qAbs(from.x() - to.x()) / steps;
        int y = from.y() + i * qAbs(from.y() - to.y()) / steps;

        if (mouse)
            QTest::mouseMove(window, QPoint(x, y));
        else
            QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(x, y));
        QTest::qWait(1); // avoid infinite velocity
        QVERIFY(!drawer->isVisible());
    }

    if (mouse)
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    else
        QTest::touchEvent(window, touchDevice.data()).release(0, to);
    QVERIFY(!drawer->isVisible());
}

void tst_QQuickDrawer::nonModal_data()
{
    QTest::addColumn<bool>("mouse");
    QTest::newRow("mouse") << true;
    QTest::newRow("touch") << false;
}

// QTBUG-59652
void tst_QQuickDrawer::nonModal()
{
    QFETCH(bool, mouse);

    QQuickControlsApplicationHelper helper(this, QStringLiteral("window.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);
    drawer->setModal(false);

    const QPoint from(1, 1);
    const QPoint to(150, 1);

    // drag to open
    QSignalSpy openedSpy(drawer, SIGNAL(opened()));
    QVERIFY(openedSpy.isValid());

    if (mouse)
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
    else
        QTest::touchEvent(window, touchDevice.data()).press(0, from);

    static const int steps = 10;
    for (int i = 0; i < steps; ++i) {
        int x = i * qAbs(from.x() - to.x()) / steps;
        int y = i * qAbs(from.y() - to.y()) / steps;

        if (mouse)
            QTest::mouseMove(window, QPoint(x, y));
        else
            QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(x, y));
        QTest::qWait(1); // avoid infinite velocity
    }
    QVERIFY(drawer->isVisible());

    if (mouse)
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    else
        QTest::touchEvent(window, touchDevice.data()).release(0, to);
    QVERIFY(openedSpy.wait());

    // drag to close
    QSignalSpy closedSpy(drawer, SIGNAL(closed()));
    QVERIFY(closedSpy.isValid());

    if (mouse)
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, to);
    else
        QTest::touchEvent(window, touchDevice.data()).press(0, to);

    for (int i = steps - 1; i >= 0; --i) {
        int x = i * qAbs(from.x() - to.x()) / steps;
        int y = i * qAbs(from.y() - to.y()) / steps;

        if (mouse)
            QTest::mouseMove(window, QPoint(x, y));
        else
            QTest::touchEvent(window, touchDevice.data()).move(0, QPoint(x, y));
        QTest::qWait(1); // avoid infinite velocity
    }
    QVERIFY(drawer->isVisible());

    if (mouse)
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, from);
    else
        QTest::touchEvent(window, touchDevice.data()).release(0, from);
    QVERIFY(closedSpy.wait());
}

void tst_QQuickDrawer::slider_data()
{
    QTest::addColumn<bool>("mouse");
    QTest::addColumn<int>("delta");

    QTest::newRow("mouse") << true << 2;
    QTest::newRow("touch") << false << 2;
    QTest::newRow("mouse,delta") << true << 296 / 8;
}

void tst_QQuickDrawer::slider()
{
    QFETCH(bool, mouse);
    QFETCH(int, delta);

    QQuickControlsApplicationHelper helper(this, QStringLiteral("slider.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    QQuickSlider *slider = window->property("slider").value<QQuickSlider *>();
    QVERIFY(slider);

    QCOMPARE(slider->value(), 1.0);
    QCOMPARE(drawer->position(), 1.0);

    const qreal y = slider->height() / 2;
    const QPoint from(slider->width() - 1, y);
    const QPoint to(1, y);

    QTest::qWait(1);

    if (mouse)
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
    else
        QTest::touchEvent(window, touchDevice.data()).press(0, from);

    QTest::qWait(1);

    int distance = qAbs(from.x() - to.x());
    for (int dx = delta; dx <= distance; dx += delta) {
        if (mouse)
            QTest::mouseMove(window, from - QPoint(dx, 0));
        else
            QTest::touchEvent(window, touchDevice.data()).move(0, from - QPoint(dx, 0));
        QTest::qWait(1); // avoid infinite velocity
    }

    QCOMPARE(slider->value(), 0.0);
    QCOMPARE(drawer->position(), 1.0);

    if (mouse)
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    else
        QTest::touchEvent(window, touchDevice.data()).release(0, to);
}

void tst_QQuickDrawer::topEdgeScreenEdge()
{
    QQuickControlsApplicationHelper helper(this, QStringLiteral("topEdgeScreenEdge.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);

    QVERIFY(QMetaObject::invokeMethod(drawer, "open"));
    QTRY_COMPARE(drawer->position(), 1.0);
}

void tst_QQuickDrawer::bookkeepingInOverlay()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(!root.isNull(), qPrintable(component.errorString()));
    QQuickWindow *window = qobject_cast<QQuickWindow *>(root.get());
    QVERIFY(window);
    QQuickDrawer *drawer = window->property("drawer").value<QQuickDrawer *>();
    QVERIFY(drawer);
    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);
#ifdef QT_BUILD_INTERNAL
    QQuickOverlayPrivate *overlayD = QQuickOverlayPrivate::get(overlay);
    QVERIFY(!overlayD->stackingOrderDrawers().isEmpty());
#endif

    delete drawer;
#ifdef QT_BUILD_INTERNAL
    QVERIFY(overlayD->stackingOrderDrawers().isEmpty());
#endif
}

void tst_QQuickDrawer::touchOutsideOverlay() // QTBUG-103811
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("itemPartialOverlayModal.qml")));
    auto *drawer = window.rootObject()->findChild<QQuickDrawer*>();
    QVERIFY(drawer);
    QSignalSpy openedSpy(drawer, &QQuickDrawer::opened);
    QSignalSpy closedSpy(drawer, &QQuickDrawer::closed);

    drawer->open();
    QVERIFY(openedSpy.size() == 1 || openedSpy.wait());
    QVERIFY(drawer->isOpened());

    // tap-dance in bottom area beyond the overlay
    QPoint p1(100, 250);
    QPoint p2(300, 250);
    QTest::touchEvent(&window, touchDevice.data()).press(1, p1);
    p1 -= QPoint(1, 0);
    QTest::touchEvent(&window, touchDevice.data()).move(1, p1).press(2, p2);
    p2 -= QPoint(1, 0);
    QTest::touchEvent(&window, touchDevice.data()).release(1, p1).move(2, p2);
    QTest::touchEvent(&window, touchDevice.data()).press(1, p1).stationary(2);
    QTest::touchEvent(&window, touchDevice.data()).release(1, p1).release(2, p2);
    QQuickTouchUtils::flush(&window);

    // tap the overlay to try to close the drawer
    QVERIFY(drawer->closePolicy().testFlag(QQuickPopup::CloseOnReleaseOutside));
    const QPoint p3(300, 100);
    QTest::touchEvent(&window, touchDevice.data()).press(3, p3);
    QTest::touchEvent(&window, touchDevice.data()).release(3, p3);
    QQuickTouchUtils::flush(&window);
    QVERIFY(closedSpy.size() == 1 || closedSpy.wait());
    QCOMPARE(drawer->isOpened(), false);
}

void tst_QQuickDrawer::destroyWhileVisible()
{
    QScopedPointer<QQuickView> window(new QQuickView());
    QVERIFY(QQuickTest::showView(*window, testFileUrl("itemPartialOverlayModal.qml")));
    auto *drawer = window->rootObject()->findChild<QQuickDrawer*>();
    QVERIFY(drawer);

    drawer->open();
    QTRY_VERIFY(drawer->isOpened());

    QQuickItem *dimmer = QQuickPopupPrivate::get(drawer)->dimmer;
    QSignalSpy dimmerDeletedSpy(dimmer, &QObject::destroyed);

    // don't crash here when the drawer closes with an exit transition
    window.reset();

    // make sure the dimmer is deleted
    QTRY_COMPARE(dimmerDeletedSpy.size(), 1);


}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickDrawer)

#include "tst_qquickdrawer.moc"
