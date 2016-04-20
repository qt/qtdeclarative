/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

// Use QtQuick 2.8 to get shaderType and the other new properties
import QtQuick 2.8

Item {
    Rectangle {
        color: "gray"
        anchors.margins: 10
        anchors.fill: parent
        Image {
            id: image
            source: "qrc:/qt.png"
        }
        ShaderEffectSource {
            id: effectSource
            sourceItem: image
            hideSource: true
        }
        ShaderEffect {
            width: image.width
            height: image.height
            anchors.centerIn: parent

            property variant source: effectSource
            property real amplitude: 0.04 * 0.2
            property real frequency: 20
            property real time: 0

            NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }

            property string glslFragmentShader:
                "uniform sampler2D source;" +
                "uniform highp float amplitude;" +
                "uniform highp float frequency;" +
                "uniform highp float time;" +
                "uniform lowp float qt_Opacity;" +
                "varying highp vec2 qt_TexCoord0;" +
                "void main() {" +
                "    highp vec2 p = sin(time + frequency * qt_TexCoord0);" +
                "    gl_FragColor = texture2D(source, qt_TexCoord0 + amplitude * vec2(p.y, -p.x)) * qt_Opacity;" +
                "}"

            property string hlslVertexShaderByteCode: "qrc:/vs_wobble.cso"
            property string hlslPixelShaderByteCode: "qrc:/ps_wobble.cso"

            // This effect does not need a custom vertex shader but have one with HLSL just to test that path as well.
            vertexShader: shaderType === ShaderEffect.GLSL ? ""
                                                           : (shaderType === ShaderEffect.HLSL ? hlslVertexShaderByteCode : "")
            fragmentShader: shaderType === ShaderEffect.GLSL ? glslFragmentShader
                                                             : (shaderType === ShaderEffect.HLSL ? hlslPixelShaderByteCode : "")
        }
    }
}
