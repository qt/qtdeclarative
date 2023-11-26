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
    name: "MonthGrid"

    Component {
        id: defaultGrid
        MonthGrid { }
    }

    Component {
        id: delegateGrid
        MonthGrid {
            delegate: Item {
                readonly property date date: model.date
                readonly property int day: model.day
                readonly property bool today: model.today
                readonly property int weekNumber: model.weekNumber
                readonly property int month: model.month
                readonly property int year: model.year
            }
        }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(defaultGrid, testCase)
        verify(control)
    }

    function test_locale() {
        let control = delegateGrid.createObject(testCase, {month: 0, year: 2013})

        compare(control.contentItem.children.length, 6 * 7 + 1)

        // January 2013
        compare(control.month, 0)
        compare(control.year, 2013)

        // en_GB
        control.locale = Qt.locale("en_GB")
        compare(control.locale.name, "en_GB")

        //                     M             T             W             T             F             S             S
        let en_GB = ["2012-12-31", "2013-01-01", "2013-01-02", "2013-01-03", "2013-01-04", "2013-01-05", "2013-01-06",
                     "2013-01-07", "2013-01-08", "2013-01-09", "2013-01-10", "2013-01-11", "2013-01-12", "2013-01-13",
                     "2013-01-14", "2013-01-15", "2013-01-16", "2013-01-17", "2013-01-18", "2013-01-19", "2013-01-20",
                     "2013-01-21", "2013-01-22", "2013-01-23", "2013-01-24", "2013-01-25", "2013-01-26", "2013-01-27",
                     "2013-01-28", "2013-01-29", "2013-01-30", "2013-01-31", "2013-02-01", "2013-02-02", "2013-02-03",
                     "2013-02-04", "2013-02-05", "2013-02-06", "2013-02-07", "2013-02-08", "2013-02-09", "2013-02-10"]

        for (let i = 0; i < 42; ++i) {
            let cellDate = new Date(en_GB[i])
            compare(control.contentItem.children[i].date.getFullYear(), cellDate.getUTCFullYear())
            compare(control.contentItem.children[i].date.getMonth(), cellDate.getUTCMonth())
            compare(control.contentItem.children[i].date.getDate(), cellDate.getUTCDate())
            compare(control.contentItem.children[i].day, cellDate.getUTCDate())
            compare(control.contentItem.children[i].today, cellDate === new Date())
            compare(control.contentItem.children[i].month, cellDate.getUTCMonth())
            compare(control.contentItem.children[i].year, cellDate.getUTCFullYear())
        }

        // en_US
        control.locale = Qt.locale("en_US")
        compare(control.locale.name, "en_US")

        //                     S             M             T             W             T             F             S
        let en_US = ["2012-12-30", "2012-12-31", "2013-01-01", "2013-01-02", "2013-01-03", "2013-01-04", "2013-01-05",
                     "2013-01-06", "2013-01-07", "2013-01-08", "2013-01-09", "2013-01-10", "2013-01-11", "2013-01-12",
                     "2013-01-13", "2013-01-14", "2013-01-15", "2013-01-16", "2013-01-17", "2013-01-18", "2013-01-19",
                     "2013-01-20", "2013-01-21", "2013-01-22", "2013-01-23", "2013-01-24", "2013-01-25", "2013-01-26",
                     "2013-01-27", "2013-01-28", "2013-01-29", "2013-01-30", "2013-01-31", "2013-02-01", "2013-02-02",
                     "2013-02-03", "2013-02-04", "2013-02-05", "2013-02-06", "2013-02-07", "2013-02-08", "2013-02-09"]

        for (let j = 0; j < 42; ++j) {
            let cellDate = new Date(en_US[j])
            compare(control.contentItem.children[j].date.getFullYear(), cellDate.getUTCFullYear())
            compare(control.contentItem.children[j].date.getMonth(), cellDate.getUTCMonth())
            compare(control.contentItem.children[j].date.getDate(), cellDate.getUTCDate())
            compare(control.contentItem.children[j].day, cellDate.getUTCDate())
            compare(control.contentItem.children[j].today, cellDate === new Date())
            compare(control.contentItem.children[j].month, cellDate.getUTCMonth())
            compare(control.contentItem.children[j].year, cellDate.getUTCFullYear())
        }

        control.destroy()
    }

    function test_range() {
        let control = defaultGrid.createObject(testCase)

        control.month = 0
        compare(control.month, 0)


        ignoreWarning(/tst_monthgrid.qml:18:9: QML (Abstract)?MonthGrid: month -1 is out of range \[0...11\]$/)
        control.month = -1
        compare(control.month, 0)

        control.month = 11
        compare(control.month, 11)

        ignoreWarning(/tst_monthgrid.qml:18:9: QML (Abstract)?MonthGrid: month 12 is out of range \[0...11\]$/)
        control.month = 12
        compare(control.month, 11)

        control.year = -271820
        compare(control.year, -271820)

        ignoreWarning(/tst_monthgrid.qml:18:9: QML (Abstract)?MonthGrid: year -271821 is out of range \[-271820...275759\]$/)
        control.year = -271821
        compare(control.year, -271820)

        control.year = 275759
        compare(control.year, 275759)

        ignoreWarning(/tst_monthgrid.qml:18:9: QML (Abstract)?MonthGrid: year 275760 is out of range \[-271820...275759\]$/)
        control.year = 275760
        compare(control.year, 275759)

        control.destroy()
    }

    function test_bce() {
        let control = defaultGrid.createObject(testCase)

        compare(control.contentItem.children.length, 6 * 7 + 1)

        // fi_FI
        control.locale = Qt.locale("fi_FI")
        compare(control.locale.name, "fi_FI")

        // January 1 BCE
        control.month = 0
        compare(control.month, 0)
        control.year = -1
        compare(control.year, -1)

        //              M   T   W   T   F   S   S
        let jan1bce = [27, 28, 29, 30, 31,  1,  2,
                        3,  4,  5,  6,  7,  8,  9,
                       10, 11, 12, 13, 14, 15, 16,
                       17, 18, 19, 20, 21, 22, 23,
                       24, 25, 26, 27, 28, 29, 30,
                       31,  1,  2,  3,  4,  5,  6]

        for (let i = 0; i < 42; ++i)
            compare(control.contentItem.children[i].text, jan1bce[i].toString())

        // February 1 BCE
        control.month = 1
        compare(control.month, 1)
        control.year = -1
        compare(control.year, -1)

        //              M   T   W   T   F   S   S
        let feb1bce = [31,  1,  2,  3,  4,  5,  6,
                        7,  8,  9, 10, 11, 12, 13,
                       14, 15, 16, 17, 18, 19, 20,
                       21, 22, 23, 24, 25, 26, 27,
                       28, 29,  1,  2,  3,  4,  5,
                        6,  7,  8,  9, 10, 11, 12]

        for (let j = 0; j < 42; ++j)
            compare(control.contentItem.children[j].text, feb1bce[j].toString())

        control.destroy()
    }

    function test_font() {
        let control = defaultGrid.createObject(testCase)

        verify(control.contentItem.children[0])

        control.font.pixelSize = 123
        compare(control.contentItem.children[0].font.pixelSize, 123)

        control.destroy()
    }

    function test_clicked_data() {
        return [
            { tag: "mouse", touch: false },
            { tag: "touch", touch: true }
        ]
    }

    function test_clicked(data) {
        let control = createTemporaryObject(defaultGrid, testCase)
        verify(control)

        compare(control.contentItem.children.length, 6 * 7 + 1)

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressed"})
        verify(pressedSpy.valid)

        let releasedSpy = signalSpy.createObject(control, {target: control, signalName: "released"})
        verify(releasedSpy.valid)

        let clickedSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(clickedSpy.valid)

        let touch = touchEvent(control)

        for (let i = 0; i < 42; ++i) {
            let cell = control.contentItem.children[i]
            verify(cell)

            if (data.touch)
                touch.press(0, cell).commit()
            else
                mousePress(cell)

            compare(pressedSpy.count, i + 1)
            compare(releasedSpy.count, i)
            compare(clickedSpy.count, i)

            if (data.touch)
                touch.release(0, cell).commit()
            else
                mouseRelease(cell)

            compare(pressedSpy.count, i + 1)
            compare(releasedSpy.count, i + 1)
            compare(clickedSpy.count, i + 1)
        }
    }
}
