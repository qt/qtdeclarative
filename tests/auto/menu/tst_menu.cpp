/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtGui/qcursor.h>
#include <QtGui/qstylehints.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickmenuseparator_p.h>

using namespace QQuickVisualTestUtil;

class tst_menu : public QQmlDataTest
{
    Q_OBJECT

public:

private slots:
    void defaults();
    void mouse();
    void contextMenuKeyboard();
    void menuButton();
    void addItem();
    void menuSeparator();
    void repeater();
    void order();
    void popup();
    void actions();
    void removeTakeItem();
};

void tst_menu::defaults()
{
    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickMenu *emptyMenu = helper.appWindow->property("emptyMenu").value<QQuickMenu*>();
    QCOMPARE(emptyMenu->isVisible(), false);
    QCOMPARE(emptyMenu->contentItem()->property("currentIndex"), QVariant(-1));
}

void tst_menu::mouse()
{
    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    menu->open();
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    QQuickItem *firstItem = menu->itemAt(0);
    QSignalSpy clickedSpy(firstItem, SIGNAL(clicked()));
    QSignalSpy triggeredSpy(firstItem, SIGNAL(triggered()));
    QSignalSpy visibleSpy(menu, SIGNAL(visibleChanged()));

    // Ensure that presses cause the current index to change,
    // so that the highlight acts as a way of illustrating press state.
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(firstItem->width() / 2, firstItem->height() / 2));
    QVERIFY(firstItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(0));
    QVERIFY(menu->isVisible());

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(firstItem->width() / 2, firstItem->height() / 2));
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(triggeredSpy.count(), 1);
    QCOMPARE(visibleSpy.count(), 1);
    QVERIFY(!menu->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()));
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    menu->open();
    QCOMPARE(visibleSpy.count(), 2);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    // Ensure that we have enough space to click outside of the menu.
    QVERIFY(window->width() > menu->contentItem()->width());
    QVERIFY(window->height() > menu->contentItem()->height());
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        QPoint(menu->contentItem()->width() + 1, menu->contentItem()->height() + 1));
    QCOMPARE(visibleSpy.count(), 3);
    QVERIFY(!menu->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    menu->open();
    QCOMPARE(visibleSpy.count(), 4);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    // Try pressing within the menu and releasing outside of it; it should close.
    // TODO: won't work until QQuickPopup::releasedOutside() actually gets emitted
//    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(firstItem->width() / 2, firstItem->height() / 2));
//    QVERIFY(firstItem->hasActiveFocus());
//    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(0));
//    QVERIFY(menu->isVisible());
//    QCOMPARE(triggeredSpy.count(), 1);

//    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(menu->contentItem()->width() + 1, firstItem->height() / 2));
//    QCOMPARE(clickedSpy.count(), 1);
//    QCOMPARE(triggeredSpy.count(), 1);
//    QCOMPARE(visibleSpy.count(), 5);
//    QVERIFY(!menu->isVisible());
//    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()));
//    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));
}

void tst_menu::contextMenuKeyboard()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("This platform only allows tab focus for text controls");

    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    QQuickItem *firstItem = menu->itemAt(0);
    QSignalSpy visibleSpy(menu, SIGNAL(visibleChanged()));

    menu->setFocus(true);
    menu->open();
    QCOMPARE(visibleSpy.count(), 1);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));
    QVERIFY(!firstItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(firstItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(0));

    QQuickItem *secondItem = menu->itemAt(1);
    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(secondItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(1));

    QSignalSpy secondTriggeredSpy(secondItem, SIGNAL(triggered()));
    QTest::keyClick(window, Qt::Key_Space);
    QCOMPARE(secondTriggeredSpy.count(), 1);
    QCOMPARE(visibleSpy.count(), 2);
    QVERIFY(!menu->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()));
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!secondItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    menu->open();
    QCOMPARE(visibleSpy.count(), 3);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!secondItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(firstItem->hasActiveFocus());

    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(secondItem->hasActiveFocus());

    QTest::keyClick(window, Qt::Key_Down);
    QQuickItem *thirdItem = menu->itemAt(2);
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!secondItem->hasActiveFocus());
    QVERIFY(thirdItem->hasActiveFocus());

    // Key navigation shouldn't wrap by default.
    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!secondItem->hasActiveFocus());
    QVERIFY(thirdItem->hasActiveFocus());

    QTest::keyClick(window, Qt::Key_Escape);
    QCOMPARE(visibleSpy.count(), 4);
    QVERIFY(!menu->isVisible());
}

