// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtTest>
#include <QtQml>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenubar_p.h>
#include <QtQuickTemplates2/private/qquickmenubaritem_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

class tst_qquickmenubar : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qquickmenubar();

private slots:
    void delegate();
    void mouse();
    void touch();
    void keys();
    void mnemonics();
    void altNavigation();
    void addRemove();
    void checkHighlightWhenMenuDismissed();

private:
    static bool hasWindowActivation();

    QScopedPointer<QPointingDevice> touchScreen = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
};

QPoint itemSceneCenter(const QQuickItem *item)
{
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
}

tst_qquickmenubar::tst_qquickmenubar()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
}

bool tst_qquickmenubar::hasWindowActivation()
{
    return (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation));
}

void tst_qquickmenubar::delegate()
{
    QQmlApplicationEngine engine(testFileUrl("empty.qml"));
    QScopedPointer<QQuickMenuBar> menuBar(qobject_cast<QQuickMenuBar *>(engine.rootObjects().value(0)));
    QVERIFY(menuBar);

    QQmlComponent *delegate = menuBar->delegate();
    QVERIFY(delegate);

    QScopedPointer<QQuickMenuBarItem> item(qobject_cast<QQuickMenuBarItem *>(delegate->create()));
    QVERIFY(item);
}

void tst_qquickmenubar::mouse()
{
    if (!hasWindowActivation())
        QSKIP("Window activation is not supported");

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Mouse highlight not functional on offscreen/minimal platforms");

    QQmlApplicationEngine engine(testFileUrl("menubar.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(engine.rootObjects().value(0)));
    QVERIFY(window);

    centerOnScreen(window.data());
    moveMouseAway(window.data());
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    QQuickMenuBar *menuBar = window->property("header").value<QQuickMenuBar *>();
    QVERIFY(menuBar);

    QQuickMenu *fileMenuBarMenu = menuBar->menuAt(0);
    QQuickMenu *editMenuBarMenu = menuBar->menuAt(1);
    QQuickMenu *viewMenuBarMenu = menuBar->menuAt(2);
    QQuickMenu *helpMenuBarMenu = menuBar->menuAt(3);
    QVERIFY(fileMenuBarMenu && editMenuBarMenu && viewMenuBarMenu && helpMenuBarMenu);

    QQuickMenuBarItem *fileMenuBarItem = qobject_cast<QQuickMenuBarItem *>(fileMenuBarMenu->parentItem());
    QQuickMenuBarItem *editMenuBarItem = qobject_cast<QQuickMenuBarItem *>(editMenuBarMenu->parentItem());
    QQuickMenuBarItem *viewMenuBarItem = qobject_cast<QQuickMenuBarItem *>(viewMenuBarMenu->parentItem());
    QQuickMenuBarItem *helpMenuBarItem = qobject_cast<QQuickMenuBarItem *>(helpMenuBarMenu->parentItem());
    QVERIFY(fileMenuBarItem && editMenuBarItem && viewMenuBarItem && helpMenuBarItem);

    // highlight a menubar item
    QTest::mouseMove(window.data(), itemSceneCenter(fileMenuBarItem));
#ifndef Q_OS_ANDROID
    // Android theme does not use hover effects, so moving the mouse would not
    // highlight an item
    QVERIFY(fileMenuBarItem->isHighlighted());
#endif
    QVERIFY(!fileMenuBarMenu->isVisible());

    // highlight another menubar item
    QTest::mouseMove(window.data(), itemSceneCenter(editMenuBarItem));
#ifndef Q_OS_ANDROID
    // Android theme does not use hover effects, so moving the mouse would not
    // highlight an item
    QVERIFY(!fileMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarItem->isHighlighted());
#endif
    QVERIFY(!fileMenuBarMenu->isVisible());
    QVERIFY(!editMenuBarMenu->isVisible());

    // trigger a menubar item to open a menu - it should open on press
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(editMenuBarItem));
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(editMenuBarItem));

    // re-trigger a menubar item to hide the menu - it should close on press
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(editMenuBarItem));
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarItem->hasActiveFocus());
    QTRY_VERIFY(!editMenuBarMenu->isVisible());
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(editMenuBarItem));

    // re-trigger a menubar item to show the menu again
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(editMenuBarItem));
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());

    // highlight another menubar item to open another menu
    QTest::mouseMove(window.data(), itemSceneCenter(helpMenuBarItem));
#ifdef Q_OS_ANDROID
    // Android theme does not use hover effects, so moving the mouse would not
    // highlight an item. Add a mouse click to change menubar item selection.
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier,
                      helpMenuBarItem->mapToScene(QPointF(helpMenuBarItem->width() / 2,
                                                  helpMenuBarItem->height() / 2)).toPoint());
