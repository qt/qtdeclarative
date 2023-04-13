// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models
//![0]
Rectangle {
    id: root

    width: 300
    height: 400

    Component {
        id: dragDelegate

        MouseArea {
            id: dragArea

            property bool held: false
            required property string name
            required property string type
            required property string size
            required property int age

            anchors {
                left: parent?.left
                right: parent?.right
            }
            height: content.height

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressAndHold: held = true
            onReleased: held = false

            Rectangle {
                id: content
//![0]
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: dragArea.width
                height: column.implicitHeight + 4

                border.width: 1
                border.color: "lightsteelblue"

                color: dragArea.held ? "lightsteelblue" : "white"
                Behavior on color { ColorAnimation { duration: 100 } }

                radius: 2
//![1]
                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
//![1]
                states: State {
                    when: dragArea.held

                    ParentChange {
                        target: content
                        parent: root
                    }
                    AnchorChanges {
                        target: content
                        anchors {
                            horizontalCenter: undefined
                            verticalCenter: undefined
                        }
                    }
                }

                Column {
                    id: column
                    anchors {
                        fill: parent
                        margins: 2
                    }

                    Text { text: qsTr('Name: ') + dragArea.name }
                    Text { text: qsTr('Type: ') + dragArea.type }
                    Text { text: qsTr('Age: ') + dragArea.age }
                    Text { text: qsTr('Size: ') + dragArea.size }
                }
//![2]
            }
//![3]
            DropArea {
                anchors {
                    fill: parent
                    margins: 10
                }

                onEntered: (drag) => {
                    visualModel.items.move(
                            drag.source.DelegateModel.itemsIndex,
                            dragArea.DelegateModel.itemsIndex)
                }
            }
//![3]
        }
    }
//![2]
//![4]
    DelegateModel {
        id: visualModel

        model: PetsModel {}
        delegate: dragDelegate
    }

    ListView {
        id: view

        anchors {
            fill: parent
            margins: 2
        }

        model: visualModel

        spacing: 4
        cacheBuffer: 50
    }
//![4]
//![5]
}
//![5]
