// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
pragma ComponentBehavior: Bound
import QtQuick
import Qt.labs.animation
import Qt.labs.folderlistmodel

//![0]
Rectangle {
    id: canvas
    width: 640
    height: 480
    color: "#333"
    property int highestZ: 0

    Repeater {
        model: FolderListModel { nameFilters: ["*.qml"] }

        delegate: Rectangle {
            required property string fileName
            required property url fileUrl
            required property int index

            id: frame
            x: index * 30; y: index * 30
            width: 320; height: 240
            property bool dragging: ldh.active || rdh.active
            onDraggingChanged: if (dragging) z = ++canvas.highestZ
            border { width: 2; color: dragging ? "red" : "steelblue" }
            color: "beige"
            clip: true

            TextEdit {
                // drag to select text
                id: textEdit
                textDocument.source: frame.fileUrl
                x: 3; y: 3

                BoundaryRule on y {
                    id: ybr
                    minimum: textEdit.parent.height - textEdit.height; maximum: 0
                    minimumOvershoot: 200; maximumOvershoot: 200
                    overshootFilter: BoundaryRule.Peak
                }
            }

            DragHandler {
                id: rdh
                // right-drag to position the "window"
                acceptedButtons: Qt.RightButton
            }

            WheelHandler {
                target: textEdit
                property: "y"
                onActiveChanged: if (!active) ybr.returnToBounds()
            }

            Rectangle {
                anchors.right: parent.right
                width: titleText.implicitWidth + 12
                height: titleText.implicitHeight + 6
                border { width: 2; color: parent.border.color }
                bottomLeftRadius: 6
                Text {
                    id: titleText
                    color: "saddlebrown"
                    anchors.centerIn: parent
                    text: frame.fileName
                    textFormat: Text.PlainText
                }
                DragHandler {
                    id: ldh
                    // left-drag to position the "window"
                    target: frame
                }
            }
        }
    }
}
//![0]
