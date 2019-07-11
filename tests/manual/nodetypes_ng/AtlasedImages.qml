/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
