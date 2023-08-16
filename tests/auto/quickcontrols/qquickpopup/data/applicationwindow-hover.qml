// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup: popup
    property alias parentButton: parentButton
    property alias childButton: childButton

    Button {
        id: parentButton
        text: "Parent"
        palette.buttonText: hovered ? "tomato" : "black"
        anchors.fill: parent
        anchors.margins: 10

        Popup {
            id: popup
            x: 10
            y: 10
            leftPadding: 10
            rightPadding: 10
            topPadding: 10
            bottomPadding: 10

            Button {
                anchors.centerIn: parent
                id: childButton
                text: "Child"
                palette.buttonText: hovered ? "tomato" : "black"
            }
        }
    }
}
