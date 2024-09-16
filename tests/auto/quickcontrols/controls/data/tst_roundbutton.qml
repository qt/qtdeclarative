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
    name: "RoundButton"

    Component {
        id: roundButton
        RoundButton { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(roundButton, testCase)
        verify(control)
    }

    function test_radius() {
        let control = createTemporaryObject(roundButton, testCase);
        verify(control);

        let implicitRadius = control.radius;
        compare(implicitRadius, Math.min(control.width, control.height) / 2);

        control.radius = 10;
        compare(control.radius, 10);

        control.radius = undefined;
        compare(control.radius, implicitRadius);

        control.width = -1;
        compare(control.radius, 0);

        control.width = 10;
        compare(control.radius, 5);
    }

    function test_spacing() {
        let control = createTemporaryObject(roundButton, testCase, { text: "Some long, long, long text" })
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

        // The implicitWidth of the Button itself should, therefore, also never include spacing while no icon is set.
        compare(control.implicitWidth, textLabel.implicitWidth + control.leftPadding + control.rightPadding)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: RoundButton.IconOnly },
            { "tag": "TextOnly", display: RoundButton.TextOnly },
            { "tag": "TextUnderIcon", display: RoundButton.TextUnderIcon },
            { "tag": "TextBesideIcon", display: RoundButton.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: RoundButton.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: RoundButton.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: RoundButton.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: RoundButton.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        let control = createTemporaryObject(roundButton, testCase, {
            text: "RoundButton",
            display: data.display,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        let iconImage = findChild(control.contentItem, "image")
        let textLabel = findChild(control.contentItem, "label")

        switch (control.display) {
        case RoundButton.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(iconImage.y, Math.round((control.availableHeight - iconImage.height) / 2))
            break;
        case RoundButton.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case RoundButton.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, Math.round((control.availableWidth - iconImage.width) / 2))
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            verify(iconImage.y < textLabel.y)
            break;
        case RoundButton.TextBesideIcon:
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
