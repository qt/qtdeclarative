// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Simulates ApplicationFlow.qml: a derived type with functions and states
// that modify form children via aliases and PropertyChanges with anchors.
// Includes stack.initialItem (like the coffee demo) to exercise StackView
// interaction during rebuild.

import QtQuick

DerivedFunctionChangeForm {
    id: flow
    state: "Home"

    stack.initialItem: Item {
        id: home
        visible: true
    }

    function selectCoffee() {
        flow.state = "Settings"
        flow.coffeeName = "Cappuccino"
        contentText.text = "Cappuccino"
        stack.push(choosingCoffee)
    }

    function backButton() {
        stack.pop()
        flow.state = "Home"
    }

    states: [
        State {
            name: "Home"
            PropertyChanges {
                target: toolbar
                backOpacity: 0
                backEnabled: false
            }
            PropertyChanges {
                target: contentText
                visible: false
            }
            PropertyChanges {
                target: stack
                anchors.top: contentText.bottom
            }
        },
        State {
            name: "Settings"
            PropertyChanges {
                target: contentText
                text: "Settings"
            }
            PropertyChanges {
                target: stack
                anchors.top: contentText.bottom
            }
        }
    ]
}
