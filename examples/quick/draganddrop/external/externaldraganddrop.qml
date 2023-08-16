// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    padding: 8
    contentItem: ColumnLayout {
        component DragAndDropArea: Rectangle {
            id: item
            property string display: qsTr("Drag items to this area, or drag this item to a different drop area.")
            property alias dropEnabled: acceptDropCB.checked
            color: dropArea.containsDrag ? Qt.darker(palette.base) : palette.base

            ColorAnimation on color {
                id: rejectAnimation
                from: "#FCC"
                to: palette.base
                duration: 1000
            }
            Label {
                anchors.fill: parent
                anchors.margins: 10
                text: item.display
                wrapMode: Text.WordWrap
            }
            DropArea {
                id: dropArea
                anchors.fill: parent
                keys: ["text/plain"]
                onEntered: (drag) => {
                    if (!acceptDropCB.checked) {
                        drag.accepted = false
                        rejectAnimation.start()
                    }
                }
                onDropped: (drop) => {
                    if (drop.hasText && acceptDropCB.checked) {
                        if (drop.proposedAction === Qt.MoveAction || drop.proposedAction === Qt.CopyAction) {
                            item.display = drop.text
                            drop.acceptProposedAction()
                        }
                    }
                }
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                drag.target: draggable
            }
            Item {
                id: draggable
                anchors.fill: parent
                Drag.active: mouseArea.drag.active
                Drag.hotSpot.x: 0
                Drag.hotSpot.y: 0
                Drag.mimeData: { "text/plain": item.display }
                Drag.dragType: Drag.Automatic
                Drag.onDragFinished: (dropAction) => {
                    if (dropAction === Qt.MoveAction)
                        item.display = ""
                }
            }
            CheckBox {
                id: acceptDropCB
                anchors.bottom: parent.bottom
                checked: true
                text: qsTr("accept drop")
            }
        }

        DragAndDropArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        DragAndDropArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
