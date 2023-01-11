// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import ExampleCustomMaterial

Item {
    id: root

    width: 640
    height: 480


//! [1]
    CustomItem {
        property real t: 1
        anchors.fill: parent
        center: Qt.point(-0.748, 0.1);
        iterationLimit: 3 * (zoom + 30)
        zoom: t * t / 10
        NumberAnimation on t {
            from: 1
            to: 60
            duration: 30*1000;
            running: true
            loops: Animation.Infinite
        }
    }
//! [1]

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
        text: qsTr("This example shows how to create a custom material in C++ and use it in QML.\n"
        + "The custom material uses a fragment shader that calculates the Mandelbrot set,"
        + " and exposes the shader uniforms as QML properties.")
    }
}
