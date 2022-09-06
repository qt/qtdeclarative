// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Item {
    Repeater {
        anchors.fill: parent
        model: 2
        delegate: ControlledShape {
            id: delegate1

            required property int index

            fillColor: "transparent"
            strokeColor: delegate1.index === 0 ? "red" : "blue"
            strokeStyle: ShapePath.DashLine
            strokeWidth: 4

            width: 200
            height: 200
            anchors.centerIn: parent
            startX: 50
            startY: 100

            delegate: [
                PathArc {
                    x: 150
                    y: 100
                    radiusX: 50
                    radiusY: 20
                    xAxisRotation: delegate1.index === 0 ? 0 : 45
                }
            ]
        }
    }

    Repeater {
        anchors.fill: parent
        model: 2
        delegate: ControlledShape {
            id: delegate2

            required property int index

            width: 200
            height: 200
            anchors.centerIn: parent

            fillColor: "transparent"
            strokeColor: delegate2.index === 0 ? "red" : "blue"

            startX: 50
            startY: 100

            delegate: [
                PathArc {
                    x: 150
                    y: 100
                    radiusX: 50
                    radiusY: 20
                    xAxisRotation: delegate2.index === 0 ? 0 : 45
                    direction: PathArc.Counterclockwise
                }
            ]
        }
    }
}
