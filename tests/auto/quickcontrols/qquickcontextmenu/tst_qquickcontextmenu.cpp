// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickContextMenu : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickContextMenu();

private slots:
    void initTestCase() override;

    void customContextMenu_data();
    void customContextMenu();
    void sharedContextMenu();
    void tapHandler();
    void notAttachedToItem();
    void nullMenu();
    void idOnMenu();
    void createOnRequested_data();
    void createOnRequested();
    void drawerShouldntPreventOpening();
    void explicitMenuPreventsBuiltInMenu();
    void menuItemShouldntTriggerOnRelease();

private:
    bool contextMenuTriggeredOnRelease = false;
};

tst_QQuickContextMenu::tst_QQuickContextMenu()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings)
{
}

void tst_QQuickContextMenu::initTestCase()
{
    QQmlDataTest::initTestCase();

    // Can't test native menus with QTest.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);

    contextMenuTriggeredOnRelease = QGuiApplicationPrivate::platformTheme()->themeHint(
        QPlatformTheme::ContextMenuOnMouseRelease).toBool();
}

void tst_QQuickContextMenu::customContextMenu_data()
{
    QTest::addColumn<QString>("qmlFileName");

    QTest::addRow("Rectangle") << "customContextMenuOnRectangle.qml";
    QTest::addRow("Label") << "customContextMenuOnLabel.qml";
    QTest::addRow("Control") << "customContextMenuOnControl.qml";
    QTest::addRow("NestedRectangle") << "customContextMenuOnNestedRectangle.qml";
    QTest::addRow("Pane") << "customContextMenuOnPane.qml";
}

void tst_QQuickContextMenu::customContextMenu()
{
    QFETCH(QString, qmlFileName);

    QQuickApplicationHelper helper(this, qmlFileName);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *tomatoItem = window->findChild<QQuickItem *>("tomato");
    QVERIFY(tomatoItem);

    const QPoint &tomatoCenter = mapCenterToWindow(tomatoItem);
    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    // Due to the menu property being deferred, the Menu isn't created until
    // the context menu event is received, so we can't look for it before the press.
    QQuickMenu *menu = window->findChild<QQuickMenu *>();
    if (!contextMenuTriggeredOnRelease) {
        QVERIFY(menu);
        QTRY_VERIFY(menu->isOpened());
    } else {
        // It's triggered on press, so it shouldn't exist yet.
        QVERIFY(!menu);
    }

    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    if (contextMenuTriggeredOnRelease)
        menu = window->findChild<QQuickMenu *>();
#ifdef Q_OS_WIN
    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QSKIP("Menu fails to open on Windows (QTBUG-132436)");
#endif
    QVERIFY(menu);
    QTRY_VERIFY(menu->isOpened());

    // Popups are positioned relative to their parent, and it should be opened at the center:
    // width (100) / 2 = 50
#ifdef Q_OS_ANDROID
    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QEXPECT_FAIL("", "This test fails on Android 14 in CI, but passes locally with 15", Abort);
#endif
    QCOMPARE(menu->position(), QPoint(50, 50));
}

void tst_QQuickContextMenu::sharedContextMenu()
{
    QQuickApplicationHelper helper(this, "sharedContextMenuOnRectangle.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *tomato = window->findChild<QQuickItem *>("tomato");
    QVERIFY(tomato);

    auto *reallyRipeTomato = window->findChild<QQuickItem *>("really ripe tomato");
    QVERIFY(reallyRipeTomato);

    // Check that parentItem allows users to distinguish which item triggered a menu.
    const QPoint &tomatoCenter = mapCenterToWindow(tomato);
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    // There should only be one menu.
    auto menus = window->findChildren<QQuickMenu *>();
    QCOMPARE(menus.count(), 1);
    QPointer<QQuickMenu> menu = menus.first();
#ifdef Q_OS_WIN
    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QSKIP("Menu fails to open on Windows (QTBUG-132436)");
#endif
    QTRY_VERIFY(menu->isOpened());
    QCOMPARE(menu->parentItem(), tomato);
    QCOMPARE(menu->itemAt(0)->property("text").toString(), "Eat tomato");

    menu->close();
    QTRY_VERIFY(!menu->isVisible());

    const QPoint &reallyRipeTomatoCenter = mapCenterToWindow(reallyRipeTomato);
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, reallyRipeTomatoCenter);
    QVERIFY(menu);
    menus = window->findChildren<QQuickMenu *>();
    QCOMPARE(menus.count(), 1);
    QCOMPARE(menus.last(), menu);
    QTRY_VERIFY(menu->isOpened());
    QCOMPARE(menu->parentItem(), reallyRipeTomato);
    QCOMPARE(menu->itemAt(0)->property("text").toString(), "Eat really ripe tomato");
}

