// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Examples

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Red heart", "Draws a red heart with bezier curves",  Qt.resolvedUrl("bezierCurve/bezierCurve.qml"));
            addExample("Talk bubble", "Draws a talk bubble with quadratic curves",  Qt.resolvedUrl("quadraticCurveTo/quadraticCurveTo.qml"));
            addExample("Squircle", "Draws a smooth squircle with simple lines",  Qt.resolvedUrl("squircle/squircle.qml"));
            addExample("Rounded rectangle", "Draws a rounded rectangle with lines and arcs",  Qt.resolvedUrl("roundedrect/roundedrect.qml"));
            addExample("Smile face", "Draws a smile face with complex paths",  Qt.resolvedUrl("smile/smile.qml"));
            addExample("Clip", "Shows the canvas clip feature",  Qt.resolvedUrl("clip/clip.qml"));
            addExample("Tiger", "Draw a tiger with a collection of SVG paths",  Qt.resolvedUrl("tiger/tiger.qml"));
        }
    }
}
