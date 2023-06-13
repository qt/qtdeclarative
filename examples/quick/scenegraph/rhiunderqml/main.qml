// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import RhiUnderQML

Item {
    width: 320
    height: 480

     RhiSquircle {
         SequentialAnimation on t {
             NumberAnimation { to: 1; duration: 2500; easing.type: Easing.InQuad }
             NumberAnimation { to: 0; duration: 2500; easing.type: Easing.OutQuad }
             loops: Animation.Infinite
             running: true
         }
     }

    Rectangle {
        color: Qt.rgba(1, 1, 1, 0.7)
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: "black"
        wrapMode: Text.WordWrap
        text: qsTr("The background here is a squircle rendered with QRhi using the beforeRendering() and beforeRenderPassRecording() signals in QQuickWindow. This text label and its border is rendered using QML")
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
