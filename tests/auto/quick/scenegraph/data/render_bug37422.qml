// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

/*
    The test verifies that batching does not interfere with overlapping
    regions.

    #samples: 8
                 PixelPos     R    G    B    Error-tolerance
    #base:         0   0     1.0  0.0  0.0        0.05
    #base:         1   1     0.0  0.0  1.0        0.05
    #base:        10  10     1.0  0.0  0.0        0.05
    #base:         1  11     0.0  0.0  1.0        0.05

    #final:        0   0     1.0  0.0  0.0        0.05
    #final:        1   1     0.0  1.0  0.0        0.05
    #final:       10  10     1.0  0.0  0.0        0.05
    #final:        1  11     0.0  1.0  0.0        0.05

*/

RenderTestBase
{
    id: root

    opacity: 0.99

    Rectangle {
        width: 100
        height: 9
        color: Qt.rgba(1, 0, 0);

        Rectangle {
            id: box
            width: 5
            height: 5
            x: 1
            y: 1
            color: Qt.rgba(0, 0, 1);
        }
    }

    ShaderEffect { // Something which blends and is different than rectangle. Will get its own batch
        width: 100
        height: 9
        y: 10
        fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL
                        ? "varying highp vec2 qt_TexCoord0; void main() { gl_FragColor = vec4(1, 0, 0, 1); }"
                        : "qrc:/data/render_bug37422.frag.qsb"

        Rectangle {
            width: 5
            height: 5
            x: 1
            y: 1
            color: box.color
        }
    }

    onEnterFinalStage: {
        box.color = Qt.rgba(0, 1, 0);
        root.finalStageComplete = true;
    }

}
