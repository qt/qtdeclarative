// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    width: 400; height: 400

    Component.onCompleted: rect.state = "wide"

    Rectangle {
        id: rect

        color: "green"
        width: 50
        height: 50
        anchors.centerIn: parent

        states: State {
            name: "wide"
            PropertyChanges {
                target: rect
                width: 100
            }
        }
        transitions: Transition {
            to: "wide"
            SequentialAnimation {
                NumberAnimation { duration: 200; property: "width" }
                ScriptAction { script: { rect.state = ""; rect.state = "wide" } }
            }
        }
    }
}