// After 70c61b12efe9d1faf24063b63cf5a69414d45cea in qtbase, accepting a press/release will not
// prevent an item beneath the accepting item from getting a context menu event.
// This test was originally written before that, and would verify that only the handler
// got the event. Now it checks that both received events.
void tst_QQuickContextMenu::tapHandler()
{
    QQuickApplicationHelper helper(this, "tapHandler.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const QSignalSpy contextMenuOpenedSpy(window, SIGNAL(contextMenuOpened()));
    QVERIFY(contextMenuOpenedSpy.isValid());

    const auto *tapHandler = window->findChild<QObject *>("tapHandler");
    QVERIFY(tapHandler);
    const QSignalSpy tappedSpy(tapHandler, SIGNAL(tapped(QEventPoint, Qt::MouseButton)));
    QVERIFY(tappedSpy.isValid());

    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    // First check that the menu was actually created, as this is an easier-to-understand
    // failure message than a signal spy count mismatch.
    const auto *menu = window->findChild<QQuickMenu *>();
    QVERIFY(menu);
    QTRY_COMPARE(contextMenuOpenedSpy.count(), 1);
    QCOMPARE(tappedSpy.count(), 1);
}

void tst_QQuickContextMenu::notAttachedToItem()
{
    // Should warn but shouldn't crash.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*ContextMenu must be attached to an Item"));
    QQuickApplicationHelper helper(this, "notAttachedToItem.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
}

void tst_QQuickContextMenu::nullMenu()
{
    QQuickApplicationHelper helper(this, "nullMenu.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // Shouldn't crash or warn.
    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    auto *menu = window->findChild<QQuickMenu *>();
    QVERIFY(!menu);
}

void tst_QQuickContextMenu::idOnMenu()
{
    QQuickApplicationHelper helper(this, "idOnMenu.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // Giving the menu an id prevents deferred execution, but the menu should still work.
    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    auto *menu = window->findChild<QQuickMenu *>();
    QVERIFY(menu);
    QTRY_VERIFY(menu->isOpened());
}

void tst_QQuickContextMenu::createOnRequested_data()
{
    QTest::addColumn<bool>("programmaticShow");

    QTest::addRow("auto") << false;
    QTest::addRow("manual") << true;
}

void tst_QQuickContextMenu::createOnRequested()
{
    QFETCH(bool, programmaticShow);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("customContextMenuOnRequested.qml")));
    auto *tomatoItem = window.findChild<QQuickItem *>("tomato");
    QVERIFY(tomatoItem);
    const QPoint &tomatoCenter = mapCenterToWindow(tomatoItem);
    window.rootObject()->setProperty("showItToo", programmaticShow);

    // On press or release (depending on QPlatformTheme::ContextMenuOnMouseRelease),
    // ContextMenu.onRequested(pos) should create a standalone custom context menu.
    // If programmaticShow, it will call popup() too; if not, QQuickContextMenu
    // will show it.  Either way, it should still be open after the release.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    QQuickMenu *menu = window.findChild<QQuickMenu *>();
    QVERIFY(menu);
    QTRY_VERIFY(menu->isOpened());
    QCOMPARE(window.rootObject()->property("pressPos").toPoint(), tomatoCenter);
}

// Drawer shouldn't prevent right clicks from opening ContextMenu: QTBUG-132765.
void tst_QQuickContextMenu::drawerShouldntPreventOpening()
{
    QQuickApplicationHelper helper(this, "drawer.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    auto *menu = window->findChild<QQuickMenu *>();
    QVERIFY(menu);
    QTRY_VERIFY(menu->isOpened());
}

void tst_QQuickContextMenu::explicitMenuPreventsBuiltInMenu()
{
    QQuickApplicationHelper helper(this, "tapHandlerMenuOverride.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *textArea = window->findChild<QQuickItem *>("textArea");
    QVERIFY(textArea);
    auto *tapHandler = window->findChild<QQuickTapHandler *>();
    QVERIFY(tapHandler);
    const QSignalSpy tapHandlerTappedSpy(tapHandler, &QQuickTapHandler::tapped);
    auto *windowMenu = window->findChild<QQuickMenu *>("windowMenu");
    QVERIFY(windowMenu);

    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    // The menu that has opened is the window's menu; TextArea's built-in menu has not been instantiated
    QCOMPARE(textArea->findChild<QQuickMenu *>(), nullptr);
    QTRY_VERIFY(windowMenu->isOpened());
    QCOMPARE(tapHandlerTappedSpy.count(), 1);
}

void tst_QQuickContextMenu::menuItemShouldntTriggerOnRelease() // QTBUG-133302
{
    QQuickApplicationHelper helper(this, "windowedContextMenuOnControl.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *tomatoItem = window->findChild<QQuickItem *>("tomato");
    QVERIFY(tomatoItem);
    QSignalSpy triggeredSpy(window, SIGNAL(triggered(QObject *)));
    QVERIFY(triggeredSpy.isValid());

    const QPoint &tomatoCenter = mapCenterToWindow(tomatoItem);
    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    auto *menu = window->findChild<QQuickMenu *>();
    QQuickMenuItem *firstMenuItem = nullptr;
    QQuickMenuItemPrivate *firstMenuItemPriv = nullptr;
    if (!contextMenuTriggeredOnRelease) {
        QVERIFY(menu);
        QTRY_VERIFY(menu->isOpened());
        firstMenuItem = qobject_cast<QQuickMenuItem *>(menu->itemAt(0));
        QVERIFY(firstMenuItem);
        // The mouse press event alone must not highlight the menu item under the mouse.
        // (A mouse move could do that, while holding the button; or, another mouse press/release pair afterwards.)
        QCOMPARE(firstMenuItem->isHighlighted(), false);
        firstMenuItemPriv = static_cast<QQuickMenuItemPrivate *>(QQuickMenuItemPrivate::get(firstMenuItem));
        QVERIFY(firstMenuItemPriv);
    } else {
        // It's triggered on press, so it shouldn't exist yet.
        QVERIFY(!menu);
    }

    // After release, the menu should still be open, and no triggered() signal received
    // (because the user did not intentionally drag over an item and release).
    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    if (contextMenuTriggeredOnRelease) {
        menu = window->findChild<QQuickMenu *>();
        QVERIFY(menu);
        QTRY_VERIFY(menu->isOpened());
        firstMenuItem = qobject_cast<QQuickMenuItem *>(menu->itemAt(0));
        QVERIFY(firstMenuItem);
        firstMenuItemPriv = static_cast<QQuickMenuItemPrivate *>(QQuickMenuItemPrivate::get(firstMenuItem));
    } else {
        QVERIFY(menu->isOpened());
    }
    // Implementation detail: in the failure case, QQuickMenuPrivate::handleReleaseWithoutGrab
    // calls menuItem->animateClick(). We now avoid that if the menu item was not already highlighted.
    QCOMPARE(firstMenuItemPriv->animateTimer, 0); // timer not started
    // menu item still not highlighted
    QCOMPARE(firstMenuItem->isHighlighted(), false);
    QCOMPARE(triggeredSpy.size(), 0);
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickContextMenu)

#include "tst_qquickcontextmenu.moc"
