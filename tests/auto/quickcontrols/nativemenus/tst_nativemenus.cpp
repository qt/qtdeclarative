// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtGui/qcursor.h>
#if QT_CONFIG(shortcut)
#include <QtGui/qkeysequence.h>
#endif
#include <QtGui/qstylehints.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickmenuseparator_p.h>
#include <QtQuickTemplates2/private/qquicknativemenuitem_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
#define HAVE_NATIVE_MENU_SUPPORT
#endif

class tst_NativeMenus : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_NativeMenus();

private slots:
    void defaults();
    void staticActionsAndSubmenus();
    void dynamicActions();
    void dynamicSubmenus();
    void menuSeparator();
    void requestNativeChanges();
};

tst_NativeMenus::tst_NativeMenus()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings)
{
    qputenv("QT_QUICK_CONTROLS_USE_NATIVE_MENUS", "1");
}

// This allows us to use QQuickMenuItem's more descriptive operator<< output
// for the QCOMPARE failure message. It doesn't seem possible to use toString
// overloads or template specialization when types declared in QML are involved,
// as is the case for the MenuItems created from Menu's delegate.
#define COMPARE_MENUITEMS(actualMenuItem, expectedMenuItem) \
QVERIFY2(actualMenuItem == expectedMenuItem, \
    qPrintable(QString::fromLatin1("\n   Actual:    %1\n   Expected:  %2") \
        .arg(QDebug::toString(actualMenuItem), QDebug::toString(expectedMenuItem))));

void tst_NativeMenus::defaults()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("emptyMenu.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    QVERIFY(contextMenu->requestNative());
    // The GTK+ platform theme does support native menus, but for now we just skip the check.
#ifdef HAVE_NATIVE_MENU_SUPPORT
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);
    QVERIFY(contextMenuPrivate->usingNativeMenu());
#endif
}

void tst_NativeMenus::staticActionsAndSubmenus()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("staticActionsAndSubmenus.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    QVERIFY(contextMenu->requestNative());
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);

    // Check that the actions of the parent menu can be accessed
    // and are in the appropriate places in contentModel and contentData.
    auto *action1 = contextMenu->actionAt(0);
    QVERIFY(action1);
    auto *action1MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(0));
    QVERIFY(action1MenuItem);
    QCOMPARE(action1MenuItem->action(), action1);
    COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(contextMenuPrivate->contentData.at(0)),
        action1MenuItem);

    auto *action2 = contextMenu->actionAt(1);
    QVERIFY(action2);
    auto *action2MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(1));
    QVERIFY(action2MenuItem);
    QCOMPARE(action2MenuItem->action(), action2);
    COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(contextMenuPrivate->contentData.at(1)),
        action2MenuItem);

    // Check that the sub-menu can be accessed and is in the
    // appropriate place in contentData.
    auto *subMenu = contextMenu->menuAt(2);
    QVERIFY(subMenu);
    auto *subMenuPrivate = QQuickMenuPrivate::get(subMenu);
    auto *subMenuAction1 = subMenu->actionAt(0);
    QVERIFY(subMenuAction1);
    auto *subMenuAction1MenuItem = qobject_cast<QQuickMenuItem *>(subMenu->itemAt(0));
    QVERIFY(subMenuAction1MenuItem);
    QCOMPARE(subMenuAction1MenuItem->action(), subMenuAction1);
    COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(subMenuPrivate->contentData.at(0)),
        subMenuAction1MenuItem);
}

void tst_NativeMenus::dynamicActions()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("emptyMenu.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);

    // Check that items can be appended to an empty menu.
    QCOMPARE(contextMenu->actionAt(0), nullptr);
    QVERIFY(QMetaObject::invokeMethod(window, "addAction",
        Q_ARG(QQuickMenu *, contextMenu), Q_ARG(QString, "action1")));
    {
        auto action1 = contextMenu->actionAt(0);
        QVERIFY(action1);
        QCOMPARE(action1->text(), "action1");
        auto *action1MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(0));
        QVERIFY(action1MenuItem);
        QCOMPARE(action1MenuItem->action(), action1);
        COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(contextMenuPrivate->contentData.at(0)),
            action1MenuItem);
    }

    // Check that actions can be appended after existing items in the parent menu.
    QCOMPARE(contextMenu->actionAt(1), nullptr);
    QVERIFY(QMetaObject::invokeMethod(window, "addAction",
        Q_ARG(QQuickMenu *, contextMenu), Q_ARG(QString, "action2")));
    {
        auto action2 = contextMenu->actionAt(1);
        QVERIFY(action2);
        QCOMPARE(action2->text(), "action2");
        auto *action2MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(1));
        QVERIFY(action2MenuItem);
        QCOMPARE(action2MenuItem->action(), action2);
        COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(contextMenuPrivate->contentData.at(1)),
            action2MenuItem);
    }

    // Check that actions can be inserted before existing items in the parent menu.
    QVERIFY(QMetaObject::invokeMethod(window, "insertAction",
        Q_ARG(QQuickMenu *, contextMenu), Q_ARG(int, 0), Q_ARG(QString, "action0")));
    {
        auto action0 = contextMenu->actionAt(0);
        QVERIFY(action0);
        QCOMPARE(action0->text(), "action0");
        auto *action0MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(0));
        QVERIFY(action0MenuItem);
        QCOMPARE(action0MenuItem->action(), action0);
        // New items are always appended to contentData, regardless of the actual insertion index
        // in contentModel.
        COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(contextMenuPrivate->contentData.at(2)),
            action0MenuItem);
    }
}

