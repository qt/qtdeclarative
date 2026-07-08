// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadAlias/CalculatorButton.qml but textColor changed to "green".

import QtQuick
import QtQuick.Controls.Basic

AbstractButton {
    id: button
    implicitWidth: 38
    implicitHeight: 38

    property color textColor: "green"
    property bool active: false
    property alias label: labelText

    states: State {
        name: "activeState"
        when: button.active
        PropertyChanges {
            labelText.scale: 0.5
        }
    }

    contentItem: Rectangle {
        color: "#222222"
        Text {
            id: labelText
            anchors.centerIn: parent
            text: button.text
            color: button.textColor
        }
    }
}
