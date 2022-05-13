// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    id: window

    width: 400
    height: 200

    gradient: Gradient {
        GradientStop { position: 0; color: "lightsteelblue" }
        GradientStop { position: 1; color: "black" }
    }

    Column {
        id: column

        y: 50
        width: 200
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4


        TextField {
            text: "A QML text field..."
            width: parent.width
            focus: true
        }

        TextField {
            text: "Another QML text field..."
            width: parent.width
        }

        layer.enabled: true
        layer.smooth: true
    }

    ShaderEffect {
        anchors.top: column.bottom
        width: column.width
        height: column.height;
        anchors.left: column.left

        property variant source: column;
        property size sourceSize: Qt.size(0.5 / column.width, 0.5 / column.height);

        fragmentShader: "reflect.frag.qsb"
    }
}
