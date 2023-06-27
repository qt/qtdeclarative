// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {

    width: 320
    height: 480

    color: "steelblue"

    Grid {
        anchors.fill: parent
        anchors.margins: 20
        columnSpacing: 30
        rowSpacing: 30
        columns: 4
        rows: 4

        property real cellWidth: (width - (columns - 1) * columnSpacing) / columns;
        property real cellHeight: (height - (rows - 1) * rowSpacing) / rows;

        Item {
            width: parent.cellWidth
            height: parent.cellHeight
//! [x on]
Rectangle {
    width: 50
    height: 50
    color: "lightsteelblue"
    XAnimator on x {
        from: 10;
        to: 0;
        duration: 1000
    }
}
//! [x on]
        }
        Item {
            width: parent.cellWidth
            height: parent.cellHeight
//! [x target]
Rectangle {
    id: xmovingBox
    width: 50
    height: 50
    color: "lightsteelblue"
    XAnimator {
        target: xmovingBox;
        from: 10;
        to: 0;
        duration: 1000
        running: true
    }
}
//! [x target]
    }
    Item {
        width: parent.cellWidth
        height: parent.cellHeight
//! [y on]
Rectangle {
    width: 50
    height: 50
    color: "lightsteelblue"
    YAnimator on y {
        from: 10;
        to: 0;
        duration: 1000
    }
}
//! [y on]
    }
    Item {
        width: parent.cellWidth
        height: parent.cellHeight
//! [y target]
Rectangle {
    id: ymovingBox
    width: 50
    height: 50
    color: "lightsteelblue"
    YAnimator {
        target: ymovingBox;
        from: 10;
        to: 0;
        duration: 1000
        running: true
    }
}
//! [y target]
    }
//! [rotation on]
Rectangle {
    width: 50
    height: 50
    color: "lightsteelblue"
    RotationAnimator on rotation {
        from: 0;
        to: 360;
        duration: 1000
    }
}
//! [rotation on]
//! [rotation target]
Rectangle {
    id: rotatingBox
    width: 50
    height: 50
    color: "lightsteelblue"
    RotationAnimator {
        target: rotatingBox;
        from: 0;
        to: 360;
        duration: 1000
        running: true
    }
}
//! [rotation target]
//! [scale on]
Rectangle {
    width: 50
    height: 50
    color: "lightsteelblue"
    ScaleAnimator on scale {
        from: 0.5;
        to: 1;
        duration: 1000
    }
}
//! [scale on]
//! [scale target]
Rectangle {
    id: scalingBox
    width: 50
    height: 50
    color: "lightsteelblue"
    ScaleAnimator {
        target: scalingBox;
        from: 0.5;
        to: 1;
        duration: 1000
        running: true
    }
}
//! [scale target]
//! [opacity on]
Rectangle {
    width: 50
    height: 50
    color: "lightsteelblue"
    OpacityAnimator on opacity{
        from: 0;
        to: 1;
        duration: 1000
    }
}
//! [opacity on]
//! [opacity target]
Rectangle {
    id: opacityBox
    width: 50
    height: 50
    color: "lightsteelblue"
    OpacityAnimator {
        target: opacityBox;
        from: 0;
        to: 1;
        duration: 1000
        running: true
    }
}
//! [opacity target]
//! [shaderon]
ShaderEffect {
    width: 50
    height: 50
    property variant t;
    UniformAnimator on t {
        from: 0
        to: 1
        duration: 1000
    }
    fragmentShader: "qrc:shader.frag.qsb"
}

//! [shaderon]
//! [shader target]
ShaderEffect {
    id: shader
    width: 50
    height: 50
    property variant t;
    UniformAnimator {
        target: shader
        uniform: "t"
        from: 0
        to: 1
        duration: 1000
        running: true
    }
    fragmentShader: "qrc:shader.frag.qsb"
}

//! [shader target]
//! [mixed]
Rectangle {
    id: mixBox
    width: 50
    height: 50
    ParallelAnimation {
        ColorAnimation {
            target: mixBox
            property: "color"
            from: "forestgreen"
            to: "lightsteelblue";
            duration: 1000
        }
        ScaleAnimator {
            target: mixBox
            from: 2
            to: 1
            duration: 1000
        }
        running: true
    }
}
//! [mixed]
    }
}
