// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls
import Qt.test.controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "MenuItem"

    readonly property bool platformSupportsShortcuts: StyleInfo.styleName !== "iOS"

    Component {
        id: menuItem
        MenuItem { }
    }

    Component {
        id: menu
        Menu { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function cleanup() {
        ApplicationAttributes.set(Qt.AA_DontShowShortcutsInContextMenus, false)
    }

    function test_defaults() {
        let control = createTemporaryObject(menuItem, testCase)
        verify(control)
    }

    function test_baseline() {
        let control = createTemporaryObject(menuItem, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_checkable() {
        let control = createTemporaryObject(menuItem, testCase)
        verify(control)
        verify(control.hasOwnProperty("checkable"))
        verify(!control.checkable)

        mouseClick(control)
        verify(!control.checked)

        control.checkable = true
        mouseClick(control)
        verify(control.checked)

        mouseClick(control)
        verify(!control.checked)
    }

    function test_highlighted() {
        let control = createTemporaryObject(menuItem, testCase)
        verify(control)
        verify(!control.highlighted)

        control.highlighted = true
        verify(control.highlighted)
    }

    Component {
        id: menuItemWithActionComponent

        MenuItem {
            action: Action {
                shortcut: "Ctrl+S"
            }
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: "darkorange"
            }
        }
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: MenuItem.IconOnly },
            { "tag": "TextOnly", display: MenuItem.TextOnly },
            { "tag": "TextUnderIcon", display: MenuItem.TextUnderIcon },
            { "tag": "TextBesideIcon", display: MenuItem.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: MenuItem.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: MenuItem.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: MenuItem.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: MenuItem.TextBesideIcon, mirrored: true },
            { "tag": "IconOnly, showShortcut", display: MenuItem.IconOnly, showShortcut: true },
            { "tag": "TextOnly, showShortcut", display: MenuItem.TextOnly, showShortcut: true },
            { "tag": "TextUnderIcon, showShortcut", display: MenuItem.TextUnderIcon,
                showShortcut: true },
            { "tag": "TextBesideIcon, showShortcut", display: MenuItem.TextBesideIcon,
                showShortcut: true },
            { "tag": "IconOnly, mirrored, showShortcut", display: MenuItem.IconOnly, mirrored: true,
                showShortcut: true },
            { "tag": "TextOnly, mirrored, showShortcut", display: MenuItem.TextOnly, mirrored: true,
                showShortcut: true },
            { "tag": "TextUnderIcon, mirrored, showShortcut", display: MenuItem.TextUnderIcon,
                mirrored: true, showShortcut: true },
            { "tag": "TextBesideIcon, mirrored, showShortcut", display: MenuItem.TextBesideIcon,
                mirrored: true, showShortcut: true }
        ]
    }

    function test_display(data) {
        // This is reset to false in cleanup().
        ApplicationAttributes.set(Qt.AA_DontShowShortcutsInContextMenus, !data.showShortcut)

        // Need to use a MenuItem with an Action set so that we can test shortcuts.
        let control = createTemporaryObject(menuItemWithActionComponent, testCase, {
            text: "MenuItem",
            display: data.display,
            // Ensure that the menu item is wide enough for the shortcut to be shown.
            width: 300,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored,
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        let padding = data.mirrored ? control.contentItem.rightPadding : control.contentItem.leftPadding
        let iconImage = findChild(control.contentItem, "image")
        let textLabel = findChild(control.contentItem, "label")
        let shortcutLabel = findChild(control.contentItem, "shortcutLabel")
        const showShortcut = data.showShortcut && testCase.platformSupportsShortcuts

        switch (control.display) {
        case MenuItem.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            verify(!shortcutLabel)

            compare(iconImage.x, Math.round(control.mirrored ? control.availableWidth - iconImage.width - padding : padding))
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            break;
        case MenuItem.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            if (showShortcut) {
                verify(shortcutLabel)
                verify(shortcutLabel.visible)
            } else {
                verify(!shortcutLabel)
            }

            compare(textLabel.x, control.mirrored ? control.availableWidth - textLabel.width - padding : padding)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            if (showShortcut) {
                compare(shortcutLabel.x, control.mirrored ? padding : control.availableWidth - shortcutLabel.width - padding)
                compare(shortcutLabel.y, (control.availableHeight - shortcutLabel.height) / 2)
            }
            break;
        case MenuItem.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            // See comment in layout() for why we don't support TextUnderIcon for shortcuts.
            verify(!shortcutLabel)

            compare(iconImage.x, Math.round(control.mirrored
                ? control.availableWidth - iconImage.width - (textLabel.width - iconImage.width) / 2 - padding
                : (textLabel.width - iconImage.width) / 2 + padding))
            compare(textLabel.x, control.mirrored ? control.availableWidth - textLabel.width - padding : padding)
            verify(iconImage.y < textLabel.y)
            break;
        case MenuItem.TextBesideIcon:
            verify(iconImage)
            verify(textLabel)
            if (showShortcut) {
                verify(shortcutLabel)
                verify(shortcutLabel.visible)
            } else {
                verify(!shortcutLabel)
            }

            if (control.mirrored) {
                verify(textLabel.x < iconImage.x)
                if (showShortcut)
                    verify(shortcutLabel.x < textLabel.x)
            } else {
                verify(iconImage.x < textLabel.x)
                if (showShortcut)
                    verify(textLabel.x < shortcutLabel.x)
            }
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            if (showShortcut)
                compare(shortcutLabel.y, (control.availableHeight - shortcutLabel.height) / 2)
            break;
        }
    }

    function test_shortcutShownAtImplicitContentWidth_data() {
        return [
            { "tag": "TextOnly", display: MenuItem.TextOnly },
            { "tag": "TextBesideIcon", display: MenuItem.TextBesideIcon }
        ]
    }

    function test_shortcutShownAtImplicitContentWidth(data) {
        if (!testCase.platformSupportsShortcuts)
            skip("Shortcuts are not shown on this platform")

        ApplicationAttributes.set(Qt.AA_DontShowShortcutsInContextMenus, false)
        let control = createTemporaryObject(menuItemWithActionComponent, testCase, {
            text: "MenuItem",
            display: data.display,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
        })
        verify(control)
        let shortcutLabel = findChild(control.contentItem, "shortcutLabel")
        verify(shortcutLabel)

        control.width = control.leftPadding + control.implicitContentWidth + control.rightPadding
        compare(control.contentItem.width, control.implicitContentWidth)
        verify(shortcutLabel.visible)

        control.width -= 1
        verify(!shortcutLabel.visible)
    }

    function test_menu() {
        let control = createTemporaryObject(menu, testCase)
        verify(control)

        let item1 = createTemporaryObject(menuItem, testCase)
        verify(item1)
        compare(item1.menu, null)

        let item2 = createTemporaryObject(menuItem, testCase)
        verify(item2)
        compare(item2.menu, null)

        control.addItem(item1)
        compare(item1.menu, control)
        compare(item2.menu, null)

        control.insertItem(1, item2)
        compare(item1.menu, control)
        compare(item2.menu, control)

        control.removeItem(control.itemAt(1))
        compare(item1.menu, control)
        compare(item2.menu, null)

        control.removeItem(control.itemAt(0))
        compare(item1.menu, null)
        compare(item2.menu, null)
    }

    Component {
        id: iconColorSetInActionComponent

        MenuItem {
            action: Action {
                icon.source: "images/heart.svg"
                icon.color: "tomato"
            }
        }
    }

    Component {
        id: iconColorSetInControlComponent

        MenuItem {
            icon.source: "images/heart.svg"
            icon.color: "wheat"
        }
    }

    function test_iconColor_data() {
        return [
            { tag: "icon color set in action", component: iconColorSetInActionComponent,
                expectedIconColor: "#ff6347" },
            { tag: "icon color set in control", component: iconColorSetInControlComponent,
                expectedIconColor: "#f5deb3"}
        ]
    }

    function test_iconColor(data) {
        let control = createTemporaryObject(data.component, testCase)
        let iconImage = findChild(control.contentItem, "image")
        verify(iconImage)
        compare(iconImage.color, data.expectedIconColor)
    }
}