void tst_NativeMenus::dynamicSubmenus()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("dynamicSubmenus.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);

    // We construct the sub-menu first in QML. At least on Windows, menu items
    // added to an empty sub-menu won't show up (tested with Widgets): QTBUG-120494.
    // So, this adds an already-populated menu as a sub-menu.
    QVERIFY(QMetaObject::invokeMethod(window, "addSubMenu", Q_ARG(QString, "subMenu1")));
    auto subMenu1 = contextMenu->menuAt(0);
    QVERIFY(subMenu1);
    QCOMPARE(subMenu1->title(), "subMenu1");
    auto *subMenu1Private = QQuickMenuPrivate::get(subMenu1);
#ifdef HAVE_NATIVE_MENU_SUPPORT
    QVERIFY(subMenu1Private->handle);
    QCOMPARE(subMenu1Private->nativeItems.size(), 1);
#endif
    auto *subMenu1MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(0));
    QVERIFY(subMenu1MenuItem);
    COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(contextMenuPrivate->contentData.at(0)),
        subMenu1MenuItem);
    QCOMPARE(contextMenuPrivate->contentData.size(), 1);
    {
        auto subMenuAction1 = subMenu1->actionAt(0);
        QVERIFY(subMenuAction1);
        QCOMPARE(subMenuAction1->text(), "subMenu1Action1");
        auto *subMenuAction1MenuItem = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(0));
        QVERIFY(subMenuAction1MenuItem);
        QCOMPARE(subMenuAction1MenuItem->action(), subMenuAction1);
        COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(subMenu1Private->contentData.at(0)),
            subMenuAction1MenuItem);
#ifdef HAVE_NATIVE_MENU_SUPPORT
        QCOMPARE(subMenu1Private->nativeItems.size(), 1);
#endif
    }

    // Check that actions can be appended after existing items in the sub-menu.
    QCOMPARE(subMenu1->actionAt(1), nullptr);
    QVERIFY(QMetaObject::invokeMethod(window, "addAction",
        Q_ARG(QQuickMenu *, subMenu1), Q_ARG(QString, "subMenu1Action2")));
    {
        auto subMenu1Action2 = subMenu1->actionAt(1);
        QVERIFY(subMenu1Action2);
        QCOMPARE(subMenu1Action2->text(), "subMenu1Action2");
        auto *subMenu1Action2MenuItem = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(1));
        QVERIFY(subMenu1Action2MenuItem);
        QCOMPARE(subMenu1Action2MenuItem->action(), subMenu1Action2);
        COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(subMenu1Private->contentData.at(1)),
            subMenu1Action2MenuItem);
        QCOMPARE(subMenu1Private->contentData.size(), 2);
    }

    // Check that actions can be inserted before existing items in the sub-menu.
    QVERIFY(QMetaObject::invokeMethod(window, "insertAction",
        Q_ARG(QQuickMenu *, subMenu1), Q_ARG(int, 0), Q_ARG(QString, "subMenu1Action0")));
    {
        auto subMenu1Action0 = subMenu1->actionAt(0);
        QVERIFY(subMenu1Action0);
        QCOMPARE(subMenu1Action0->text(), "subMenu1Action0");
        auto *subMenu1Action0MenuItem = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(0));
        QVERIFY(subMenu1Action0MenuItem);
        QCOMPARE(subMenu1Action0MenuItem->action(), subMenu1Action0);
        // New items are always appended to contentData, regardless of the actual insertion index
        // in contentModel.
        COMPARE_MENUITEMS(qobject_cast<QQuickMenuItem *>(subMenu1Private->contentData.at(2)),
            subMenu1Action0MenuItem);
        QCOMPARE(subMenu1Private->contentData.size(), 3);
    }

    {
        // Check that takeMenu works.
        auto *takenSubMenu = contextMenu->takeMenu(0);
        QCOMPARE(takenSubMenu, subMenu1);
        QCOMPARE(contextMenuPrivate->contentData.size(), 0);
#ifdef HAVE_NATIVE_MENU_SUPPORT
        QVERIFY(!subMenu1Private->handle);
        QCOMPARE(subMenu1Private->nativeItems.size(), 0);
#endif

        // Check that the sub-menu can be added back in to the menu.
        contextMenu->addMenu(takenSubMenu);
        QCOMPARE(contextMenuPrivate->contentData.size(), 1);
        auto *subMenu1MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(0));
        QVERIFY(subMenu1MenuItem);
        QCOMPARE(subMenu1MenuItem->text(), "subMenu1");
