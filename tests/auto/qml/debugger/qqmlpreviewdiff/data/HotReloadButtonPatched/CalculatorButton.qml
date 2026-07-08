// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadButton/CalculatorButton.qml, but the textColor default
// value was changed from "#FFFFFF" to "green" — the edit that, in calqlatr,
// made all CalculatorButton instances disappear on hot reload.

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

    contentItem: Text {
        text: button.text
        color: button.textColor
    }
}
