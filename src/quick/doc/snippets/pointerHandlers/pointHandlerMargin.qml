// Copyright (C) 2023 Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 480; height: 320

    Rectangle {
        anchors.fill: handlingContainer
        anchors.margins: -handler.margin
        color: "beige"
    }

    Rectangle {
        id: handlingContainer
        width: 200; height: 200
        anchors.centerIn: parent
        border.color: "green"
        color: handler.active ? "lightsteelblue" : "khaki"

        Text {
            text: "X"
            x: handler.point.position.x - width / 2
            y: handler.point.position.y - height / 2
            visible: handler.active
        }

        PointHandler {
            id: handler
            margin: 30
        }
    }

}
//![0]