#ifdef HAVE_NATIVE_MENU_SUPPORT
        QVERIFY(subMenu1Private->handle);
        QCOMPARE(subMenu1Private->nativeItems.size(), 3);
#endif
        QCOMPARE(subMenu1Private->contentData.size(), 3);

        auto *subMenu1Action0MenuItem = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(0));
        QVERIFY(subMenu1Action0MenuItem);
    }

    // Check that removeMenu works.
    QVERIFY(contextMenu->menuAt(0));
    contextMenu->removeMenu(contextMenu->menuAt(0));
    QCOMPARE(contextMenuPrivate->contentData.size(), 0);
}

void tst_NativeMenus::menuSeparator()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("menuSeparator.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // Check that separators in menus are where we expect them to be.
    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    auto *contextMenuSeparatorAsItem = contextMenu->itemAt(1);
    QVERIFY(contextMenuSeparatorAsItem);
    auto *contextMenuSeparator = qobject_cast<QQuickMenuSeparator *>(contextMenuSeparatorAsItem);
    QVERIFY(contextMenuSeparator);
#ifdef HAVE_NATIVE_MENU_SUPPORT
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);
    QCOMPARE(contextMenuPrivate->nativeItems.size(), 3);
    auto *contextMenuSeparatorNativeItem = contextMenuPrivate->nativeItems.at(1);
    QVERIFY(contextMenuSeparatorNativeItem);
    QVERIFY(contextMenuSeparatorNativeItem->separator());
#endif

    // Check that separators in sub-menus are where we expect them to be.
    QQuickMenu *subMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(subMenu);
    auto *subMenuSeparatorAsItem = subMenu->itemAt(1);
    QVERIFY(subMenuSeparatorAsItem);
    auto *subMenuSeparator = qobject_cast<QQuickMenuSeparator *>(subMenuSeparatorAsItem);
    QVERIFY(subMenuSeparator);
#ifdef HAVE_NATIVE_MENU_SUPPORT
    auto *subMenuPrivate = QQuickMenuPrivate::get(subMenu);
    QCOMPARE(subMenuPrivate->nativeItems.size(), 3);
    auto *subMenuSeparatorNativeItem = subMenuPrivate->nativeItems.at(1);
    QVERIFY(subMenuSeparatorNativeItem);
    QVERIFY(subMenuSeparatorNativeItem->separator());
#endif
}

void tst_NativeMenus::requestNativeChanges()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("staticActionsAndSubmenus.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    QVERIFY(contextMenu->requestNative());
    QCOMPARE(contextMenu->count(), 3);
    // Sub-menus should respect the value of requestNative of their parents.
    auto *subMenu = contextMenu->menuAt(2);
    auto *subMenuPrivate = QQuickMenuPrivate::get(subMenu);
    QVERIFY(subMenuPrivate->useNativeMenu());
#ifdef HAVE_NATIVE_MENU_SUPPORT
    QVERIFY(subMenuPrivate->usingNativeMenu());
#else
    QVERIFY(!subMenuPrivate->usingNativeMenu());
#endif

    // Ensure that the menu and its sub-menu have enough room to open.
    if (window->width() / 2 <= contextMenu->width())
        window->setWidth(contextMenu->width() * 2 + 1);
    if (window->height() <= contextMenu->height())
        window->setHeight(contextMenu->height() + 1);
    QTRY_COMPARE(window->contentItem()->size(), window->size());

    // We can't test that aboutToShow/aboutToHide is emitted for native menus
    // because when they are shown, the event loop is blocked until they are closed.
    // So we just check that a native menu is actually in use before going on to test
    // non-native menus.
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);
#ifdef HAVE_NATIVE_MENU_SUPPORT
    QVERIFY(contextMenuPrivate->usingNativeMenu());
