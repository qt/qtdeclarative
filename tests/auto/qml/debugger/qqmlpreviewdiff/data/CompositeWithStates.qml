// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type with states and transitions that reference internal IDs.

import QtQuick

Item {
    id: root
    width: 200
    height: 200

    property alias indicator: rect
    property alias stateLabel: stateText

    state: "inactive"

    Rectangle {
        id: rect
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "gray"
    }

    Text {
        id: stateText
        text: root.state
        anchors.bottom: parent.bottom
    }

    states: [
        State {
            name: "active"
            PropertyChanges { target: rect; color: "green" }
            PropertyChanges { target: stateText; text: "ACTIVE" }
        },
        State {
            name: "inactive"
            PropertyChanges { target: rect; color: "gray" }
            PropertyChanges { target: stateText; text: "inactive" }
        }
    ]

    transitions: [
        Transition {
            from: "inactive"; to: "active"
            ColorAnimation { target: rect; duration: 200 }
        }
    ]
}
