// Copyright (C) 2014 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import SceneGraphRendering

Item {
    id: root

    // The checkers background
    ShaderEffect {
        anchors.fill: parent

        property real tileSize: 16
        property color color1: Qt.rgba(0.9, 0.9, 0.9, 1);
        property color color2: Qt.rgba(0.8, 0.8, 0.8, 1);

        property size pixelSize: Qt.size(width / tileSize, height / tileSize);

        fragmentShader: "qrc:/scenegraph/twotextureproviders/shaders/checker.frag.qsb"
    }

    width: 320
    height: 480

    Item {
        id: box
        width: root.width * 0.9
        height: width

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.9
            height: parent.width * 0.4
            radius: width * 0.1;
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.hsla(0.6, 0.9, 0.9, 1); }
                GradientStop { position: 1; color: Qt.hsla(0.6, 0.6, 0.3, 1); }
            }
            RotationAnimator on rotation { from: 0; to: 360; duration: 10000; loops: Animation.Infinite }
        }

        visible: false
        layer.enabled: true
    }

    Item {
        id: text
        width: box.width
        height: width
        Text {
            anchors.centerIn: parent
            color: "black" // Qt.hsla(0.8, 0.8, 0.8);
            text: qsTr("Qt\nQuick")

            horizontalAlignment: Text.AlignHCenter

            font.bold: true
            font.pixelSize: text.width * 0.25
            RotationAnimator on rotation { from: 360; to: 0; duration: 9000; loops: Animation.Infinite }
        }
        visible: false
        layer.enabled: true
    }

    XorBlender {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.height * 0.05;
        width: box.width
        height: box.height
        source1: box
        source2: text
    }

    Rectangle {
        id: labelFrame
        anchors.margins: -10
        radius: 10
        color: "white"
        border.color: "black"
        opacity: 0.8
        anchors.fill: description
    }

    Text {
        id: description
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        wrapMode: Text.WordWrap
        text: qsTr("This example creates two animated items and sets 'layer.enabled: true' on both of them. This turns the items into texture providers and we can access their texture from C++ in a custom material. The XorBlender is a custom C++ item which uses performs an Xor blend between them.")
    }
}
