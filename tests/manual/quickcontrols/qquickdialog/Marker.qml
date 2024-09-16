// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    anchors.fill: parent
    color: "transparent"
    border.color: "darkorange"

    property alias text: label.text

    Text {
        id: label
        font.pixelSize: Qt.application.font.pixelSize * 0.6
        color: parent.border.color
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 4
    }
}
