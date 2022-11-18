// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "RadioDelegate"

    Component {
        id: radioDelegate
        RadioDelegate {}
    }

    // TODO: data-fy tst_radiobutton (rename to tst_radio?) so we don't duplicate its tests here?

    function test_defaults() {
        failOnWarning(/.?/)

        var control = createTemporaryObject(radioDelegate, testCase);
        verify(control);
        verify(!control.checked);
    }

    function test_checked() {
        var control = createTemporaryObject(radioDelegate, testCase);
        verify(control);

        mouseClick(control);
        verify(control.checked);

        mouseClick(control);
        verify(control.checked);
    }

    function test_baseline() {
        var control = createTemporaryObject(radioDelegate, testCase);
        verify(control);
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset);
    }

    function test_spacing() {
        var control = createTemporaryObject(radioDelegate, testCase, { text: "Some long, long, long text" })
        verify(control)
        verify(control.contentItem.implicitWidth + control.leftPadding + control.rightPadding > control.background.implicitWidth)

        var textLabel = findChild(control.contentItem, "label")
        verify(textLabel)

        // The implicitWidth of the IconLabel that all buttons use as their contentItem should be
        // equal to the implicitWidth of the Text and the radio indicator + spacing while no icon is set.
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth + control.indicator.width + control.spacing)

        control.spacing += 100
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth + control.indicator.width + control.spacing)

        compare(control.implicitWidth, textLabel.implicitWidth + control.indicator.width + control.spacing + control.leftPadding + control.rightPadding)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: RadioDelegate.IconOnly },
            { "tag": "TextOnly", display: RadioDelegate.TextOnly },
            { "tag": "TextUnderIcon", display: RadioDelegate.TextUnderIcon },
            { "tag": "TextBesideIcon", display: RadioDelegate.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: RadioDelegate.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: RadioDelegate.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: RadioDelegate.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: RadioDelegate.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        var control = createTemporaryObject(radioDelegate, testCase, {
            text: "RadioDelegate",
            display: data.display,
            width: 400,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        var iconImage = findChild(control.contentItem, "image")
        var textLabel = findChild(control.contentItem, "label")

        var availableWidth = control.availableWidth - control.indicator.width - control.spacing
        var indicatorOffset = control.mirrored ? control.indicator.width + control.spacing : 0

        switch (control.display) {
        case RadioDelegate.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, indicatorOffset + (availableWidth - iconImage.width) / 2)
            compare(iconImage.y, (control.availableHeight - iconImage.height) / 2)
            break;
        case RadioDelegate.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, control.mirrored ? control.availableWidth - textLabel.width : 0)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case RadioDelegate.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, indicatorOffset + (availableWidth - iconImage.width) / 2)
            compare(textLabel.x, indicatorOffset + (availableWidth - textLabel.width) / 2)
            verify(iconImage.y < textLabel.y)
            break;
        case RadioDelegate.TextBesideIcon:
            verify(iconImage)
            verify(textLabel)
            if (control.mirrored)
                verify(textLabel.x < iconImage.x)
            else
                verify(iconImage.x < textLabel.x)
            compare(iconImage.y, (control.availableHeight - iconImage.height) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        }
    }
}
