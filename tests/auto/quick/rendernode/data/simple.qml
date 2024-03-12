// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Test

Rectangle {
    color: "red"
    Rectangle {
        color: "transparent"
        border.color: "yellow"
        border.width: 2
        width: parent.width / 2
        height: parent.height / 2
        anchors.centerIn: parent
        SimpleItem {
            anchors.fill: parent
            Rectangle {
                color: "gray"
                width: 50
                height: 50
                anchors.centerIn: parent
            }
        }
    }
}