#endif
    QVERIFY(!fileMenuBarItem->isHighlighted());
    QVERIFY(!editMenuBarItem->isHighlighted());
    QVERIFY(!viewMenuBarItem->isHighlighted());
    QVERIFY(helpMenuBarItem->isHighlighted());
    QVERIFY(!fileMenuBarMenu->isVisible());
    QVERIFY(!viewMenuBarMenu->isVisible());
    QVERIFY(helpMenuBarMenu->isVisible());
    QTRY_VERIFY(!editMenuBarMenu->isVisible());
    QTRY_VERIFY(helpMenuBarMenu->isOpened());

    // trigger a menu item to close the menu
    QQuickMenuItem *aboutMenuItem = qobject_cast<QQuickMenuItem *>(helpMenuBarMenu->itemAt(0));
    QVERIFY(aboutMenuItem);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, aboutMenuItem->mapToScene(QPointF(aboutMenuItem->width() / 2, aboutMenuItem->height() / 2)).toPoint());
    QVERIFY(!helpMenuBarItem->isHighlighted());
    QTRY_VERIFY(!helpMenuBarMenu->isVisible());

    // highlight a menubar item
    QTest::mouseMove(window.data(), itemSceneCenter(editMenuBarItem));
#ifndef Q_OS_ANDROID
    // Android theme does not use hover effects, so moving the mouse would not
    // highlight an item
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(!helpMenuBarItem->isHighlighted());
#endif
    QVERIFY(!editMenuBarMenu->isVisible());
    QVERIFY(!helpMenuBarMenu->isVisible());

    // trigger a menubar item to open a menu
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(viewMenuBarItem));
    QVERIFY(!editMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarMenu->isVisible());
    QTRY_VERIFY(viewMenuBarMenu->isOpened());

    // trigger a menu item to open a sub-menu
    QQuickMenuItem *alignmentSubMenuItem = qobject_cast<QQuickMenuItem *>(viewMenuBarMenu->itemAt(0));
    QVERIFY(alignmentSubMenuItem);
    QQuickMenu *alignmentSubMenu = alignmentSubMenuItem->subMenu();
    QVERIFY(alignmentSubMenu);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(alignmentSubMenuItem));
#if !defined(Q_OS_ANDROID) and !defined(Q_OS_WEBOS)
    // The screen on Android is too small to fit the whole hierarchy, so the
    // Alignment sub-menu is shown on top of View menu.
    // WebOS also shows alignment sub-menu on top of View menu.
    QVERIFY(viewMenuBarMenu->isVisible());
#endif
    QVERIFY(alignmentSubMenu->isVisible());
    QTRY_VERIFY(alignmentSubMenu->isOpened());

    // trigger a menu item to open a sub-sub-menu
    QQuickMenuItem *verticalSubMenuItem = qobject_cast<QQuickMenuItem *>(alignmentSubMenu->itemAt(1));
    QVERIFY(verticalSubMenuItem);
    QQuickMenu *verticalSubMenu = verticalSubMenuItem->subMenu();
    QVERIFY(verticalSubMenu);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(verticalSubMenuItem));
#if !defined(Q_OS_ANDROID) and !defined(Q_OS_WEBOS)
    // The screen on Android is too small to fit the whole hierarchy, so the
    // Vertical sub-menu is shown on top of View menu and Alignment sub-menu.
    // WebOS also shows vertical sub-menu on top of View menu and Alignment sub-menu.
    QVERIFY(viewMenuBarMenu->isVisible());
    QVERIFY(alignmentSubMenu->isVisible());
#endif
    QVERIFY(verticalSubMenu->isVisible());
    QTRY_VERIFY(verticalSubMenu->isOpened());

    // trigger a menu item to close the whole chain of menus
    QQuickMenuItem *centerMenuItem = qobject_cast<QQuickMenuItem *>(verticalSubMenu->itemAt(1));
    QVERIFY(centerMenuItem);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(centerMenuItem));
    QVERIFY(!viewMenuBarItem->isHighlighted());
    QTRY_VERIFY(!viewMenuBarMenu->isVisible());
    QTRY_VERIFY(!alignmentSubMenu->isVisible());
    QTRY_VERIFY(!verticalSubMenu->isVisible());

    // re-highlight the same menubar item
#ifndef Q_OS_ANDROID
    // Android theme does not use hover effects, so moving the mouse would not
    // highlight an item
    QTest::mouseMove(window.data(), viewMenuBarItem->mapToScene(QPointF(viewMenuBarItem->width() / 2, viewMenuBarItem->height() / 2)).toPoint());
    QVERIFY(viewMenuBarItem->isHighlighted());
