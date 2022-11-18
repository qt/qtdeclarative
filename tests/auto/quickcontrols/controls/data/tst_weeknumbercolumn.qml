// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "WeekNumberColumn"

    Component {
        id: component
        WeekNumberColumn { }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(component, testCase)
        verify(control)
    }

    function test_locale() {
        var control = component.createObject(testCase)

        compare(control.contentItem.children.length, 6 + 1)

        control.month = 11
        control.year = 2015

        // en_US: [48...53]
        control.locale = Qt.locale("en_US")
        for (var i = 0; i < 6; ++i)
            compare(control.contentItem.children[i].text, (i + 48).toString())

        // no_NO: [49...1]
        control.locale = Qt.locale("no_NO")
        for (var j = 0; j < 5; ++j)
            compare(control.contentItem.children[j].text, (j + 49).toString())
        compare(control.contentItem.children[5].text, "1")

        control.destroy()
    }

    function test_range() {
        var control = component.createObject(testCase)

        control.month = 0
        compare(control.month, 0)

        ignoreWarning(/tst_weeknumbercolumn.qml:18:9: QML (Abstract)?WeekNumberColumn: month -1 is out of range \[0...11\]$/)
        control.month = -1
        compare(control.month, 0)

        control.month = 11
        compare(control.month, 11)

        ignoreWarning(/tst_weeknumbercolumn.qml:18:9: QML (Abstract)?WeekNumberColumn: month 12 is out of range \[0...11\]$/)
        control.month = 12
        compare(control.month, 11)

        control.year = -271820
        compare(control.year, -271820)

        ignoreWarning(/tst_weeknumbercolumn.qml:18:9: QML (Abstract)?WeekNumberColumn: year -271821 is out of range \[-271820...275759\]$/)
        control.year = -271821
        compare(control.year, -271820)

        control.year = 275759
        compare(control.year, 275759)

        ignoreWarning(/tst_weeknumbercolumn.qml:18:9: QML (Abstract)?WeekNumberColumn: year 275760 is out of range \[-271820...275759\]$/)
        control.year = 275760
        compare(control.year, 275759)

        control.destroy()
    }

    function test_font() {
        var control = component.createObject(testCase)

        verify(control.contentItem.children[0])

        control.font.pixelSize = 123
        compare(control.contentItem.children[0].font.pixelSize, 123)

        control.destroy()
    }
}
