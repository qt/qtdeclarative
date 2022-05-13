// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 300
    height: 300

    Flickable {
        id: flickable
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            Repeater {
                model: 255

                delegate: Rectangle {
                    width: column.width
                    height: 50
                    color: flickable.dragging ? "steelblue" : "gray"
                    Text {
                        anchors.centerIn: parent
                        text: index
                    }
                }
            }
        }
    }
    PinchHandler {
        target: flickable
    }
}
