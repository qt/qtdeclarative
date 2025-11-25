// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/qclipboard.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>
#include <QtQuickTemplates2/private/qquickcontextmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupwindow_p_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTest/quicktest.h>

using namespace QQuickControlsTestUtils;
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
    void textControlsMenuKey();
    void mouseAreaUnderTextArea();
    void textEditingContextMenuUndoRedo_data();
    void textEditingContextMenuUndoRedo();
    void textEditingContextMenuCut_data();
    void textEditingContextMenuCut();
    void textEditingContextMenuCopy_data();
    void textEditingContextMenuCopy();
    void textEditingContextMenuPaste_data();
    void textEditingContextMenuPaste();
    void textEditingContextMenuDelete_data();
    void textEditingContextMenuDelete();
    void textEditingContextMenuSelectAll_data();
    void textEditingContextMenuSelectAll();

private:
    void textEditingContextMenuData();

    enum class TextEditingContextMenuItemType {
        Undo = 0,
        Redo = 1,
        // (separator)
        Cut = 3,
        Copy = 4,
        Paste = 5,
        Delete = 6,
        // (separator)
        SelectAll = 8
    };

    int textEditingContextMenuItemIndex(TextEditingContextMenuItemType type);

    bool contextMenuTriggeredOnRelease = false;

    bool hasClipboardSupport =
#if QT_CONFIG(clipboard)
        true;
#else
        false;
#endif

    bool hasUndoRedo = false;

    QString textFirstHalf;
    QString textSecondHalf;
    QString textComplete;
    QString textCompleteLocaleSpecific;
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

    hasUndoRedo = QQuickStyle::name() != "iOS";

    textFirstHalf = QLatin1String("123");
    textSecondHalf = QLatin1String("456");
    textComplete = QLatin1String("123,456");
    textCompleteLocaleSpecific = QLocale().toString(123456);
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
    const QSignalSpy tappedSpy(tapHandler, SIGNAL(tapped(QEventPoint,Qt::MouseButton)));
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
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on android. See QTBUG-137400");
#endif

    QSKIP("Test function has been flaky from the start, QTBUG-141406.");

    QQuickApplicationHelper helper(this, "windowedContextMenuOnControl.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *tomatoItem = window->findChild<QQuickItem *>("tomato");
    QVERIFY(tomatoItem);
    QSignalSpy triggeredSpy(window, SIGNAL(triggered(QObject*)));
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

void tst_QQuickContextMenu::textControlsMenuKey()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on android. See QTBUG-139400");
#endif
    QQuickApplicationHelper helper(this, "textControlsAndParentMenus.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *textArea = window->findChild<QQuickItem *>("textArea");
    QVERIFY(textArea);
    auto *textField = window->findChild<QQuickItem *>("textField");
    QVERIFY(textField);
    auto *windowMenu = window->findChild<QQuickMenu *>("windowMenu");
    QVERIFY(windowMenu);
    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());

    // give position in the middle of the window: expect the window menu
    {
        QContextMenuEvent cme(QContextMenuEvent::Keyboard, windowCenter, window->mapToGlobal(windowCenter));
        QGuiApplication::sendEvent(window, &cme);
        auto *openMenu = window->findChild<QQuickMenu *>();
        QVERIFY(openMenu);
        QCOMPARE(openMenu->objectName(), "windowMenu");
        openMenu->close();
    }

    // focus the TextArea and give position 0, 0: expect the TextArea's menu
    {
        textArea->forceActiveFocus();
        QContextMenuEvent cme(QContextMenuEvent::Keyboard, {}, window->mapToGlobal(QPoint()));
        QGuiApplication::sendEvent(window, &cme);
        auto *openMenu = textArea->findChild<QQuickMenu *>();
        QVERIFY(openMenu);
        openMenu->close();
    }

    // focus the TextField and give position 0, 0: expect the TextField's menu
    {
        textField->forceActiveFocus();
        QContextMenuEvent cme(QContextMenuEvent::Keyboard, {}, window->mapToGlobal(QPoint()));
        QGuiApplication::sendEvent(window, &cme);
        auto *openMenu = textField->findChild<QQuickMenu *>();
        QVERIFY(openMenu);
        openMenu->close();
    }
}

