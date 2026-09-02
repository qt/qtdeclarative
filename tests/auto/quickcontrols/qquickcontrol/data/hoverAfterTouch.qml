// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200

    property alias upperButton: upperButton
    property alias lowerButton: lowerButton

    Column {
        anchors.fill: parent

        Button {
            id: upperButton
            objectName: "upperButton"
            width: 200
            height: 100
            hoverEnabled: true
        }

        Button {
            id: lowerButton
            objectName: "lowerButton"
            width: 200
            height: 100
            hoverEnabled: true
        }
    }
}
