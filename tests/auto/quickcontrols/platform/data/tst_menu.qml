// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import Qt.labs.platform
import QtQuick.Controls as Controls
import org.qtproject.Test

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Menu"

    Component {
        id: item
        MenuItem { }
    }

    Component {
        id: menu
        Menu { }
    }

    SignalSpy {
        id: itemsSpy
        signalName: "itemsChanged"
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    function init() {
        verify(!itemsSpy.target)
        compare(itemsSpy.count, 0)
    }

    function cleanup() {
        itemsSpy.target = null
        itemsSpy.clear()
    }

    function test_addRemove() {
        var control = menu.createObject(testCase)

        itemsSpy.target = control
        verify(itemsSpy.valid)

        control.addItem(item.createObject(control, {text: "1"}))
        compare(control.items.length, 1)
        compare(control.items[0].text, "1")
        compare(itemsSpy.count, 1)

        control.addItem(item.createObject(control, {text: "2"}))
        compare(control.items.length, 2)
        compare(control.items[0].text, "1")
        compare(control.items[1].text, "2")
        compare(itemsSpy.count, 2)

        control.insertItem(1, item.createObject(control, {text: "3"}))
        compare(control.items.length, 3)
        compare(control.items[0].text, "1")
        compare(control.items[1].text, "3")
        compare(control.items[2].text, "2")
        compare(itemsSpy.count, 3)

        control.insertItem(0, item.createObject(control, {text: "4"}))
        compare(control.items.length, 4)
        compare(control.items[0].text, "4")
        compare(control.items[1].text, "1")
        compare(control.items[2].text, "3")
        compare(control.items[3].text, "2")
        compare(itemsSpy.count, 4)

        control.insertItem(control.items.length, item.createObject(control, {text: "5"}))
        compare(control.items.length, 5)
        compare(control.items[0].text, "4")
        compare(control.items[1].text, "1")
        compare(control.items[2].text, "3")
        compare(control.items[3].text, "2")
        compare(control.items[4].text, "5")
        compare(itemsSpy.count, 5)

        control.removeItem(control.items[4])
        compare(control.items.length, 4)
        compare(control.items[0].text, "4")
        compare(control.items[1].text, "1")
        compare(control.items[2].text, "3")
        compare(control.items[3].text, "2")
        compare(itemsSpy.count, 6)

        control.removeItem(control.items[0])
        compare(control.items.length, 3)
        compare(control.items[0].text, "1")
        compare(control.items[1].text, "3")
        compare(control.items[2].text, "2")
        compare(itemsSpy.count, 7)

        control.removeItem(control.items[1])
        compare(control.items.length, 2)
        compare(control.items[0].text, "1")
        compare(control.items[1].text, "2")
        compare(itemsSpy.count, 8)

        control.removeItem(control.items[1])
        compare(control.items.length, 1)
        compare(control.items[0].text, "1")
        compare(itemsSpy.count, 9)

        control.removeItem(control.items[0])
        compare(control.items.length, 0)
        compare(itemsSpy.count, 10)

        control.destroy()
    }

    Component {
        id: contentMenu
        Menu {
            QtObject { objectName: "object" }
            MenuItem { objectName: "item1" }
            Timer { objectName: "timer" }
            MenuItem { objectName: "item2" }
            Component { MenuItem { } }
        }
    }

    function test_content() {
        var control = contentMenu.createObject(testCase)

        function compareObjectNames(content, names) {
            if (content.length !== names.length)
                return false
            for (var i = 0; i < names.length; ++i) {
                if (content[i].objectName !== names[i])
                    return false
            }
            return true
        }

        itemsSpy.target = control
        verify(itemsSpy.valid)

        verify(compareObjectNames(control.data, ["object", "item1", "timer", "item2", ""]))
        verify(compareObjectNames(control.items, ["item1", "item2"]))

        control.addItem(item.createObject(control, {objectName: "item3"}))
        verify(compareObjectNames(control.data, ["object", "item1", "timer", "item2", "", "item3"]))
        verify(compareObjectNames(control.items, ["item1", "item2", "item3"]))
        compare(itemsSpy.count, 1)

        control.insertItem(0, item.createObject(control, {objectName: "item4"}))
        verify(compareObjectNames(control.data, ["object", "item1", "timer", "item2", "", "item3", "item4"]))
        verify(compareObjectNames(control.items, ["item4", "item1", "item2", "item3"]))
        compare(itemsSpy.count, 2)

        control.removeItem(control.items[1])
        verify(compareObjectNames(control.data, ["object", "timer", "item2", "", "item3", "item4"]))
        verify(compareObjectNames(control.items, ["item4", "item2", "item3"]))
        compare(itemsSpy.count, 3)

        control.destroy()
    }

    Component {
        id: dynamicMenu
        Menu {
            id: dmenu
            MenuItem { text: "static" }
            Component.onCompleted: {
                addItem(item.createObject(dmenu, {text: "added"}))
                insertItem(0, item.createObject(dmenu, {text: "inserted"}))
            }
        }
    }

    function test_dynamic() {
        var control = dynamicMenu.createObject(testCase)

        // insertItem(), addItem(), and static MenuItem {}
        compare(control.items.length, 3)
        compare(control.items[0].text, "inserted")

        var dying = item.createObject(control, {text: "dying"})
        control.addItem(dying)
        compare(control.items.length, 4)
        compare(control.items[3].text, "dying")
        dying.destroy()
        wait(0)
        compare(control.items.length, 3)

        control.destroy()
    }

    function test_type() {
        // Q_ENUMS(QPlatformMenu::MenuType)
        compare(Menu.DefaultMenu, 0)
        compare(Menu.EditMenu, 1)
    }

    function test_subMenus() {
        var parentMenu = createTemporaryObject(menu, testCase)
        verify(parentMenu)

        var subMenu = menu.createObject(parentMenu)
        verify(subMenu)

        var subMenuItem = subMenu.menuItem
        verify(subMenuItem)

        parentMenu.addMenu(subMenu)
        compare(parentMenu.items.length, 1)
        verify(parentMenu.items[0], subMenuItem)

        subMenu.title = "Title"
        compare(subMenu.title, "Title")
        compare(subMenuItem.text, "Title")
    }

    Component {
        id: disabledMenuItemAndActionComponent

        Item {
            property alias action: action
            property alias menu: menu

            Controls.Action {
                id: action
                shortcut: StandardKey.Copy
            }

            Menu {
                id: menu

                MenuItem {
                    enabled: !action.enabled
                    shortcut: StandardKey.Copy
                    text: "test"
                }
            }
        }
    }

    function test_shortcuts() {
        if (!TestHelper.shortcutsSupported)
            skip("This test requires shortcut support")

        let root = createTemporaryObject(disabledMenuItemAndActionComponent, testCase)
        verify(root)
        let menu = root.menu
        let menuItem = menu.items[0]
        verify(menuItem)
        let action = root.action

        let actionTriggeredSpy = signalSpyComponent.createObject(root,
            { target: action, signalName: "triggered" })
        verify(actionTriggeredSpy.valid)
        let menuItemTriggeredSpy = signalSpyComponent.createObject(root,
            { target: menuItem, signalName: "triggered" })
        verify(menuItemTriggeredSpy.valid)

        // Make sure the window has focus before continuing, otherwise this test will be
        // flaky on webOS's QEMU
        tryCompare(testCase.Window.window, "active", true)

        // Perform the shortcut; the Action should be triggered since the MenuItem is disabled.
        keySequence(StandardKey.Copy)
        compare(actionTriggeredSpy.count, 1)
        compare(menuItemTriggeredSpy.count, 0)

        // Disable the Action, enabling the MenuItem in the process.
        action.enabled = false
        verify(menuItem.enabled)
        // Perform the shortcut; the MenuItem should be triggered since the Action is disabled.
        keySequence(StandardKey.Copy)
        compare(actionTriggeredSpy.count, 1)
        compare(menuItemTriggeredSpy.count, 1)

        // Re-enable the Action, disabling the MenuItem in the process.
        action.enabled = true
        verify(!menuItem.enabled)
        // Perform the shortcut; the Action should be triggered since the MenuItem is disabled.
        keySequence(StandardKey.Copy)
        compare(actionTriggeredSpy.count, 2)
        compare(menuItemTriggeredSpy.count, 1)
    }
}
