// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Examples

Item {
    id: root
    height: 480
    width: 320
    Examples.LauncherList {
        anchors.fill: root
        Component.onCompleted: {
            addExample(qsTr("Red heart"), qsTr("Draws a red heart with bezier curves"),
                Qt.resolvedUrl("bezierCurve/bezierCurve.qml"))
            addExample(qsTr("Talk bubble"), qsTr("Draws a talk bubble with quadratic curves"),
                Qt.resolvedUrl("quadraticCurveTo/quadraticCurveTo.qml"))
            addExample(qsTr("Squircle"), qsTr("Draws a smooth squircle with simple lines"),
                Qt.resolvedUrl("squircle/squircle.qml"))
            addExample(qsTr("Rounded rectangle"), qsTr("Draws a rounded rectangle with lines and arcs"),
                Qt.resolvedUrl("roundedrect/roundedrect.qml"))
            addExample(qsTr("Smile face"), qsTr("Draws a smile face with complex paths"),
                Qt.resolvedUrl("smile/smile.qml"))
            addExample(qsTr("Clip"), qsTr("Shows the canvas clip feature"),
                Qt.resolvedUrl("clip/clip.qml"))
            addExample(qsTr("Tiger"), qsTr("Draw a tiger with a collection of SVG paths"),
                Qt.resolvedUrl("tiger/tiger.qml"))
        }
    }
}
