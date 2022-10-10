// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import "content"

Rectangle {
    id: root
    width: 600; height: 480; color: "#f0f0f0"

    property int globalGesturePolicy : TapHandler.DragThreshold

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: 600
        contentWidth: 1000
        objectName: "Flick"

        Repeater {
            model: flick.contentHeight/200
            Rectangle {
                width: flick.contentWidth
                height: 101
                x: 0
                y: index * 200
                border.color: "#808080"
                border.width: 1
                color: "transparent"
            }
        }

        Repeater {
            model: flick.contentWidth/200
            Rectangle {
                width: 101
                height: flick.contentHeight
                x: index * 200
                y: 0
                border.color: "#808080"
                border.width: 1
                color: "transparent"
            }
        }

        TextBox {
            x: 0; y: 0
            width: 100; height: 100
            label: "DragHandler"
            objectName: "dragSquircle1"
            DragHandler {
                dragThreshold: ckZeroDragThreshold1.checked ? 0 : undefined
            }
            CheckBox {
                id: ckZeroDragThreshold1
                label: " Zero threshold"
                anchors.horizontalCenter: parent.horizontalCenter
                y: 20
                checked: false
            }
        }

        TextBox {
            x: 100; y: 0
            width: 100; height: 100
            label: "TapHandler"
            color: queryColor(tap1.pressed)

            TapHandler {
                id: tap1
                gesturePolicy: root.globalGesturePolicy
            }
        }

        TextBox {
            x: 200; y: 0
            width: 100; height: 100
            label: "TapHandler\nDragHandler"
            color: queryColor(tap2.pressed)
            TapHandler {
                id: tap2
                gesturePolicy: root.globalGesturePolicy
            }
            DragHandler {
                dragThreshold: ckZeroDragThreshold2.checked ? 0 : undefined
            }
            CheckBox {
                id: ckZeroDragThreshold2
                label: " Zero threshold"
                anchors.horizontalCenter: parent.horizontalCenter
                y: 32
                checked: false
            }
        }

        TextBox {
            x: 300; y: 0
            width: 100; height: 100
            label: "DragHandler\nTapHandler"
            color: queryColor(tap3.pressed)
            DragHandler {
                dragThreshold: ckZeroDragThreshold3.checked ? 0 : undefined
            }
            CheckBox {
                id: ckZeroDragThreshold3
                label: " Zero threshold"
                anchors.horizontalCenter: parent.horizontalCenter
                y: 32
                checked: false
            }
            TapHandler {
                id: tap3
                gesturePolicy: root.globalGesturePolicy
            }
        }

        TextBox {
            x: 400; y: 0
            width: 100; height: 100
            label: "DragHandler"
            DragHandler {
                dragThreshold: ckZeroDragThreshold4.checked ? 0 : undefined
            }
            CheckBox {
                id: ckZeroDragThreshold4
                label: " Zero threshold"
                anchors.horizontalCenter: parent.horizontalCenter
                y: 20
                checked: false
            }

            TextBox {
                label: "TapHandler"
                x: (parent.width - width)/2
                y: 60
                color: queryColor(tap4.pressed)
                TapHandler {
                    id: tap4
                    gesturePolicy: root.globalGesturePolicy
                }
            }
        }

        TextBox {
            objectName: "dragSquircle5"
            x: 500; y: 0
            width: 100; height: 100
            label: "TapHandler"
            color: queryColor(tap5.pressed)
            CheckBox {
                id: ckGreedyDrag
                x: 10
                anchors.bottom: dragRect5.top
                label: " Greedy ↓"
                checked: true
            }
            CheckBox {
                id: ckZeroDragThreshold5
                label: " Zero threshold"
                x: 10
                anchors.bottom: ckGreedyDrag.top
                checked: false
            }
            TapHandler {
                id: tap5
                gesturePolicy: root.globalGesturePolicy
            }

            TextBox {
                id: dragRect5
                objectName: "dragRect5"
                label: "DragHandler"
                x: (parent.width - width)/2
                y: 60
                DragHandler {
                    grabPermissions: ckGreedyDrag ? DragHandler.CanTakeOverFromAnything :
                        DragHandler.CanTakeOverFromItems | DragHandler.CanTakeOverFromHandlersOfDifferentType | DragHandler.ApprovesTakeOverByAnything
                    dragThreshold: ckZeroDragThreshold5.checked ? 0 : undefined
                }
            }
        }


        TextBox {
            x: 0; y: 100
            width: 100; height: 100
            label: "No MouseArea"

            TextBox {
                objectName: "dragRect01"
                label: "DragHandler"
                x: (parent.width - width)/2
                y: 60
                DragHandler {
                    dragThreshold: ckZeroDragThreshold6.checked ? 0 : undefined
                }
            }
            CheckBox {
                id: ckZeroDragThreshold6
                label: " Zero threshold"
                anchors.horizontalCenter: parent.horizontalCenter
                y: 20
                checked: false
            }
        }

        TextBox {
            id: r2
            label: "MouseArea"
            x: 100; y: 100
            width: 100; height: 100

            MouseArea {
                id: ma
                enabled: ckEnabled.checked
                drag.target: ckDrag.checked ? r2 : undefined
                drag.threshold: ckExtendDragThreshold.checked ? 50 : undefined
                anchors.fill: parent
            }
            Column {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                x: 10
                CheckBox {
                    id: ckEnabled
                    label: " Enabled"
                    checked: true
                }

                CheckBox {
                    id: ckDrag
                    label: " Drag"
                    checked: true
                }

                CheckBox {
                    id: ckExtendDragThreshold
                    label: " Extend threshold"
                    checked: false
                }
            }
        }

        TextBox {
            objectName: "dragSquircle9"
            x: 200; y: 100
            width: 100; height: 100
            label: "DragHandler"
            color: queryColor(drag9.active)
            CheckBox {
                id: ckGreedyDrag9
                x: 10
                y: 20
                label: " Greedy"
                checked: true
            }
            CheckBox {
                id: ckZeroDragThreshold9
                label: " Zero threshold"
                x: 10
                anchors.bottom: ckGreedyDragR9.top
                checked: false
            }
            CheckBox {
                id: ckGreedyDragR9
                x: 10
                anchors.bottom: dragRect9.top
                label: " Greedy ↓"
                checked: false
            }
            DragHandler {
                id: drag9
                objectName: "drag9"
                grabPermissions: ckGreedyDrag9.checked ? DragHandler.CanTakeOverFromAnything :
                                                DragHandler.CanTakeOverFromItems | DragHandler.CanTakeOverFromHandlersOfDifferentType | DragHandler.ApprovesTakeOverByAnything
                dragThreshold: ckZeroDragThreshold9.checked ? 0 : undefined
            }

            TextBox {
                id: dragRect9
                objectName: "dragRect9"
                label: "DragHandler"
                x: (parent.width - width)/2
                y: 65
                DragHandler {
                    objectName: "dragRect9"
                    grabPermissions: ckGreedyDragR9.checked ? DragHandler.CanTakeOverFromAnything :
                                                    DragHandler.CanTakeOverFromItems | DragHandler.CanTakeOverFromHandlersOfDifferentType | DragHandler.ApprovesTakeOverByAnything
                    dragThreshold: ckZeroDragThreshold9.checked ? 0 : undefined
                }
            }
        }
    }
}
