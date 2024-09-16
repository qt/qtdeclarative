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
    name: "TabButton"

    Component {
        id: tabButton
        TabButton { }
    }

    Component {
        id: repeater
        Column {
            Repeater {
                model: 3
                delegate: TabButton { }
            }
        }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(tabButton, testCase)
        verify(control)
    }

    function test_autoExclusive() {
        let container = createTemporaryObject(repeater, testCase)

        for (let i = 0; i < 3; ++i) {
            container.children[i].checked = true
            compare(container.children[i].checked, true)

            // check that all other buttons are unchecked
            for (let j = 0; j < 3; ++j) {
                if (j !== i)
                    compare(container.children[j].checked, false)
            }
        }
    }

    function test_baseline() {
        let control = createTemporaryObject(tabButton, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_spacing() {
        let control = createTemporaryObject(tabButton, testCase, { text: "Some long, long, long text" })
        verify(control)
        if (control.background)
            verify(control.contentItem.implicitWidth + control.leftPadding + control.rightPadding > control.background.implicitWidth)

        let textLabel = findChild(control.contentItem, "label")
        verify(textLabel)

        // The implicitWidth of the IconLabel that all buttons use as their contentItem
        // should be equal to the implicitWidth of the Text while no icon is set.
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth)

        // That means that spacing shouldn't affect it.
        control.spacing += 100
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth)

        // The implicitWidth of the TabButton itself should, therefore, also never include spacing while no icon is set.
        compare(control.implicitWidth, textLabel.implicitWidth + control.leftPadding + control.rightPadding)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: TabButton.IconOnly },
            { "tag": "TextOnly", display: TabButton.TextOnly },
            { "tag": "TextUnderIcon", display: TabButton.TextUnderIcon },
            { "tag": "TextBesideIcon", display: TabButton.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: TabButton.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: TabButton.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: TabButton.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: TabButton.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        let control = createTemporaryObject(tabButton, testCase, {
            text: "TabButton",
            display: data.display,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        let iconImage = findChild(control.contentItem, "image")
        let textLabel = findChild(control.contentItem, "label")

        switch (control.display) {
        case TabButton.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            break;
        case TabButton.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case TabButton.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            verify(iconImage.y < textLabel.y)
            break;
        case TabButton.TextBesideIcon:
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
