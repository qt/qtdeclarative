// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

//![1]
Item {
    id: container
//![4]
    property alias cellColor: rectangle.color
//![4]
//![5]
    signal clicked(cellColor: color)
//![5]

    width: 40; height: 25
//![1]

//![2]
    Rectangle {
        id: rectangle
        border.color: "white"
        anchors.fill: parent
    }
//![2]

//![3]
    MouseArea {
        anchors.fill: parent
        onClicked: container.clicked(container.cellColor)
    }
//![3]
}
//![0]