#endif

    // re-open the chain of menus
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(viewMenuBarItem));
    QTRY_VERIFY(viewMenuBarMenu->isOpened());
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(alignmentSubMenuItem));
    QTRY_VERIFY(alignmentSubMenu->isOpened());
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, itemSceneCenter(verticalSubMenuItem));
    QTRY_VERIFY(verticalSubMenu->isOpened());

    // click outside to close the whole chain of menus
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 1, window->height() - 1));
    QVERIFY(!viewMenuBarItem->isHighlighted());
    QTRY_VERIFY(!viewMenuBarMenu->isVisible());
    QTRY_VERIFY(!alignmentSubMenu->isVisible());
    QTRY_VERIFY(!verticalSubMenu->isVisible());
}

// Not sure how relevant MenuBar is for touch, but this test is here to make
// sure that only release events cause the menu to open, as:
// - That is how it has always behaved, so maintain that behavior.
// - It's what happens with e.g. overflow menus on Android.
void tst_qquickmenubar::touch()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("touch.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    centerOnScreen(helper.window);
    helper.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(helper.window));

    QQuickMenuBar *menuBar = helper.window->property("header").value<QQuickMenuBar *>();
    QVERIFY(menuBar);

    QQuickMenu *fileMenuBarMenu = menuBar->menuAt(0);
    QVERIFY(fileMenuBarMenu);

    QQuickMenuBarItem *fileMenuBarItem = qobject_cast<QQuickMenuBarItem *>(fileMenuBarMenu->parentItem());
    QVERIFY(fileMenuBarItem);

    // Trigger a menubar item to open a menu - it should only open on release.
    QTest::touchEvent(helper.window, touchScreen.data()).press(0, itemSceneCenter(fileMenuBarItem));
    QVERIFY(!fileMenuBarItem->isHighlighted());
    QVERIFY(!fileMenuBarMenu->isVisible());
    QTest::touchEvent(helper.window, touchScreen.data()).release(0, itemSceneCenter(fileMenuBarItem));
    QVERIFY(fileMenuBarItem->isHighlighted());
    QVERIFY(fileMenuBarMenu->isVisible());
    QTRY_VERIFY(fileMenuBarMenu->isOpened());
}

