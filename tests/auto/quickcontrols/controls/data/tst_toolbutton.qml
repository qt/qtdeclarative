// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "ToolButton"

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: toolButton
        ToolButton { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(toolButton, testCase)
        verify(control)
    }

    function test_text() {
        let control = createTemporaryObject(toolButton, testCase)
        verify(control)

        compare(control.text, "")
        control.text = "ToolButton"
        compare(control.text, "ToolButton")
        control.text = ""
        compare(control.text, "")
    }

    function test_mouse() {
        let control = createTemporaryObject(toolButton, testCase)
        verify(control)

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        let downSpy = signalSpy.createObject(control, {target: control, signalName: "downChanged"})
        verify(downSpy.valid)

        let clickedSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(clickedSpy.valid)

        // check
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(downSpy.count, 1)
        compare(control.pressed, true)
        compare(control.down, true)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(clickedSpy.count, 1)
        compare(pressedSpy.count, 2)
        compare(downSpy.count, 2)
        compare(control.pressed, false)
        compare(control.down, false)

        // uncheck
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(downSpy.count, 3)
        compare(control.pressed, true)
        compare(control.down, true)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(clickedSpy.count, 2)
        compare(pressedSpy.count, 4)
        compare(downSpy.count, 4)
        compare(control.pressed, false)
        compare(control.down, false)

        // release outside
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedSpy.count, 5)
        compare(downSpy.count, 5)
        compare(control.pressed, true)
        compare(control.down, true)
        mouseMove(control, control.width * 2, control.height * 2)
        compare(control.pressed, false)
        compare(control.down, false)
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(clickedSpy.count, 2)
        compare(pressedSpy.count, 6)
        compare(downSpy.count, 6)
        compare(control.pressed, false)
        compare(control.down, false)

        // right button
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(pressedSpy.count, 6)
        compare(downSpy.count, 6)
        compare(control.pressed, false)
        compare(control.down, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(clickedSpy.count, 2)
        compare(pressedSpy.count, 6)
        compare(downSpy.count, 6)
        compare(control.pressed, false)
        compare(control.down, false)
    }

    function test_keys() {
        let control = createTemporaryObject(toolButton, testCase)
        verify(control)

        let clickedSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(clickedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // check
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 1)

        // uncheck
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 2)

        // no change
        // Not testing Key_Enter and Key_Return because QGnomeTheme uses them for
        // pressing buttons and the CI uses the QGnomeTheme platform theme.
        let keys = [Qt.Key_Escape, Qt.Key_Tab]
        for (let i = 0; i < keys.length; ++i) {
            keyClick(keys[i])
            compare(clickedSpy.count, 2)
        }
    }

    function test_baseline() {
        let control = createTemporaryObject(toolButton, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: ToolButton.IconOnly },
            { "tag": "TextOnly", display: ToolButton.TextOnly },
            { "tag": "TextUnderIcon", display: ToolButton.TextUnderIcon },
            { "tag": "TextBesideIcon", display: ToolButton.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: ToolButton.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: ToolButton.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: ToolButton.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: ToolButton.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        let control = createTemporaryObject(toolButton, testCase, {
            text: "ToolButton",
            display: data.display,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        let iconImage = findChild(control.contentItem, "image")
        let textLabel = findChild(control.contentItem, "label")

        switch (control.display) {
        case ToolButton.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            break;
        case ToolButton.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case ToolButton.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            verify(iconImage.y < textLabel.y)
            break;
        case ToolButton.TextBesideIcon:
            verify(iconImage)
            verify(textLabel)
            if (control.mirrored)
                verify(textLabel.x < iconImage.x)
            else
                verify(iconImage.x < textLabel.x)
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        }
    }
}
