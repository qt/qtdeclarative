// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as Old but with the function body changed (string literal modified).
// Reproduces the ASSERT "canGetTypeFromVariant<T>(this)" crash from the
// coffee demo when changing cappuccino() in ApplicationFlow.qml.

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
        flow.coffeeName = "Cappuccinooooo"
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
