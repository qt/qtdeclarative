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
        anchors.fill: parent

        Popup {
            id: popup
            x: 1
            y: 1
            leftPadding: 1
            rightPadding: 1
            topPadding: 1
            bottomPadding: 1


            Button {
                anchors.centerIn: parent
                id: childButton
                text: "Child"
            }
        }
    }
}