void tst_qquickmenubar::keys()
{
    if (!hasWindowActivation())
        QSKIP("Window activation is not supported");

    QQmlApplicationEngine engine(testFileUrl("menubar.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(engine.rootObjects().value(0)));
    QVERIFY(window);

    centerOnScreen(window.data());
    moveMouseAway(window.data());
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    QQuickMenuBar *menuBar = window->property("header").value<QQuickMenuBar *>();
    QVERIFY(menuBar);

    QQuickMenu *fileMenuBarMenu = menuBar->menuAt(0);
    QQuickMenu *editMenuBarMenu = menuBar->menuAt(1);
    QQuickMenu *viewMenuBarMenu = menuBar->menuAt(2);
    QQuickMenu *helpMenuBarMenu = menuBar->menuAt(3);
    QVERIFY(fileMenuBarMenu && editMenuBarMenu && viewMenuBarMenu && helpMenuBarMenu);

    QQuickMenuBarItem *fileMenuBarItem = qobject_cast<QQuickMenuBarItem *>(fileMenuBarMenu->parentItem());
    QQuickMenuBarItem *editMenuBarItem = qobject_cast<QQuickMenuBarItem *>(editMenuBarMenu->parentItem());
    QQuickMenuBarItem *viewMenuBarItem = qobject_cast<QQuickMenuBarItem *>(viewMenuBarMenu->parentItem());
    QQuickMenuBarItem *helpMenuBarItem = qobject_cast<QQuickMenuBarItem *>(helpMenuBarMenu->parentItem());
    QVERIFY(fileMenuBarItem && editMenuBarItem && viewMenuBarItem && helpMenuBarItem);

    // trigger a menubar item to open a menu
    editMenuBarItem->forceActiveFocus();
    QTest::keyClick(window.data(), Qt::Key_Space);
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());
    QVERIFY(editMenuBarMenu->hasActiveFocus());

    // navigate down to the menu
    QQuickMenuItem *cutMenuItem = qobject_cast<QQuickMenuItem *>(editMenuBarMenu->itemAt(0));
    QVERIFY(cutMenuItem);
    QVERIFY(!cutMenuItem->isHighlighted());
    QVERIFY(!cutMenuItem->hasActiveFocus());
    QTest::keyClick(window.data(), Qt::Key_Down);
    QVERIFY(cutMenuItem->isHighlighted());
    QVERIFY(cutMenuItem->hasActiveFocus());

    // navigate up, back to the menubar
    QTest::keyClick(window.data(), Qt::Key_Up);
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarItem->hasActiveFocus());
    QTRY_VERIFY(!editMenuBarMenu->isVisible());

// There seem to be problems in focus handling in webOS QPA, see https://bugreports.qt.io/browse/WEBOSCI-45
#ifdef Q_OS_WEBOS
    QEXPECT_FAIL("", "WEBOSCI-45", Abort);
#endif
    QVERIFY(!cutMenuItem->isHighlighted());
    QVERIFY(!cutMenuItem->hasActiveFocus());

    // navigate down to re-open the menu
    QTest::keyClick(window.data(), Qt::Key_Down);
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(!editMenuBarItem->hasActiveFocus());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());
    QVERIFY(editMenuBarMenu->hasActiveFocus());
    QVERIFY(cutMenuItem->isHighlighted());
    QVERIFY(cutMenuItem->hasActiveFocus());

    // navigate left in popup mode (menu open)
    QTest::keyClick(window.data(), Qt::Key_Left);
    QVERIFY(fileMenuBarItem->isHighlighted());
    QVERIFY(!editMenuBarItem->isHighlighted());
    QVERIFY(fileMenuBarMenu->isVisible());
    QTRY_VERIFY(fileMenuBarMenu->isOpened());
    QTRY_VERIFY(!editMenuBarMenu->isVisible());

    // navigate left in popup mode (wrap)
    QTest::keyClick(window.data(), Qt::Key_Left);
    QVERIFY(helpMenuBarItem->isHighlighted());
    QVERIFY(!fileMenuBarItem->isHighlighted());
    QVERIFY(helpMenuBarMenu->isVisible());
    QTRY_VERIFY(helpMenuBarMenu->isOpened());
    QTRY_VERIFY(!fileMenuBarMenu->isVisible());

    // navigate up to close the menu
    QTest::keyClick(window.data(), Qt::Key_Up);
    QVERIFY(helpMenuBarItem->isHighlighted());
    QTRY_VERIFY(!helpMenuBarMenu->isVisible());

    // navigate right in non-popup mode (wrap)
    QTest::keyClick(window.data(), Qt::Key_Right);
    QVERIFY(fileMenuBarItem->isHighlighted());
    QVERIFY(!helpMenuBarItem->isHighlighted());
    QVERIFY(!fileMenuBarMenu->isVisible());
    QVERIFY(!helpMenuBarMenu->isVisible());

    // navigate right in non-popup mode (menu closed)
    QTest::keyClick(window.data(), Qt::Key_Right);
    QVERIFY(!fileMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(!fileMenuBarMenu->isVisible());
    QVERIFY(!editMenuBarMenu->isVisible());

    // open a menu
    viewMenuBarItem->forceActiveFocus();
    QTest::keyClick(window.data(), Qt::Key_Space);
    QVERIFY(viewMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarMenu->isVisible());
    QTRY_VERIFY(viewMenuBarMenu->isOpened());
    QVERIFY(!viewMenuBarItem->hasActiveFocus());
    QVERIFY(viewMenuBarMenu->hasActiveFocus());

    // open a sub-menu
    QQuickMenuItem *alignmentSubMenuItem = qobject_cast<QQuickMenuItem *>(viewMenuBarMenu->itemAt(0));
    QVERIFY(alignmentSubMenuItem);
    QQuickMenu *alignmentSubMenu = alignmentSubMenuItem->subMenu();
    QVERIFY(alignmentSubMenu);
    QTest::keyClick(window.data(), Qt::Key_Down);
    QVERIFY(alignmentSubMenuItem->isHighlighted());
    QVERIFY(!alignmentSubMenu->isVisible());
    QTest::keyClick(window.data(), Qt::Key_Right);
    QVERIFY(alignmentSubMenu->isVisible());
    QTRY_VERIFY(alignmentSubMenu->isOpened());

    // open a sub-sub-menu
    QQuickMenuItem *horizontalSubMenuItem = qobject_cast<QQuickMenuItem *>(alignmentSubMenu->itemAt(0));
    QVERIFY(horizontalSubMenuItem);
    QVERIFY(horizontalSubMenuItem->isHighlighted());
    QQuickMenu *horizontalSubMenu = horizontalSubMenuItem->subMenu();
    QVERIFY(horizontalSubMenu);
    QTest::keyClick(window.data(), Qt::Key_Right);
    QVERIFY(viewMenuBarMenu->isVisible());
    QVERIFY(alignmentSubMenu->isVisible());
    QVERIFY(horizontalSubMenu->isVisible());
    QTRY_VERIFY(horizontalSubMenu->isOpened());

    // navigate left to close a sub-menu
    QTest::keyClick(window.data(), Qt::Key_Left);
    QTRY_VERIFY(!horizontalSubMenu->isVisible());
    QVERIFY(viewMenuBarMenu->isVisible());
    QVERIFY(alignmentSubMenu->isVisible());

    // navigate right to re-open the sub-menu
    QTest::keyClick(window.data(), Qt::Key_Right);
    QVERIFY(horizontalSubMenuItem->isHighlighted());
    QVERIFY(horizontalSubMenu->isVisible());
    QTRY_VERIFY(horizontalSubMenu->isOpened());

    // navigate right to the next menubar menu
    QTest::keyClick(window.data(), Qt::Key_Right);
    QVERIFY(!viewMenuBarItem->isHighlighted());
    QVERIFY(helpMenuBarItem->isHighlighted());
    QVERIFY(helpMenuBarMenu->isVisible());
    QTRY_VERIFY(!viewMenuBarMenu->isVisible());
    QTRY_VERIFY(!alignmentSubMenu->isVisible());
    QTRY_VERIFY(!horizontalSubMenu->isVisible());
    QTRY_VERIFY(helpMenuBarMenu->isOpened());

    // navigate back
    QTest::keyClick(window.data(), Qt::Key_Left);
    QVERIFY(!helpMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarMenu->isVisible());
    QTRY_VERIFY(!helpMenuBarMenu->isVisible());
    QTRY_VERIFY(viewMenuBarMenu->isOpened());

    // re-open the chain of menus
    QTest::keyClick(window.data(), Qt::Key_Down);
    QVERIFY(alignmentSubMenuItem->isHighlighted());
    QTest::keyClick(window.data(), Qt::Key_Right);
    QTRY_VERIFY(alignmentSubMenu->isOpened());
    QTest::keyClick(window.data(), Qt::Key_Right);
    QTRY_VERIFY(horizontalSubMenu->isOpened());

    // repeat escape to close the whole chain of menus one by one
    QTest::keyClick(window.data(), Qt::Key_Escape);
    QTRY_VERIFY(!horizontalSubMenu->isVisible());
    QVERIFY(viewMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarMenu->isVisible());
    QVERIFY(alignmentSubMenu->isVisible());

    QTest::keyClick(window.data(), Qt::Key_Escape);
    QTRY_VERIFY(!alignmentSubMenu->isVisible());
    QVERIFY(viewMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarMenu->isVisible());

    QTest::keyClick(window.data(), Qt::Key_Escape);
    QVERIFY(!viewMenuBarItem->isHighlighted());
    QTRY_VERIFY(!viewMenuBarMenu->isVisible());
}

