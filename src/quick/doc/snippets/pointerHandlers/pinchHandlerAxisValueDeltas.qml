// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 100; height: 100
    color: "lightsteelblue"; antialiasing: true

    PinchHandler {
        id: handler
        target: null
        xAxis.onActiveValueChanged: (delta) => parent.radius -= delta
        yAxis.onActiveValueChanged: (delta) => parent.border.width += delta
        rotationAxis.onActiveValueChanged: (delta) => parent.rotation += delta // add
        scaleAxis.onActiveValueChanged: (delta) => parent.scale *= delta // multiply
    }

    WheelHandler {
        acceptedModifiers: Qt.NoModifier
        property: "rotation"
    }

    WheelHandler {
        acceptedModifiers: Qt.ControlModifier
        property: "scale"
    }
}
//![0]
