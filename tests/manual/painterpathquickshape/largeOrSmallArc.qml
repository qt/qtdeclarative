// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
            startX: 50
            startY: 100

            delegate: [
                PathArc {
                    x: 100
                    y: 150
                    radiusX: 50
                    radiusY: 50
                    useLargeArc: delegate.index === 1
                }
            ]
        }
    }
}
