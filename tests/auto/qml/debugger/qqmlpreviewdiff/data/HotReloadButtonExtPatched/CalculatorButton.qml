// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadButtonExt/CalculatorButton.qml but with textColor changed
// from "#FFFFFF" to "green".

import QtQuick
import QtQuick.Controls

RoundButton {
    id: button
    implicitWidth: 38
    implicitHeight: 38

    property color textColor: "green"
    readonly property color backgroundColor: "#222222"

    background: Rectangle {
        color: button.backgroundColor
    }

    contentItem: Badge {
        text: button.text
        tint: button.textColor
    }
}
