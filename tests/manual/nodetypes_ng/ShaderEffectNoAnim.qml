// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.8

Item {
    // make sure we render the scene continuously
    Rectangle { color: "red"; width: 10; height: 10; NumberAnimation on rotation { from: 0; to: 360; loops: -1 } }

    Rectangle {
        color: "gray"
        anchors.margins: 10
        anchors.fill: parent
        Image {
            id: image1
            source: "qrc:/qt.png"
        }
        ShaderEffectSource {
            id: effectSource1
            sourceItem: image1
            hideSource: true
        }
        ShaderEffect { // wobble, no animation -> should not cause re-rendering into the texture
            id: eff
            width: image1.width
            height: image1.height
            anchors.centerIn: parent

            property variant source: effectSource1
            property real amplitude: 0.04 * 0.2
            property real frequency: 20
            property real time: 0

            vertexShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/wobble_legacy_gl.vert" : "qrc:/wobble.vert.qsb"
            fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/wobble_legacy_gl.frag" : "qrc:/wobble.frag.qsb"
        }
    }
}
