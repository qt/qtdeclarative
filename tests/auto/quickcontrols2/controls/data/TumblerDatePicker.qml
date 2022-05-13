// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Row {
    id: datePicker

    readonly property var days: [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

    property alias dayTumbler: dayTumbler
    property alias monthTumbler: monthTumbler
    property alias yearTumbler: yearTumbler

    Tumbler {
        id: dayTumbler
        objectName: "dayTumbler"

        Component.onCompleted: updateModel()

        function updateModel() {
            var previousIndex = dayTumbler.currentIndex;
            var array = [];
            var newDays = datePicker.days[monthTumbler.currentIndex];
            for (var i = 0; i < newDays; ++i) {
                array.push(i + 1);
            }
            dayTumbler.model = array;
            dayTumbler.currentIndex = Math.min(newDays - 1, previousIndex);
        }
    }
    Tumbler {
        id: monthTumbler
        objectName: "monthTumbler"
        model: ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
        onCurrentIndexChanged: dayTumbler.updateModel()
    }
    Tumbler {
        id: yearTumbler
        objectName: "yearTumbler"
        model: ListModel {
            objectName: "yearTumblerListModel"
            Component.onCompleted: {
                for (var i = 2000; i < 2100; ++i) {
                    append({value: i.toString()});
                }
            }
        }
    }
}
