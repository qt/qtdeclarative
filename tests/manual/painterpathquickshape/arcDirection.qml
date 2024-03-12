// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    Repeater {
        anchors.fill: parent
        model: 2
        delegate: ControlledShape {
            id: delegate

            required property int index

            anchors.fill: parent
            fillColor: "transparent"
            strokeColor: delegate.index === 0 ? "red" : "blue"
            strokeStyle: ShapePath.DashLine
            strokeWidth: 4

            startX: 4
            startY: 4

            delegate: [
                PathArc {
                    id: arc
                    x: 96
                    y: 96
                    radiusX: 100
                    radiusY: 100
                    direction: delegate.index === 0 ? PathArc.Clockwise : PathArc.Counterclockwise
                }
            ]
        }
    }
}
