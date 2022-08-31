// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import Charts
import QtQuick

Item {
    width: 300; height: 200

    PieChart {
        id: aPieChart
        anchors.centerIn: parent
        width: 100; height: 100
        name: "A simple pie chart"
        color: "red"
    }

    Text {
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 20 }
        text: aPieChart.name
    }
}
//![0]
