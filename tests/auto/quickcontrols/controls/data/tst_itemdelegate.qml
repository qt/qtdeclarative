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
    name: "ItemDelegate"

    Component {
        id: itemDelegate
        ItemDelegate { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(itemDelegate, testCase)
        verify(control)
    }

    function test_baseline() {
        let control = createTemporaryObject(itemDelegate, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_highlighted() {
        let control = createTemporaryObject(itemDelegate, testCase)
        verify(control)
        verify(!control.highlighted)

        control.highlighted = true
        verify(control.highlighted)
    }

    function test_spacing() {
        let control = createTemporaryObject(itemDelegate, testCase, { text: "Some long, long, long text" })
        verify(control)
        verify(control.contentItem.implicitWidth + control.leftPadding + control.rightPadding > control.background.implicitWidth)

        let textLabel = findChild(control.contentItem, "label")
        verify(textLabel)

        // The implicitWidth of the IconLabel that all buttons use as their contentItem
        // should be equal to the implicitWidth of the Text while no icon is set.
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth)

        // That means that spacing shouldn't affect it.
        control.spacing += 100
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth)

        // The implicitWidth of the ItemDelegate itself should, therefore, also never include spacing while no icon is set.
        compare(control.implicitWidth, textLabel.implicitWidth + control.leftPadding + control.rightPadding)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: ItemDelegate.IconOnly },
            { "tag": "TextOnly", display: ItemDelegate.TextOnly },
            { "tag": "TextUnderIcon", display: ItemDelegate.TextUnderIcon },
            { "tag": "TextBesideIcon", display: ItemDelegate.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: ItemDelegate.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: ItemDelegate.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: ItemDelegate.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: ItemDelegate.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        let control = createTemporaryObject(itemDelegate, testCase, {
            text: "ItemDelegate",
            display: data.display,
            width: 400,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        let iconImage = findChild(control.contentItem, "image")
        let textLabel = findChild(control.contentItem, "label")

        switch (control.display) {
        case ItemDelegate.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            break;
        case ItemDelegate.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, control.mirrored ? control.availableWidth - textLabel.width : 0)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case ItemDelegate.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            verify(iconImage.y < textLabel.y)
            break;
        case ItemDelegate.TextBesideIcon:
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
