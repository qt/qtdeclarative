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
            { tag: "2013", from: "2013-01-01", to: "2013-12-31", count: 12 },
            { tag: "2016", from: "2016-01-01", to: "2016-03-31", count: 3 }
        ]
    }

    function test_indices(data) {
        var model = calendarModel.createObject(testCase, {from: data.from, to: data.to})
        verify(model)

        compare(model.count, data.count)

        var y = parseInt(data.tag)
        for (var m = 0; m < 12; ++m) {
            compare(model.yearAt(m), y)
            compare(model.indexOf(y, m), m)
            compare(model.indexOf(new Date(y, m, 1)), m)
            compare(model.monthAt(m), m)
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
