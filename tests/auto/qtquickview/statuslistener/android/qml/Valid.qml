// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "#00FF00"

        Text {
            anchors.centerIn: parent
            text: "QML"
            color: "black"
        }
    }
}
