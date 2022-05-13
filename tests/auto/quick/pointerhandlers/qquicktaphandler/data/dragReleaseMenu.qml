// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 480
    height: 480
    property alias feedbackText: feedback.text

    Rectangle {
        id: rect
        anchors.fill: parent; anchors.margins: 40
        color: tapHandler.active ? "lightgrey" : "darkgrey"

        TapHandler {
            id: tapHandler
            gesturePolicy: TapHandler.DragWithinBounds
            onPressedChanged: if (pressed) {
                menu.x = point.position.x - menu.width / 2
                menu.y = point.position.y - menu.height / 2
            } else {
                if (menu.highlightedMenuItem !== "")
                    feedback.text = menu.highlightedMenuItem
            }
            onCanceled: feedback.text = "canceled"
        }

        Column {
            id: menu
            visible: tapHandler.pressed
            opacity: Math.min(1, tapHandler.timeHeld)
            property string highlightedMenuItem: ""
            Repeater {
                model: [ "top", "middle", "bottom" ]
                delegate: Rectangle {
                    property bool highlighted: tapHandler.pressed &&
                            contains(mapFromItem(rect, tapHandler.point.position))
                    onHighlightedChanged: {
                        if (highlighted)
                            menu.highlightedMenuItem = menuItemText.text
                        else if (menu.highlightedMenuItem === menuItemText.text)
                            menu.highlightedMenuItem = ""
                    }
                    width: 100
                    height: 20
                    color: highlighted ? "lightsteelblue" : "aliceblue"
                    Text {
                        id: menuItemText
                        anchors.centerIn: parent
                        text: modelData
                    }
                }
            }
        }

        Text {
            id: feedback
            x: 6; y: 6
            textFormat: Text.MarkdownText
            text: "hold for context menu"
        }
    }
}
