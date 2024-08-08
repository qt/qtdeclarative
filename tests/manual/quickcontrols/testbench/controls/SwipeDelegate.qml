// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["highlighted"],
        ["highlighted", "pressed"]
    ]

    property Component actionComponent: Component {
        Rectangle {
            color: SwipeDelegate.pressed ? "#333" : "#444"
            width: parent ? parent.width : 0
            height: parent ? parent.height: 0
            clip: true

            Label {
                text: "Test"
                color: "white"
                anchors.centerIn: parent
            }
        }
    }

    property Component component: SwipeDelegate {
        id: swipeDelegate
        text: "SwipeDelegate"
        enabled: !is("disabled")
        checkable: is("checkable")
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
        highlighted: is("highlighted")
        focusPolicy: Qt.StrongFocus

        swipe.left: actionComponent
        swipe.right: actionComponent
    }

    property Component exampleComponent: ListView {
        implicitWidth: 200
        implicitHeight: 200
        clip: true
        model: 20
        delegate: SwipeDelegate {
            width: ListView.view.width
            text: "SwipeDelegate"
            focusPolicy: Qt.StrongFocus

            swipe.left: actionComponent
            swipe.right: actionComponent
        }
    }
}
