// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mirrors calqlatr's CalculatorButton.qml more closely than HotReloadButton:
// the deferred contentItem/background bindings call the type's own JS methods
// (getTextColor()/getBackgroundColor()). Those methods are VME methods, so
// their meta-object index shifts when a property (and its change signal) is
// added in the patched version.

import QtQuick
import QtQuick.Controls

RoundButton {
    id: button
    implicitWidth: 38
    implicitHeight: 38

    property color textColor: "#FFFFFF"
    readonly property color backgroundColor: "#222222"

    function getBackgroundColor() {
        if (button.pressed)
            return textColor;
        return backgroundColor;
    }

    function getTextColor() {
        if (button.pressed)
            return backgroundColor;
        return textColor;
    }

    background: Rectangle {
        color: button.getBackgroundColor()
    }

    contentItem: Text {
        text: button.text
        color: button.getTextColor()
    }
}
