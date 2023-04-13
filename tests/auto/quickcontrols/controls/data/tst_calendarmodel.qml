// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
    id: testCase
    name: "CalendarModel"

    Component {
        id: calendarModel
        CalendarModel { }
    }

    Component {
        id: instantiator
        Instantiator {
            model: CalendarModel {
                from: new Date(2016, 0, 1)
                to: new Date(2016, 11, 31)
            }
            QtObject {
                readonly property int month: model.month
                readonly property int year: model.year
            }
        }
    }

    function test_indices_data() {
        return [
            // "from" and "to" must currently be in the same year.
            { tag: "2013", from: "2013-01-01", to: "2013-12-31", count: 12 },
            { tag: "2016", from: "2016-01-01", to: "2016-03-31", count: 3 },
            { tag: "2016-02-01 to 2016-12-31", from: "2016-02-01", to: "2016-12-31", count: 11 },
            { tag: "2014-11-30 to 2016-01-01", from: "2014-11-30", to: "2016-01-01", count: 15 }
        ]
    }

    function test_indices(data) {
        let model = calendarModel.createObject(testCase, {from: data.from, to: data.to})
        verify(model)

        compare(model.count, data.count)

        const from = new Date(data.from)
        const to = new Date(data.to)
        let index = 0
        for (let date = from; date <= to; date.setMonth(date.getMonth() + 1, 28), ++index) {
            compare(model.yearAt(index), date.getFullYear(),
                `yearAt(${index}) returned incorrect value`)
            compare(model.indexOf(date.getFullYear(), date.getMonth()), index,
                `indexOf(${date.getFullYear()}, ${date.getMonth()}) returned incorrect value`)
            compare(model.indexOf(date), index,
                `indexOf(${date}) returned incorrect value`)
            compare(model.monthAt(index), date.getMonth(),
                `monthAt(${index}) returned incorrect value`)
        }

        model.destroy()
    }

    function test_invalid() {
        var model = calendarModel.createObject(testCase)
        verify(model)

        compare(model.indexOf(-1, -1), -1)
        compare(model.indexOf(new Date(-1, -1, -1)), -1)

        model.destroy()
    }

    function test_instantiator() {
        var inst = instantiator.createObject(testCase)
        verify(inst)

        compare(inst.count, 12)
        for (var m = 0; m < inst.count; ++m) {
            compare(inst.objectAt(m).month, m)
            compare(inst.objectAt(m).year, 2016)
        }

        inst.destroy()
    }
}
