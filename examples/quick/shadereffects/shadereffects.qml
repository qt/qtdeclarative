// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQml.Models
import QtQuick.Controls

Rectangle {
    id: root
    width: 320
    height: 480
    property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.tint(root.col, "#20FFFFFF") }
        GradientStop { position: 0.1; color: Qt.tint(root.col, "#20AAAAAA") }
        GradientStop { position: 0.9; color: Qt.tint(root.col, "#20666666") }
        GradientStop { position: 1.0; color: Qt.tint(root.col, "#20000000") }
    }

    //! [source]
    ShaderEffectSource {
        id: theSource
        sourceItem: theItem
    }
    //! [source]

    function saturate(x) {
        return Math.min(Math.max(x, 0), 1)
    }

    function sliderToColor(x) {
        return Qt.rgba(saturate(Math.max(2 - 6 * x, 6 * x - 4)),
                        saturate(Math.min(6 * x, 4 - 6 * x)),
                        saturate(Math.min(6 * x - 2, 6 - 6 * x)))
    }

    Grid {
        anchors.centerIn: parent
        columns: 2

        Item {
            id: theItem
            width: 160
            height: 160
            ListView {
                anchors.centerIn: parent
                width: 160
                height: 140
                clip: true
                snapMode: ListView.SnapOneItem
                model: ObjectModel {
                    Text {
                        width: 160
                        height: 140
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 118
                        font.family: "Times"
                        color: "blue"
                        text: "Qt"
                    }
                    Image {
                        width: 160
                        height: 140
                        source: "content/qt-logo.png"
                    }
                    Image {
                        width: 160
                        height: 140
                        source: "content/face-smile.png"
                    }
                }
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property real amplitude: 0.04 * wobbleSlider.value
            property real frequency: 20
            property real time
            NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 600 }
            //! [fragment]
            fragmentShader: "content/shaders/wobble.frag.qsb"
            //! [fragment]
            Slider {
                id: wobbleSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                height: 40
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property variant shadow: ShaderEffectSource {
                sourceItem: ShaderEffect {
                    width: theItem.width
                    height: theItem.height
                    property variant delta: Qt.size(0.0, 1.0 / height)
                    property variant source: ShaderEffectSource {
                        sourceItem: ShaderEffect {
                            width: theItem.width
                            height: theItem.height
                            property variant delta: Qt.size(1.0 / width, 0.0)
                            property variant source: theSource
                            fragmentShader: "content/shaders/blur.frag.qsb"
                        }
                    }
                    fragmentShader: "content/shaders/blur.frag.qsb"
                }
            }
            property real angle
            property variant offset: Qt.point(15.0 * Math.cos(angle), 15.0 * Math.sin(angle))
            NumberAnimation on angle { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 6000 }
            property variant delta: Qt.size(offset.x / width, offset.y / height)
            property real darkness: shadowSlider.value
            fragmentShader: "content/shaders/shadow.frag.qsb"
            Slider {
                id: shadowSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                height: 40
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property variant delta: Qt.size(0.5 / width, 0.5 / height)
            fragmentShader: "content/shaders/outline.frag.qsb"
        }
        ShaderEffect {
            width: 160
            height: 160
            property variant source: theSource
            property color tint: root.sliderToColor(colorizeSlider.value)
            fragmentShader: "content/shaders/colorize.frag.qsb"
            Slider {
                id: colorizeSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                height: 40
            }
        }
        ShaderEffect {
            width: 160
            height: 160
            //! [properties]
            property variant source: theSource
            property real bend
            property real minimize
            property real side: genieSlider.value
            SequentialAnimation on bend {
                loops: Animation.Infinite
                NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1600 }
                NumberAnimation { to: 0; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1000 }
            }
            SequentialAnimation on minimize {
                loops: Animation.Infinite
                PauseAnimation { duration: 300 }
                NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1000 }
                NumberAnimation { to: 0; duration: 700; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 1300 }
            }
            //! [properties]
            //! [vertex]
            mesh: Qt.size(10, 10)
            vertexShader: "content/shaders/genie.vert.qsb"
            //! [vertex]
            Slider {
                id: genieSlider
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                height: 40
            }
        }
    }
}