void tst_QQuickContextMenu::mouseAreaUnderTextArea()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on android. See QTBUG-139400");
#endif
    if (!arePopupWindowsSupported())
        QSKIP("The platform doesn't support popup windows. Skipping test.");

    QQuickApplicationHelper helper(this, "mouseAreaUnderTextArea.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *textArea = window->findChild<QQuickTextArea *>();
    QVERIFY(textArea);
    auto *mouseArea = window->findChild<QQuickMouseArea *>();
    QVERIFY(mouseArea);

    // Open the menu by right clicking on the TextArea.
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(textArea));
    auto *menu = window->findChild<QQuickMenu *>();
    QVERIFY(menu);
    QTRY_VERIFY(menu->isOpened());

    QQuickMenuPrivate *menuPrivate = QQuickMenuPrivate::get(menu);
    QVERIFY(menuPrivate);
    QVERIFY(menuPrivate->usePopupWindow());
    QVERIFY(menuPrivate->popupWindow);
    QVERIFY(menuPrivate->popupWindow->isActive());
    QTRY_COMPARE(QQuickTest::qIsPolishScheduled(menuPrivate->popupWindow), false);

    // Click on the menu item to close the menu; the MouseArea shouldn't emit pressed().
    auto *selectAllMenuItem = qobject_cast<QQuickMenuItem *>(menu->itemAt(menu->count() - 1));
    QVERIFY(selectAllMenuItem);
    QCOMPARE(selectAllMenuItem->text(), "Select All");
    const QSignalSpy mouseAreaPressedSpy(mouseArea, &QQuickMouseArea::pressed);
    QVERIFY(mouseAreaPressedSpy.isValid());
    const QSignalSpy menuItemTriggeredSpy(selectAllMenuItem, &QQuickMenuItem::triggered);
    QVERIFY(menuItemTriggeredSpy.isValid());
    const QPointF menuItemCenter(selectAllMenuItem->width() / 2, selectAllMenuItem->height() / 2);
    const QPoint posGlobal = selectAllMenuItem->mapToGlobal(menuItemCenter).toPoint();
    const QPoint posMainWindowLocal = window->mapFromGlobal(posGlobal);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, posMainWindowLocal);
    QTRY_COMPARE(menuItemTriggeredSpy.size(), 1);
    QCOMPARE(mouseAreaPressedSpy.size(), 0);
    QTRY_COMPARE(menuPrivate->popupWindow->isVisible(), false);
}

void tst_QQuickContextMenu::textEditingContextMenuData()
{
    QTest::addColumn<QString>("qmlFileName");
    QTest::addColumn<QString>("expectedTextComplete");

    QTest::addRow("TextArea") << "textAreaInPane.qml" << textComplete;
    QTest::addRow("TextField") << "textFieldInPane.qml" << textComplete;
    QTest::addRow("SpinBox") << "spinBoxInPane.qml" << textCompleteLocaleSpecific;
}

int tst_QQuickContextMenu::textEditingContextMenuItemIndex(TextEditingContextMenuItemType type)
{
    const int index = static_cast<int>(type);
    if (hasUndoRedo)
        return index;

    if (type == TextEditingContextMenuItemType::Undo
            || type == TextEditingContextMenuItemType::Redo) {
        qWarning() << "This platform/style doesn't support undo/redo";
        return -1;
    }

    // 3 items don't exist, so indices are lowered.
    return index - 3;
}

void tst_QQuickContextMenu::textEditingContextMenuUndoRedo_data()
{
    textEditingContextMenuData();
}

