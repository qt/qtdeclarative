// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Mirrors coffee's CustomButtonForm.ui.qml: an AbstractButton (deferred
// contentItem) whose deferred content is exposed via a property alias and is
// targeted by a state. Reloading must recreate the deferred content AND keep
// the alias and the state target pointing at the new object.

import QtQuick
import QtQuick.Controls.Basic

AbstractButton {
    id: button
    implicitWidth: 38
    implicitHeight: 38

    property color textColor: "#FFFFFF"
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
