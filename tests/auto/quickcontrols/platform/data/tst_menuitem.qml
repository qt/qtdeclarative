// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import Qt.labs.platform
import org.qtproject.Test

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "MenuItem"

    Component {
        id: menuItem
        // Check that icon.name can be used in this Qt.labs.platform version
        MenuItem {
            icon.name: ""
        }
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    function test_properties_data() {
        return [
            {tag: "enabled", signal: "enabledChanged", init: true, value: false},
            {tag: "visible", signal: "visibleChanged", init: true, value: false},
            {tag: "separator", signal: "separatorChanged", init: false, value: true},
            {tag: "checkable", signal: "checkableChanged", init: false, value: true},
            {tag: "checked", signal: "checkedChanged", init: false, value: true},
            {tag: "role", signal: "roleChanged", init: MenuItem.TextHeuristicRole, value: MenuItem.AboutRole},
            {tag: "text", signal: "textChanged", init: "", value: "text"},
            {tag: "icon.source", signal: "iconChanged", init: "", value: "qrc:/undo.png"},
            {tag: "icon.name", signal: "iconChanged", init: "", value: "edit-undo"},
            {tag: "shortcut", signal: "shortcutChanged", init: undefined, value: StandardKey.Undo}
        ]
    }

    function test_properties(data) {
        let item = createTemporaryObject(menuItem, testCase)
        verify(item)

        let groupedProperty = data.tag.indexOf(".") !== -1
        let spy = createTemporaryObject(signalSpyComponent, testCase, {
            target: item, signalName: data.signal
        })
        verify(spy)
        verify(spy.valid)

        let propertyName = groupedProperty ? data.tag.split('.')[1] : data.tag
        let object = !groupedProperty ? item : item.icon
        compare(object[propertyName], data.init)
        object[propertyName] = data.value
        compare(spy.count, 1)
        compare(object[propertyName], data.value)

        object[propertyName] = data.value
        compare(spy.count, 1)
    }

    function test_shortcut() {
        if (!TestHelper.shortcutsSupported)
            return;

        let item = createTemporaryObject(menuItem, testCase)
        verify(item)
        let spy = createTemporaryObject(signalSpyComponent, testCase, {
            target: item, signalName: "triggered"
        })
        verify(spy)
        verify(spy.valid)

        data = [TestHelper.shortcutInt, TestHelper.shortcutString, TestHelper.shortcutKeySequence]
        for (let i = 0; i < data.length; ++i) {
            item.shortcut = data[i]

            compare(spy.count, i)
            keySequence("CTRL+P")
            compare(spy.count, i + 1)

            item.shortcut = {}
        }
    }

    function test_role() {
        // Q_ENUMS(QPlatformMenuItem::MenuRole)
        compare(MenuItem.NoRole, 0)
        compare(MenuItem.TextHeuristicRole, 1)
        compare(MenuItem.ApplicationSpecificRole, 2)
        compare(MenuItem.AboutQtRole, 3)
        compare(MenuItem.AboutRole, 4)
        compare(MenuItem.PreferencesRole, 5)
        compare(MenuItem.QuitRole, 6)
    }
}
