// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

    //![0]
Rectangle {
    width: 320
    height: 480
    Flickable {
        anchors.fill: parent
        contentWidth: 1200
        contentHeight: 1200
        Rectangle {
            width: 1000
            height: 1000
    //![0]
            x: 100
            y: 100
            radius: 128
            border.width: 4
            border.color: "black"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#000000" }
                GradientStop { position: 0.2; color: "#888888" }
                GradientStop { position: 0.4; color: "#FFFFFF" }
                GradientStop { position: 0.6; color: "#FFFFFF" }
                GradientStop { position: 0.8; color: "#888888" }
                GradientStop { position: 1.0; color: "#000000" }
            }
        }
    }
}
