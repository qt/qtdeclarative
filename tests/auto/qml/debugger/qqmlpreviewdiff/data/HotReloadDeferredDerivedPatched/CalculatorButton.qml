// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadButtonFunc/CalculatorButton.qml, but with an extra property
// (and thus an extra "blaChanged" signal) added. This shifts the meta-object
// index of the getBackgroundColor()/getTextColor() VME methods, which is the
// edit that made calqlatr assert on hot reload.

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

    property int bla
}