void tst_QQuickContextMenu::textEditingContextMenuUndoRedo()
{
    QFETCH(QString, qmlFileName);
    QFETCH(QString, expectedTextComplete);

    SKIP_IF_NO_WINDOW_ACTIVATION;

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
            || (QGuiApplication::platformName() == QLatin1String("minimal"))) {
        QSKIP("offscreen platform plugin can't return focus to the main window after "
            "the menu's window closes, even with requestActivate(). With minimal,"
            "showView fails (\"Position failed to update\").");
    }

    if (!hasUndoRedo)
        QSKIP("iOS doesn't have undo/redo context menu items");

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFileName)));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *editor = window.rootObject()->property("editor").value<QQuickItem *>();
    QVERIFY(editor);
    editor->forceActiveFocus();
    // Don't use clear(), as that affects the undo stack.
    QVERIFY(editor->setProperty("text", ""));

    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    auto *contextMenu = editor->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    QTRY_VERIFY(contextMenu->menu()->isOpened());
    auto *undoMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::Undo)));
    QCOMPARE(undoMenuItem->text(), "Undo");
    QVERIFY(!undoMenuItem->isEnabled());
    auto *redoMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::Redo)));
    QCOMPARE(redoMenuItem->text(), "Redo");
    QVERIFY(!redoMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Enter some text. Undo should then be enabled, but not redo.
    QTest::keyClick(&window, Qt::Key_1);
    QCOMPARE(editor->property("text").toString(), QLatin1String("1"));
    QVERIFY(undoMenuItem->isEnabled());
    QVERIFY(!redoMenuItem->isEnabled());

    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    QTRY_VERIFY(contextMenu->menu()->isOpened());

    // Click on the Undo menu item. Redo should then be enabled.
    QVERIFY(clickButton(undoMenuItem));
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
    QCOMPARE(editor->property("text").toString(), QString());
    QVERIFY(!undoMenuItem->isEnabled());
    QVERIFY(redoMenuItem->isEnabled());

    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    QTRY_VERIFY(contextMenu->menu()->isOpened());

    // Click on the Redo menu item. Undo should then be enabled.
    QVERIFY(clickButton(redoMenuItem));
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
    QCOMPARE(editor->property("text").toString(), QLatin1String("1"));
    QVERIFY(undoMenuItem->isEnabled());
    QVERIFY(!redoMenuItem->isEnabled());
}

void tst_QQuickContextMenu::textEditingContextMenuCut_data()
{
    textEditingContextMenuData();
}

void tst_QQuickContextMenu::textEditingContextMenuCut()
{
    QFETCH(QString, qmlFileName);
    QFETCH(QString, expectedTextComplete);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFileName)));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *editor = window.rootObject()->property("editor").value<QQuickItem *>();
    QVERIFY(editor);
    editor->forceActiveFocus();
    // Ensure that our expected text accounts for locale-specific formatting.
    QCOMPARE(editor->property("text").toString(), expectedTextComplete);

    // Right click on the editor to open the context menu.
    // Right-clicking without a selection should result in the Cut menu item being disabled.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    auto *contextMenu = editor->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    auto *cutMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::Cut)));
    QVERIFY(cutMenuItem);
    QCOMPARE(cutMenuItem->text(), "Cut");
    QVERIFY(!cutMenuItem->isEnabled());
    QVERIFY(editor->property("selectedText").toString().isEmpty());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Right-clicking with a selection should result in the Cut menu item being enabled
    // if Qt was built with clipboard support.
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, textFirstHalf.length()),
        Q_ARG(int, editor->property("length").toInt())));
    const QString editorPreCutSelectedText = editor->property("selectedText").toString();
    // Don't hard-code the expected text here, because locales can affect the numeric separators.
    const auto cutText = editorPreCutSelectedText.mid(editorPreCutSelectedText.indexOf('3'));
    QCOMPARE(editor->property("selectedText").toString(), cutText);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(cutMenuItem->isEnabled(), hasClipboardSupport);
    // QTBUG-133302: the first menu item shouldn't be immediately triggered.
    QCOMPARE(QQuickMenuItemPrivate::get(cutMenuItem)->animateTimer, 0);

    // Click on the Cut menu item (if enabled) and close the menu.
