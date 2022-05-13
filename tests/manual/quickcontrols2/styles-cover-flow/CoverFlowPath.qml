// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Path {
    // Point 1
    property PathView pathView

    startX: 0
    startY: pathView.centerY

    PathAttribute {
        name: "rotateY"
        value: 50.0
    }
    PathAttribute {
        name: "scale"
        value: 0.7
    }
    PathAttribute {
        name: "zOrder"
        value: 1.0
    }

    // Line to point 2
    PathLine {
        x: pathView.centerX - pathView.delegateSize * 0.4
        y: pathView.centerY
    }
    PathPercent {
        value: 0.44
    }
    PathAttribute {
        name: "rotateY"
        value: 50.0
    }
    PathAttribute {
        name: "scale"
        value: 0.7
    }
    PathAttribute {
        name: "zOrder"
        value: 10.0
    }

    // Quad to point 3
    PathQuad {
        x: pathView.centerX
        y: pathView.centerY + pathView.delegateSize * 0.04
        controlX: pathView.centerX - pathView.delegateSize * 0.2
        controlY: pathView.centerY + pathView.delegateSize * 0.04
    }
    PathPercent {
        value: 0.5
    }
    PathAttribute {
        name: "rotateY"
        value: 0.0
    }
    PathAttribute {
        name: "scale"
        value: 1.0
    }
    PathAttribute {
        name: "zOrder"
        value: 50.0
    }

    // Quad to point 4
    PathQuad {
        x: pathView.centerX + pathView.delegateSize * 0.4
        y: pathView.centerY
        controlX: pathView.centerX + pathView.delegateSize * 0.2
        controlY: pathView.centerY + pathView.delegateSize * 0.04
    }
    PathPercent {
        value: 0.56
    }
    PathAttribute {
        name: "rotateY"
        value: -50.0
    }
    PathAttribute {
        name: "scale"
        value: 0.7
    }
    PathAttribute {
        name: "zOrder"
        value: 10.0
    }

    // Line to point 5
    PathLine {
        x: pathView.width
        y: pathView.centerY
    }
    PathAttribute {
        name: "rotateY"
        value: -50.0
    }
    PathAttribute {
        name: "scale"
        value: 0.7
    }
    PathAttribute {
        name: "zOrder"
        value: 1.0
    }
}
