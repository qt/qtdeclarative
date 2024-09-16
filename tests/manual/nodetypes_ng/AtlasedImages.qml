// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.8

// The images here should result in a single draw call that uses an atlas
// texture. The ShaderEffect is then another one (and exercises having an
// effect on an Image backed by an atlased texture).

Item {
    Row {
        Image {
            source: "qrc:/qt.png"
            sourceSize: Qt.size(64, 64)
        }
        Image {
            source: "qrc:/face-smile.png"
        }
        Image {
            source: "qrc:/arrow-down.png"
        }
        Image {
            source: "qrc:/arrow-up.png"
            NumberAnimation on rotation {
                from: 0; to: 360; duration: 3000
                loops: Animation.Infinite
            }
        }
        Image {
            id: minusSign
            source: "qrc:/minus-sign.png"
        }
        // Using a ShaderEffectSource would go through an extra render target
        // texture. By specifying the Image directly as the source, no extra
        // texture is created. However, when the source Image is atlased, extra
        // steps are taken internally to create a non-atlased texture for the
        // effect.
        ShaderEffect {
            id: eff
            width: minusSign.width
            height: minusSign.height
            property variant source: minusSign
            property real amplitude: 0.05
            property real frequency: 20
            property real time: 0
            NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }
            vertexShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/wobble_legacy_gl.vert" : "qrc:/wobble.vert.qsb"
            fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/wobble_legacy_gl.frag" : "qrc:/wobble.frag.qsb"
        }
        Image {
            source: "qrc:/plus-sign.png"
        }
    }
}
