// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick

ListView {
    id: row

    width: 300
    height: 300

    orientation: Qt.Horizontal
    snapMode: ListView.SnapOneItem
    highlightRangeMode: ListView.StrictlyEnforceRange

    model: 3
    delegate: ListView {
        id: column
        objectName: "vertical column " + index

        required property int index

        width: 300
        height: 300

        orientation: Qt.Vertical
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange

        model: 3
        delegate: Rectangle {
            id: cell

            required property int index

            width: 300
            height: 300
            color: "transparent"
            border.color: "#000"
            border.width: 5
            radius: 15

            Text {
                anchors.centerIn: parent
                text: `Row: ${cell.index}`
            }
        }

        Text {
            anchors.verticalCenterOffset: -height
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            text: `Column: ${column.index}\ncurrentIndex: ${column.currentIndex}`
        }
    }

    Text {
        x: 10; y: 10
        text: `currentIndex: ${row.currentIndex}`
    }
}
