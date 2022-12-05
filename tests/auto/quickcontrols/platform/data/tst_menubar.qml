// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import Qt.labs.platform

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "MenuBar"

    Component {
        id: menu
        Menu { }
    }

    Component {
        id: menuBar
        MenuBar { }
    }

    SignalSpy {
        id: menusSpy
        signalName: "menusChanged"
    }

    function init() {
        verify(!menusSpy.target)
        compare(menusSpy.count, 0)
    }

    function cleanup() {
        menusSpy.target = null
        menusSpy.clear()
    }

    function test_addRemove() {
        var control = menuBar.createObject(testCase)

        menusSpy.target = control
        verify(menusSpy.valid)

        control.addMenu(menu.createObject(control, {title: "1"}))
        compare(control.menus.length, 1)
        compare(control.menus[0].title, "1")
        compare(menusSpy.count, 1)

        control.addMenu(menu.createObject(control, {title: "2"}))
        compare(control.menus.length, 2)
        compare(control.menus[0].title, "1")
        compare(control.menus[1].title, "2")
        compare(menusSpy.count, 2)

        control.insertMenu(1, menu.createObject(control, {title: "3"}))
        compare(control.menus.length, 3)
        compare(control.menus[0].title, "1")
        compare(control.menus[1].title, "3")
        compare(control.menus[2].title, "2")
        compare(menusSpy.count, 3)

        control.insertMenu(0, menu.createObject(control, {title: "4"}))
        compare(control.menus.length, 4)
        compare(control.menus[0].title, "4")
        compare(control.menus[1].title, "1")
        compare(control.menus[2].title, "3")
        compare(control.menus[3].title, "2")
        compare(menusSpy.count, 4)

        control.insertMenu(control.menus.length, menu.createObject(control, {title: "5"}))
        compare(control.menus.length, 5)
        compare(control.menus[0].title, "4")
        compare(control.menus[1].title, "1")
        compare(control.menus[2].title, "3")
        compare(control.menus[3].title, "2")
        compare(control.menus[4].title, "5")
        compare(menusSpy.count, 5)

        control.removeMenu(control.menus[4])
        compare(control.menus.length, 4)
        compare(control.menus[0].title, "4")
        compare(control.menus[1].title, "1")
        compare(control.menus[2].title, "3")
        compare(control.menus[3].title, "2")
        compare(menusSpy.count, 6)

        control.removeMenu(control.menus[0])
        compare(control.menus.length, 3)
        compare(control.menus[0].title, "1")
        compare(control.menus[1].title, "3")
        compare(control.menus[2].title, "2")
        compare(menusSpy.count, 7)

        control.removeMenu(control.menus[1])
        compare(control.menus.length, 2)
        compare(control.menus[0].title, "1")
        compare(control.menus[1].title, "2")
        compare(menusSpy.count, 8)

        control.removeMenu(control.menus[1])
        compare(control.menus.length, 1)
        compare(control.menus[0].title, "1")
        compare(menusSpy.count, 9)

        control.removeMenu(control.menus[0])
        compare(control.menus.length, 0)
        compare(menusSpy.count, 10)

        control.destroy()
    }

    Component {
        id: contentBar
        MenuBar {
            QtObject { objectName: "object" }
            Menu { objectName: "menu1" }
            Timer { objectName: "timer" }
            Menu { objectName: "menu2" }
            Component { Menu { } }
        }
    }

    function test_content() {
        var control = contentBar.createObject(testCase)

        function compareObjectNames(content, names) {
            if (content.length !== names.length)
                return false
            for (var i = 0; i < names.length; ++i) {
                if (content[i].objectName !== names[i])
                    return false
            }
            return true
        }

        menusSpy.target = control
        verify(menusSpy.valid)

        verify(compareObjectNames(control.data, ["object", "menu1", "timer", "menu2", ""]))
        verify(compareObjectNames(control.menus, ["menu1", "menu2"]))

        control.addMenu(menu.createObject(control, {objectName: "menu3"}))
        verify(compareObjectNames(control.data, ["object", "menu1", "timer", "menu2", "", "menu3"]))
        verify(compareObjectNames(control.menus, ["menu1", "menu2", "menu3"]))
        compare(menusSpy.count, 1)

        control.insertMenu(0, menu.createObject(control, {objectName: "menu4"}))
        verify(compareObjectNames(control.data, ["object", "menu1", "timer", "menu2", "", "menu3", "menu4"]))
        verify(compareObjectNames(control.menus, ["menu4", "menu1", "menu2", "menu3"]))
        compare(menusSpy.count, 2)

        control.removeMenu(control.menus[1])
        verify(compareObjectNames(control.data, ["object", "timer", "menu2", "", "menu3", "menu4"]))
        verify(compareObjectNames(control.menus, ["menu4", "menu2", "menu3"]))
        compare(menusSpy.count, 3)

        control.destroy()
    }

    Component {
        id: dynamicBar
        MenuBar {
            id: dbar
            Menu { title: "static" }
            Component.onCompleted: {
                addMenu(menu.createObject(dbar, {title: "added"}))
                insertMenu(0, menu.createObject(dbar, {title: "inserted"}))
            }
        }
    }

    function test_dynamic() {
        var control = dynamicBar.createObject(testCase)

        // insertMenu(), addMenu(), and static Menu {}
        compare(control.menus.length, 3)
        compare(control.menus[0].title, "inserted")

        var dying = menu.createObject(control, {title: "dying"})
        control.addMenu(dying)
        compare(control.menus.length, 4)
        compare(control.menus[3].title, "dying")
        dying.destroy()
        wait(0)
        compare(control.menus.length, 3)

        control.destroy()
    }
}