void tst_menu::menuButton()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("This platform only allows tab focus for text controls");

    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QQuickButton *menuButton = window->property("menuButton").value<QQuickButton*>();
    QSignalSpy visibleSpy(menu, SIGNAL(visibleChanged()));

    menuButton->setVisible(true);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        menuButton->mapToScene(QPointF(menuButton->width() / 2, menuButton->height() / 2)).toPoint());
    QCOMPARE(visibleSpy.count(), 1);
    QVERIFY(menu->isVisible());

    QTest::keyClick(window, Qt::Key_Tab);
    QQuickItem *firstItem = menu->itemAt(0);
    QVERIFY(firstItem->hasActiveFocus());
}

void tst_menu::addItem()
{
    QQuickApplicationHelper helper(this, QLatin1String("addItem.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    QQuickItem *menuItem = menu->itemAt(0);
    QVERIFY(menuItem);
    QTRY_VERIFY(!QQuickItemPrivate::get(menuItem)->culled); // QTBUG-53262

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        menuItem->mapToScene(QPointF(menuItem->width() / 2, menuItem->height() / 2)).toPoint());
    QTRY_VERIFY(!menu->isVisible());
}

void tst_menu::menuSeparator()
{
    QQuickApplicationHelper helper(this, QLatin1String("menuSeparator.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    QQuickMenuItem *newMenuItem = qobject_cast<QQuickMenuItem*>(menu->itemAt(0));
    QVERIFY(newMenuItem);
    QCOMPARE(newMenuItem->text(), QStringLiteral("New"));

    QQuickMenuSeparator *menuSeparator = qobject_cast<QQuickMenuSeparator*>(menu->itemAt(1));
    QVERIFY(menuSeparator);

    QQuickMenuItem *saveMenuItem = qobject_cast<QQuickMenuItem*>(menu->itemAt(2));
    QVERIFY(saveMenuItem);
    QCOMPARE(saveMenuItem->text(), QStringLiteral("Save"));
    QTRY_VERIFY(!QQuickItemPrivate::get(saveMenuItem)->culled); // QTBUG-53262

    // Clicking on items should still close the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        newMenuItem->mapToScene(QPointF(newMenuItem->width() / 2, newMenuItem->height() / 2)).toPoint());
    QTRY_VERIFY(!menu->isVisible());

    menu->open();
    QVERIFY(menu->isVisible());

    // Clicking on a separator shouldn't close the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        menuSeparator->mapToScene(QPointF(menuSeparator->width() / 2, menuSeparator->height() / 2)).toPoint());
    QVERIFY(menu->isVisible());

    // Clicking on items should still close the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        saveMenuItem->mapToScene(QPointF(saveMenuItem->width() / 2, saveMenuItem->height() / 2)).toPoint());
    QTRY_VERIFY(!menu->isVisible());
}

void tst_menu::repeater()
{
    QQuickApplicationHelper helper(this, QLatin1String("repeater.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    QObject *repeater = window->property("repeater").value<QObject*>();
    QVERIFY(repeater);

    int count = repeater->property("count").toInt();
    QCOMPARE(count, 5);

    for (int i = 0; i < count; ++i) {
        QQuickItem *item = menu->itemAt(i);
        QVERIFY(item);
        QCOMPARE(item->property("idx").toInt(), i);

        QQuickItem *repeaterItem = nullptr;
        QVERIFY(QMetaObject::invokeMethod(repeater, "itemAt", Q_RETURN_ARG(QQuickItem*, repeaterItem), Q_ARG(int, i)));
        QCOMPARE(item, repeaterItem);
    }

    repeater->setProperty("model", 3);

    count = repeater->property("count").toInt();
    QCOMPARE(count, 3);

    for (int i = 0; i < count; ++i) {
        QQuickItem *item = menu->itemAt(i);
        QVERIFY(item);
        QCOMPARE(item->property("idx").toInt(), i);

        QQuickItem *repeaterItem = nullptr;
        QVERIFY(QMetaObject::invokeMethod(repeater, "itemAt", Q_RETURN_ARG(QQuickItem*, repeaterItem), Q_ARG(int, i)));
        QCOMPARE(item, repeaterItem);
    }
}

void tst_menu::order()
{
    QQuickApplicationHelper helper(this, QLatin1String("order.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    const QStringList texts = {"dynamic_0", "static_1", "repeated_2", "repeated_3", "static_4", "dynamic_5", "dynamic_6"};

    for (int i = 0; i < texts.count(); ++i) {
        QQuickItem *item = menu->itemAt(i);
        QVERIFY(item);
        QCOMPARE(item->property("text").toString(), texts.at(i));
    }
}

void tst_menu::popup()
{
    QQuickApplicationHelper helper(this, QLatin1String("popup.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu *>();
    QVERIFY(menu);

    QQuickMenuItem *menuItem1 = window->property("menuItem1").value<QQuickMenuItem *>();
    QVERIFY(menuItem1);

    QQuickMenuItem *menuItem2 = window->property("menuItem2").value<QQuickMenuItem *>();
    QVERIFY(menuItem2);

    QQuickMenuItem *menuItem3 = window->property("menuItem3").value<QQuickMenuItem *>();
    QVERIFY(menuItem3);

#if QT_CONFIG(cursor)
    QPoint cursorPos = window->mapToGlobal(QPoint(11, 22));
    QCursor::setPos(cursorPos);
    QTRY_COMPARE(QCursor::pos(), cursorPos);

    QVERIFY(QMetaObject::invokeMethod(window, "popupAtCursor"));
    QTRY_COMPARE(menu->property("x").toInt(), 11);
    QTRY_COMPARE(menu->property("y").toInt(), 22);
    menu->close();

    QVERIFY(QMetaObject::invokeMethod(window, "popupAtPos", Q_ARG(QVariant, QPointF(33, 44))));
    QTRY_COMPARE(menu->property("x").toInt(), 33);
    QTRY_COMPARE(menu->property("y").toInt(), 44);
    menu->close();

    QVERIFY(QMetaObject::invokeMethod(window, "popupAtCoord", Q_ARG(QVariant, 55), Q_ARG(QVariant, 66)));
    QTRY_COMPARE(menu->property("x").toInt(), 55);
    QTRY_COMPARE(menu->property("y").toInt(), 66);
    menu->close();

    cursorPos = window->mapToGlobal(QPoint(12, window->height() / 2));
    QCursor::setPos(cursorPos);
    QTRY_COMPARE(QCursor::pos(), cursorPos);

    const QList<QQuickMenuItem *> menuItems = QList<QQuickMenuItem *>() << menuItem1 << menuItem2 << menuItem3;
    for (QQuickMenuItem *menuItem : menuItems) {
        QVERIFY(QMetaObject::invokeMethod(window, "popupItemAtCursor", Q_ARG(QVariant, QVariant::fromValue(menuItem))));
        QTRY_COMPARE(menu->property("x").toInt(), 12);
        QTRY_COMPARE(menu->property("y").toInt(), window->height() / 2 + menu->topPadding() - menuItem->y());
        menu->close();

        QVERIFY(QMetaObject::invokeMethod(window, "popupItemAtPos", Q_ARG(QVariant, QPointF(33, window->height() / 3)), Q_ARG(QVariant, QVariant::fromValue(menuItem))));
        QTRY_COMPARE(menu->property("x").toInt(), 33);
        QTRY_COMPARE(menu->property("y").toInt(), window->height() / 3 + menu->topPadding() - menuItem->y());
        menu->close();

        QVERIFY(QMetaObject::invokeMethod(window, "popupItemAtCoord", Q_ARG(QVariant, 55), Q_ARG(QVariant, window->height() / 3 * 2), Q_ARG(QVariant, QVariant::fromValue(menuItem))));
        QTRY_COMPARE(menu->property("x").toInt(), 55);
        QTRY_COMPARE(menu->property("y").toInt(), window->height() / 3 * 2 + menu->topPadding() - menuItem->y());
        menu->close();
    }
#endif
}

void tst_menu::actions()
{
    QQuickApplicationHelper helper(this, QLatin1String("actions.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu *>();
    QVERIFY(menu);

    QQuickMenuItem *menuItem1 = qobject_cast<QQuickMenuItem *>(menu->itemAt(0));
    QVERIFY(menuItem1);
    QVERIFY(menuItem1->action());
    QCOMPARE(menuItem1->text(), "action1");

    QQuickMenuItem *menuItem2 = qobject_cast<QQuickMenuItem *>(menu->itemAt(1));
    QVERIFY(menuItem2);
    QVERIFY(!menuItem2->action());
    QCOMPARE(menuItem2->text(), "menuitem2");

    QQuickMenuItem *menuItem3 = qobject_cast<QQuickMenuItem *>(menu->itemAt(2));
    QVERIFY(menuItem3);
    QVERIFY(menuItem3->action());
    QCOMPARE(menuItem3->text(), "action3");

    QQuickMenuItem *menuItem4 = qobject_cast<QQuickMenuItem *>(menu->itemAt(3));
    QVERIFY(menuItem4);
    QVERIFY(!menuItem4->action());
    QCOMPARE(menuItem4->text(), "menuitem4");
}

void tst_menu::removeTakeItem()
{
    QQuickApplicationHelper helper(this, QLatin1String("removeTakeItem.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu *>();
    QVERIFY(menu);

    QPointer<QQuickMenuItem> menuItem1 = window->property("menuItem1").value<QQuickMenuItem *>();
    QVERIFY(!menuItem1.isNull());
    QCOMPARE(menuItem1->menu(), menu);

    QPointer<QQuickMenuItem> menuItem2 = window->property("menuItem2").value<QQuickMenuItem *>();
    QVERIFY(!menuItem2.isNull());
    QCOMPARE(menuItem2->menu(), menu);

    QPointer<QQuickMenuItem> menuItem3 = window->property("menuItem3").value<QQuickMenuItem *>();
    QVERIFY(!menuItem3.isNull());
    QCOMPARE(menuItem3->menu(), menu);

    // takeItem(int) does not destroy
    QVariant ret;
    QVERIFY(QMetaObject::invokeMethod(window, "takeSecondItem", Q_RETURN_ARG(QVariant, ret)));
    QCOMPARE(ret.value<QQuickMenuItem *>(), menuItem2);
    QVERIFY(!menuItem2->menu());
    QCoreApplication::sendPostedEvents(menuItem2, QEvent::DeferredDelete);
    QVERIFY(!menuItem2.isNull());

    // removeItem(Item) destroys
    QVERIFY(QMetaObject::invokeMethod(window, "removeFirstItem"));
    QVERIFY(!menuItem1->menu());
    QCoreApplication::sendPostedEvents(menuItem1, QEvent::DeferredDelete);
    QVERIFY(menuItem1.isNull());

    // removeItem(null) must not call removeItem(0)
    QVERIFY(QMetaObject::invokeMethod(window, "removeNullItem"));
    QCOMPARE(menuItem3->menu(), menu);
    QCoreApplication::sendPostedEvents(menuItem3, QEvent::DeferredDelete);
    QVERIFY(!menuItem3.isNull());

    // deprecated removeItem(int) does not destroy
    QVERIFY(QMetaObject::invokeMethod(window, "removeFirstIndex"));
    QVERIFY(!menuItem3->menu());
    QCoreApplication::sendPostedEvents(menuItem3, QEvent::DeferredDelete);
    QVERIFY(!menuItem3.isNull());
}

QTEST_MAIN(tst_menu)

#include "tst_menu.moc"