#if QT_CONFIG(clipboard)
    QVERIFY(clickButton(cutMenuItem));
    QCOMPARE(editor->property("text").toString(), textFirstHalf);
    QCOMPARE(qGuiApp->clipboard()->text(), cutText);
#else
    QTest::keyClick(&window, Qt::Key_Escape);
#endif
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the editor read-only. Cut should no longer be enabled.
    editor->setProperty("readOnly", true);
    QVERIFY(QMetaObject::invokeMethod(editor, "selectAll"));
    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(cutMenuItem->text(), "Cut");
    QVERIFY(!cutMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickContextMenu::textEditingContextMenuCopy_data()
{
    textEditingContextMenuData();
}

void tst_QQuickContextMenu::textEditingContextMenuCopy()
{
    QFETCH(QString, qmlFileName);
    QFETCH(QString, expectedTextComplete);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFileName)));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *editor = window.rootObject()->property("editor").value<QQuickItem *>();
    QVERIFY(editor);
    editor->forceActiveFocus();

    // Right click on the editor to open the context menu.
    // Right-clicking without a selection should result in the Copy menu item being disabled.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    auto *contextMenu = editor->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    auto *copyMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::Copy)));
    QVERIFY(copyMenuItem);
    QCOMPARE(copyMenuItem->text(), "Copy");
    QVERIFY(!copyMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Right-clicking with a selection should result in the Copy menu item being enabled
    // if Qt was built with clipboard support.
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 0), Q_ARG(int, textFirstHalf.length())));
    QCOMPARE(editor->property("selectedText").toString(), textFirstHalf);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(copyMenuItem->isEnabled(), hasClipboardSupport);

    // Click on the Copy menu item (if enabled) and close the menu.
#if QT_CONFIG(clipboard)
    QVERIFY(clickButton(copyMenuItem));
    QCOMPARE(editor->property("text").toString(), expectedTextComplete);
    const auto *clipboard = QGuiApplication::clipboard();
    QCOMPARE(clipboard->text(), textFirstHalf);
#else
    QTest::keyClick(&window, Qt::Key_Escape);
#endif
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the editor read-only. Copy should still be enabled if Qt was built with clipboard support.
    editor->setProperty("readOnly", true);
    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(copyMenuItem->text(), "Copy");
    // Select some text.
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 0), Q_ARG(int, textFirstHalf.length())));
    QCOMPARE(copyMenuItem->isEnabled(), hasClipboardSupport);

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickContextMenu::textEditingContextMenuPaste_data()
{
    textEditingContextMenuData();
}

void tst_QQuickContextMenu::textEditingContextMenuPaste()
{
    QFETCH(QString, qmlFileName);
    QFETCH(QString, expectedTextComplete);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFileName)));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *editor = window.rootObject()->property("editor").value<QQuickItem *>();
    QVERIFY(editor);
    editor->forceActiveFocus();

#if QT_CONFIG(clipboard)
    auto *clipboard = QGuiApplication::clipboard();
    clipboard->setText("789");
#endif

    // Right click on the editor to open the context menu.
    // For some reason the cursor is at the beginning of the text, even when
    // right-clicking to the right of it, so first left-click.
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier,
        mapToWindow(editor, editor->width() - 50, editor->height() / 2));
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier,
        mapToWindow(editor, editor->width() - 50, editor->height() / 2));
    auto *contextMenu = editor->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());

    auto *pasteMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::Paste)));
    QVERIFY(pasteMenuItem);
    QCOMPARE(pasteMenuItem->text(), "Paste");
    QCOMPARE(pasteMenuItem->isEnabled(), hasClipboardSupport);

    // Click on the Paste menu item (if enabled) and close the menu.
