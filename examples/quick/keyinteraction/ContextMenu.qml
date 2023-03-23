// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

FocusScope {
    id: container

    required property Item keyRightTarget

    property bool open: false

    Item {
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: "#D1DBBD"
            focus: true
            Keys.onRightPressed: container.keyRightTarget.focus = true

            Text {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                    margins: 30
                }
                color: "black"
                font.pixelSize: 14
                text: qsTr("Context Menu")
            }
        }
    }
}
