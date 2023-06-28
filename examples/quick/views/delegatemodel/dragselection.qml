// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick
import QtQml.Models

pragma ComponentBehavior: Bound

Item {
    id: root

    width: 320
    height: 480

    property bool dragging: false

    Component {
        id: packageDelegate
        Package {
            id: packageRoot

            required property var modelData

            MouseArea {
                id: visibleContainer
                Package.name: "visible"

                width: 64
                height: 64
                enabled: packageRoot.DelegateModel.inSelected

                drag.target: draggable

                Item {
                    id: draggable

                    width: 64
                    height: 64

                    Drag.active: visibleContainer.drag.active

                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }

                    states: State {
                        when: visibleContainer.drag.active
                        AnchorChanges {
                            target:  draggable
                            anchors {
                                horizontalCenter: undefined
                                verticalCenter: undefined
                            }
                        }
                        ParentChange {
                            target: selectionView
                            parent: draggable
                            x: 0
                            y: 0
                        }
                        PropertyChanges {
                            root.dragging: true
                        }
                        ParentChange {
                            target: draggable
                            parent: root
                        }
                    }
                }
                DropArea {
                    anchors.fill: parent
                    onEntered: selectedItems.move(0, visualModel.items.get(packageRoot.DelegateModel.itemsIndex), selectedItems.count)
                }
            }
            Item {
                id: selectionContainer
                Package.name: "selection"

                width: 64
                height: 64

                visible: PathView.onPath
            }
            Rectangle {
                id: content
                parent: visibleContainer

                width: 58
                height: 58

                radius: 8

                gradient: Gradient {
                    GradientStop {
                        id: gradientStart
                        position: 0.0
                        color: "#8AC953"
                    }
                    GradientStop {
                        id: gradientEnd
                        position: 1.0
                        color: "#8BC953"
                    }
                }

                border.width: 2
                border.color: "#007423"

                state: root.dragging && packageRoot.DelegateModel.inSelected ? "selected" : "visible"

                Text {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "white"
                    text: packageRoot.modelData
                    font.pixelSize: 18
                }

                Rectangle {
                    anchors {
                        right: parent.right
                        top: parent.top
                        margins: 3
                    }
                    width: 12
                    height: 12
                    color: packageRoot.DelegateModel.inSelected ? "black" : "white"
                    radius: 6

                    border.color: "white"
                    border.width: 2

                    MouseArea {
                        anchors.fill: parent
                        onClicked: packageRoot.DelegateModel.inSelected = !packageRoot.DelegateModel.inSelected
                    }
                }

                states: [
                    State {
                        name: "selected"
                        ParentChange {
                            target: content
                            parent: selectionContainer
                            x: 3
                            y: 3
                        }
                        PropertyChanges {
                            packageRoot.DelegateModel.inItems: visibleContainer.drag.active
                            gradientStart.color: "#017423"
                        }
                        PropertyChanges {
                            gradientStart.color: "#007423"
                        }
                    }, State {
                        name: "visible"
                        PropertyChanges {
                            packageRoot.DelegateModel.inItems: true
                        }
                        ParentChange {
                            target: content
                            parent: visibleContainer
                            x: 3
                            y: 3
                        }
                        PropertyChanges {
                            gradientStart.color: "#8AC953"
                        }
                        PropertyChanges {
                            gradientStart.color: "#8BC953"
                        }
                    }
                ]
                transitions: Transition {
                    PropertyAction {
                        target: packageRoot
                        properties: "DelegateModel.inItems"
                    }
                    ParentAnimation {
                        target: content
                        NumberAnimation {
                            target: content
                            properties: "x,y"
                            duration: 500
                        }
                    }
                    ColorAnimation {
                        targets: [gradientStart, gradientEnd]
                        duration: 500
                    }
                }
            }
        }
    }

    DelegateModel {
        id: visualModel
        model: 35
        delegate: packageDelegate

        groups: DelegateModelGroup {
            id: selectedItems
            name: "selected"
        }

        Component.onCompleted:  parts.selection.filterOnGroup = "selected"
    }

    PathView {
        id: selectionView

        height: 64
        width: 64

        model: visualModel.parts.selection

        path: Path {
            startX: 0
            startY: 0
            PathLine {
                x: 64
                y: 64
            }
        }
    }

    GridView {
        id: itemsView
        anchors.fill: parent
        cellWidth: 64
        cellHeight: 64
        model: visualModel.parts.visible
    }
}
