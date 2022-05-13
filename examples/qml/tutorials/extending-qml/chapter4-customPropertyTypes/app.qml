// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import Charts 1.0
import QtQuick 2.0

Item {
    width: 300; height: 200

    PieChart {
        id: chart
        anchors.centerIn: parent
        width: 100; height: 100

        pieSlice: PieSlice {
            anchors.fill: parent
            color: "red"
        }
    }

    Component.onCompleted: console.log("The pie is colored " + chart.pieSlice.color)
}
//![0]