#else
    QVERIFY(!contextMenuPrivate->usingNativeMenu());
#endif
    contextMenu->setRequestNative(false);
    QVERIFY(!contextMenu->requestNative());
    QVERIFY(!contextMenuPrivate->usingNativeMenu());
    QVERIFY(!subMenuPrivate->useNativeMenu());
    QVERIFY(!subMenuPrivate->usingNativeMenu());

    // Check that we can open the menu by right-clicking (or just open it manually
    // if the platform doesn't support (moving) QCursor).
    QSignalSpy aboutToShowSpy(contextMenu, &QQuickMenu::aboutToShow);
    QVERIFY(aboutToShowSpy.isValid());
    bool couldMoveCursorPos = false;
    const QPoint cursorPos(1, 1);
#if QT_CONFIG(cursor)
    // Try moving the cursor from the current position to test if the platform
    // supports moving the cursor.
    const QPoint point = QCursor::pos() + QPoint(1, 1);
    QCursor::setPos(point);
    if (QTest::qWaitFor([point]{ return QCursor::pos() == point; })) {
        couldMoveCursorPos = true;
        const QPoint globalCursorPos = window->mapToGlobal(cursorPos);
        QCursor::setPos(globalCursorPos);
        QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, cursorPos);
    }
#endif
    if (!couldMoveCursorPos) {
        contextMenu->setX(cursorPos.x());
        contextMenu->setY(cursorPos.y());
        contextMenu->open();
    }
    QVERIFY(contextMenu->isVisible());
    QTRY_VERIFY(contextMenu->isOpened());
    QCOMPARE(aboutToShowSpy.size(), 1);
    // Check that it opened at the mouse cursor and actually has menu items.
    QCOMPARE(contextMenu->x(), cursorPos.x());
    QCOMPARE(contextMenu->y(), cursorPos.y());
    auto *action1MenuItem = qobject_cast<QQuickMenuItem *>(contextMenu->itemAt(0));
    QVERIFY(action1MenuItem);
    QCOMPARE(action1MenuItem->text(), "action1");

    // Test that we warn if trying to set requestNative while visible.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Cannot set requestNative while menu is visible"));
    contextMenu->setRequestNative(true);
    // Shouldn't have changed.
    QVERIFY(!contextMenu->requestNative());

    // Also check the submenu.
    auto *subAction1MenuItem = qobject_cast<QQuickMenuItem *>(subMenu->itemAt(0));
    QVERIFY(subAction1MenuItem);
    QCOMPARE(subAction1MenuItem->text(), "subAction1");

    // Test closing the non-native menu by clicking on an item.
    QSignalSpy aboutToHideSpy(contextMenu, &QQuickMenu::aboutToHide);
    QVERIFY(aboutToHideSpy.isValid());
    QVERIFY(clickButton(action1MenuItem));
    QVERIFY(!contextMenu->isOpened());
    QTRY_VERIFY(!contextMenu->isVisible());
    QCOMPARE(aboutToShowSpy.size(), 1);

    // Although we can't open the native menu, we can at least check that
    // making the menu native again doesn't e.g. crash.
    contextMenu->setRequestNative(true);
    QVERIFY(contextMenuPrivate->useNativeMenu());
    QVERIFY(subMenuPrivate->useNativeMenu());
#ifdef HAVE_NATIVE_MENU_SUPPORT
    QVERIFY(contextMenuPrivate->usingNativeMenu());
    QVERIFY(subMenuPrivate->usingNativeMenu());
#else
    QVERIFY(!contextMenuPrivate->usingNativeMenu());
    QVERIFY(!subMenuPrivate->usingNativeMenu());
#endif

    // Check that we warn when requestNative is set on a sub-menu.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Cannot set requestNative on a sub-menu"));
    subMenu->setRequestNative(false);
    QVERIFY(subMenuPrivate->useNativeMenu());
#ifdef HAVE_NATIVE_MENU_SUPPORT
    QVERIFY(subMenuPrivate->usingNativeMenu());
#else
    QVERIFY(!subMenuPrivate->usingNativeMenu());
#endif
}

// TODO: add a test that mixes items with native items
// and ensure that all items are recreated as non-native

QTEST_MAIN(tst_NativeMenus)

#include "tst_nativemenus.moc"
