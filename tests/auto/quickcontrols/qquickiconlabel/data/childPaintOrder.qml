// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

AbstractButton {
    id: control
    width: 200
    height: 200
    text: "Button"
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
    icon.color: enabled ? "transparent" : "red"

    contentItem: IconLabel {
        id: iconLabel
        objectName: "iconLabel"
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font

        Text {
            objectName: "childText"
            color: "red"
            anchors.centerIn: parent
            text: "Should be in front"
            font.bold: true
        }
    }
}