void tst_qquickmenubar::mnemonics()
{
    if (!hasWindowActivation())
        QSKIP("Window activation is not supported");

#if defined(Q_OS_MACOS) or defined(Q_OS_WEBOS)
    QSKIP("Mnemonics are not used on this platform");
#endif

    QQmlApplicationEngine engine(testFileUrl("menubar.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(engine.rootObjects().value(0)));
    QVERIFY(window);

    centerOnScreen(window.data());
    moveMouseAway(window.data());
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    MnemonicKeySimulator keySim(window.data());

    QQuickMenuBar *menuBar = window->property("header").value<QQuickMenuBar *>();
    QVERIFY(menuBar);

    QQuickMenu *fileMenuBarMenu = menuBar->menuAt(0);
    QQuickMenu *editMenuBarMenu = menuBar->menuAt(1);
    QQuickMenu *viewMenuBarMenu = menuBar->menuAt(2);
    QQuickMenu *helpMenuBarMenu = menuBar->menuAt(3);
    QVERIFY(fileMenuBarMenu && editMenuBarMenu && viewMenuBarMenu && helpMenuBarMenu);

    QQuickMenuBarItem *fileMenuBarItem = qobject_cast<QQuickMenuBarItem *>(fileMenuBarMenu->parentItem());
    QQuickMenuBarItem *editMenuBarItem = qobject_cast<QQuickMenuBarItem *>(editMenuBarMenu->parentItem());
    QQuickMenuBarItem *viewMenuBarItem = qobject_cast<QQuickMenuBarItem *>(viewMenuBarMenu->parentItem());
    QQuickMenuBarItem *helpMenuBarItem = qobject_cast<QQuickMenuBarItem *>(helpMenuBarMenu->parentItem());
    QVERIFY(fileMenuBarItem && editMenuBarItem && viewMenuBarItem && helpMenuBarItem);

    QQuickButton *oopsButton = window->property("oopsButton").value<QQuickButton *>();
    QVERIFY(oopsButton);
    QSignalSpy oopsButtonSpy(oopsButton, &QQuickButton::clicked);
    QVERIFY(oopsButtonSpy.isValid());

    // trigger a menubar item to open a menu
    keySim.press(Qt::Key_Alt);
    keySim.click(Qt::Key_E); // "&Edit"
    keySim.release(Qt::Key_Alt);
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(!editMenuBarItem->hasActiveFocus());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());
    QVERIFY(editMenuBarMenu->hasActiveFocus());

    // press Alt to hide the menu
    keySim.click(Qt::Key_Alt);
    QVERIFY(!editMenuBarItem->isHighlighted());
    QVERIFY(!editMenuBarItem->hasActiveFocus());
    QVERIFY(!editMenuBarMenu->hasActiveFocus());
    QTRY_VERIFY(!editMenuBarMenu->isVisible());

    // re-trigger a menubar item to show the menu again
    keySim.press(Qt::Key_Alt);
    keySim.click(Qt::Key_E); // "&Edit"
    keySim.release(Qt::Key_Alt);
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());
    QVERIFY(editMenuBarMenu->hasActiveFocus());
    QVERIFY(!editMenuBarItem->hasActiveFocus());

    // trigger another menubar item to open another menu, leave Alt pressed
    keySim.press(Qt::Key_Alt);
    QTRY_VERIFY(!editMenuBarMenu->isVisible());
    keySim.click(Qt::Key_H); // "&Help"
    QVERIFY(!editMenuBarItem->isHighlighted());
    QVERIFY(helpMenuBarItem->isHighlighted());
    QVERIFY(!viewMenuBarMenu->isVisible());
    QVERIFY(helpMenuBarMenu->isVisible());
    QTRY_VERIFY(helpMenuBarMenu->isOpened());

    // trigger a menu item to close the menu
    keySim.click(Qt::Key_A); // "&About"
    keySim.release(Qt::Key_Alt);
    QVERIFY(!helpMenuBarItem->isHighlighted());
    QTRY_VERIFY(!helpMenuBarMenu->isVisible());

    // trigger a menubar item to open a menu, leave Alt pressed
    keySim.press(Qt::Key_Alt);
    keySim.click(Qt::Key_V); // "&View"
    QVERIFY(!editMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarItem->isHighlighted());
    QVERIFY(viewMenuBarMenu->isVisible());
    QTRY_VERIFY(viewMenuBarMenu->isOpened());

    // trigger a menu item to open a sub-menu, leave Alt pressed
    QQuickMenuItem *alignmentSubMenuItem = qobject_cast<QQuickMenuItem *>(viewMenuBarMenu->itemAt(0));
    QVERIFY(alignmentSubMenuItem);
    QQuickMenu *alignmentSubMenu = alignmentSubMenuItem->subMenu();
    QVERIFY(alignmentSubMenu);
    keySim.click(Qt::Key_A); // "&Alignment"
#ifndef Q_OS_ANDROID
    // On Android sub-menus are not cascading, so the Alignment sub-menu is
    // shown instead of View menu.
    QVERIFY(viewMenuBarMenu->isVisible());
#endif
    QVERIFY(alignmentSubMenu->isVisible());
    QTRY_VERIFY(alignmentSubMenu->isOpened());

    // trigger a menu item to open a sub-sub-menu, leave Alt pressed
    QQuickMenuItem *verticalSubMenuItem = qobject_cast<QQuickMenuItem *>(alignmentSubMenu->itemAt(1));
    QVERIFY(verticalSubMenuItem);
    QQuickMenu *verticalSubMenu = verticalSubMenuItem->subMenu();
    QVERIFY(verticalSubMenu);
    keySim.click(Qt::Key_V); // "&Vertical"
#ifndef Q_OS_ANDROID
    // On Android sub-menus are not cascading, so the Vertical sub-menu is
    // shown instead of View menu and Alignment sub-menu.
    QVERIFY(viewMenuBarMenu->isVisible());
    QVERIFY(alignmentSubMenu->isVisible());
#endif
    QVERIFY(verticalSubMenu->isVisible());
    QTRY_VERIFY(verticalSubMenu->isOpened());

    // trigger a menu item to close the whole chain of menus
    keySim.click(Qt::Key_C); // "&Center"
    keySim.release(Qt::Key_Alt);
    QVERIFY(!viewMenuBarItem->isHighlighted());
    QTRY_VERIFY(!viewMenuBarMenu->isVisible());
    QTRY_VERIFY(!alignmentSubMenu->isVisible());
    QTRY_VERIFY(!verticalSubMenu->isVisible());

    // trigger a menubar item to open a menu, leave Alt pressed
    keySim.press(Qt::Key_Alt);
    keySim.click(Qt::Key_F); // "&File"
    QVERIFY(fileMenuBarItem->isHighlighted());
    QVERIFY(fileMenuBarMenu->isVisible());
    QTRY_VERIFY(fileMenuBarMenu->isOpened());
    QVERIFY(fileMenuBarMenu->hasActiveFocus());

    // trigger a menu item to close the menu, which shouldn't trigger a button
    // action behind the menu (QTBUG-86276)
    QCOMPARE(oopsButtonSpy.size(), 0);
    keySim.click(Qt::Key_O); // "&Open..."
    keySim.release(Qt::Key_Alt);
    QVERIFY(!fileMenuBarItem->isHighlighted());
    QVERIFY(!fileMenuBarMenu->isOpened());
    QTRY_VERIFY(!fileMenuBarMenu->isVisible());
    QCOMPARE(oopsButtonSpy.size(), 0);

    // trigger a button action while menu is closed
    keySim.press(Qt::Key_Alt);
    keySim.click(Qt::Key_O); // "&Oops"
    keySim.release(Qt::Key_Alt);
    QCOMPARE(oopsButtonSpy.size(), 1);
}

