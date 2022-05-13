// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Use QtQuick 2.8 to get GraphicsInfo and others
import QtQuick 2.8

Item {
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
        ShaderEffect { // wobble
            id: eff
            width: image1.width
            height: image1.height
            anchors.centerIn: parent

            property variant source: effectSource1
            property real amplitude: 0.04 * 0.2
            property real frequency: 20
            property real time: 0

            NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }

            vertexShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/wobble_legacy_gl.vert" : "qrc:/wobble.vert.qsb"
            fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/wobble_legacy_gl.frag" : "qrc:/wobble.frag.qsb"
        }

        Image {
            id: image2
            source: "qrc:/face-smile.png"
        }
        ShaderEffectSource {
            id: effectSource2
            sourceItem: image2
            hideSource: true
        }
        ShaderEffect { // dropshadow
            id: eff2
            width: image2.width
            height: image2.height
            scale: 2
            x: 40
            y: 40

            property variant source: effectSource2

            property variant shadow: ShaderEffectSource {
                sourceItem: ShaderEffect {
                    width: eff2.width
                    height: eff2.height
                    property variant delta: Qt.size(0.0, 1.0 / height)
                    property variant source: ShaderEffectSource {
                        sourceItem: ShaderEffect {
                            id: innerEff
                            width: eff2.width
                            height: eff2.height
                            property variant delta: Qt.size(1.0 / width, 0.0)
                            property variant source: effectSource2
                            fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/shadow_pass1_legacy_gl.frag" : "qrc:/shadow_pass1.frag.qsb"
                        }
                    }
                    fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/shadow_pass1_legacy_gl.frag" : "qrc:/shadow_pass1.frag.qsb"
                }
            }
            property real angle: 0
            property variant offset: Qt.point(5.0 * Math.cos(angle), 5.0 * Math.sin(angle))
            NumberAnimation on angle { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 6000 }
            property variant delta: Qt.size(offset.x / width, offset.y / height)
            property real darkness: 0.5
            fragmentShader: GraphicsInfo.shaderType === GraphicsInfo.GLSL ? "qrc:/shadow_pass2_legacy_gl.frag" : "qrc:/shadow_pass2.frag.qsb"
        }

        Column {
            anchors.bottom: parent.bottom
            Text {
                color: "yellow"
                font.pointSize: 24
                text: {
                    if (GraphicsInfo.api === GraphicsInfo.OpenGL)
                        "OpenGL";
                    else if (GraphicsInfo.api === GraphicsInfo.Software)
                        "Software";
                    else if (GraphicsInfo.api === GraphicsInfo.Direct3D12)
                        "D3D12";
                    else if (GraphicsInfo.api === GraphicsInfo.OpenVG)
                        "OpenVG";
                    else if (GraphicsInfo.api === GraphicsInfo.OpenGLRhi)
                        "OpenGL via QRhi";
                    else if (GraphicsInfo.api === GraphicsInfo.Direct3D11Rhi)
                        "D3D11 via QRhi";
                    else if (GraphicsInfo.api === GraphicsInfo.VulkanRhi)
                        "Vulkan via QRhi";
                    else if (GraphicsInfo.api === GraphicsInfo.MetalRhi)
                        "Metal via QRhi";
                    else if (GraphicsInfo.api === GraphicsInfo.Null)
                        "Null via QRhi";
                    else
                        "Unknown API";
                }
            }
            Text {
                color: "yellow"
                font.pointSize: 24
                text: "Shader effect is " + (GraphicsInfo.shaderType === GraphicsInfo.HLSL
                                             ? "HLSL" : (GraphicsInfo.shaderType === GraphicsInfo.GLSL
                                                         ? "GLSL" : (GraphicsInfo.shaderType === GraphicsInfo.RhiShader
                                                                     ? "QRhiShader" : "UNKNOWN"))) + " based";
            }
            Text {
                text: GraphicsInfo.shaderType + " " + GraphicsInfo.shaderCompilationType + " " + GraphicsInfo.shaderSourceType
            }
            Text {
                //text: eff.status + " " + eff.log
            }
        }
    }
}
