// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Item {
    Rectangle {
        color: "lightGray"
        anchors.fill: parent
        anchors.margins: 10

        Column {
            anchors.fill: parent
            spacing: 10

            Row {
                width: parent.width
                Rectangle {
                    color: "red"
                    width: 300
                    height: 100
                    layer.enabled: true
                    Text { text: "this is in a layer, going through an offscreen render target" }
                    clip: true
                    Rectangle {
                        color: "lightGreen"
                        width: 50
                        height: 50
                        x: 275
                        y: 75
                    }
                }
                Rectangle {
                    color: "white"
                    width: 300
                    height: 100
                    Text { text: "this is not a layer" }
                }
                Rectangle {
                    color: "green"
                    width: 300
                    height: 100
                    layer.enabled: true
                    Text { text: "this is another layer" }
                    Rectangle {
                        border.width: 4
                        border.color: "black"
                        anchors.centerIn: parent
                        width: 150
                        height: 50
                        layer.enabled: true
                        Text {
                            anchors.centerIn: parent
                            text: "layer in a layer"
                        }
                    }
                    Image {
                        source: "qrc:/face-smile.png"
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
                    }
                }
            }

            Row {
                width: parent.width
                Rectangle {
                    color: "white"
                    border.color: "black"
                    border.width: 4
                    width: 300
                    height: 100
                    layer.enabled: true
                    layer.smooth: true // sets min/mag filter in the sampler to Linear
                    layer.textureSize: Qt.size(width * 2, height * 2)
                    Text { x: 10; y: 10; text: "supersampled layer\n(rendered at 2x, sampled with linear min/mag)" }
                    Rectangle {
                        width: 30
                        height: 30
                        anchors.centerIn: parent
                        color: "red"
                        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
                    }
                }
                Rectangle {
                    color: "white"
                    border.color: "black"
                    border.width: 4
                    width: 300
                    height: 100
                    layer.enabled: true
                    layer.samples: 4 // 4x MSAA
                    Text { x: 10; y: 10; text: "4x MSAA layer\n(rendered into multisample texture/renderbuffer,\nthen resolved into non-msaa texture)" }
                    Rectangle {
                        width: 30
                        height: 30
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 10
                        color: "red"
                        NumberAnimation on rotation { from: 360; to: 0; duration: 2000; loops: Animation.Infinite; }
                    }
                }
                Rectangle {
                    color: "white"
                    border.color: "black"
                    border.width: 4
                    width: 300
                    height: 100
                    layer.enabled: true
                    layer.mipmap: true
                    Text { x: 10; y: 10; text: "Mipmapped layer" }
                    Rectangle {
                        width: 30
                        height: 30
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 10
                        color: "red"
                        NumberAnimation on rotation { from: 360; to: 0; duration: 2000; loops: Animation.Infinite; }
                    }
                }
            }
        }
    }
}
