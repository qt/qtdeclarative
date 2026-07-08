// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Like the inline-component test's CalculatorButton, but the deferred contentItem
// is an instance of a composite type from an extra .qml file (Badge.qml).

import QtQuick
import QtQuick.Controls

RoundButton {
    id: button
    implicitWidth: 38
    implicitHeight: 38

    property color textColor: "#FFFFFF"
    readonly property color backgroundColor: "#222222"

    background: Rectangle {
        color: button.backgroundColor
    }

    contentItem: Badge {
        text: button.text
        tint: button.textColor
    }
}