void tst_qquickmenubar::altNavigation()
{
    if (!QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::MenuBarFocusOnAltPressRelease).toBool())
        QSKIP("Menu doesn't get focus via Alt press&release on this platform");

    QQmlApplicationEngine engine(testFileUrl("menubar.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(engine.rootObjects().value(0)));
    QVERIFY(window);

    centerOnScreen(window.data());
    moveMouseAway(window.data());
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    QQuickMenuBar *menuBar = window->property("header").value<QQuickMenuBar *>();
    QVERIFY(menuBar);

    QQuickMenu *fileMenuBarMenu = menuBar->menuAt(0);
    QQuickMenuBarItem *fileMenuBarItem = qobject_cast<QQuickMenuBarItem *>(fileMenuBarMenu->parentItem());

    // This logic is somewhat inverted, but QKeyEvent::modifiers() XOR's the modifier
    // corresponding to the activated key, so pressing Alt adds the AltModifier, and
    // releasing Alt with AltModifier removes the AltModifier.
    QTest::keyPress(window.get(), Qt::Key_Alt);
    QTest::keyRelease(window.get(), Qt::Key_Alt, Qt::AltModifier);
    QVERIFY(menuBar->hasActiveFocus());
    QVERIFY(fileMenuBarItem->isHighlighted());

    // if menu has focus, pressing the mnemonic without Alt should open the menu
    QQuickMenu *editMenuBarMenu = menuBar->menuAt(1);
    QQuickMenuBarItem *editMenuBarItem = qobject_cast<QQuickMenuBarItem *>(editMenuBarMenu->parentItem());

    QTest::keyPress(window.get(), Qt::Key_E);
    QVERIFY(editMenuBarItem->isHighlighted());
    QVERIFY(editMenuBarMenu->isVisible());
    QTRY_VERIFY(editMenuBarMenu->isOpened());
    QVERIFY(editMenuBarMenu->hasActiveFocus());
}

void tst_qquickmenubar::addRemove()
{
    QQmlApplicationEngine engine(testFileUrl("empty.qml"));

    QScopedPointer<QQuickMenuBar> menuBar(qobject_cast<QQuickMenuBar *>(engine.rootObjects().value(0)));
    QVERIFY(menuBar);

    QQmlComponent component(&engine);
    component.setData("import QtQuick.Controls; Menu { }", QUrl());

    QPointer<QQuickMenu> menu1(qobject_cast<QQuickMenu *>(component.create()));
    QVERIFY(!menu1.isNull());
    menuBar->addMenu(menu1.data());
    QCOMPARE(menuBar->count(), 1);
    QCOMPARE(menuBar->menuAt(0), menu1.data());

    QPointer<QQuickMenuBarItem> menuBarItem1(qobject_cast<QQuickMenuBarItem *>(menuBar->itemAt(0)));
    QVERIFY(menuBarItem1);
    QCOMPARE(menuBarItem1->menu(), menu1.data());
    QCOMPARE(menuBar->itemAt(0), menuBarItem1.data());

    QScopedPointer<QQuickMenu> menu2(qobject_cast<QQuickMenu *>(component.create()));
    QVERIFY(!menu2.isNull());
    menuBar->insertMenu(0, menu2.data());
    QCOMPARE(menuBar->count(), 2);
    QCOMPARE(menuBar->menuAt(0), menu2.data());
    QCOMPARE(menuBar->menuAt(1), menu1.data());

    QPointer<QQuickMenuBarItem> menuBarItem2(qobject_cast<QQuickMenuBarItem *>(menuBar->itemAt(0)));
    QVERIFY(menuBarItem2);
    QCOMPARE(menuBarItem2->menu(), menu2.data());
    QCOMPARE(menuBar->itemAt(0), menuBarItem2.data());
    QCOMPARE(menuBar->itemAt(1), menuBarItem1.data());

    // takeMenu(int) does not destroy the menu, but does destroy the respective item in the menubar
    QCOMPARE(menuBar->takeMenu(1), menu1.data());
    QCOMPARE(menuBar->count(), 1);
    QVERIFY(!menuBar->menuAt(1));
    QVERIFY(!menuBar->itemAt(1));
    QCoreApplication::sendPostedEvents(menu1.data(), QEvent::DeferredDelete);
    QVERIFY(!menu1.isNull());
    QCoreApplication::sendPostedEvents(menuBarItem1, QEvent::DeferredDelete);
    QVERIFY(menuBarItem1.isNull());

    // addMenu(Menu) re-creates the respective item in the menubar
    menuBar->addMenu(menu1.data());
    QCOMPARE(menuBar->count(), 2);
    menuBarItem1 = qobject_cast<QQuickMenuBarItem *>(menuBar->itemAt(1));
    QVERIFY(!menuBarItem1.isNull());

    // removeMenu(Menu) destroys both the menu and the respective item in the menubar
    menuBar->removeMenu(menu1.data());
    QCOMPARE(menuBar->count(), 1);
    QVERIFY(!menuBar->itemAt(1));
    QCoreApplication::sendPostedEvents(menu1.data(), QEvent::DeferredDelete);
    QVERIFY(menu1.isNull());
    QCoreApplication::sendPostedEvents(menuBarItem1, QEvent::DeferredDelete);
    QVERIFY(menuBarItem1.isNull());
}

void tst_qquickmenubar::checkHighlightWhenMenuDismissed()
{
    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Mouse highlight not functional on offscreen/minimal platforms");

    QQmlApplicationEngine engine(testFileUrl("checkHighlightWhenDismissed.qml"));
    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(engine.rootObjects().value(0)));
    QVERIFY(window);

    centerOnScreen(window.data());
    moveMouseAway(window.data());
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickMenuBar *menuBar = window->findChild<QQuickMenuBar *>("menuBar");
    QVERIFY(menuBar);

    QQuickMenu *staticMenu = menuBar->menuAt(0);
    QQuickMenu *dynamicMenu = menuBar->menuAt(1);
    QVERIFY(staticMenu && dynamicMenu);
    QQuickMenuBarItem *staticMenuBarItem = qobject_cast<QQuickMenuBarItem *>(staticMenu->parentItem());
    QQuickMenuBarItem *dynamicMenuBarItem = qobject_cast<QQuickMenuBarItem *>(dynamicMenu->parentItem());
    QVERIFY(staticMenuBarItem && dynamicMenuBarItem);

    // highlight the static MenuBarItem and open the menu
    QTest::mouseMove(window.data(), staticMenuBarItem->mapToScene(
        QPointF(staticMenuBarItem->width() / 2, staticMenuBarItem->height() / 2)).toPoint());
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier,
        staticMenuBarItem->mapToScene(QPointF(staticMenuBarItem->width() / 2, staticMenuBarItem->height() / 2)).toPoint());
    QVERIFY(staticMenuBarItem->isHighlighted());
    QVERIFY(staticMenu->isVisible());
    QTRY_VERIFY(staticMenu->isOpened());
    // click a menu item to dismiss the menu and unhighlight the static MenuBarItem
    QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(staticMenu->itemAt(0));
    QVERIFY(menuItem);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier,
        menuItem->mapToScene(QPointF(menuItem->width() / 2, menuItem->height() / 2)).toPoint());
    QVERIFY(!staticMenuBarItem->isHighlighted());
    // wait for the menu to be fully gone so that it doesn't interfere with the next test
    QTRY_VERIFY(!staticMenu->isVisible());

    // highlight the dynamic MenuBarItem and open the menu
    QTest::mouseMove(window.data(), dynamicMenuBarItem->mapToScene(
        QPointF(dynamicMenuBarItem->width() / 2, dynamicMenuBarItem->height() / 2)).toPoint());
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier,
        dynamicMenuBarItem->mapToScene(QPointF(dynamicMenuBarItem->width() / 2, dynamicMenuBarItem->height() / 2)).toPoint());
    QVERIFY(dynamicMenuBarItem->isHighlighted());
    QVERIFY(dynamicMenu->isVisible());
    QTRY_VERIFY(dynamicMenu->isOpened());

    // click a menu item to dismiss the menu and unhighlight the dynamic MenuBarItem
    menuItem = qobject_cast<QQuickMenuItem *>(dynamicMenu->itemAt(0));
    QVERIFY(menuItem);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier,
        menuItem->mapToScene(QPointF(menuItem->width() / 2, menuItem->height() / 2)).toPoint());
    QVERIFY(!dynamicMenuBarItem->isHighlighted());
}

QTEST_QUICKCONTROLS_MAIN(tst_qquickmenubar)

#include "tst_qquickmenubar.moc"
