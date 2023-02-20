// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Rectangle {
    id: rect
    width: 140; height: 100

    //![1]
    TapHandler {
        id: menuPopupHandler
        gesturePolicy: TapHandler.DragWithinBounds
        onPressedChanged:
            if (pressed) {
                menu.x = point.position.x - menu.width / 2
                menu.y = point.position.y - menu.height / 2
            } else {
                feedback.text = menu.highlightedMenuItem
                selectFlash.start()
            }
        onCanceled: feedback.text = "canceled"
    }
    //![1]

    Column {
        id: menu
        visible: menuPopupHandler.pressed
        opacity: Math.min(1, menuPopupHandler.timeHeld)
        property string highlightedMenuItem: ""
        Repeater {
            model: [ "top", "middle", "bottom" ]
            delegate: Rectangle {
                property bool highlighted: menuPopupHandler.pressed &&
                                           contains(mapFromItem(rect, menuPopupHandler.point.position))
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
        y: 6; anchors.horizontalCenter: parent.horizontalCenter
        textFormat: Text.MarkdownText
        text: "hold for context menu"

        SequentialAnimation on font.weight {
            id: selectFlash
            running: false
            loops: 3
            PropertyAction { value: Font.Black }
            PauseAnimation { duration: 100 }
            PropertyAction { value: Font.Normal }
            PauseAnimation { duration: 100 }
        }
    }
}
//![0]
