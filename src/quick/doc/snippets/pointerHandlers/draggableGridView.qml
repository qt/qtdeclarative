// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQml
import QtQuick
import QtQml.Models

//! [entire]
GridView {
    id: root
    width: 320
    height: 480
    cellWidth: 80
    cellHeight: 80
    interactive: false

    displaced: Transition {
        NumberAnimation {
            properties: "x,y"
            easing.type: Easing.OutQuad
        }
    }

    model: DelegateModel {
        id: visualModel
        model: 24
        property var dropTarget: undefined
        property bool copy: false
        delegate: DropArea {
            id: delegateRoot

            width: 80
            height: 80

            onEntered: drag => {
                if (visualModel.copy) {
                    if (drag.source !== icon)
                        visualModel.dropTarget = icon
                } else {
                    visualModel.items.move(drag.source.DelegateModel.itemsIndex, icon.DelegateModel.itemsIndex)
                }
            }

            Rectangle {
                id: icon
                objectName: DelegateModel.itemsIndex

                property string text
                Component.onCompleted: {
                    color = Qt.rgba(0.2 + (48 - DelegateModel.itemsIndex) * Math.random() / 48,
                                    0.3 + DelegateModel.itemsIndex * Math.random() / 48,
                                    0.4 * Math.random(),
                                    1.0)
                    text = DelegateModel.itemsIndex
                }
                border.color: visualModel.dropTarget === this ? "black" : "transparent"
                border.width: 2
                radius: 3
                width: 72
                height: 72
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                states: [
                    State {
                        when: dragHandler.active || controlDragHandler.active
                        ParentChange {
                            target: icon
                            parent: root
                        }

                        AnchorChanges {
                            target: icon
                            anchors {
                                horizontalCenter: undefined
                                verticalCenter: undefined
                            }
                        }
                    }
                ]

                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pointSize: 14
                    text: controlDragHandler.active ? "+" : icon.text
                }

                //! [draghandlers]
                DragHandler {
                    id: dragHandler
                    acceptedModifiers: Qt.NoModifier
                    onActiveChanged: if (!active) visualModel.dropTarget = undefined
                }

                DragHandler {
                    id: controlDragHandler
                    acceptedModifiers: Qt.ControlModifier
                    onActiveChanged: {
                        visualModel.copy = active
                        if (!active) {
                            visualModel.dropTarget.text = icon.text
                            visualModel.dropTarget.color = icon.color
                            visualModel.dropTarget = undefined
                        }
                    }
                }
                //! [draghandlers]

                Drag.active: dragHandler.active || controlDragHandler.active
                Drag.source: icon
                Drag.hotSpot.x: 36
                Drag.hotSpot.y: 36
            }
        }
    }
}
//! [entire]
