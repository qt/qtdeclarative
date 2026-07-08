// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Unrelated composite type; must be left untouched when CalculatorButton is patched.

import QtQuick
import QtQuick.Controls

RoundButton {
    id: button
    implicitWidth: 38
    implicitHeight: 38
    text: "bs"

    background: Rectangle {
        color: "#222222"
    }

    contentItem: Text {
        text: "<-"
        color: "#DE2C2C"
    }
}
