// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mirrors calqlatr's NumberPad.qml: many CalculatorButton instances (via
// inline-component derived types) laid out as visual children of a GridLayout,
// plus a single BackspaceButton of an unrelated composite type.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Item {
    id: controller
    width: 400
    height: 400

    component DigitButton: CalculatorButton {}

    component OperatorButton: CalculatorButton {
        textColor: "#2CDE85"
    }

    GridLayout {
        id: mainGrid
        columns: 5

        BackspaceButton { objectName: "backspace" }
        DigitButton { text: "7"; objectName: "d7" }
        DigitButton { text: "8"; objectName: "d8" }
        DigitButton { text: "9"; objectName: "d9" }
        OperatorButton { text: "÷"; objectName: "opdiv" }

        DigitButton { text: "4"; objectName: "d4" }
        DigitButton { text: "5"; objectName: "d5" }
        DigitButton { text: "6"; objectName: "d6" }
        OperatorButton { text: "×"; objectName: "opmul" }
        DigitButton { text: "1"; objectName: "d1" }
    }
}
