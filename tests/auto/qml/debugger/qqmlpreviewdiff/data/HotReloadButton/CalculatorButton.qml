// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mirrors calqlatr's CalculatorButton.qml: a composite type deriving from a
// C++ control, with a color property whose default value is what we change.

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

    contentItem: Text {
        text: button.text
        color: button.textColor
    }
}