#if QT_CONFIG(clipboard)
    QVERIFY(clickButton(pasteMenuItem));
    QCOMPARE(editor->property("text").toString(), expectedTextComplete + clipboard->text());
#else
    QTest::keyClick(&window, Qt::Key_Escape);
#endif
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the editor read-only. Paste should no longer be enabled.
    editor->setProperty("readOnly", true);
    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(pasteMenuItem->text(), "Paste");
    QVERIFY(!pasteMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickContextMenu::textEditingContextMenuDelete_data()
{
    textEditingContextMenuData();
}

void tst_QQuickContextMenu::textEditingContextMenuDelete()
{
    QFETCH(QString, qmlFileName);
    QFETCH(QString, expectedTextComplete);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFileName)));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *editor = window.rootObject()->property("editor").value<QQuickItem *>();
    QVERIFY(editor);
    editor->forceActiveFocus();

    // Right click on the editor to open the context menu.
    // Right-clicking without a selection should result in the Delete menu item being disabled.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    auto *contextMenu = editor->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    auto *deleteMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::Delete)));
    QCOMPARE(deleteMenuItem->text(), "Delete");
    QVERIFY(!deleteMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Right-clicking with a selection should result in the Delete menu item being enabled.
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, textFirstHalf.length()),
        Q_ARG(int, editor->property("length").toInt())));
    const QString editorPreDeleteSelectedText = editor->property("selectedText").toString();
    // Don't hard-code the expected text here, because locales can affect the numeric separators.
    const auto deleteText = editorPreDeleteSelectedText.mid(editorPreDeleteSelectedText.indexOf('3') + 1);
    QCOMPARE(editor->property("selectedText").toString(), deleteText);
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QVERIFY(deleteMenuItem->isEnabled());

    // Click on the Delete menu item and close the menu.
    QVERIFY(clickButton(deleteMenuItem));
    QCOMPARE(editor->property("text").toString(), textFirstHalf);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the editor read-only. Delete should no longer be enabled.
    editor->setProperty("readOnly", true);
    QVERIFY(QMetaObject::invokeMethod(editor, "selectAll"));
    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(deleteMenuItem->text(), "Delete");
    QVERIFY(!deleteMenuItem->isEnabled());

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

void tst_QQuickContextMenu::textEditingContextMenuSelectAll_data()
{
    textEditingContextMenuData();
}

void tst_QQuickContextMenu::textEditingContextMenuSelectAll()
{
    QFETCH(QString, qmlFileName);
    QFETCH(QString, expectedTextComplete);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFileName)));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *editor = window.rootObject()->property("editor").value<QQuickItem *>();
    QVERIFY(editor);
    editor->forceActiveFocus();

    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    auto *contextMenu = editor->findChild<QQuickContextMenu *>();
    QVERIFY(contextMenu);
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    auto *selectAllMenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->menu()->itemAt(
        textEditingContextMenuItemIndex(TextEditingContextMenuItemType::SelectAll)));
    QVERIFY(selectAllMenuItem);
    QCOMPARE(selectAllMenuItem->text(), "Select All");

    // Click on the Select All menu item and close the menu.
    QVERIFY(clickButton(selectAllMenuItem));
    QCOMPARE(editor->property("selectedText").toString(), expectedTextComplete);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());

    // Make the editor read-only. Select All should still be enabled.
    editor->setProperty("readOnly", true);
    // Right click on the editor to open the context menu.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, mapCenterToWindow(editor));
    TRY_VERIFY_POPUP_OPENED(contextMenu->menu());
    QCOMPARE(selectAllMenuItem->text(), "Select All");
    QVERIFY(selectAllMenuItem->isEnabled());
    editor->setProperty("readOnly", false);

    // Close the context menu.
    QTest::keyClick(&window, Qt::Key_Escape);
    QTRY_VERIFY(!contextMenu->menu()->isVisible());
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickContextMenu)

#include "tst_qquickcontextmenu.moc"
