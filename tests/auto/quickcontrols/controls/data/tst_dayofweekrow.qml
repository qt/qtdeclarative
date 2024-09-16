// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "DayOfWeekRow"

    Component {
        id: component
        DayOfWeekRow { }
    }

    function init () {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(component, testCase)
        verify(control)
    }

    function test_locale() {
        let control = component.createObject(testCase)

        verify(control.contentItem.children[0])

        control.locale = Qt.locale("en_US")
        compare(control.contentItem.children[0].text, "Sun")

        control.locale = Qt.locale("no_NO")
        compare(control.contentItem.children[0].text, "man.")

        control.locale = Qt.locale("fi_FI")
        compare(control.contentItem.children[0].text, "ma")

        control.destroy()
    }

    function test_font() {
        let control = component.createObject(testCase)

        verify(control.contentItem.children[0])

        control.font.pixelSize = 123
        compare(control.contentItem.children[0].font.pixelSize, 123)

        control.destroy()
    }
}
