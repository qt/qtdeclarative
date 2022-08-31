// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import Charts
import QtQuick

Item {
    width: 300; height: 200

    Row {
        anchors.centerIn: parent
        spacing: 20

        PieChart {
            id: chartA
            width: 100; height: 100
            color: "red"
        }

        PieChart {
            id: chartB
            width: 100; height: 100
            color: chartA.color
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: { chartA.color = "blue" }
    }

    Text {
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 20 }
        text: "Click anywhere to change the chart color"
    }
}
//![0]
