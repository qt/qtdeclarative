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

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

/*
    We have a separate test project for native menus because we don't
    want to run them for every style, just the platforms that have
    native menu support.
*/

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
};

tst_NativeMenus::tst_NativeMenus()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qputenv("QT_QUICK_CONTROLS_USE_NATIVE_MENUS", "1");
}

void tst_NativeMenus::defaults()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("emptyMenu.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);
    QVERIFY(contextMenuPrivate->usingNativeMenu());
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
    // and are in the appropriate places in contentData.
    auto *action1 = contextMenu->actionAt(0);
    QVERIFY(action1);
    QCOMPARE(contextMenuPrivate->contentData.at(0), action1);

    auto *action2 = contextMenu->actionAt(1);
    QVERIFY(action2);
    QCOMPARE(contextMenuPrivate->contentData.at(1), action2);

    // Check that the sub-menu can be accessed and is in the
    // appropriate place in contentData.
    auto *subMenu = contextMenu->menuAt(2);
    QVERIFY(subMenu);

    // TODO: check that sub-menus exist
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
        QCOMPARE(contextMenuPrivate->contentData.at(0), action1);
    }

    // Check that actions can be appended after existing items in the parent menu.
    QCOMPARE(contextMenu->actionAt(1), nullptr);
    QVERIFY(QMetaObject::invokeMethod(window, "addAction",
        Q_ARG(QQuickMenu *, contextMenu), Q_ARG(QString, "action2")));
    {
        auto action2 = contextMenu->actionAt(1);
        QVERIFY(action2);
        QCOMPARE(action2->text(), "action2");
        QCOMPARE(contextMenuPrivate->contentData.at(1), action2);
    }

    // Check that actions can be inserted before existing items in the parent menu.
    QVERIFY(QMetaObject::invokeMethod(window, "insertAction",
        Q_ARG(QQuickMenu *, contextMenu), Q_ARG(int, 0), Q_ARG(QString, "action0")));
    {
        auto action0 = contextMenu->actionAt(0);
        QVERIFY(action0);
        QCOMPARE(action0->text(), "action0");
        QCOMPARE(contextMenuPrivate->contentData.at(0), action0);
    }
}

void tst_NativeMenus::dynamicSubmenus()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("emptyMenu.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMenu *contextMenu = window->property("contextMenu").value<QQuickMenu*>();
    QVERIFY(contextMenu);
    auto *contextMenuPrivate = QQuickMenuPrivate::get(contextMenu);

    // Check that sub-menus (with no menu items, yet) can be appended to an empty parent menu.
    QVERIFY(QMetaObject::invokeMethod(window, "addMenu",
        Q_ARG(QQuickMenu *, contextMenu), Q_ARG(QString, "subMenu1")));
    auto subMenu1 = contextMenu->menuAt(0);
    QVERIFY(subMenu1);
    QCOMPARE(subMenu1->title(), "subMenu1");
    QCOMPARE(contextMenuPrivate->contentData.at(0), subMenu1);

    //
    QVERIFY(QMetaObject::invokeMethod(window, "addAction",
        Q_ARG(QQuickMenu *, subMenu1), Q_ARG(QString, "subMenuAction1")));

    // TODO: insert another sub-menu action before the first one
}

// TODO: add a test that mixes items with native items
// and ensure that all items are recreated as non-native

QTEST_MAIN(tst_NativeMenus)

#include "tst_nativemenus.moc"
