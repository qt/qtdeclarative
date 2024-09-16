// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    delegate: [
        PathMove { x: p0.cx; y: p0.cy },
        PathQuad { x: p1.cx; y: p1.cy; controlX: c0.cx; controlY: c0.cy },
        PathQuad { x: p2.cx; y: p2.cy; controlX: c1.cx; controlY: c1.cy },
        PathQuad { x: p3.cx; y: p3.cy; controlX: c2.cx; controlY: c2.cy },
        PathQuad { x: p0.cx; y: p0.cy; controlX: c3.cx; controlY: c3.cy },
        PathMove { x: p4.cx; y: p4.cy; },
        PathLine { x: p5.cx; y: p5.cy; },
        PathLine { x: p6.cx; y: p6.cy; },
        PathLine { x: p7.cx; y: p7.cy; },
        PathLine { x: p4.cx; y: p4.cy; }
    ]

    ControlPoint {
        id: p0
        cx: 200
        cy: 200
    }
    ControlPoint {
        id: c0
        color: "blue"
        cx: 600
        cy: 1000
    }
    ControlPoint {
        id: p1
        cx: 1000
        cy: 200
    }
    ControlPoint {
        id: c1
        color: "blue"
        cx: -100
        cy: 200
    }
    ControlPoint {
        id: p2
        color: "red"
        cx: 200
        cy: 1000
    }
    ControlPoint {
        id: c2
        color: "blue"
        cx: -100
        cy: 1000
    }
    ControlPoint {
        id: p3
        color: "red"
        cx: -100
        cy: 500
    }
    ControlPoint {
        id: c3
        color: "blue"
        cx: -300
        cy: 200
    }

    ControlPoint {
        id: p4
        color: "green"
        cx: 2000
        cy: 200
    }
    ControlPoint {
        id: p5
        color: "green"
        cx: 2500
        cy: 700
    }
    ControlPoint {
        id: p6
        color: "green"
        cx: 2000
        cy: 700
    }
    ControlPoint {
        id: p7
        color: "green"
        cx: 2500
        cy: 200
    }

    Text {
        anchors.centerIn: p0
        text: "p0"
    }
    Text {
        anchors.centerIn: p1
        text: "p1"
    }
    Text {
        anchors.centerIn: p2
        text: "p2"
    }
    Text {
        anchors.centerIn: p3
        text: "p3"
    }
    Text {
        anchors.centerIn: c0
        text: "c0"
    }
    Text {
        anchors.centerIn: c1
        text: "c1"
    }
    Text {
        anchors.centerIn: c2
        text: "c2"
    }
    Text {
        anchors.centerIn: c3
        text: "c3"
    }
    Text {
        anchors.centerIn: p4
        text: "p4"
    }
    Text {
        anchors.centerIn: p5
        text: "p5"
    }
    Text {
        anchors.centerIn: p6
        text: "p6"
    }
    Text {
        anchors.centerIn: p7
        text: "p7"
    }
}
