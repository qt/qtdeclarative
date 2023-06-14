// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    id: point1
    color: "red"
    border.width: 1
    border.color: "black"
    opacity: 0.3
    width: 20
    height: 20

    property real cx: 400
    property real cy: 800

    property point pt: Qt.point(cx, cy)

    DragHandler {
        xAxis.minimum: -controlPanel.pathMargin
        yAxis.minimum: -controlPanel.pathMargin
    }
    onXChanged: {
        cx = (x + width/2) / controlPanel.scale
        controlPanel.updatePath()
    }
    onYChanged: {
        cy = (y + height/2) / controlPanel.scale
        controlPanel.updatePath()
    }

    Component.onCompleted: {
        x = cx * controlPanel.scale - width/2
        y = cy * controlPanel.scale - height/2
    }

    Connections {
        target: controlPanel
        function onScaleChanged() {
            x = cx * controlPanel.scale - width/2
            y = cy * controlPanel.scale - height/2
        }
    }

}